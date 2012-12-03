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
 *      Mouse cursors.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_system.h"


/* Function: al_create_mouse_cursor
 */
ALLEGRO_MOUSE_CURSOR *al_create_mouse_cursor(ALLEGRO_BITMAP *bmp,
   int x_focus, int y_focus)
{
   ALLEGRO_SYSTEM *sysdrv = al_get_system_driver();
   ASSERT(bmp);

   ASSERT(sysdrv->vt->create_mouse_cursor);
   return sysdrv->vt->create_mouse_cursor(bmp, x_focus, y_focus);
}


/* Function: al_destroy_mouse_cursor
 */
void al_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_SYSTEM *sysdrv;

   if (!cursor) {
      return;
   }

   sysdrv = al_get_system_driver();

   ASSERT(sysdrv->vt->destroy_mouse_cursor);
   sysdrv->vt->destroy_mouse_cursor(cursor);
}


/* Function: al_set_mouse_cursor
 */
bool al_set_mouse_cursor(ALLEGRO_DISPLAY *display, ALLEGRO_MOUSE_CURSOR *cursor)
{
   if (!cursor) {
      return false;
   }

   if (display) {
      ASSERT(display->vt->set_mouse_cursor);
      return display->vt->set_mouse_cursor(display, cursor);
   }

   return false;
}


/* Function: al_set_system_mouse_cursor
 */
bool al_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   /* XXX should you be able to set ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE? */
   ASSERT(cursor_id > ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE);
   ASSERT(cursor_id < ALLEGRO_NUM_SYSTEM_MOUSE_CURSORS);
   ASSERT(display);

   if (cursor_id <= ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE) {
      return false;
   }
   if (cursor_id > ALLEGRO_NUM_SYSTEM_MOUSE_CURSORS) {
      return false;
   }
   if (!display) {
      return false;
   }

   ASSERT(display->vt->set_system_mouse_cursor);
   return display->vt->set_system_mouse_cursor(display, cursor_id);
}


/* Function: al_show_mouse_cursor
 */
bool al_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   if (display) {
      ASSERT(display->vt->show_mouse_cursor);
      return display->vt->show_mouse_cursor(display);
   }

   return false;
}


/* Function: al_hide_mouse_cursor
 */
bool al_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   if (display) {
      ASSERT(display->vt->hide_mouse_cursor);
      return display->vt->hide_mouse_cursor(display);
   }

   return false;
}


/* vim: set sts=3 sw=3 et: */
