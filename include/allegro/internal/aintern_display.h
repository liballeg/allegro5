#ifndef ALLEGRO_INTERNAL_DISPLAY_NEW_H
#define ALLEGRO_INTERNAL_DISPLAY_NEW_H

#include "allegro.h"
#include "allegro/display_new.h"
#include "allegro/bitmap_new.h"
#include "allegro/internal/aintern_events.h"

typedef struct AL_DISPLAY_INTERFACE AL_DISPLAY_INTERFACE;

struct AL_DISPLAY_INTERFACE
{
   int id;
   AL_DISPLAY *(*create_display)(int w, int h);
   void (*destroy_display)(AL_DISPLAY *display);
   void (*set_current_display)(AL_DISPLAY *d);
   void (*clear)(AL_DISPLAY *d, AL_COLOR *color);
   void (*draw_line)(AL_DISPLAY *d, float fx, float fy, float tx, float ty,
      AL_COLOR *color);
   void (*draw_filled_rectangle)(AL_DISPLAY *d, float fx, float fy, float tx,
    float ty, AL_COLOR *color);
   void (*flip_display)(AL_DISPLAY *d);
   bool (*update_display_region)(AL_DISPLAY *d, int x, int y,
   	int width, int height);
   bool (*acknowledge_resize)(AL_DISPLAY *d);
   bool (*resize_display)(AL_DISPLAY *d, int width, int height);

   AL_BITMAP *(*create_bitmap)(AL_DISPLAY *d,
   	int w, int h);
   
   void (*upload_compat_screen)(struct BITMAP *bitmap, int x, int y, int width, int height);
   void (*set_target_bitmap)(AL_DISPLAY *display, AL_BITMAP *bitmap);
   AL_BITMAP *(*get_backbuffer)(AL_DISPLAY *display);
   AL_BITMAP *(*get_frontbuffer)(AL_DISPLAY *display);

   bool (*is_compatible_bitmap)(AL_DISPLAY *display, AL_BITMAP *bitmap);
   void (*switch_out)(void);

   void (*draw_memory_bitmap_region)(AL_DISPLAY *display, AL_BITMAP *bitmap,
      float sx, float sy, float sw, float sh, float dx, float dy, int flags);

   AL_BITMAP *(*create_sub_bitmap)(AL_DISPLAY *display, AL_BITMAP *parent,
      int x, int y, int width, int height);
};

struct AL_DISPLAY
{
   /* Must be first, so the display can be used as event source. */
   struct AL_EVENT_SOURCE es; 
   AL_DISPLAY_INTERFACE *vt;
   int format;
   int refresh_rate;
   int flags;
   int w, h;
};

#define _al_current_display al_get_current_display()

//AL_DISPLAY_INTERFACE *_al_display_d3ddummy_driver(void);

void _al_clear_memory(AL_COLOR *color);
void _al_draw_filled_rectangle_memory(int x1, int y1, int x2, int y2, AL_COLOR *color);
void _al_draw_line_memory(int x1, int y1, int x2, int y2, AL_COLOR *color);

#endif
