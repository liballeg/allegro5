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
 *      New display driver.
 *
 *      By Elias Pschernig.
 *
 *      Modified by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Display routines
 */



#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"



static int current_video_adapter = 0;
static int new_window_x = INT_MAX;
static int new_window_y = INT_MAX;

/* Function: al_create_display
 *
 * Create a display, or window, with the specified dimensions.
 * The parameters of the display are determined by the last calls to
 * al_set_new_display_*. Default parameters are used if none are set
 * explicitly.
 * Creating a new display will automatically make it the active one, with the
 * backbuffer selected for drawing. 
 *
 * Returns NULL on error.
 *
 * See Also: <al_set_new_display_format>, <al_set_new_display_refresh_rate>, 
 * 	<al_set_new_display_flags>
 */
ALLEGRO_DISPLAY *al_create_display(int w, int h)
{
   // FIXME: We need to ask the system driver for a list of possible display
   // drivers here, then select a suitable one according to configuration
   // variables like "display/driver" and according to flags (e.g. OpenGL
   // requested or not).

   // Right now, the X11 driver is hardcoded.

   ALLEGRO_SYSTEM *system = al_system_driver();
   ALLEGRO_DISPLAY_INTERFACE *driver = system->vt->get_display_driver();
   ALLEGRO_DISPLAY *display = driver->create_display(w, h);

   if (!display)
      return NULL;

   _al_vector_init(&display->bitmaps, sizeof(ALLEGRO_BITMAP*));

   {
   ALLEGRO_COLOR black = al_map_rgb(0, 0, 0);
   al_set_current_display(display);
   al_set_target_bitmap(al_get_backbuffer());
   al_clear(black);
   al_flip_display();
   }

   return display;
}



/* Function: al_destroy_display
 *
 * Destroy a display.
 */
void al_destroy_display(ALLEGRO_DISPLAY *display)
{
   display->vt->destroy_display(display);
}



/* Function: al_get_backbuffer
 *
 * Return a special bitmap representing the back-buffer of the
 * current display.
 */
ALLEGRO_BITMAP *al_get_backbuffer(void)
{
   ASSERT(_al_current_display);

   return _al_current_display->vt->get_backbuffer(_al_current_display);
}



/* Function: al_get_frontbuffer
 *
 * Return a special bitmap representing the front-buffer of
 * the current display. This may not be supported by the driver;
 * returns NULL in that case.
 */
ALLEGRO_BITMAP *al_get_frontbuffer(void)
{
   ASSERT(_al_current_display);

   return _al_current_display->vt->get_frontbuffer(_al_current_display);
}



/* Function: al_flip_display
 *
 * Copies or updates the front and back buffers so that what has
 * been drawn previously on the currently selected display becomes
 * visible on screen. Pointers to the special back and front buffer
 * bitmaps remain valid and retain their semantics as back and front
 * buffers respectively, although their contents may have changed.
 */
void al_flip_display(void)
{
   ASSERT(_al_current_display);

   _al_current_display->vt->flip_display(_al_current_display);
}



/* Function: al_update_display_region
 *
 * Update the the front buffer from the backbuffer in the
 * specified region. This does not flip the whole buffer
 * and preserves the contents of the front buffer outside of
 * the given rectangle. This may not be supported by all drivers,
 * in which case it returns false.
 */
bool al_update_display_region(int x, int y, int width, int height)
{
   ASSERT(_al_current_display);

   return _al_current_display->vt->update_display_region(
      _al_current_display, x, y, width, height);
}



/* Function: al_acknowledge_resize
 *
 * When the user receives a resize event from a resizable display,
 * if they wish the display to be resized they must call this
 * function to let the graphics driver know that it can now resize
 * the display. Returns true on success.
 *
 * Adjusts the clipping rectangle to the full size of the backbuffer.
 */
bool al_acknowledge_resize(ALLEGRO_DISPLAY *display)
{
   ASSERT(display);

   if (!(display->flags & ALLEGRO_FULLSCREEN)) {
      if (display->vt->acknowledge_resize)
         return display->vt->acknowledge_resize(display);
   }
   return false;
}



