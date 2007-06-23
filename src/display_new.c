#include "allegro.h"
#include "internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"

// FIXME: The system driver must be used to get drivers!
extern AL_DISPLAY_INTERFACE *_al_glx_vt(void);

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
   if (!display)
      return NULL;
   AL_COLOR black;
   al_set_current_display(display);
   _al_map_rgba(display->format, &black, 0, 0, 0, 0);
   al_clear(&black);
   al_set_target_bitmap(al_get_backbuffer());

   return display;
}

void al_destroy_display(AL_DISPLAY *display)
{
	display->vt->destroy_display(display);
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
 */
bool al_update_display_region(int x, int y,
	int width, int height)
{
   return _al_current_display->vt->update_display_region(
      _al_current_display, x, y, width, height);
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

bool al_is_compatible_bitmap(AL_BITMAP *bitmap)
{
   return _al_current_display->vt->is_compatible_bitmap(
      _al_current_display, bitmap);
}

