#ifndef __al_included_allegro5_aintern_wlsystem_h
#define __al_included_allegro5_aintern_wlsystem_h

#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/xdg-shell-client-protocol.h"

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

    bool have_wlevents_thread;
    _AL_THREAD wlevents_thread;
    _AL_MUTEX lock;

    struct wl_seat *seat;

    struct wl_data_device_manager *data_device_manager;

    struct xkb_context *xkb_context;
};

#endif