/* Function: al_resize_display
 *
 * Resize the current display. Returns true on success, or false on error.
 * This works on both fullscreen and windowed displays, regardless of the
 * ALLEGRO_RESIZABLE flag.
 *
 * Adjusts the clipping rectangle to the full size of the backbuffer.
 */
bool al_resize_display(int width, int height)
{
   ASSERT(_al_current_display);

   if (_al_current_display->vt->resize_display) {
      return _al_current_display->vt->resize_display(_al_current_display,
         width, height);
   }
   return false;
}



/* Function: al_clear
 *
 * Clear a complete display, but confined by the clipping rectangle.
 */
void al_clear(ALLEGRO_COLOR color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   ASSERT(target);
   ASSERT(_al_current_display);

   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_clear_memory(&color);
   }
   else {
      _al_current_display->vt->clear(_al_current_display, &color);
   }
}



/* Function: al_draw_line
 *
 * Draws a line to the current target.
 *
 * fx - from x
 * fy - from y
 * tx - to x
 * ty - to y
 * color - can be obtained with al_map_*
 */
void al_draw_line(float fx, float fy, float tx, float ty,
   ALLEGRO_COLOR color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   ASSERT(target);
   ASSERT(_al_current_display);

   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_line_memory(fx, fy, tx, ty, &color);
   }
   else {
      _al_current_display->vt->draw_line(_al_current_display,
         fx, fy, tx, ty, &color);
   }
}



/* Function: al_draw_rectangle
 *
 * Draws a rectangle to the current target.
 *
 * tlx - top left x
 * tly - top left y
 * brx - bottom right x
 * bry - bottom right y
 *
 * flags can be:
 *
 * ALLEGRO_FILLED - fill the interior of the rectangle
 * ALLEGRO_OUTLINED - draw only the rectangle borders
 * 
 * Outlined is the default.
 */
void al_draw_rectangle(float tlx, float tly, float brx, float bry,
   ALLEGRO_COLOR color, int flags)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   ASSERT(target);
   ASSERT(_al_current_display);

   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_rectangle_memory(tlx, tly, brx, bry, &color, flags);
   }
   else {
      _al_current_display->vt->draw_rectangle(_al_current_display,
         tlx, tly, brx, bry, &color, flags);
   }
}


/* Function: al_draw_pixel
 *
 * Draws a single pixel at x, y. This function, unlike
 * al_put_pixel, does blending.
 *
 * x - destination x
 * y - destination y
 * color - color of the pixel
 */
void al_draw_pixel(float x, float y, ALLEGRO_COLOR color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   ASSERT(target);
   ASSERT(_al_current_display);

   if (target->flags & ALLEGRO_MEMORY_BITMAP
         || !_al_current_display->vt->draw_pixel) {
      _al_draw_pixel_memory(x, y, &color);
   }
   else {
      _al_current_display->vt->draw_pixel(_al_current_display,
         x, y, &color);
   }
}




/* Function: al_is_compatible_bitmap
 *
 * D3D and OpenGL allow sharing a texture in a way so it can be used for
 * multiple windows. Each ALLEGRO_BITMAP created with <al_create_bitmap>
 * however is usually tied to a single ALLEGRO_DISPLAY. This function can
 * be used to know if the bitmap is compatible with the current display,
 * even if it is another display than the one it was created with. It
 * returns true if the bitmap is compatible (things like a cached texture
 * version can be used) and false otherwise (blitting in the current
 * display will be slow). The only time this function is useful is if you
 * are using multiple windows and need accelerated blitting of the same
 * bitmaps to both. 
 */
bool al_is_compatible_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ASSERT(bitmap);
   ASSERT(_al_current_display);

   return _al_current_display->vt->is_compatible_bitmap(
      _al_current_display, bitmap);
}



/* Function: al_get_display_width
 *
 * Gets the width of the current display. This is like SCREEN_W in Allegro 4.x.
 */
