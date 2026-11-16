#ifndef __al_included_allegro5_aintern_wlinput_h
#define __al_included_allegro5_aintern_wlinput_h

#include <stddef.h>

#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_mouse.h"

/* A custom cursor is a wl_shm-backed surface with an Allegro hotspot. */
typedef struct ALLEGRO_MOUSE_CURSOR_WAYLAND {
    struct wl_surface *surface;
    struct wl_buffer *buffer;
    void *shm_data;
    size_t shm_size;
    int x_focus;
    int y_focus;
} ALLEGRO_MOUSE_CURSOR_WAYLAND;

/* Called from the registry handler when a wl_seat global appears. */
void _al_wl_seat_add(ALLEGRO_SYSTEM_WAYLAND *s, struct wl_seat *seat,
    uint32_t registry_name);
void _al_wl_seat_remove(ALLEGRO_SYSTEM_WAYLAND *s, uint32_t registry_name);

/* Destroys the seat and its keyboard/pointer objects. */
void _al_wl_input_shutdown(ALLEGRO_SYSTEM_WAYLAND *s);
/* Clear input focus before freeing a display; caller holds the system lock. */
void _al_wl_input_display_destroyed(ALLEGRO_DISPLAY *display);

/* Emits held-key repeat events; called periodically by the event thread. */
void _al_wl_keyboard_repeat_tick(void);

/* System cursor support via wp_cursor_shape_device_v1. */
bool _al_wl_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
    ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id);
ALLEGRO_MOUSE_CURSOR *_al_wl_create_mouse_cursor(ALLEGRO_BITMAP *bmp,
    int x_focus, int y_focus);
void _al_wl_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor);
bool _al_wl_set_mouse_cursor(ALLEGRO_DISPLAY *display,
    ALLEGRO_MOUSE_CURSOR *cursor);
bool _al_wl_show_mouse_cursor(ALLEGRO_DISPLAY *display);
bool _al_wl_hide_mouse_cursor(ALLEGRO_DISPLAY *display);

ALLEGRO_KEYBOARD_DRIVER *_al_wl_keyboard_driver(void);
ALLEGRO_MOUSE_DRIVER *_al_wl_mouse_driver(void);

#endif