#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_wlsystem.h"
#include "allegro5/internal/aintern_wlevents.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/platform/aintwl.h"

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
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name)
{
	/* This space deliberately left blank */
    (void)data;
    (void)registry;
    (void)name;
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

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, s);
	wl_display_roundtrip(display);
    /* The roundtrip implicitly creates the compositor, the
     * shared memory (used for software rendering), and the
     * XDG window management base.
     */

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

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_wayland_driver(void)
{
    if (wl_vt) return wl_vt;

    wl_vt = al_calloc(1, sizeof *wl_vt);

    wl_vt->id = ALLEGRO_SYSTEM_ID_WAYLAND;
    wl_vt->initialize = wl_initialize;
    wl_vt->get_display_driver = wl_get_display_driver;
    /* The rest of the functions need to go here */
    wl_vt->shutdown_system = wl_shutdown_system;
    wl_vt->get_path = _al_unix_get_path;
    wl_vt->get_time = _al_unix_get_time;
    wl_vt->rest = _al_unix_rest;
    wl_vt->init_timeout = _al_unix_init_timeout;

    return wl_vt;
}