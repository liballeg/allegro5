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

    /* EGL/OpenGL */
    struct wl_egl_window *egl_window;
    EGLSurface egl_surface;
    EGLContext egl_context;
    EGLConfig egl_config;
};

#endif