#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_wleglconfig.h"
#include "allegro5/internal/aintern_wlfullscreen.h"
#include "allegro5/internal/aintern_wlsystem.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/platform/aintwl.h"
#include "allegro5/platform/xdg-decoration-client-protocol.h"

#include <libdecor-0/libdecor.h>

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE wldpy_vt;

static void wldpy_destroy_display(ALLEGRO_DISPLAY *display);
static void wldpy_free_display(ALLEGRO_DISPLAY *display);

static bool wldpy_set_display_flag(ALLEGRO_DISPLAY *display, int flag, bool onoff);

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) 
{ 
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)data;
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();

    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_commit(d->surface);

    /* This is dispatched while the event thread holds the system lock,
     * so it is safe to wake the thread blocked in
     * wldpy_create_display_locked(). */
    d->configured = true;
    _al_cond_broadcast(&system->configured_cond);
}

static const struct xdg_surface_listener xdg_surface_listener = { 
    .configure = xdg_surface_configure, 
};


/* Emit a display event from the Wayland event thread into the display's
 * event source.  Nothing is emitted when there are no listeners, which
 * conveniently suppresses the initial configure during al_create_display. */
static void wldpy_emit_display_event(ALLEGRO_DISPLAY *display, int type)
{
    ALLEGRO_EVENT_SOURCE *es = &display->es;

    _al_event_source_lock(es);
    if (_al_event_source_needs_to_generate_event(es)) {
        ALLEGRO_EVENT event;
        event.display.type = type;
        event.display.timestamp = al_get_time();
        event.display.x = 0;
        event.display.y = 0;
        event.display.width = display->w;
        event.display.height = display->h;
        _al_event_source_emit_event(es, &event);
    }
    _al_event_source_unlock(es);
}


/* Shared size-change handling for xdg_toplevel configure and libdecor
 * configure callbacks: record the compositor's requested size and emit an
 * ALLEGRO_EVENT_DISPLAY_RESIZE, which the app acknowledges. */
static void wldpy_handle_configure_size(ALLEGRO_DISPLAY_WAYLAND *d,
    int width, int height)
{
    /* 0x0 means the compositor has no preference; keep the current size. */
    if (width <= 0 || height <= 0)
        return;

    /* A configure echoing the pre-programmatic-resize size was generated
     * before the app requested the new size (eg. GNOME's delayed initial
     * configure); drop it so al_resize_display() isn't reverted. */
    if (d->programmatic_resize) {
        d->programmatic_resize = false;
        if (width == d->pre_resize_w && height == d->pre_resize_h)
            return;
    }

    d->pending_w = width;
    d->pending_h = height;

    /* Report a size change to the app, which will call al_acknowledge_resize
     * to actually apply it. */
    if (width != d->display.w || height != d->display.h) {
        ALLEGRO_EVENT_SOURCE *es = &d->display.es;
        _al_event_source_lock(es);
        if (_al_event_source_needs_to_generate_event(es)) {
            ALLEGRO_EVENT event;
            event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
            event.display.timestamp = al_get_time();
            event.display.x = 0;
            event.display.y = 0;
            event.display.width = width;
            event.display.height = height;
            _al_event_source_emit_event(es, &event);
        }
        _al_event_source_unlock(es);
    }
}


static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
    int32_t width, int32_t height, struct wl_array *states)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)data;
    (void)xdg_toplevel;
    (void)states;

    wldpy_handle_configure_size(d, width, height);
}


static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)data;
    (void)xdg_toplevel;

    wldpy_emit_display_event(&d->display, ALLEGRO_EVENT_DISPLAY_CLOSE);
}


static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};


/* libdecor frame callbacks.  libdecor dispatches its xdg events through the
 * same display/event queue that our background thread pumps, so these run on
 * the event thread exactly like the xdg handlers above. */

