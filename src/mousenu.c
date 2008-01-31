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
 *      New mouse API.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Mouse routines
 */


#define ALLEGRO_NO_COMPATIBILITY

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_bitmap.h"



/* the active keyboard driver */
static ALLEGRO_MOUSE_DRIVER *new_mouse_driver = NULL;



/* Function: al_is_mouse_installed
 *  Returns true if <al_install_mouse> was called successfully.
 */
bool al_is_mouse_installed(void)
{
   return new_mouse_driver;
}



/* Function: al_install_mouse
 *  Install a mouse driver. Returns true if successful. If a driver
 *  was already installed, nothing happens and true is returned.
 */
bool al_install_mouse(void)
{
   _DRIVER_INFO *driver_list;
   const char *name;
   int i;

   if (new_mouse_driver)
      return true;

   if (system_driver->mouse_drivers)
      driver_list = system_driver->mouse_drivers();
   else
      driver_list = _al_mouse_driver_list;

   ASSERT(driver_list);

   for (i=0; driver_list[i].driver; i++) {
      new_mouse_driver = driver_list[i].driver;
      name = get_config_text(new_mouse_driver->msedrv_ascii_name);
      new_mouse_driver->msedrv_name = name;
      new_mouse_driver->msedrv_desc = name;
      if (new_mouse_driver->init_mouse()) {
	 break;
      }
   }

   if (!driver_list[i].driver) {
      new_mouse_driver = NULL;
      return false;
   }

   _add_exit_func(al_uninstall_mouse, "al_uninstall_mouse");

   return true;
}



/* Function: al_uninstall_mouse
 *  Uninstalls the active mouse driver, if any.  This will
 *  automatically unregister the mouse event source with any event
 *  queues.
 *
 *  This function is automatically called when Allegro is shut down.
 */
void al_uninstall_mouse(void)
{
   if (!new_mouse_driver)
      return;

   new_mouse_driver->exit_mouse();
   new_mouse_driver = NULL;

   _remove_exit_func(al_uninstall_mouse);
}



/* Function: al_get_mouse
 *  Return a pointer to an object representing the mouse, that can
 *  be used as an event source.
 */
ALLEGRO_MOUSE *al_get_mouse(void)
{
   ALLEGRO_MOUSE *mse;

   ASSERT(new_mouse_driver);

   mse = new_mouse_driver->get_mouse();
   ASSERT(mse);

   return mse;
}



/* Function: al_get_mouse_num_buttons
 *  Return the number of buttons on the mouse.
 */
unsigned int al_get_mouse_num_buttons(void)
{
   ASSERT(new_mouse_driver);

   return new_mouse_driver->get_mouse_num_buttons();
}



/* Function: al_get_mouse_num_axes
 *  Return the number of buttons on the mouse.
 */
unsigned int al_get_mouse_num_axes(void)
{
   ASSERT(new_mouse_driver);

   return new_mouse_driver->get_mouse_num_axes();
}



/* Function: al_set_mouse_xy
 *  Try to position the mouse at the given coordinates.
 *  Returns true on success, false on failure.
 *  XXX: This should be relative to an ALLEGRO_DISPLAY.
 */
bool al_set_mouse_xy(int x, int y)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_xy);

   return new_mouse_driver->set_mouse_xy(x, y);
}



/* Function: al_set_mouse_z
 *  Set the mouse wheel position to the given value.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_z(int z)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_axis);

   return new_mouse_driver->set_mouse_axis(2, z);
}



/* Function: al_set_mouse_w
 *  Set the mouse wheel position to the given value.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_w(int w)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_axis);

   return new_mouse_driver->set_mouse_axis(3, w);
}



/* Function: al_set_mouse_axis
 *  Set the given mouse axis to the given value.
 *  Returns true on success, false on failure.
 *
 *  For now: the axis number must not be 0 or 1, which are the X and Y axes.
 */
bool al_set_mouse_axis(int which, int value)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_axis);
   ASSERT(which >= 2);
   ASSERT(which < 4 + ALLEGRO_MOUSE_MAX_EXTRA_AXES);

   return new_mouse_driver->set_mouse_axis(which, value);
}



/* Function: al_set_mouse_range
 *  Sets the area of the screen within which the mouse can move.
 *  The coordinates are inclusive. (XXX: change this?)
 *  XXX: This should be relative to an ALLEGRO_DISPLAY.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_range(int x1, int y1, int x2, int y2)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_range);

   return new_mouse_driver->set_mouse_range(x1, y1, x2, y2);
}



/* Function: al_get_mouse_state
 *  Save the state of the mouse specified at the time the function
 *  is called into the structure pointed to by RET_STATE.
 */
void al_get_mouse_state(ALLEGRO_MSESTATE *ret_state)
{
   ASSERT(new_mouse_driver);
   ASSERT(ret_state);

   new_mouse_driver->get_mouse_state(ret_state);
}



/* Function: al_get_state_axis
 *  Extract the mouse axis value from the saved state.
 */
