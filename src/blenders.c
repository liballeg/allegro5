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
 *      Memory bitmap blending routines
 *
 *      by Trent Gamblin.
 *
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_display.h"
#include <string.h>

void _al_blend_memory(ALLEGRO_COLOR *scol,
   ALLEGRO_BITMAP *dest,
   int dx, int dy, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR dcol;
   int op, src_blend, dest_blend, alpha_op, alpha_src_blend, alpha_dest_blend;
   dcol = al_get_pixel(dest, dx, dy);
   al_get_separate_blender(&op, &src_blend, &dest_blend,
                           &alpha_op, &alpha_src_blend, &alpha_dest_blend);
   _al_blend_inline(scol, &dcol,
                    op, src_blend, dest_blend,
                    alpha_op, alpha_src_blend, alpha_dest_blend,
                    result);
   (void) _al_blend_alpha_inline; // silence compiler
}