static void wldpy_frame_configure(struct libdecor_frame *frame,
    struct libdecor_configuration *configuration, void *data)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)data;
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    int w = 0, h = 0;

    if (libdecor_configuration_get_content_size(configuration, frame, &w, &h))
        wldpy_handle_configure_size(d, w, h);

    /* This is also the signal that the window is live: it wakes up the
     * thread blocked in wldpy_create_display_locked(). */
    d->configured = true;
    _al_cond_broadcast(&system->configured_cond);

    /* Apply the content size we are actually presenting and ack the
     * configure (libdecor_frame_commit() acks when given the
     * configuration). */
    struct libdecor_state *state =
        libdecor_state_new(d->display.w, d->display.h);
    libdecor_frame_commit(frame, state, configuration);
    libdecor_state_free(state);
}


static void wldpy_frame_close(struct libdecor_frame *frame, void *data)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)data;
    (void)frame;

    wldpy_emit_display_event(&d->display, ALLEGRO_EVENT_DISPLAY_CLOSE);
}


static void wldpy_frame_commit(struct libdecor_frame *frame, void *data)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)data;
    (void)frame;

    /* The decoration asked the main surface to be committed. */
    wl_surface_commit(d->surface);
}


/* Informational: reports what decoration mode the compositor settled on. */
static void xdg_toplevel_decoration_configure(void *data,
    struct zxdg_toplevel_decoration_v1 *decoration, uint32_t mode)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)data;
    (void)decoration;

    switch (mode) {
    case ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE:
        ALLEGRO_DEBUG("wldpy: using server-side decorations\n");
        d->server_side_decorated = true;
        break;
    case ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE:
    default:
        ALLEGRO_DEBUG("wldpy: no server-side decorations\n");
        d->server_side_decorated = false;
        break;
    }
}


static const struct zxdg_toplevel_decoration_v1_listener
    xdg_toplevel_decoration_listener = {
    .configure = xdg_toplevel_decoration_configure,
};

static struct libdecor_frame_interface wldpy_frame_interface = {
    .configure = wldpy_frame_configure,
    .close = wldpy_frame_close,
    .commit = wldpy_frame_commit,
};

