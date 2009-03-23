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
#include "allegro5/internal/aintern_memory.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"



static int current_video_adapter = -1;
static int new_window_x = INT_MAX;
static int new_window_y = INT_MAX;

/* Function: al_create_display
 */
ALLEGRO_DISPLAY *al_create_display(int w, int h)
{
   ALLEGRO_SYSTEM *system;
   ALLEGRO_DISPLAY_INTERFACE *driver;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds;

   eds = _al_get_new_display_settings();
   /* We cannot depend on al_reset_new_display_options() having been called
    * anywhere because al_create_display() may be called from new threads.
    */
   if (!eds)
      al_reset_new_display_options();
   else
      _al_fill_display_settings(eds);

   system = al_system_driver();
   driver = system->vt->get_display_driver();
   display = driver->create_display(w, h);
   if (!display)
      return NULL;

   _al_vector_init(&display->bitmaps, sizeof(ALLEGRO_BITMAP*));

   al_set_current_display(display);
   if (display->extra_settings.settings[ALLEGRO_COMPATIBLE_DISPLAY])
      al_clear(al_map_rgb(0, 0, 0));
   al_flip_display();

   return display;
}



/* Function: al_destroy_display
 */
void al_destroy_display(ALLEGRO_DISPLAY *display)
{
   if (display) {
      display->vt->destroy_display(display);
   }
}



/* Function: al_get_backbuffer
 */
ALLEGRO_BITMAP *al_get_backbuffer(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   return display->vt->get_backbuffer(display);
}



/* Function: al_get_frontbuffer
 */
ALLEGRO_BITMAP *al_get_frontbuffer(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   return display->vt->get_frontbuffer(display);
}



/* Function: al_flip_display
 */
void al_flip_display(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   display->vt->flip_display(display);
}



/* Function: al_update_display_region
 */
void al_update_display_region(int x, int y, int width, int height)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   display->vt->update_display_region(display, x, y, width, height);
}



/* Function: al_acknowledge_resize
 */
bool al_acknowledge_resize(ALLEGRO_DISPLAY *display)
{
   ASSERT(display);

   if (!(display->flags & ALLEGRO_FULLSCREEN)) {
      if (display->vt->acknowledge_resize) {
         return display->vt->acknowledge_resize(display);
      }
   }
   return false;
}



/* Function: al_resize_display
 */
bool al_resize_display(int width, int height)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   if (display->vt->resize_display) {
      return display->vt->resize_display(display, width, height);
   }
   return false;
}



/* Function: al_clear
 */
void al_clear(ALLEGRO_COLOR color)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   ASSERT(target);

   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_clear_memory(&color);
   }
   else {
      ASSERT(display);
      display->vt->clear(display, &color);
   }
}



/* Function: al_draw_pixel
 */
void al_draw_pixel(float x, float y, ALLEGRO_COLOR color)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   ASSERT(target);

   if (target->flags & ALLEGRO_MEMORY_BITMAP || !display->vt->draw_pixel) {
      _al_draw_pixel_memory(target, x, y, &color);
   }
   else {
      ASSERT(display);
      display->vt->draw_pixel(display, x, y, &color);
   }
}




/* Function: al_is_compatible_bitmap
 */
bool al_is_compatible_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(bitmap);
   ASSERT(display);

   return display->vt->is_compatible_bitmap(display, bitmap);
}



/* Function: al_get_display_width
 */
int al_get_display_width(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   return display->w;
}



/* Function: al_get_display_height
 */
int al_get_display_height(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   return display->h;
}


/* Function: al_get_display_format
 */
int al_get_display_format(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   return _al_deduce_color_format(&display->extra_settings);
}


/* Function: al_get_display_refresh_rate
 */
int al_get_display_refresh_rate(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   return display->refresh_rate;
}



/* Function: al_get_display_flags
 */
int al_get_display_flags(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   return display->flags;
}



/* Function: al_get_num_display_modes
 */
int al_get_num_display_modes(void)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_num_display_modes();
}



/* Function: al_get_display_mode
 */
