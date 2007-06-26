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

BITMAP *create_video_bitmap(int width, int height)
{
   return al_create_video_bitmap(width, height);
}



BITMAP *create_system_bitmap(int width, int height)
{
   return al_create_system_bitmap(width, height);
}



int scroll_screen(int x, int y)
{
   return al_scroll_display(x, y);
}

int request_scroll(int x, int y)
{
   return al_request_scroll(x, y);
}

int poll_scroll(void)
{
   return al_poll_scroll();
}

int show_video_bitmap(BITMAP *bitmap)
{
   return al_show_video_bitmap(bitmap);
}

int request_video_bitmap(BITMAP *bitmap)
{
   return al_request_video_bitmap(bitmap);
}

int enable_triple_buffer(void)
{
   return al_enable_triple_buffer();
}