static bool wldpy_create_display_window(ALLEGRO_SYSTEM_WAYLAND *system,
    ALLEGRO_DISPLAY_WAYLAND *d, int w, int h, int adapter)
{
    /* create the Wayland window now */
    d->surface = wl_compositor_create_surface(system->compositor);

    if (system->decor) {
        /* Decorate the content surface with libdecor, which creates and
         * manages the xdg_surface/xdg_toplevel itself. */
        d->frame = libdecor_decorate(system->decor, d->surface,
            &wldpy_frame_interface, d);
        if (!d->frame) {
            ALLEGRO_ERROR("libdecor_decorate failed\n");
            return false;
        }

        const char *title = al_get_new_window_title();
        if (title)
            libdecor_frame_set_title(d->frame, title);
        libdecor_frame_set_app_id(d->frame, al_get_app_name());
        libdecor_frame_set_min_content_size(d->frame, 1, 1);

        /* Present the requested content size and map the window. */
        struct libdecor_state *state = libdecor_state_new(w, h);
        libdecor_frame_commit(d->frame, state, NULL);
        libdecor_state_free(state);
        libdecor_frame_map(d->frame);
        wl_surface_commit(d->surface);
    }
    else {
        /* Fallback: bare xdg-toplevel without decorations. */
        d->xdg_surface = xdg_wm_base_get_xdg_surface(system->wm_base, d->surface);

        xdg_surface_add_listener(d->xdg_surface, &xdg_surface_listener, d);

        d->xdg_toplevel = xdg_surface_get_toplevel(d->xdg_surface);
        xdg_toplevel_add_listener(d->xdg_toplevel, &xdg_toplevel_listener, d);

        /* Request server-side decorations if the compositor offers them. */
        if (system->decoration_manager) {
            d->toplevel_decoration =
                zxdg_decoration_manager_v1_get_toplevel_decoration(
                    system->decoration_manager, d->xdg_toplevel);
            zxdg_toplevel_decoration_v1_add_listener(d->toplevel_decoration,
                &xdg_toplevel_decoration_listener, d);
            zxdg_toplevel_decoration_v1_set_mode(d->toplevel_decoration,
                ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        }

        /* new window position */

        /* todo: where to place window title? */
        const char *new_title = al_get_new_window_title();
        if (new_title)
            xdg_toplevel_set_title(d->xdg_toplevel, new_title);

        wl_surface_commit(d->surface);
    }

    return true;
}

static ALLEGRO_DISPLAY_WAYLAND *wldpy_create_display_locked(
    ALLEGRO_SYSTEM_WAYLAND *system, int flags, int w, int h, int adapter)
{
    ALLEGRO_DISPLAY_WAYLAND *d = al_calloc(1, sizeof *d);
    ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;
    ALLEGRO_OGL_EXTRAS *ogl = al_calloc(1, sizeof *ogl);
    display->ogl_extras = ogl;

    display->w = w;
    display->h = h;
    display->vt = _al_display_wayland_driver();
    display->refresh_rate = al_get_new_display_refresh_rate();
    display->flags = flags;
    display->flags |= ALLEGRO_OPENGL;

    ALLEGRO_DEBUG("selected adapter %i\n", adapter);
    if (adapter < 0)
        d->adapter = _al_wayland_get_default_adapter(system);
    else
        d->adapter = adapter;
    
    ALLEGRO_DEBUG("wldpy: selected adapter %i\n", d->adapter);

    /* Pick an EGL config for this display before creating anything. */
    _al_wlegl_config_select_visual(d);

    ALLEGRO_DISPLAY_WAYLAND **add;
    add = _al_vector_alloc_back(&system->system.displays);
    *add = d;

    _al_event_source_init(&display->es);

    if (!wldpy_create_display_window(system, d, w, h, adapter)) {
        /* not sure what to do here */
    }

    /* Wait for the compositor to configure the surface.  The
     * background event thread is the only one allowed to read from the
     * Wayland socket; it dispatches the configure event (which sets
     * d->configured and wakes us) so we block on the condition
     * variable rather than dispatching the socket directly.
     * dispatching anything already queued here as well, just in case
     * the event thread is not running. */
    while (!d->configured) {
        wl_display_dispatch_pending(system->display);
        wl_display_flush(system->display);
        _al_cond_wait(&system->configured_cond, &system->lock);
    }

    /* wl_output found after the configure wait */
    if (display->flags & ALLEGRO_FULLSCREEN) {
        wldpy_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, true);
    }

    return d;
}

static ALLEGRO_DISPLAY *wldpy_create_display(int w, int h)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d;
    ALLEGRO_DISPLAY *display;
    ALLEGRO_OGL_EXTRAS *ogl;
    int flags;
    int adapter;

    if (system->display == NULL) {
        ALLEGRO_WARN("Not connected to Wayland.\n");
        return NULL;
    }

    if (w <= 0 || h <= 0) {
        ALLEGRO_ERROR("Invalid window size %dx%d\n", w, h);
        return NULL;
    }

    flags = al_get_new_display_flags();

    _al_mutex_lock(&system->lock);
    adapter = al_get_new_display_adapter();
    d = wldpy_create_display_locked(system, flags, w, h, adapter);
    _al_mutex_unlock(&system->lock);
    if (!d)
        return NULL;

    display = (ALLEGRO_DISPLAY *)d;
    ogl = display->ogl_extras;

    /* Create the EGL context and window surface, then load the OpenGL
     * extensions.  This must happen *without* holding the system lock:
     * Mesa may do a wl_display_roundtrip() while creating the context and
     * surface, and that cannot make progress while the background event
     * thread is blocked waiting for the lock (it would deadlock). */
    if (!_al_wlegl_config_create_context(d)) {
        ALLEGRO_ERROR("Failed to create EGL context for display.\n");
        _al_mutex_lock(&system->lock);
        wldpy_free_display(display);
        _al_mutex_unlock(&system->lock);
        return NULL;
    }

    /* Load the OpenGL function pointers and extension API. */
    _al_ogl_manage_extensions(display);
    _al_ogl_set_extensions(ogl->extension_api);

    /* Print out OpenGL version info. */
    ALLEGRO_INFO("OpenGL Version: %s\n", (const char *)glGetString(GL_VERSION));
    ALLEGRO_INFO("Vendor: %s\n", (const char *)glGetString(GL_VENDOR));
    ALLEGRO_INFO("Renderer: %s\n", (const char *)glGetString(GL_RENDERER));

    /* Record the actual OpenGL version in the display settings. */
    const int v = display->ogl_extras->ogl_info.version;
    display->extra_settings.settings[ALLEGRO_OPENGL_MAJOR_VERSION] = (v >> 24) & 0xFF;
    display->extra_settings.settings[ALLEGRO_OPENGL_MINOR_VERSION] = (v >> 16) & 0xFF;

    /* Create the backbuffer (and the video bitmap machinery). */
    if (display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY])
        _al_ogl_setup_gl(display);

    return (ALLEGRO_DISPLAY *)d;
}