ALLEGRO_DISPLAY_MODE *al_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_display_mode(index, mode);
}



/* Function: al_get_num_display_formats
 */
int al_get_num_display_formats(void)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_num_display_formats();
}



/* Function: al_get_display_format_option
 */
int al_get_display_format_option(int i, int option)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_display_format_option(i, option);
}



/* Function: al_set_new_display_format
 */
void al_set_new_display_format(int i)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   system->vt->set_new_display_format(i);
}


/* Function: al_wait_for_vsync
 */
bool al_wait_for_vsync(void)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);

   if (display->vt->wait_for_vsync)
      return display->vt->wait_for_vsync(display);
   else
      return false;
}



/* Function: al_set_clipping_rectangle
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
   if (x + width > bitmap->w) {
      width = bitmap->w - x;
   }
   if (y + height > bitmap->h) {
      height = bitmap->h - y;
   }

   bitmap->cl = x;
   bitmap->ct = y;
   bitmap->cr_excl = x + width;
   bitmap->cb_excl = y + height;

   if (bitmap->vt && bitmap->vt->update_clipping_rectangle) {
      bitmap->vt->update_clipping_rectangle(bitmap);
   }
}



/* Function: al_get_clipping_rectangle
 */
/* XXX this seems like it belongs in bitmap_new.c */
void al_get_clipping_rectangle(int *x, int *y, int *w, int *h)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   ASSERT(bitmap);

   if (x) *x = bitmap->cl;
   if (y) *y = bitmap->ct;
   if (w) *w = bitmap->cr_excl - bitmap->cl;
   if (h) *h = bitmap->cb_excl - bitmap->ct;
}



/* Function: al_set_display_icon
 */
void al_set_display_icon(ALLEGRO_BITMAP *icon)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);
   if (display->vt->set_icon) {
      display->vt->set_icon(display, icon);
   }
}



/* Destroys all bitmaps created for this display.
 */
void _al_destroy_display_bitmaps(ALLEGRO_DISPLAY *d)
{
   while (_al_vector_size(&d->bitmaps) > 0) {
      ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&d->bitmaps);
      ALLEGRO_BITMAP *b = *bptr;
      al_destroy_bitmap(b);
   }
}


/* Function: al_get_num_video_adapters
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
 */
int al_get_current_video_adapter(void)
{
   return current_video_adapter;
}

/* Function: al_set_current_video_adapter
 */
void al_set_current_video_adapter(int adapter)
{
   current_video_adapter = adapter;
}

/* Function: al_set_new_window_position
 */
void al_set_new_window_position(int x, int y)
{
   new_window_x = x;
   new_window_y = y;
}

/* Function: al_get_new_window_position
 */
void al_get_new_window_position(int *x, int *y)
{
   if (x)
      *x = new_window_x;
   if (y)
      *y = new_window_y;
}

/* Function: al_set_window_position
 */
void al_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   ASSERT(display);

   if (display->flags & ALLEGRO_FULLSCREEN) {
      return;
   }

   if (display && display->vt && display->vt->set_window_position) {
      display->vt->set_window_position(display, x, y);
   }
}

/* Function: al_get_window_position
 */
void al_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   ASSERT(x);
   ASSERT(y);

   if (display && display->vt && display->vt->get_window_position) {
      display->vt->get_window_position(display, x, y);
   }
   else {
      *x = *y = -1;
   }
}

/* Function: al_toggle_window_frame
 */
void al_toggle_window_frame(ALLEGRO_DISPLAY *display, bool onoff)
{
   ASSERT(display);

   if (display->flags & ALLEGRO_FULLSCREEN) {
      return;
   }

   if (display && display->vt && display->vt->toggle_frame) {
      display->vt->toggle_frame(display, onoff);
   }
}

/* Function: al_set_window_title
 */
void al_set_window_title(AL_CONST char *title)
{
   ALLEGRO_DISPLAY *current_display = al_get_current_display();

   if (current_display && current_display->vt && current_display->vt->set_window_title)
      current_display->vt->set_window_title(current_display, title);
}

