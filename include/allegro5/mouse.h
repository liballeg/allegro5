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

/* Title: Mouse types
 */


#ifndef _al_included_mouse_h
#define _al_included_mouse_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Allow up to four extra axes for future expansion. */
#define ALLEGRO_MOUSE_MAX_EXTRA_AXES	 4


/* Abstract data type */
typedef struct ALLEGRO_MOUSE ALLEGRO_MOUSE;

/* Type: ALLEGRO_MOUSE_STATE
 */
typedef struct ALLEGRO_MOUSE_STATE
{
   /* (x, y) Primary mouse position
    * (z) Mouse wheel position (1D 'wheel'), or,
    * (w, z) Mouse wheel position (2D 'ball')
    * display - the display the mouse is on (coordinates are relative to this)
    */
   int x;
   int y;
   int z;
   int w;
   int more_axes[ALLEGRO_MOUSE_MAX_EXTRA_AXES];
   int buttons;
   struct ALLEGRO_DISPLAY *display;
} ALLEGRO_MOUSE_STATE;

/* Mouse cursors */
typedef struct ALLEGRO_MOUSE_CURSOR ALLEGRO_MOUSE_CURSOR;

typedef enum ALLEGRO_SYSTEM_MOUSE_CURSOR
{
   ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE        = 0,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW       = 1,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY        = 2,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION    = 3,
   ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT        = 4,
   ALLEGRO_NUM_SYSTEM_MOUSE_CURSORS
} ALLEGRO_SYSTEM_MOUSE_CURSOR;


AL_FUNC(bool,           al_is_mouse_installed,  (void));
AL_FUNC(bool,           al_install_mouse,       (void));
AL_FUNC(void,           al_uninstall_mouse,     (void));
AL_FUNC(ALLEGRO_MOUSE*, al_get_mouse,           (void));
AL_FUNC(unsigned int,   al_get_mouse_num_buttons, (void));
AL_FUNC(unsigned int,   al_get_mouse_num_axes,  (void));
AL_FUNC(bool,           al_set_mouse_xy,        (int x, int y));
    /* XXX: how is this going to work with multiple windows? 
     * Probably it will require an AL_DISPLAY parameter.
     */
AL_FUNC(bool,           al_set_mouse_z,         (int z));
AL_FUNC(bool,           al_set_mouse_w,         (int w));
AL_FUNC(bool,           al_set_mouse_axis,      (int axis, int value));
AL_FUNC(bool,           al_set_mouse_range,     (int x1, int y1, int x2, int y2));
AL_FUNC(void,           al_get_mouse_state,     (ALLEGRO_MOUSE_STATE *ret_state));
AL_FUNC(bool,           al_mouse_button_down,   (ALLEGRO_MOUSE_STATE *state, int button));
AL_FUNC(int,            al_get_mouse_state_axis, (ALLEGRO_MOUSE_STATE *state, int axis));


struct ALLEGRO_BITMAP;

/*
 * Cursors:
 *
 * This will probably become part of the display API.  It provides for
 * hardware cursors only; software cursors may or may not be provided
 * for later (it would need significant cooperation from the display
 * API).
 */
AL_FUNC(ALLEGRO_MOUSE_CURSOR *, al_create_mouse_cursor, (struct ALLEGRO_BITMAP *sprite, int xfocus, int yfocus));
AL_FUNC(void, al_destroy_mouse_cursor, (ALLEGRO_MOUSE_CURSOR *));
AL_FUNC(bool, al_set_mouse_cursor, (ALLEGRO_MOUSE_CURSOR *cursor));
AL_FUNC(bool, al_set_system_mouse_cursor, (ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id));
AL_FUNC(bool, al_show_mouse_cursor, (void));
AL_FUNC(bool, al_hide_mouse_cursor, (void));
AL_FUNC(bool, al_get_cursor_position, (int *ret_x, int *ret_y));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_MOUSE_H */

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
