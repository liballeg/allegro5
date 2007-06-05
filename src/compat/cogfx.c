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

#include "allegro/display.h"

int set_gfx_mode(int card, int w, int h, int v_w, int v_h)
{
/*
   al_destroy_display(al_main_display);
   
   al_create_display(card, AL_UPDATE_NONE, get_color_depth(), w, h);
   if (!al_main_display) {
      return -1;
   }

   screen = al_main_display->screen;
   gfx_driver = al_main_display->gfx_driver;
   return 0;
  */ 
  return -1;
}



BITMAP *create_video_bitmap(int width, int height)
{
   return al_create_video_bitmap(al_main_display, width, height);
}



BITMAP *create_system_bitmap(int width, int height)
{
   return al_create_system_bitmap(al_main_display, width, height);
}



int scroll_screen(int x, int y)
{
   return al_scroll_display(al_main_display, x, y);
}

int request_scroll(int x, int y)
{
   return al_request_scroll(al_main_display, x, y);
}

int poll_scroll(void)
{
   return al_poll_scroll(al_main_display);
}

int show_video_bitmap(BITMAP *bitmap)
{
   return al_show_video_bitmap(al_main_display, bitmap);
}

int request_video_bitmap(BITMAP *bitmap)
{
   return al_request_video_bitmap(al_main_display, bitmap);
}

int enable_triple_buffer(void)
{
   return al_enable_triple_buffer(al_main_display);
}