/* Free a display's resources and remove it from the system's display list.
 * The system lock must be held by the caller. */
static void wldpy_free_display(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_SYSTEM *sys = (ALLEGRO_SYSTEM *)system;
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;

    ALLEGRO_DEBUG("wldpy_free_display\n");

    /* Remove this display from the system's display list.  This is the
     * step that lets al_destroy_display() shrink the list so the
     * close-all-displays loop in wl_shutdown_system() terminates. */
    _al_vector_find_and_delete(&sys->displays, &display);

    /* Tear down the EGL context and surface. */
    if (d->egl_context) {
        eglMakeCurrent(system->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
            EGL_NO_CONTEXT);
        eglDestroyContext(system->egl_display, d->egl_context);
    }
    if (d->egl_surface) {
        eglDestroySurface(system->egl_display, d->egl_surface);
    }
    if (d->egl_window) {
        wl_egl_window_destroy(d->egl_window);
    }

    /* Tear down the Wayland shell objects. */
    if (d->frame)
        libdecor_frame_unref(d->frame);
    if (d->toplevel_decoration)
        zxdg_toplevel_decoration_v1_destroy(d->toplevel_decoration);
    if (d->xdg_toplevel)
        xdg_toplevel_destroy(d->xdg_toplevel);
    if (d->xdg_surface)
        xdg_surface_destroy(d->xdg_surface);
    if (d->surface)
        wl_surface_destroy(d->surface);

    _al_event_source_free(&display->es);

    al_free(display->ogl_extras);
    al_free(display);

    ALLEGRO_DEBUG("wldpy_free_display finished\n");
}


static void wldpy_destroy_display(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();

    ALLEGRO_DEBUG("wldpy_destroy_display\n");

    _al_mutex_lock(&system->lock);
    wldpy_free_display(display);
    _al_mutex_unlock(&system->lock);

    ALLEGRO_DEBUG("wldpy_destroy_display finished\n");
}

/* Make the display's EGL context current for the current thread. */
static bool wldpy_make_current(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;

    return eglMakeCurrent(system->egl_display, d->egl_surface,
        d->egl_surface, d->egl_context);
}


static bool wldpy_set_current_display(ALLEGRO_DISPLAY *display)
{
    bool rc;

    rc = wldpy_make_current(display);
    if (rc) {
        ALLEGRO_OGL_EXTRAS *ogl = display->ogl_extras;
        _al_ogl_set_extensions(ogl->extension_api);
        _al_ogl_update_render_state(display);
    }

    return rc;
}


static void wldpy_unset_current_display(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    (void)display;

    eglMakeCurrent(system->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
        EGL_NO_CONTEXT);
}


static void wldpy_flip_display(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;

    eglSwapBuffers(system->egl_display, d->egl_surface);
}


static void wldpy_update_display_region(ALLEGRO_DISPLAY *display,
    int x, int y, int w, int h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    wldpy_flip_display(display);
}


/* Apply a new size to the display: resize the EGL window (takes effect on
 * the next commit, flushed by the event thread), update the window geometry
 * hint, and resize the backbuffer.  Called from the app thread. */
