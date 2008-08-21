#ifndef ALLEGRO_INTERNAL_DISPLAY_NEW_H
#define ALLEGRO_INTERNAL_DISPLAY_NEW_H

#include "allegro5/allegro5.h"
#include "allegro5/display_new.h"
#include "allegro5/bitmap_new.h"
#include "allegro5/internal/aintern_events.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALLEGRO_DISPLAY_INTERFACE ALLEGRO_DISPLAY_INTERFACE;

struct ALLEGRO_DISPLAY_INTERFACE
{
   int id;
   ALLEGRO_DISPLAY *(*create_display)(int w, int h);
   void (*destroy_display)(ALLEGRO_DISPLAY *display);
   bool (*set_current_display)(ALLEGRO_DISPLAY *d);
   void (*clear)(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color);
   void (*draw_line)(ALLEGRO_DISPLAY *d, float fx, float fy, float tx, float ty,
      ALLEGRO_COLOR *color);
   void (*draw_rectangle)(ALLEGRO_DISPLAY *d, float fx, float fy, float tx,
    float ty, ALLEGRO_COLOR *color, int flags);
   void (*draw_pixel)(ALLEGRO_DISPLAY *d, float x, float y, ALLEGRO_COLOR *color);
   void (*flip_display)(ALLEGRO_DISPLAY *d);
   bool (*update_display_region)(ALLEGRO_DISPLAY *d, int x, int y,
   	int width, int height);
   bool (*acknowledge_resize)(ALLEGRO_DISPLAY *d);
   bool (*resize_display)(ALLEGRO_DISPLAY *d, int width, int height);

   ALLEGRO_BITMAP *(*create_bitmap)(ALLEGRO_DISPLAY *d,
   	int w, int h);
   
   void (*set_target_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
   ALLEGRO_BITMAP *(*get_backbuffer)(ALLEGRO_DISPLAY *d);
   ALLEGRO_BITMAP *(*get_frontbuffer)(ALLEGRO_DISPLAY *d);

   bool (*is_compatible_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
   void (*switch_out)(ALLEGRO_DISPLAY *display);
   void (*switch_in)(ALLEGRO_DISPLAY *display);

   void (*draw_memory_bitmap_region)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap,
      float sx, float sy, float sw, float sh, float dx, float dy, int flags);

   ALLEGRO_BITMAP *(*create_sub_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *parent,
      int x, int y, int width, int height);

   bool (*wait_for_vsync)(ALLEGRO_DISPLAY *display);

   ALLEGRO_MOUSE_CURSOR *(*create_mouse_cursor)(ALLEGRO_DISPLAY *display,
      ALLEGRO_BITMAP *bmp, int x_focus, int y_focus);
   void (*destroy_mouse_cursor)(ALLEGRO_DISPLAY *display,
      ALLEGRO_MOUSE_CURSOR *cursor);
   bool (*set_mouse_cursor)(ALLEGRO_DISPLAY *display,
      ALLEGRO_MOUSE_CURSOR *cursor);
   bool (*set_system_mouse_cursor)(ALLEGRO_DISPLAY *display,
      ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id);
   bool (*show_mouse_cursor)(ALLEGRO_DISPLAY *display);
   bool (*hide_mouse_cursor)(ALLEGRO_DISPLAY *display);

   void (*set_icon)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);

   void (*set_window_position)(ALLEGRO_DISPLAY *display, int x, int y);
   void (*get_window_position)(ALLEGRO_DISPLAY *display, int *x, int *y);
   void (*toggle_frame)(ALLEGRO_DISPLAY *display, bool onoff);
};

struct ALLEGRO_DISPLAY
{
   /* Must be first, so the display can be used as event source. */
   struct ALLEGRO_EVENT_SOURCE es; 
   ALLEGRO_DISPLAY_INTERFACE *vt;
   int format;
   int refresh_rate;
   int flags;
   int w, h;

   _AL_VECTOR bitmaps; /* A list of bitmaps created for this display. */
};

//ALLEGRO_DISPLAY_INTERFACE *_al_display_d3ddummy_driver(void);

void _al_clear_memory(ALLEGRO_COLOR *color);
void _al_draw_rectangle_memory(int x1, int y1, int x2, int y2, ALLEGRO_COLOR *color, int flags);
void _al_draw_line_memory(int x1, int y1, int x2, int y2, ALLEGRO_COLOR *color);
void _al_draw_pixel_memory(int x, int y, ALLEGRO_COLOR *color);

void _al_destroy_display_bitmaps(ALLEGRO_DISPLAY *d);

int _al_display_type(void);


#ifdef __cplusplus
}
#endif

#endif
