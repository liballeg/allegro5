#ifndef __al_included_allegro5_aintern_wlinput_h
#define __al_included_allegro5_aintern_wlinput_h

#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_mouse.h"

/* Called from the registry handler when a wl_seat global appears. */
void _al_wl_seat_add(ALLEGRO_SYSTEM_WAYLAND *s, struct wl_seat *seat);

/* Destroys the seat and its keyboard/pointer objects. */
void _al_wl_input_shutdown(ALLEGRO_SYSTEM_WAYLAND *s);

/* Emits held-key repeat events; called periodically by the event thread. */
void _al_wl_keyboard_repeat_tick(void);

ALLEGRO_KEYBOARD_DRIVER *_al_wl_keyboard_driver(void);
ALLEGRO_MOUSE_DRIVER *_al_wl_mouse_driver(void);

#endif