static void wldpy_apply_size(ALLEGRO_DISPLAY *display, int w, int h)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;

    _al_mutex_lock(&system->lock);

    display->w = w;
    display->h = h;

    if (d->egl_window)
        wl_egl_window_resize(d->egl_window, w, h, 0, 0);
    if (d->xdg_surface)
        xdg_surface_set_window_geometry(d->xdg_surface, 0, 0, w, h);

    /* Tell libdecor about the new content size (also needed for
     * application-driven resizes); outside a configure callback, pass
     * a NULL configuration. */
    if (d->frame) {
        struct libdecor_state *state = libdecor_state_new(w, h);
        libdecor_frame_commit(d->frame, state, NULL);
        libdecor_state_free(state);
    }

    /* Resize the backbuffer and update its transformations. */
    if (display->ogl_extras->backbuffer)
        _al_ogl_setup_gl(display);

    _al_mutex_unlock(&system->lock);
}


static bool wldpy_resize_display(ALLEGRO_DISPLAY *display, int w, int h)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;

    ALLEGRO_DEBUG("wldpy: resize_display (%d, %d)\n", w, h);

    /* Compositors may send a configure echoing the old size that was
     * generated before this request arrived; remember the pre-resize size
     * so the configure handler can recognise and drop the stale event. */
    d->programmatic_resize = true;
    d->pre_resize_w = display->w;
    d->pre_resize_h = display->h;

    wldpy_apply_size(display, w, h);
    return true;
}


static bool wldpy_acknowledge_resize(ALLEGRO_DISPLAY *display)
{
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;

    /* Apply the size the compositor requested via xdg_toplevel configure. */
    if (d->pending_w != 0 && d->pending_h != 0
        && (d->pending_w != display->w || d->pending_h != display->h)) {
        int w = d->pending_w;
        int h = d->pending_h;
        ALLEGRO_DEBUG("wldpy: acknowledge_resize (%d, %d)\n", w, h);
        wldpy_apply_size(display, w, h);
    }

    return true;
}


static bool wldpy_is_compatible_bitmap(ALLEGRO_DISPLAY *display,
    ALLEGRO_BITMAP *bitmap)
{
    /* All EGL bitmaps are compatible. */
    (void)display;
    (void)bitmap;
    return true;
}

static void wldpy_set_window_title(ALLEGRO_DISPLAY *display, const char *title)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;

    /* libdecor owns the toplevel when active; otherwise we manage a bare
     * xdg-toplevel ourselves.  libdecor/GTK is not thread-safe, so these
     * calls must be serialized with the configure callback, which runs on
     * the event thread under system->lock. */
    _al_mutex_lock(&system->lock);
    if (d->frame)
        libdecor_frame_set_title(d->frame, title);
    else if (d->xdg_toplevel)
        xdg_toplevel_set_title(d->xdg_toplevel, title);
    _al_mutex_unlock(&system->lock);
}

static bool wldpy_set_display_flag(ALLEGRO_DISPLAY *display, int flag, bool onoff)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;
    bool ret = false;

    /* libdecor/GTK is not thread-safe, so this lock is necessary */
    _al_mutex_lock(&system->lock);

    switch (flag) {
        case ALLEGRO_FRAMELESS:
        {
            if (d->frame) {
                libdecor_frame_set_visibility(d->frame, !onoff);
                ret = true;
            }
            break;
        }
        case ALLEGRO_MAXIMIZED:
        {
            if (onoff) {
                if (d->frame)
                    libdecor_frame_set_maximized(d->frame);
                else if (d->xdg_toplevel)
                    xdg_toplevel_set_maximized(d->xdg_toplevel);
            } else {
                if (d->frame)
                    libdecor_frame_unset_maximized(d->frame);
                else if (d->xdg_toplevel)
                    xdg_toplevel_unset_maximized(d->xdg_toplevel);
            }
            ret = true;
            break;
        }
        /* On Wayland there is no mode-switching, so ALLEGRO_FULLSCREEN and
         * ALLEGRO_FULLSCREEN_WINDOW both just request fullscreen. */
        case ALLEGRO_FULLSCREEN:
        case ALLEGRO_FULLSCREEN_WINDOW:
        {
            if (onoff) {
                if (d->frame)
                    libdecor_frame_set_fullscreen(d->frame, NULL);
                else if (d->xdg_toplevel)
                    xdg_toplevel_set_fullscreen(d->xdg_toplevel, NULL);
            } else {
                if (d->frame)
                    libdecor_frame_unset_fullscreen(d->frame);
                else if (d->xdg_toplevel)
                    xdg_toplevel_unset_fullscreen(d->xdg_toplevel);
            }
            ret = true;
            break;
        }
    }

    _al_mutex_unlock(&system->lock);
    return ret;
}