int al_get_display_width(void)
{
   return _al_current_display->w;
}



/* Function: al_get_display_height
 *
 * Gets the height of the current display. This is like SCREEN_H in Allegro 4.x.
 */
int al_get_display_height(void)
{
   ASSERT(_al_current_display);

   return _al_current_display->h;
}



/* Function: al_get_display_format
 *
 * Gets the pixel format of the current display.
 */
int al_get_display_format(void)
{
   ASSERT(_al_current_display);

   return _al_current_display->format;
}



/* Function: al_get_display_refresh_rate
 *
 * Gets the refresh rate of the current display.
 */
int al_get_display_refresh_rate(void)
{
   ASSERT(_al_current_display);

   return _al_current_display->refresh_rate;
}



/* Function: al_get_display_flags
 *
 * Gets the flags of the current display.
 */
int al_get_display_flags(void)
{
   ASSERT(_al_current_display);

   return _al_current_display->flags;
}



/* Function: al_get_num_display_modes
 *
 * Get the number of available fullscreen display modes
 * for the current set of display parameters. This will
 * use the values set with <al_set_new_display_format>,
 * <al_set_new_display_refresh_rate>, and <al_set_new_display_flags>
 * to find the number of modes that match. Settings the new
 * display parameters to zero will give a list of all modes
 * for the default driver.
 */
int al_get_num_display_modes(void)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_num_display_modes();
}



/* Function: al_get_display_mode
 *
 * Retrieves a display mode. Display parameters should not be
 * changed between a call of <al_get_num_display_modes> and
 * <al_get_display_mode>. index must be between 0 and the number
 * returned from al_get_num_display_modes-1. mode must be an
 * allocated ALLEGRO_DISPLAY_MODE structure. This function will
 * return NULL on failure, and the mode parameter that was passed
 * in on success.
 */
ALLEGRO_DISPLAY_MODE *al_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_display_mode(index, mode);
}



/* Function: al_wait_for_vsync
 *
 * Wait for the beginning of a vertical retrace. Some
 * driver/card/monitor combinations may not be capable
 * of this. Returns false if not possible, true if successful.
 */
bool al_wait_for_vsync(void)
{
   ASSERT(_al_current_display);

   if (_al_current_display->vt && _al_current_display->vt->wait_for_vsync)
      return _al_current_display->vt->wait_for_vsync(_al_current_display);
   else
      return false;
}



/* Function: al_set_clipping_rectangle
 *
 * Set the region of the target bitmap or display that
 * pixels get clipped to. The default is to clip pixels
 * to the entire bitmap.
 */
/* XXX this seems like it belongs in bitmap_new.c */
void al_set_clipping_rectangle(int x, int y, int width, int height)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   ASSERT(bitmap);

   if (x < 0) {
      width += x;
      x = 0;
   }
   if (y < 0) {
      height += y;
      y = 0;
   }
   if (x+width >= bitmap->w) {
      width = bitmap->w - x - 1;
   }
   if (y+height >= bitmap->h) {
      height = bitmap->h - y - 1;
   }

   bitmap->cl = x;
   bitmap->ct = y;
   bitmap->cr = x + width;
   bitmap->cb = y + height;

   if (bitmap->vt->update_clipping_rectangle) {
      bitmap->vt->update_clipping_rectangle(bitmap);
   }
}



/* Function: al_get_clipping_rectangle
 *
 * Gets the clipping rectangle of the target bitmap.
 */
/* XXX this seems like it belongs in bitmap_new.c */
void al_get_clipping_rectangle(int *x, int *y, int *w, int *h)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   ASSERT(bitmap);

   if (x) *x = bitmap->cl;
   if (y) *y = bitmap->ct;
   if (w) *w = bitmap->cr - bitmap->cl + 1;
   if (h) *h = bitmap->cb - bitmap->ct + 1;
}



/* Function: al_set_display_icon
 *
 * Changes the icon associated with the current display (window).
 *
 * Note: If the underlying OS can not use an icon with the size of the provided
 * bitmap, it will be scaled.
 *
 * TODO: Describe best practice for the size?
 * TODO: Allow providing multiple icons in differet sizes?
 */
