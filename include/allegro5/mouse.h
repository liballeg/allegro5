/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Mouse routines.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __al_included_allegro5_mouse_h
#define __al_included_allegro5_mouse_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Allow up to four extra axes for future expansion. */
#define ALLEGRO_MOUSE_MAX_EXTRA_AXES	 4


typedef struct ALLEGRO_MOUSE ALLEGRO_MOUSE;


/* Type: ALLEGRO_MOUSE_STATE
 */
typedef struct ALLEGRO_MOUSE_STATE ALLEGRO_MOUSE_STATE;

struct ALLEGRO_MOUSE_STATE
{
   /* (x, y) Primary mouse position
    * (z) Mouse wheel position (1D 'wheel'), or,
    * (w, z) Mouse wheel position (2D 'ball')
    * display - the display the mouse is on (coordinates are relative to this)
    * pressure - the pressure appleid to the mouse (for stylus/tablet)
    */
   int x;
   int y;
   int z;
   int w;
   int more_axes[ALLEGRO_MOUSE_MAX_EXTRA_AXES];
   int buttons;
   float pressure;
   struct ALLEGRO_DISPLAY *display;
};


/* Mouse cursors */
typedef struct ALLEGRO_MOUSE_CURSOR ALLEGRO_MOUSE_CURSOR;

typedef enum ALLEGRO_SYSTEM_MOUSE_CURSOR
{
   ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE        =  0,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT     =  1,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW       =  2,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY        =  3,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION    =  4,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT        =  5,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE        =  6,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N    =  7,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_W    =  8,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_S    =  9,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E    = 10,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW   = 11,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SW   = 12,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SE   = 13,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE   = 14,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_PROGRESS    = 15,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_PRECISION   = 16,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_LINK        = 17,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_ALT_SELECT  = 18,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_UNAVAILABLE = 19,
   ALLEGRO_NUM_SYSTEM_MOUSE_CURSORS
} ALLEGRO_SYSTEM_MOUSE_CURSOR;


AL_FUNC(bool,           al_is_mouse_installed,  (void));
AL_FUNC(bool,           al_install_mouse,       (void));
AL_FUNC(void,           al_uninstall_mouse,     (void));
AL_FUNC(unsigned int,   al_get_mouse_num_buttons, (void));
AL_FUNC(unsigned int,   al_get_mouse_num_axes,  (void));
AL_FUNC(bool,           al_set_mouse_xy,        (struct ALLEGRO_DISPLAY *display, int x, int y));
AL_FUNC(bool,           al_set_mouse_z,         (int z));
AL_FUNC(bool,           al_set_mouse_w,         (int w));
AL_FUNC(bool,           al_set_mouse_axis,      (int axis, int value));
AL_FUNC(void,           al_get_mouse_state,     (ALLEGRO_MOUSE_STATE *ret_state));
AL_FUNC(bool,           al_mouse_button_down,   (const ALLEGRO_MOUSE_STATE *state, int button));
AL_FUNC(int,            al_get_mouse_state_axis, (const ALLEGRO_MOUSE_STATE *state, int axis));

AL_FUNC(ALLEGRO_EVENT_SOURCE *, al_get_mouse_event_source, (void));

struct ALLEGRO_BITMAP;

/*
 * Cursors:
 *
 * This will probably become part of the display API.  It provides for
 * hardware cursors only; software cursors may or may not be provided
 * for later (it would need significant cooperation from the display
 * API).
 */
AL_FUNC(ALLEGRO_MOUSE_CURSOR *, al_create_mouse_cursor, (
        struct ALLEGRO_BITMAP *sprite, int xfocus, int yfocus));
AL_FUNC(void, al_destroy_mouse_cursor, (ALLEGRO_MOUSE_CURSOR *));
AL_FUNC(bool, al_set_mouse_cursor, (struct ALLEGRO_DISPLAY *display,
                                    ALLEGRO_MOUSE_CURSOR *cursor));
AL_FUNC(bool, al_set_system_mouse_cursor, (struct ALLEGRO_DISPLAY *display,
                                           ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id));
AL_FUNC(bool, al_show_mouse_cursor, (struct ALLEGRO_DISPLAY *display));
AL_FUNC(bool, al_hide_mouse_cursor, (struct ALLEGRO_DISPLAY *display));
AL_FUNC(bool, al_get_mouse_cursor_position, (int *ret_x, int *ret_y));
AL_FUNC(bool, al_grab_mouse, (struct ALLEGRO_DISPLAY *display));
AL_FUNC(bool, al_ungrab_mouse, (void));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
