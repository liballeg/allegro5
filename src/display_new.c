#include "allegro.h"
#include "internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "internal/aintern_system.h"
#include "internal/aintern_display.h"

AL_DISPLAY *_al_current_display;
AL_DISPLAY *al_main_display = NULL;

AL_COLOR al_mask_color;

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

void al_destroy_display(AL_DISPLAY *display)
{
	display->vt->destroy_display(display);
}

/* Make a display the current display. All the following Allegro commands in
 * the same thread will implicitly use this display from now on.
 */
void al_make_display_current(AL_DISPLAY *display)
{
   display->vt->make_display_current(display);
   _al_current_display = display;
}

/* Clear a complete display, but confined by the clipping rectangle. */
void al_clear(AL_COLOR color)
{
   _al_current_display->vt->clear(_al_current_display, color);
}

/* Draws a line from fx/fy to tx/ty, including start as well as end pixel. */
void al_line(float fx, float fy, float tx, float ty, AL_COLOR color)
{
   _al_current_display->vt->line(_al_current_display, fx, fy, tx, ty, color);
}

/* Draws a rectangle with top left corner tlx/tly abd bottom right corner
 * brx/bry. Both points are inclusive. */
void al_filled_rectangle(float tlx, float tly, float brx, float bry,
   AL_COLOR color)
{
   _al_current_display->vt->filled_rectangle(_al_current_display,
      tlx, tly, brx, bry, color);
}

/* Makes all graphics which were drawn since the display was created or since
 * the last call to al_flip visible.
 */
void al_flip(unsigned int x, unsigned int y,
	unsigned int width, unsigned int height)
{
   if (width <= 0 || height <= 0) {
      _al_current_display->vt->flip(_al_current_display, x, y,
         _al_current_display->w, _al_current_display->h);
   }
   else {
      _al_current_display->vt->flip(_al_current_display, x, y, width, height);
   }
}

// TODO: maybe can be done in al_flip?
// TODO: should the display parameter stay explicit?
// TODO: find a better name
/* Called from the main thread to actually resize the display after it has been
 * resized by the user.
 */
void al_acknowledge_resize(void)
{
   _al_current_display->vt->acknowledge_resize(_al_current_display);
}

#if 0
/* destroy_bitmap:
 *  Destroys a memory bitmap.
 */
void destroy_bitmap(BITMAP *bitmap)
{
}

/* _make_bitmap:
 *  Helper function for creating the screen bitmap. Sets up a bitmap 
 *  structure for addressing video memory at addr, and fills the bank 
 *  switching table using bank size/granularity information from the 
 *  specified graphics driver.
 */
BITMAP *_make_bitmap(int w, int h, uintptr_t addr, GFX_DRIVER *driver, int color_depth, int bpl)
{
}

GFX_VTABLE *_get_vtable(int color_depth)
{
   GFX_VTABLE *vt;
   int i;

   ASSERT(system_driver);

   if (system_driver->get_vtable) {
      vt = system_driver->get_vtable(color_depth);

      if (vt) {
	 LOCK_DATA(vt, sizeof(GFX_VTABLE));
	 LOCK_CODE(vt->draw_sprite, (long)vt->draw_sprite_end - (long)vt->draw_sprite);
	 LOCK_CODE(vt->blit_from_memory, (long)vt->blit_end - (long)vt->blit_from_memory);
	 return vt;
      }
   }

   for (i=0; _vtable_list[i].vtable; i++) {
      if (_vtable_list[i].color_depth == color_depth) {
	 LOCK_DATA(_vtable_list[i].vtable, sizeof(GFX_VTABLE));
	 LOCK_CODE(_vtable_list[i].vtable->draw_sprite, (long)_vtable_list[i].vtable->draw_sprite_end - (long)_vtable_list[i].vtable->draw_sprite);
	 LOCK_CODE(_vtable_list[i].vtable->blit_from_memory, (long)_vtable_list[i].vtable->blit_end - (long)_vtable_list[i].vtable->blit_from_memory);
	 return _vtable_list[i].vtable;
      }
   }

   return NULL;
}

BITMAP *al_create_video_bitmap(AL_DISPLAY *display, int width, int height)
{
}

int al_scroll_display(AL_DISPLAY *display, int x, int y)
{
}

int al_poll_scroll(AL_DISPLAY *display)
{
}
void _sort_out_virtual_width(int *width, GFX_DRIVER *driver)
{
}

int al_request_scroll(AL_DISPLAY *display, int x, int y)
{
}

int al_show_video_bitmap(AL_DISPLAY *display, BITMAP *bitmap)
{
}

int al_set_update_method(AL_DISPLAY *display, int method)
{
}

void al_destroy_display(AL_DISPLAY *display)
{
}

void al_flip_display(AL_DISPLAY *display)
{
}

BITMAP *al_get_buffer(const AL_DISPLAY *display)
{
}

int al_get_update_method(const AL_DISPLAY *display)
{
}

void al_enable_vsync(AL_DISPLAY *display)
{
}

void al_disable_vsync(AL_DISPLAY *display)
{
}

void al_toggle_vsync(AL_DISPLAY *display)
{
}

int al_vsync_is_enabled(const AL_DISPLAY *display)
{
}

BITMAP *al_create_system_bitmap(AL_DISPLAY *display, int width, int height)
{
}

int al_enable_triple_buffer(AL_DISPLAY *display)
{
}

int al_request_video_bitmap(AL_DISPLAY *display, BITMAP *bitmap)
{
}
#endif