void al_set_display_icon(ALLEGRO_BITMAP *icon)
{
   _al_current_display->vt->set_icon(_al_current_display, icon);
}



/* Destroys all bitmaps created for this display.
 */
void _al_destroy_display_bitmaps(ALLEGRO_DISPLAY *d) {
   while (_al_vector_size(&d->bitmaps) > 0) {
      ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&d->bitmaps);
      ALLEGRO_BITMAP *b = *bptr;
      al_destroy_bitmap(b);
   }
}


/* Function: al_get_num_video_adapters
 *
 * Get the number of video "adapters" attached to the computer. Each
 * video card attached to the computer counts as one or more adapters.
 * An adapter is thus really a video port that can have a monitor connected
 * to it.
 */
int al_get_num_video_adapters(void)
{
	ALLEGRO_SYSTEM *system = al_system_driver();

	if (system && system->vt && system->vt->get_num_video_adapters) {
		return system->vt->get_num_video_adapters();
	}

	return 0;
}

/* Function: al_get_monitor_info
 *
 * Get information about a monitor's position on the desktop.
 * adapter is a number from 0 to al_get_num_video_adapters()-1.
 *
 * See Also: <ALLEGRO_MONITOR_INFO>
 */
void al_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
	ALLEGRO_SYSTEM *system = al_system_driver();

	ASSERT(adapter < al_get_num_video_adapters());

	if (system && system->vt && system->vt->get_monitor_info) {
		system->vt->get_monitor_info(adapter, info);
	}
	else {
		info->x1 = info->y1 = info->x2 = info->y2 = INT_MAX;
	}
}


/* Function: al_get_current_video_adapter
 *
 * Gets the video adapter index where new displays
 * will be created.
 */
int al_get_current_video_adapter(void)
{
   return current_video_adapter;
}

/* Function: al_set_current_video_adapter
 *
 * Sets the adapter to use for newly created displays.
 * The adapter has a monitor attached to it. Information
 * about the monitor can be gotten using al_get_num_video_adapters
 * and al_get_monitor_info.
 *
 * See Also: <al_get_num_video_adapters>, <al_get_monitor_info>
 */
void al_set_current_video_adapter(int adapter)
{
   current_video_adapter = adapter;
}

/* Function: al_set_new_window_position
 *
 * Sets where the top left pixel of the client area of newly
 * created windows (non-fullscreen) will be on screen.
 * Negative values allowed on some multihead systems.
 *
 * See Also: <al_set_new_window_position>
 */
void al_set_new_window_position(int x, int y)
{
	new_window_x = x;
	new_window_y = y;
}

/* Function: al_get_new_window_position
 *
 * Gets the position where newly created non-fullscreen
 * displays will be placed.
 *
 * See Also: <al_set_new_window_position>
 */
void al_get_new_window_position(int *x, int *y)
{
	if (x)
		*x = new_window_x;
	if (y)
		*y = new_window_y;
}

/* Function: al_set_window_position
 *
 * Sets the position on screen of a non-fullscreen display.
 *
 * See Also: <al_get_window_position>
 */
void al_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
	if (display->flags & ALLEGRO_FULLSCREEN) {
		return;
	}

	if (display && display->vt && display->vt->set_window_position) {
		display->vt->set_window_position(display, x, y);
	}
}

/* Function: al_get_window_position
 *
 * Gets the position of a non-fullscreen display.
 *
 * See Also: <al_set_window_position>
 */
void al_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   if (display && display->vt && display->vt->get_window_position) {
      display->vt->get_window_position(display, x, y);
   }
   else {
      *x = *y = -1;
   }
}

void al_toggle_window_frame(ALLEGRO_DISPLAY *display, bool onoff)
{
	if (display->flags & ALLEGRO_FULLSCREEN) {
		return;
	}

	if (display && display->vt && display->vt->toggle_frame) {
		display->vt->toggle_frame(display, onoff);
	}
}

