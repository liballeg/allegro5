#ifndef __al_included_allegro5_aintern_wlsystem_h
#define __al_included_allegro5_aintern_wlsystem_h

#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/cursor-shape-client-protocol.h"
#include "allegro5/platform/xdg-shell-client-protocol.h"
#include "allegro5/platform/pointer-constraints-client-protocol.h"
#ifdef ALLEGRO_WAYLAND_FRACTIONAL_SCALE
#include "allegro5/platform/fractional-scale-client-protocol.h"
#include "allegro5/platform/viewporter-client-protocol.h"
#endif
#ifdef ALLEGRO_WAYLAND_IDLE_INHIBIT
#include "allegro5/platform/idle-inhibit-client-protocol.h"
#endif

struct ALLEGRO_WL_CLIPBOARD_TRANSFER;

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
    bool egl_initialized;

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

#ifdef ALLEGRO_WAYLAND_FRACTIONAL_SCALE
    struct wp_fractional_scale_manager_v1 *fractional_scale_manager;
    struct wp_viewporter *viewporter;
#endif

    /* pointer-constraints: used to emulate mouse warping (set_mouse_xy) via
     * a locked pointer + cursor position hint */
    struct zwp_pointer_constraints_v1 *pointer_constraints;

    struct wl_seat *seat;
    uint32_t seat_registry_name;

    struct wl_data_device_manager *data_device_manager;
    uint32_t data_device_manager_registry_name;
    struct wl_data_device *data_device;
    void *clipboard_source_state;
    struct ALLEGRO_WL_CLIPBOARD_TRANSFER *clipboard_transfers;
    void *clipboard_offer;
    void *pending_clipboard_offer;
    uint32_t input_serial;

#ifdef ALLEGRO_WAYLAND_IDLE_INHIBIT
    struct zwp_idle_inhibit_manager_v1 *idle_inhibit_manager;
    bool inhibit_screensaver;
#endif

    struct xkb_context *xkb_context;
};

#endif