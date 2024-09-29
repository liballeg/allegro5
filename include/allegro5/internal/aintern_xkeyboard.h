#ifndef __al_included_allegro5_aintern_xkeyboard_h
#define __al_included_allegro5_aintern_xkeyboard_h

#include "allegro5/internal/aintern_keyboard.h"

A5O_KEYBOARD_DRIVER *_al_xwin_keyboard_driver(void);
void _al_xwin_keyboard_handler(XKeyEvent *event, A5O_DISPLAY *display);
void _al_xwin_keyboard_switch_handler(A5O_DISPLAY *display, bool focus_in);

#endif

/* vim: set sts=3 sw=3 et: */
