#ifndef __al_included_allegro5_aintern_wlsystem_h
#define __al_included_allegro5_aintern_wlsystem_h

#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/cursor-shape-client-protocol.h"
#include "allegro5/platform/xdg-shell-client-protocol.h"
#include "allegro5/platform/pointer-constraints-client-protocol.h"

/* ALLEGRO_SYSTEM with Wayland extra data */
struct ALLEGRO_SYSTEM_WAYLAND
{
    ALLEGRO_SYSTEM system;

    /* Driver specifics */

    /* One display, surfaces are derived from here */
    struct wl_display *display;
    struct wl_registry *registry;

    struct wl_compositor *compositor;
    struct xdg_wm_base *wm_base;
    struct wl_shm *shm;

    /* OpenGL stuff */
    EGLDisplay egl_display;

    /* for events */
    bool have_wlevents_thread;
    _AL_THREAD wlevents_thread;

    /* to access anything Wayland */
    _AL_MUTEX lock;

    /* libdecor context for window decorations; NULL if it failed to
     * initialise, which leaves windows undecorated */
    struct libdecor *decor;

    /* server-side window decorations, if the compositor offers them
     * (only used when libdecor is not active) */
    struct zxdg_decoration_manager_v1 *decoration_manager;

    /* signalled (while holding the lock) whenever a surface gets
     * configured by the compositor, so display creation can block
     * until the initial configure arrives */
    _AL_COND configured_cond;

    /* video adapters: one entry per wl_output, of struct ALLEGRO_WL_OUTPUT * */
    _AL_VECTOR outputs;

    /* core cursor-shape protocol, so we can control the pointer cursor */
    struct wp_cursor_shape_manager_v1 *cursor_shape_manager;

    /* pointer-constraints: used to emulate mouse warping (set_mouse_xy) via
     * a locked pointer + cursor position hint */
    struct zwp_pointer_constraints_v1 *pointer_constraints;

    struct wl_seat *seat;

    struct wl_data_device_manager *data_device_manager;

    struct xkb_context *xkb_context;
};

#endif