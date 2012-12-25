#ifndef __al_included_allegro5_aintern_xkeyboard_h
#define __al_included_allegro5_aintern_xkeyboard_h

#include "allegro5/internal/aintern_keyboard.h"

ALLEGRO_KEYBOARD_DRIVER *_al_xwin_keyboard_driver(void);
void _al_xwin_keyboard_handler(XKeyEvent *event, ALLEGRO_DISPLAY *display);
void _al_xwin_keyboard_switch_handler(ALLEGRO_DISPLAY *display, bool focus_in);
void _al_xwin_keyboard_handler_alternative(bool press, int hardware_keycode,
   uint32_t unichar, ALLEGRO_DISPLAY *display);

#endif

/* vim: set sts=3 sw=3 et: */
