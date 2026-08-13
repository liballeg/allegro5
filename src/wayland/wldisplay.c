#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_wleglconfig.h"
#include "allegro5/internal/aintern_wlfullscreen.h"
#include "allegro5/internal/aintern_wlsystem.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/platform/aintwl.h"

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE wldpy_vt;

static void wldpy_destroy_display(ALLEGRO_DISPLAY *display);
static void wldpy_free_display(ALLEGRO_DISPLAY *display);

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

static bool wldpy_create_display_window(ALLEGRO_SYSTEM_WAYLAND *system,
    ALLEGRO_DISPLAY_WAYLAND *d, int w, int h, int adapter)
{
    ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)d;

    /* create the Wayland window now */
    d->surface = wl_compositor_create_surface(system->compositor);
    d->xdg_surface = xdg_wm_base_get_xdg_surface(system->wm_base, d->surface);

    xdg_surface_add_listener(d->xdg_surface, &xdg_surface_listener, d);
    d->xdg_toplevel = xdg_surface_get_toplevel(d->xdg_surface);

    /* new window position */

    /* todo: where to place window title? */
    const char* new_title = al_get_new_window_title();
    if (new_title)
        xdg_toplevel_set_title(d->xdg_toplevel, new_title);

    wl_surface_commit(d->surface);

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

    /* fullscreen behavior? need to find "wl_output" first */
    if (display->flags & ALLEGRO_FULLSCREEN) {
        /* xdg_toplevel_set_fullscreen() */
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


static bool wldpy_acknowledge_resize(ALLEGRO_DISPLAY *display)
{
    /* Resize handling is not implemented yet; the window keeps its size. */
    (void)display;
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


/* Obtain a refernence to this driver. */
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
    wldpy_vt.create_bitmap = _al_ogl_create_bitmap;
    wldpy_vt.get_backbuffer = _al_ogl_get_backbuffer;
    wldpy_vt.set_target_bitmap = _al_ogl_set_target_bitmap;
    wldpy_vt.is_compatible_bitmap = wldpy_is_compatible_bitmap;
    wldpy_vt.update_render_state = _al_ogl_update_render_state;

    _al_ogl_add_drawing_functions(&wldpy_vt);

    return &wldpy_vt;
}