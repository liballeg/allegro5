#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_wlsystem.h"
#include "allegro5/internal/aintern_wlevents.h"
#include "allegro5/internal/aintern_wlfullscreen.h"
#include "allegro5/internal/aintern_wlinput.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintwl.h"
#include "allegro5/platform/cursor-shape-client-protocol.h"
#include "allegro5/platform/xdg-decoration-client-protocol.h"

#include <libdecor.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include <EGL/egl.h>

ALLEGRO_DEBUG_CHANNEL("system")

static ALLEGRO_SYSTEM_INTERFACE *wl_vt;

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void registry_handle_global(void *data,
                struct wl_registry *registry,
                uint32_t name,
                const char *interface,
                uint32_t version)
{
    ALLEGRO_SYSTEM_WAYLAND *s = data;
    (void)version;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        s->compositor = wl_registry_bind(
            registry,
            name,
            &wl_compositor_interface,
            4);
        if (s->compositor) {
            ALLEGRO_INFO("Wayland compositor created\n");
        }
    }

    if (strcmp(interface, wl_shm_interface.name) == 0) {
        s->shm = wl_registry_bind(
            registry,
            name,
            &wl_shm_interface,
            1);
        if (s->shm) {
            ALLEGRO_INFO("Wayland shared memory created\n");
        }
    }

    if (strcmp(interface, wl_output_interface.name) == 0) {
        struct wl_output *output = wl_registry_bind(
            registry, name, &wl_output_interface, 4);
        if (output)
            _al_wayland_add_output(s, output, name);
    }

    if (strcmp(interface, wl_seat_interface.name) == 0) {
        struct wl_seat *seat = wl_registry_bind(
            registry, name, &wl_seat_interface, 8);
        if (seat)
            _al_wl_seat_add(s, seat);
    }

    if (strcmp(interface, wp_cursor_shape_manager_v1_interface.name) == 0) {
        s->cursor_shape_manager = wl_registry_bind(
            registry, name, &wp_cursor_shape_manager_v1_interface, 2);
        ALLEGRO_INFO("Wayland cursor shape manager created\n");
    }

    if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        s->wm_base = wl_registry_bind(
            registry,
            name,
            &xdg_wm_base_interface,
            1);

        xdg_wm_base_add_listener(
            s->wm_base,
            &xdg_wm_base_listener,
            NULL);
    }

    if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        /* Server-side window decorations.  This is optional: compositors
         * that don't offer it leave us with client-side decorations, which
         * for now means none at all. */
        s->decoration_manager = wl_registry_bind(
            registry,
            name,
            &zxdg_decoration_manager_v1_interface,
            1);
        if (s->decoration_manager) {
            ALLEGRO_INFO("Wayland decoration manager created\n");
        }
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name)
{
	ALLEGRO_SYSTEM_WAYLAND *s = data;
    (void)registry;

    /* Only outputs are tracked at the moment. */
    _al_wayland_remove_output(s, name);
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

static ALLEGRO_SYSTEM *wl_initialize(int flags) {
    struct wl_display *display;
    struct wl_registry *registry;
    EGLint major;
    EGLint minor;

    ALLEGRO_SYSTEM_WAYLAND *s;

    (void)flags;

    display = wl_display_connect(NULL);
    if (!display) {
        ALLEGRO_ERROR("wl_display_connect failed.\n");
        return NULL;
    }

    _al_unix_init_time();

    s = al_calloc(1, sizeof *s);

    /* These are filled in by the registry listener, which runs during the
     * roundtrip below, so they must exist before then. */
    _al_vector_init(&s->outputs, sizeof (struct ALLEGRO_WL_OUTPUT *));

    /* Needed as soon as the first keymap event arrives (which can happen
     * during the roundtrips below). */
    s->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!s->xkb_context)
        ALLEGRO_WARN("Failed to create xkb context; keyboard input disabled\n");

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, s);
	wl_display_roundtrip(display);
    /* The roundtrip implicitly creates the compositor, the
     * shared memory (used for software rendering), and the
     * XDG window management base.
     */

    /* A second roundtrip delivers the async events that arrive after
     * binding: wl_output geometry/mode, and the wl_seat capabilities
     * (which bind the keyboard and pointer objects). */
    wl_display_roundtrip(display);

    /* EGL initialization.  The Wayland display is the native display
     * handle, and eglGetPlatformDisplay() (core in EGL 1.5) implicitly
     * tells the driver which extension to load. */
    s->egl_display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, display, NULL);
    if (s->egl_display == EGL_NO_DISPLAY) {
        ALLEGRO_ERROR("eglGetPlatformDisplay failed: %#x\n", eglGetError());
        wl_display_disconnect(display);
        al_free(s);
        return NULL;
    }

    if (!eglInitialize(s->egl_display, &major, &minor)) {
        ALLEGRO_ERROR("eglInitialize failed: %#x\n", eglGetError());
        wl_display_disconnect(display);
        al_free(s);
        return NULL;
    }
    ALLEGRO_INFO("Initialized EGL %d.%d on Wayland\n", major, minor);

    /* libdecor provides window decorations and manages the xdg-shell
     * surfaces.  It is a hard build dependency, but may still fail to
     * initialise at runtime (eg. no decoration plugin installed), in
     * which case we fall back to undecorated windows. */
    s->decor = libdecor_new(display, NULL);
    if (s->decor) {
        ALLEGRO_INFO("libdecor initialised\n");
        /* Let libdecor finish its startup handshake (it needs an event
         * roundtrip for its internal sync callback) before any window
         * is created. */
        wl_display_roundtrip(display);
    }
    else {
        ALLEGRO_WARN("libdecor failed to initialise; "
            "windows will be undecorated\n");
    }

    _al_vector_init(&s->system.displays, sizeof (ALLEGRO_DISPLAY_WAYLAND *));

    s->system.vt = wl_vt;

    s->display = display;
    s->registry = registry;

    if (s->display) {
        ALLEGRO_INFO("Wayland driver connected to Wayland\n");

        _al_mutex_init(&s->lock);
        _al_cond_init(&s->configured_cond);

        _al_thread_create(&s->wlevents_thread, _al_wl_background_thread, s);
        s->have_wlevents_thread = true;
    }

    return &s->system;
}

