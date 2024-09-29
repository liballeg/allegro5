#ifndef __al_included_allegro5_aintern_xmouse_h
#define __al_included_allegro5_aintern_xmouse_h

#include "allegro5/internal/aintern_mouse.h"

A5O_MOUSE_DRIVER *_al_xwin_mouse_driver(void);
void _al_xwin_mouse_button_press_handler(int button, A5O_DISPLAY *display);
void _al_xwin_mouse_button_release_handler(int button, A5O_DISPLAY *d);
void _al_xwin_mouse_motion_notify_handler(int x, int y, A5O_DISPLAY *d);
void _al_xwin_mouse_switch_handler(A5O_DISPLAY *display,
   const XCrossingEvent *event);
bool _al_xwin_grab_mouse(A5O_DISPLAY *display);
bool _al_xwin_ungrab_mouse(void);

#endif
