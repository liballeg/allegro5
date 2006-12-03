#include "allegro/internal/system_new.h"
#include "allegro/internal/display_new.h"

static AL_DISPLAY *current_display;

// FIXME: The system driver must be used to get drivers!
extern AL_DISPLAY_INTERFACE *_al_glx_vt(void);

/* Create a new display. This is usually a window or fullscreen display.
 */
AL_DISPLAY *al_create_display(int w, int h, int flags)
{
   // FIXME: We need to ask the system driver for a list of possible display
   // drivers here, then select a suitable one according to configuration
   // variables like "display/driver" and according to flags (e.g. OpenGL
   // requested or not).

   // Right now, the X11 driver is hardcoded.

   AL_SYSTEM *system = al_system_driver();
   AL_DISPLAY_INTERFACE *driver = system->vt->get_display_driver();
   AL_DISPLAY *display = driver->create_display(w, h, flags);

   return display;
}

/* Make a display the current display. All the following Allegro commands in
 * the same thread will implicitly use this display from now on.
 */
void al_make_display_current(AL_DISPLAY *display)
{
   display->vt->make_display_current(display);
   current_display = display;
}

/* Clear a complete display, but confined by the clipping rectangle. */
void al_clear(AL_COLOR color)
{
   current_display->vt->clear(current_display, color);
}

/* Draws a line from fx/fy to tx/ty, including start as well as end pixel. */
void al_line(float fx, float fy, float tx, float ty, AL_COLOR color)
{
   current_display->vt->line(current_display, fx, fy, tx, ty, color);
}

/* Draws a rectangle with top left corner tlx/tly abd bottom right corner
 * brx/bry. Both points are inclusive. */
void al_filled_rectangle(float tlx, float tly, float brx, float bry,
   AL_COLOR color)
{
   current_display->vt->filled_rectangle(current_display,
      tlx, tly, brx, bry, color);
}

/* Makes all graphics which were drawn since the display was created or since
 * the last call to al_flip visible.
 */
void al_flip(void)
{
   current_display->vt->flip(current_display);
}