static void wl_shutdown_system(void)
{
    ALLEGRO_SYSTEM *s = al_get_system_driver();
    ALLEGRO_SYSTEM_WAYLAND *swl = (void*)s;

    ALLEGRO_INFO("shutting down.\n");

    if (swl->have_wlevents_thread) {
        _al_thread_join(&swl->wlevents_thread);
        swl->have_wlevents_thread = false;
    }

    while (_al_vector_size(&s->displays) > 0) {
        ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
        ALLEGRO_DISPLAY *d = *dptr;
        al_destroy_display(d);
    }
    _al_vector_free(&s->displays);

    while (_al_vector_size(&swl->outputs) > 0) {
        struct ALLEGRO_WL_OUTPUT *o;
        o = *((struct ALLEGRO_WL_OUTPUT **)_al_vector_ref(&swl->outputs, 0));
        wl_output_destroy(o->output);
        al_free(o);
        _al_vector_delete_at(&swl->outputs, 0);
    }
    _al_vector_free(&swl->outputs);

    _al_wl_input_shutdown(swl);

    if (swl->xkb_context) {
        xkb_context_unref(swl->xkb_context);
        swl->xkb_context = NULL;
    }

    /* shm, compositor, wm_base were implicitly created */
    if (swl->shm) {
        wl_shm_destroy(swl->shm);
    }

    if (swl->compositor) {
        wl_compositor_destroy(swl->compositor);
    }

    if (swl->wm_base) {
        xdg_wm_base_destroy(swl->wm_base);
    }

    if (swl->decoration_manager) {
        zxdg_decoration_manager_v1_destroy(swl->decoration_manager);
    }

    if (swl->cursor_shape_manager) {
        wp_cursor_shape_manager_v1_destroy(swl->cursor_shape_manager);
    }

    if (swl->decor) {
        libdecor_unref(swl->decor);
        swl->decor = NULL;
    }

    if (swl->registry) {
        wl_registry_destroy(swl->registry);
    }

    if (swl->egl_display != EGL_NO_DISPLAY) {
        eglTerminate(swl->egl_display);
    }

    if (swl->display) {
        wl_display_disconnect(swl->display);
    }

    _al_cond_destroy(&swl->configured_cond);
    _al_mutex_destroy(&swl->lock);

    al_free(swl);
}

static ALLEGRO_DISPLAY_INTERFACE *wl_get_display_driver(void)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    _al_mutex_lock(&system->lock);
    ALLEGRO_DISPLAY_INTERFACE *driver = _al_display_wayland_driver();
    _al_mutex_unlock(&system->lock);
    return driver;
}

static int wl_get_num_video_adapters(void)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    return _al_wayland_get_num_video_adapters(system);
}


static bool wl_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    return _al_wayland_get_monitor_info(system, adapter, info);
}


static ALLEGRO_KEYBOARD_DRIVER *wl_get_keyboard_driver(void)
{
    return _al_wl_keyboard_driver();
}


static ALLEGRO_MOUSE_DRIVER *wl_get_mouse_driver(void)
{
    return _al_wl_mouse_driver();
}


/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_wayland_driver(void)
{
    if (wl_vt) return wl_vt;

    wl_vt = al_calloc(1, sizeof *wl_vt);

    wl_vt->id = ALLEGRO_SYSTEM_ID_WAYLAND;
    wl_vt->initialize = wl_initialize;
    wl_vt->get_display_driver = wl_get_display_driver;
    wl_vt->get_num_video_adapters = wl_get_num_video_adapters;
    wl_vt->get_monitor_info = wl_get_monitor_info;
    wl_vt->get_keyboard_driver = wl_get_keyboard_driver;
    wl_vt->get_mouse_driver = wl_get_mouse_driver;
    wl_vt->shutdown_system = wl_shutdown_system;
    wl_vt->get_path = _al_unix_get_path;
    wl_vt->get_time = _al_unix_get_time;
    wl_vt->rest = _al_unix_rest;
    wl_vt->init_timeout = _al_unix_init_timeout;

    return wl_vt;
}