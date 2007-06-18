#include "allegro.h"
#include "internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"


/* Display parameters */
static int _display_format = 0;
static int _display_refresh_rate = 0;
static int _display_flags = 0;


AL_DISPLAY *_al_current_display;
AL_BITMAP *_al_target_bitmap;


// FIXME: The system driver must be used to get drivers!
extern AL_DISPLAY_INTERFACE *_al_glx_vt(void);


/*
 * Sets the format, refresh rate, and flags for all displays created after this call.
 * format determines the pixel format of the display's buffers. For example:
 * AL_FORMAT_ANY = 0    // Let the driver decide the best format
 * AL_FORMAT_ARGB_8888
 * AL_FORMAT_RGBA_8888
 * AL_FORMAT_PALETTE_8
 * AL_FORMAT_RGBA_2222
 * ...
 *
 * refresh_rate sets the refresh rate to use for new displays. This only has effect
 * with fullscreen displays, and may be ignored if the requested refresh rate
 *  cannot be set. Specifying 0 as a refresh rate will use a default value.
 * 
 * The flags parameter can be 0 for default settings, or else a combination of:
 * AL_WINDOWED - prefer a windowed mode
 * AL_FULLSCREEN - prefer a fullscreen mode
 * AL_OPENGL - require the driver to provide an initialized opengl context after returning successfully
 * AL_DIRECT3D - require the driver to do rendering with Direct3D and provide a Direct3D device
 * AL_DOUBLEBUFFER - use double buffering
 * AL_PAGEFLIP - use page flipping
 *
 */
void al_set_display_parameters(int format, int refresh_rate, int flags)
{
	_display_format = format;

	if (refresh_rate)
		_display_refresh_rate = refresh_rate;

	if (flags)
		_display_flags = flags;
}

/*
 * Retrieve the display parameters.
 */
void al_get_display_parameters(int *format, int *refresh_rate, int *flags)
{
	if (format)
		*format = _display_format;
	if (refresh_rate)
		*refresh_rate = _display_refresh_rate;
	if (flags)
		*flags = _display_flags;
}

/*
 * Create a new display. This is usually a window or fullscreen display.
 */
AL_DISPLAY *al_create_display(int w, int h)
{
   // FIXME: We need to ask the system driver for a list of possible display
   // drivers here, then select a suitable one according to configuration
   // variables like "display/driver" and according to flags (e.g. OpenGL
   // requested or not).

   // Right now, the X11 driver is hardcoded.

   AL_SYSTEM *system = al_system_driver();
   AL_DISPLAY_INTERFACE *driver = system->vt->get_display_driver();
   AL_DISPLAY *display = driver->create_display(w, h);

   return display;
}

void al_destroy_display(AL_DISPLAY *display)
{
	display->vt->destroy_display(display);
}

/*
 * Make a display the current display. All the following Allegro commands in
 * the same thread will implicitly use this display from now on.
 */
void al_set_current_display(AL_DISPLAY *display)
{
   display->vt->set_current_display(display);
   _al_current_display = display;
}

/*
 * Get the current display.
 */
AL_DISPLAY *al_get_current_display(void)
{
	return _al_current_display;
}

/*
 * Select the bitmap to which all subsequent drawing operation
 * will draw.
 */
void al_set_target_bitmap(AL_BITMAP *bitmap)
{
	_al_target_bitmap = bitmap;
	_al_current_display->vt->set_target_bitmap(_al_current_display, bitmap);
}

/*
 * Retrieve the target for drawing operations.
 */
AL_BITMAP *al_get_target_bitmap(void)
{
	return _al_target_bitmap;
}

AL_BITMAP *al_get_backbuffer(void)
{
	return _al_current_display->vt->get_backbuffer(_al_current_display);
}

AL_BITMAP *al_get_frontbuffer(void)
{
	return _al_current_display->vt->get_frontbuffer(_al_current_display);
}

/*
 * Make all of the graphics which were drawn since the display
 * was created or since the last to to al_flip visible.
 */
void al_flip_display(void)
{
   _al_current_display->vt->flip_display(_al_current_display);
}

/* 
 * Makes a region of the graphics which were drawn since the
 * display was created or since the last call to al_flip visible.
 */
void al_flip_display_region(int x, int y,
	int width, int height)
{
   _al_current_display->vt->flip_display_region(_al_current_display, x, y,
      width, height);
}

/*
 * When the user receives a resize event from a resizable display,
 * if they wish the display to be resized they must call this
 * function to let the graphics driver know that it can now resize
 * the display.
 */
void al_notify_resize(void)
{
   _al_current_display->vt->notify_resize(_al_current_display);
}
/* Clear a complete display, but confined by the clipping rectangle. */
void al_clear(AL_COLOR *color)
{
   _al_current_display->vt->clear(_al_current_display, color);
}

/* Draws a line from fx/fy to tx/ty, including start as well as end pixel. */
void al_draw_line(float fx, float fy, float tx, float ty, AL_COLOR* color)
{
   _al_current_display->vt->draw_line(_al_current_display, fx, fy, tx, ty, color);
}

/* Draws a rectangle with top left corner tlx/tly abd bottom right corner
 * brx/bry. Both points are inclusive. */
void al_draw_filled_rectangle(float tlx, float tly, float brx, float bry,
   AL_COLOR *color)
{
   _al_current_display->vt->draw_filled_rectangle(_al_current_display,
      tlx, tly, brx, bry, color);
}