int al_mouse_state_axis(ALLEGRO_MSESTATE *ret_state, int axis)
{
   ASSERT(ret_state);
   ASSERT(axis >= 0);
   ASSERT(axis < (4 + ALLEGRO_MOUSE_MAX_EXTRA_AXES));

   switch (axis) {
      case 0:
         return ret_state->x;
      case 1:
         return ret_state->y;
      case 2:
         return ret_state->z;
      case 3:
         return ret_state->w;
      default:
         return ret_state->more_axes[axis - 4];
   }
}



/* Function: al_mouse_button_down
 *  Return true if the mouse button specified was held down in the state
 *  specified.
 */
bool al_mouse_button_down(ALLEGRO_MSESTATE *state, int button)
{
   ASSERT(state);
   ASSERT(button > 0);

   return (state->buttons & (1 << (button-1)));
}



/*****************************************************************************
 *      Cursor API
 *****************************************************************************/


ALLEGRO_MOUSE_CURSOR *al_create_mouse_cursor_old(BITMAP *bmp, int x_focus, int y_focus)
{
   ASSERT(gfx_driver);
   ASSERT(bmp);

   if ((gfx_driver) && (gfx_driver->create_mouse_cursor))
      return gfx_driver->create_mouse_cursor(bmp, x_focus, y_focus);

   return NULL;
}


/* Function: al_create_mouse_cursor
 *  Create a mouse cursor from the bitmap provided.  There must be a
 *  graphics driver in effect.
 *  Returns a pointer to the cursor on success, or NULL on failure.
 */
ALLEGRO_MOUSE_CURSOR *al_create_mouse_cursor(ALLEGRO_BITMAP *bmp, int x_focus, int y_focus)
{
   ASSERT(gfx_driver);
   ASSERT(bmp);

   /* Convert to BITMAP */
   BITMAP *oldbmp = create_bitmap_ex(32,
      al_get_bitmap_width(bmp), al_get_bitmap_height(bmp));
   int x, y;
   for (y = 0; y < oldbmp->h; y++) {
      for (x = 0; x < oldbmp->w; x++) {
         ALLEGRO_COLOR color = al_get_pixel(bmp, x, y);
         unsigned char r, g, b, a;
         al_unmap_rgba(color, &r, &g, &b, &a);
         int oldcolor;
         if (a == 0)
            oldcolor = makecol32(255, 0, 255);
         else
            oldcolor = makecol32(r, g, b);
         putpixel(oldbmp, x, y, oldcolor);
      }
   }

   ALLEGRO_MOUSE_CURSOR *result = al_create_mouse_cursor_old(oldbmp, x_focus, y_focus);

   destroy_bitmap(oldbmp);

   return result;
}



/* Function: al_destroy_mouse_cursor
 *  Free the memory used by the given cursor.  The graphics driver that
 *  was in effect when the cursor was created must still be in effect.
 */
void al_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
   ASSERT(gfx_driver);

   if (!cursor)
      return;

   if ((gfx_driver) && (gfx_driver->destroy_mouse_cursor))
      gfx_driver->destroy_mouse_cursor(cursor);
}



/* Function: al_set_mouse_cursor
 *  Set the given mouse cursor to be the current mouse cursor.  The
 *  graphics driver that was in effect when the cursor was created
 *  must still be in effect.  If the cursor is currently 'shown' (as
 *  opposed to 'hidden') the change is immediately visible.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
   ASSERT(gfx_driver);
   ASSERT(cursor);

   if ((gfx_driver) && (gfx_driver->set_mouse_cursor))
      return gfx_driver->set_mouse_cursor(cursor);

   return false;
}



/* Function: al_set_system_mouse_cursor
 *  Set the given system mouse cursor to be the current mouse cursor.
 *  The graphics driver that was in effect when the cursor was created
 *  must still be in effect.  If the cursor is currently 'shown' (as
 *  opposed to 'hidden') the change is immediately visible.
 *  Returns true on success, false on failure.
 */
bool al_set_system_mouse_cursor(ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   ASSERT(gfx_driver);

   if ((gfx_driver) && (gfx_driver->set_system_mouse_cursor))
      return gfx_driver->set_system_mouse_cursor(cursor_id);

   return false;
}



/* Function: al_show_mouse_cursor
 *  Make a mouse cursor visible in the current display of the calling thread.
 *  Returns true if a mouse cursor is shown as a result of the call (or one
 *  already was visible), false otherwise.
 */
bool al_show_mouse_cursor(void)
{
   return _al_current_display->vt->show_cursor(_al_current_display);
}



/* Function: al_hide_mouse_cursor
 *  Hide the mouse cursor in the current display of the calling thread. This has
 *  no effect on what the current mouse cursor looks like; it just makes it
 *  disappear.
 *  Returns true on success (or if the cursor already was hidden), false
 *  otherwise.
 */
bool al_hide_mouse_cursor(void)
{
   return _al_current_display->vt->hide_cursor(_al_current_display);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
