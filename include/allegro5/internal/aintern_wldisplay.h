#ifndef __al_included_allegro5_aintern_wldisplay_h
#define __al_included_allegro5_aintern_wldisplay_h

#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_wl.h"

/* ALLEGRO_DISPLAY with Wayland-specific data */
struct ALLEGRO_DISPLAY_WAYLAND {
    /* Needs to be first member */
    ALLEGRO_DISPLAY display;

    /* driver specifics */
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    int adapter;
    uint32_t configure_serial;

    bool configured;

    /* size the compositor most recently requested via xdg_toplevel
     * configure, not yet applied by al_acknowledge_resize */
    int pending_w, pending_h;

    /* libdecor frame wrapping this display's surface when windows are
     * decorated; NULL when falling back to a bare xdg-toplevel */
    struct libdecor_frame *frame;

    /* server-side decoration object, if the compositor supports it
     * (only used when libdecor is not active) */
    struct zxdg_toplevel_decoration_v1 *toplevel_decoration;
    bool server_side_decorated;

    /* True while a configure event may be pending that was generated
     * before the app called al_resize_display(); pre_resize_w/h is the
     * size before that request, used to recognise the stale event. */
    bool programmatic_resize;
    int pre_resize_w, pre_resize_h;

    /* EGL/OpenGL */
    struct wl_egl_window *egl_window;
    EGLSurface egl_surface;
    EGLContext egl_context;
    EGLConfig egl_config;
};

#endif