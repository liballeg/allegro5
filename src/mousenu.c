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


#define ALLEGRO_NO_COMPATIBILITY

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern2.h"



/* the active keyboard driver */
static AL_MOUSE_DRIVER *new_mouse_driver = NULL;



/* al_install_mouse: [primary thread]
 *  Install a mouse driver. Returns true if successful. If a driver
 *  was already installed, nothing happens and true is returned.
 */
bool al_install_mouse(void)
{
   _DRIVER_INFO *driver_list;
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
      new_mouse_driver->name = new_mouse_driver->desc = get_config_text(new_mouse_driver->ascii_name);
      if (new_mouse_driver->init())
	 break;
   }

   if (!driver_list[i].driver) {
      new_mouse_driver = NULL;
      return false;
   }

   _add_exit_func(al_uninstall_mouse, "al_uninstall_mouse");

   return true;
}



/* al_uninstall_mouse: [primary thread]
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

   new_mouse_driver->exit();
   new_mouse_driver = NULL;

   _remove_exit_func(al_uninstall_mouse);
}



/* al_get_mouse:
 *  Return a pointer to an object representing the mouse, that can
 *  be used as an event source.
 */
AL_MOUSE *al_get_mouse(void)
{
   AL_MOUSE *mse;

   ASSERT(new_mouse_driver);

   mse = new_mouse_driver->get_mouse();
   ASSERT(mse);

   return mse;
}



/* al_get_mouse_num_buttons:
 *  Return the number of buttons on the mouse.
 */
unsigned int al_get_mouse_num_buttons(void)
{
   ASSERT(new_mouse_driver);

   return new_mouse_driver->get_mouse_num_buttons();
}

/* al_get_mouse_num_axes:
 *  Return the number of buttons on the mouse.
 */
unsigned int al_get_mouse_num_axes(void)
{
   ASSERT(new_mouse_driver);

   return new_mouse_driver->get_mouse_num_axes();
}



/* al_set_mouse_xy: [primary thread]
 *  Try to position the mouse at the given coordinates.
 *  Returns true on success, false on failure.
 *  XXX: This should be relative to an AL_DISPLAY.
 */
bool al_set_mouse_xy(int x, int y)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_xy);

   return new_mouse_driver->set_mouse_xy(x, y);
}



/* al_set_mouse_z: [primary thread]
 *  Set the mouse wheel position to the given value.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_z(int z)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_axis);

   return new_mouse_driver->set_mouse_axis(2, z);
}

/* al_set_mouse_w: [primary thread]
 *  Set the mouse wheel position to the given value.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_w(int w)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_axis);

   return new_mouse_driver->set_mouse_axis(3, w);
}

/* al_set_mouse_range: [primary thread]
 *  Sets the area of the screen within which the mouse can move.
 *  The coordinates are inclusive. (XXX: change this?)
 *  XXX: This should be relative to an AL_DISPLAY.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_range(int x1, int y1, int x2, int y2)
{
   ASSERT(new_mouse_driver);
   ASSERT(new_mouse_driver->set_mouse_range);

   return new_mouse_driver->set_mouse_range(x1, y1, x2, y2);
}



/* al_get_mouse_state: [primary thread]
 *  Save the state of the mouse specified at the time the function
 *  is called into the structure pointed to by RET_STATE.
 */
void al_get_mouse_state(AL_MSESTATE *ret_state)
{
   ASSERT(new_mouse_driver);
   ASSERT(ret_state);

   new_mouse_driver->get_state(ret_state);
}

/* al_get_state_axis: 
 *  Extract the mouse axis value from the saved state.
 */
int al_mouse_state_axis(AL_MSESTATE *ret_state, int axis)
{
   ASSERT(ret_state);
   ASSERT(axis >= 0);
   ASSERT(axis < (sizeof(ret_state->more_axes) / sizeof(*ret_state->more_axes)) + 4);
   switch(axis)
   {
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



/* al_mouse_button_down:
 *  Return true if the mouse button specified was held down in the state
 *  specified.
 */
bool al_mouse_button_down(AL_MSESTATE *state, int button)
{
   ASSERT(state);
   ASSERT(button > 0);

   return (state->buttons & (1 << (button-1)));
}



/*****************************************************************************
 *      Cursor API
 *****************************************************************************/


/* al_create_mouse_cursor:
 *  Create a mouse cursor from the bitmap provided.  There must be a
 *  graphics driver in effect.
 *  Returns a pointer to the cursor on success, or NULL on failure.
 */
AL_MOUSE_CURSOR *al_create_mouse_cursor(BITMAP *bmp, int x_focus, int y_focus)
{
   ASSERT(gfx_driver);
   ASSERT(bmp);

   if ((gfx_driver) && (gfx_driver->create_mouse_cursor))
      return gfx_driver->create_mouse_cursor(bmp, x_focus, y_focus);

   return NULL;
}



/* al_destroy_mouse_cursor:
 *  Free the memory used by the given cursor.  The graphics driver that
 *  was in effect when the cursor was created must still be in effect.
 */
void al_destroy_mouse_cursor(AL_MOUSE_CURSOR *cursor)
{
   ASSERT(gfx_driver);

   if (!cursor)
      return;

   if ((gfx_driver) && (gfx_driver->destroy_mouse_cursor))
      gfx_driver->destroy_mouse_cursor(cursor);
}



/* al_set_mouse_cursor:
 *  Set the given mouse cursor to be the current mouse cursor.  The
 *  graphics driver that was in effect when the cursor was created
 *  must still be in effect.  If the cursor is currently 'shown' (as
 *  opposed to 'hidden') the change is immediately visible.
 *  Returns true on success, false on failure.
 */
bool al_set_mouse_cursor(AL_MOUSE_CURSOR *cursor)
{
   ASSERT(gfx_driver);
   ASSERT(cursor);

   if ((gfx_driver) && (gfx_driver->set_mouse_cursor))
      return gfx_driver->set_mouse_cursor(cursor);

   return false;
}



/* al_set_system_mouse_cursor:
 *  Set the given system mouse cursor to be the current mouse cursor.
 *  The graphics driver that was in effect when the cursor was created
 *  must still be in effect.  If the cursor is currently 'shown' (as
 *  opposed to 'hidden') the change is immediately visible.
 *  Returns true on success, false on failure.
 */
bool al_set_system_mouse_cursor(AL_SYSTEM_MOUSE_CURSOR cursor_id)
{
   ASSERT(gfx_driver);

   if ((gfx_driver) && (gfx_driver->set_system_mouse_cursor))
      return gfx_driver->set_system_mouse_cursor(cursor_id);

   return false;
}



/* al_show_mouse_cursor:
 *  Make the current mouse cursor visible.  By default the mouse
 *  cursor is hidden.  A graphics driver must be in effect.
 *  Returns true on success, false on failure.
 */
bool al_show_mouse_cursor(void)
{
   ASSERT(gfx_driver);

   if ((gfx_driver) && (gfx_driver->show_mouse_cursor))
      return gfx_driver->show_mouse_cursor();

   return false;
}



/* al_hide_mouse_cursor:
 *  Hide the current mouse cursor.  This has no effect on what the
 *  current mouse cursor looks like; it just makes it disappear.
 *  A graphics driver must be in effect.
 *  Returns true on success, false on failure.
 */
bool al_hide_mouse_cursor(void)
{
   ASSERT(gfx_driver);

   if ((gfx_driver) && (gfx_driver->hide_mouse_cursor))
      return gfx_driver->hide_mouse_cursor();

   return false;
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
