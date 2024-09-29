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
 *      Simple drawing primitives.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memdraw.h"
#include "allegro5/internal/aintern_pixels.h"


/* Function: al_clear_to_color
 */
void al_clear_to_color(A5O_COLOR color)
{
   A5O_BITMAP *target = al_get_target_bitmap();
   ASSERT(target);

   if (al_get_bitmap_flags(target) & A5O_MEMORY_BITMAP ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      _al_clear_bitmap_by_locking(target, &color);
   }
   else {
      A5O_DISPLAY *display = _al_get_bitmap_display(target);
      ASSERT(display);
      ASSERT(display->vt);
      display->vt->clear(display, &color);
   }
}


/* Function: al_clear_depth_buffer
 */
void al_clear_depth_buffer(float z)
{
   A5O_BITMAP *target = al_get_target_bitmap();
   ASSERT(target);

   if (al_get_bitmap_flags(target) & A5O_MEMORY_BITMAP) {
      /* has no depth buffer */
   }
   else {
      A5O_DISPLAY *display = _al_get_bitmap_display(target);
      ASSERT(display);
      display->vt->clear_depth_buffer(display, z);
   }
}


/* Function: al_draw_pixel
 */
void al_draw_pixel(float x, float y, A5O_COLOR color)
{
   A5O_BITMAP *target = al_get_target_bitmap();

   ASSERT(target);

   if (al_get_bitmap_flags(target) & A5O_MEMORY_BITMAP ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      _al_draw_pixel_memory(target, x, y, &color);
   }
   else {
      A5O_DISPLAY *display = _al_get_bitmap_display(target);
      ASSERT(display);
      ASSERT(display->vt);
      display->vt->draw_pixel(display, x, y, &color);
   }
}


/* vim: set sts=3 sw=3 et: */
