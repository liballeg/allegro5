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
 *      4.0/4.2 screen update compatibility file
 *
 *	By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"

int set_gfx_mode(int card, int w, int h, int v_w, int v_h)
{

   return 0;
}


void destroy_bitmap(BITMAP *bmp) {}


BITMAP *create_video_bitmap(int width, int height)
{
   return NULL;
}



BITMAP *create_system_bitmap(int width, int height)
{
   return NULL;
}

BITMAP *_make_bitmap(int w, int h, uintptr_t addr, GFX_DRIVER *driver, int color_depth, int bpl)
{
   return NULL;
}

GFX_VTABLE *_get_vtable(int color_depth)
{
   int i;
   for (i = 0; _vtable_list[i].vtable; i++) {
      if (_vtable_list[i].color_depth == color_depth)
         return _vtable_list[i].vtable; 
   }
   return NULL;
}

int scroll_screen(int x, int y)
{
   return 0;
}

int request_scroll(int x, int y)
{
   return 0;
}

int poll_scroll(void)
{
   return 0;
}

int show_video_bitmap(BITMAP *bitmap)
{
   return 0;
}

int request_video_bitmap(BITMAP *bitmap)
{
   return 0;
}

int enable_triple_buffer(void)
{
   return 0;
}
