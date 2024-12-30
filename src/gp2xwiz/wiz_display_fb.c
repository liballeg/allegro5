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
 *      GP2X Wiz framebuffer display driver
 *
 *      By Trent Gamblin.
 *
 */

#include "allegro5/internal/aintern_gp2xwiz.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"

ALLEGRO_DEBUG_CHANNEL("display")

static ALLEGRO_DISPLAY_INTERFACE *gp2xwiz_vt;

static bool set_gfx_mode = false;

/* Create a new X11 display, which maps directly to a GLX window. */
static ALLEGRO_DISPLAY *gp2xwiz_create_display_fb(int w, int h)
{
   (void)w;
   (void)h;

   /* Only one display allowed at a time */
   if (set_gfx_mode)
      return NULL;

   lc_init_rest();

   ALLEGRO_DISPLAY_GP2XWIZ_FB *d = al_calloc(1, sizeof *d);
   ALLEGRO_DISPLAY *display = (void *)d;

   ALLEGRO_SYSTEM_GP2XWIZ *system = (void *)al_get_system_driver();

   display->w = 320;
   display->h = 240;
   display->vt = _al_display_gp2xwiz_framebuffer_driver();
   display->refresh_rate = 60;
   display->flags = al_get_new_display_flags();

   display->flags |= ALLEGRO_FULLSCREEN;

   /* Add ourself to the list of displays. */
   ALLEGRO_DISPLAY_GP2XWIZ_FB **add;
   add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   /* Create a backbuffer and point it to the framebuffer */
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   d->backbuffer = al_create_bitmap(320, 240);
   d->screen_mem = d->backbuffer->memory;
   d->backbuffer->memory = (unsigned char *)lc_fb1;

   set_gfx_mode = true;

   ALLEGRO_DEBUG("Display created successfully\n");

   return display;
}


static void gp2xwiz_destroy_display_fb(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_GP2XWIZ *s = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_GP2XWIZ_FB *wiz_disp = (void *)d;

   _al_vector_find_and_delete(&s->system.displays, &d);

   /* All bitmaps are memory bitmaps, no need to do anything */

   _al_vector_free(&d->bitmaps);
   _al_event_source_free(&d->es);

   wiz_disp->backbuffer->memory = wiz_disp->screen_mem;
   al_destroy_bitmap(wiz_disp->backbuffer);

   al_free(d->vertex_cache);
   al_free(d);

   set_gfx_mode = false;
}


static bool gp2xwiz_set_current_display_fb(ALLEGRO_DISPLAY *d)
{
   (void)d;
   return true;
}


static void gp2xwiz_flip_display_fb(ALLEGRO_DISPLAY *d)
{
   (void)d;
}

static void gp2xwiz_update_display_region_fb(ALLEGRO_DISPLAY *d, int x, int y,
   int w, int h)
{
   (void)x;
   (void)y;
   (void)w;
   (void)h;
   gp2xwiz_flip_display_fb(d);
}

static bool gp2xwiz_acknowledge_resize_fb(ALLEGRO_DISPLAY *d)
{
   (void)d;
   return false;
}


static bool gp2xwiz_resize_display_fb(ALLEGRO_DISPLAY *d, int w, int h)
{
   (void)d;
   (void)w;
   (void)h;
   return false;
}


static bool gp2xwiz_is_compatible_bitmap_fb(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap)
{
   (void)display;
   (void)bitmap;
   return true;
}


static void gp2xwiz_get_window_position_fb(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   (void)display;
   *x = 0;
   *y = 0;
}


static bool gp2xwiz_wait_for_vsync_fb(ALLEGRO_DISPLAY *display)
{
   (void)display;
   return false;
}


static ALLEGRO_BITMAP *gp2xwiz_get_backbuffer_fb(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_GP2XWIZ_FB *d = (void *)display;
   return d->backbuffer;
}


/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_gp2xwiz_framebuffer_driver(void)
{
   if (gp2xwiz_vt)
      return gp2xwiz_vt;

   gp2xwiz_vt = al_calloc(1, sizeof *gp2xwiz_vt);

   gp2xwiz_vt->create_display = gp2xwiz_create_display_fb;
   gp2xwiz_vt->destroy_display = gp2xwiz_destroy_display_fb;
   gp2xwiz_vt->set_current_display = gp2xwiz_set_current_display_fb;
   gp2xwiz_vt->flip_display = gp2xwiz_flip_display_fb;
   gp2xwiz_vt->update_display_region = gp2xwiz_update_display_region_fb;
   gp2xwiz_vt->acknowledge_resize = gp2xwiz_acknowledge_resize_fb;
   gp2xwiz_vt->create_bitmap = NULL;
   gp2xwiz_vt->get_backbuffer = gp2xwiz_get_backbuffer_fb;
   gp2xwiz_vt->set_target_bitmap = NULL;
   gp2xwiz_vt->is_compatible_bitmap = gp2xwiz_is_compatible_bitmap_fb;
   gp2xwiz_vt->resize_display = gp2xwiz_resize_display_fb;
   gp2xwiz_vt->set_icons = NULL;
   gp2xwiz_vt->set_window_title = NULL;
   gp2xwiz_vt->set_window_position = NULL;
   gp2xwiz_vt->get_window_position = gp2xwiz_get_window_position_fb;
   gp2xwiz_vt->set_window_constraints = NULL;
   gp2xwiz_vt->get_window_constraints = NULL;
   gp2xwiz_vt->set_display_flag = NULL;
   gp2xwiz_vt->wait_for_vsync = gp2xwiz_wait_for_vsync_fb;

   return gp2xwiz_vt;
}

/* vi: set sts=3 sw=3 et: */
