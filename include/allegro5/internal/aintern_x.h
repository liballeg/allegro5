#ifndef _AINTERN_X_H
#define _AINTERN_X_H

#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_mouse.h"

#include <X11/Xcursor/Xcursor.h>

typedef struct ALLEGRO_MOUSE_CURSOR_XWIN ALLEGRO_MOUSE_CURSOR_XWIN;

struct ALLEGRO_MOUSE_CURSOR_XWIN
{
   Cursor cursor;
};

ALLEGRO_KEYBOARD_DRIVER *_al_xwin_keyboard_driver(void);
ALLEGRO_MOUSE_DRIVER *_al_xwin_mouse_driver(void);
void _al_xwin_mouse_button_press_handler(int button, ALLEGRO_DISPLAY *display);
void _al_xwin_mouse_button_release_handler(int button, ALLEGRO_DISPLAY *d);
void _al_xwin_mouse_motion_notify_handler(int x, int y, ALLEGRO_DISPLAY *d);
void _al_xwin_add_cursor_functions(ALLEGRO_DISPLAY_INTERFACE *vt);
void _al_xwin_keyboard_handler(XKeyEvent *event, ALLEGRO_DISPLAY *display);
void _al_xwin_keyboard_handler_alternative(bool press, int hardware_keycode,
   uint32_t unichar, ALLEGRO_DISPLAY *display);
void _al_xwin_background_thread(_AL_THREAD *self, void *arg);
bool _al_xwin_grab_mouse(ALLEGRO_DISPLAY *display);
bool _al_xwin_ungrab_mouse(void);
void _al_xwin_set_size_hints(ALLEGRO_DISPLAY *d, int x_off, int y_off);
void _al_xwin_reset_size_hints(ALLEGRO_DISPLAY *d);
void _al_xwin_set_fullscreen_window(ALLEGRO_DISPLAY *display, int value);

#endif