static bool wldpy_set_window_constraints(ALLEGRO_DISPLAY *display, int min_w, int min_h, int max_w, int max_h)
{
    display->min_w = min_w;
    display->min_h = min_h;
    display->max_w = max_w;
    display->max_h = max_h;
    
    return true;
}

static bool wldpy_get_window_constraints(ALLEGRO_DISPLAY *display, int *min_w, int *min_h, int *max_w, int *max_h)
{
    /* Is the x11 implementation supposed to allow dereferencing potentially null ptrs? */
    if (min_w) *min_w = display->min_w;
    if (min_h) *min_h = display->min_h;
    if (max_w) *max_w = display->max_w;
    if (max_h) *max_h = display->max_h;

    return true;
}

static bool wldpy_apply_window_constraints(ALLEGRO_DISPLAY *display, bool onoff)
{
    ALLEGRO_SYSTEM_WAYLAND *system = (ALLEGRO_SYSTEM_WAYLAND *)al_get_system_driver();
    ALLEGRO_DISPLAY_WAYLAND *d = (ALLEGRO_DISPLAY_WAYLAND *)display;
    display->use_constraints = onoff;

    /* libdecor/GTK is not thread-safe, so this lock is necessary */
    _al_mutex_lock(&system->lock);

    /* Branchless... */
    if (d->frame) {
        libdecor_frame_set_min_content_size(d->frame, display->min_w * onoff, display->min_h * onoff);
        libdecor_frame_set_max_content_size(d->frame, display->max_w * onoff, display->max_h * onoff);
    } else if (d->xdg_toplevel) {
        xdg_toplevel_set_min_size(d->xdg_toplevel, display->min_w * onoff, display->min_h * onoff);
        xdg_toplevel_set_max_size(d->xdg_toplevel, display->max_w * onoff, display->max_h * onoff);
    }

    _al_mutex_unlock(&system->lock);
    /* This contains an implicit call to libdecor_commit_frame which resizes */
    al_resize_display(display, display->w, display->h);
    wldpy_emit_display_event(display, ALLEGRO_EVENT_DISPLAY_RESIZE);
}

/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_wayland_driver(void)
{
    if (wldpy_vt.create_display)
        return &wldpy_vt;

    wldpy_vt.create_display = wldpy_create_display;
    wldpy_vt.destroy_display = wldpy_destroy_display;
    wldpy_vt.set_current_display = wldpy_set_current_display;
    wldpy_vt.unset_current_display = wldpy_unset_current_display;
    wldpy_vt.flip_display = wldpy_flip_display;
    wldpy_vt.update_display_region = wldpy_update_display_region;
    wldpy_vt.acknowledge_resize = wldpy_acknowledge_resize;
    wldpy_vt.resize_display = wldpy_resize_display;
    wldpy_vt.create_bitmap = _al_ogl_create_bitmap;
    wldpy_vt.get_backbuffer = _al_ogl_get_backbuffer;
    wldpy_vt.set_target_bitmap = _al_ogl_set_target_bitmap;
    wldpy_vt.is_compatible_bitmap = wldpy_is_compatible_bitmap;
    wldpy_vt.update_render_state = _al_ogl_update_render_state;

    wldpy_vt.set_window_title = wldpy_set_window_title;
    wldpy_vt.set_display_flag = wldpy_set_display_flag;
    wldpy_vt.get_window_constraints = wldpy_get_window_constraints;
    wldpy_vt.set_window_constraints = wldpy_set_window_constraints;
    wldpy_vt.apply_window_constraints = wldpy_apply_window_constraints;

    _al_ogl_add_drawing_functions(&wldpy_vt);

    return &wldpy_vt;
}