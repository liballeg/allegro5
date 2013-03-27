#ifndef __al_included_allegro5_aintern_display_h
#define __al_included_allegro5_aintern_display_h

#include "allegro5/allegro.h"
#include "allegro5/transformations.h"
#include "allegro5/display.h"
#include "allegro5/bitmap.h"
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
   void (*unset_current_display)(ALLEGRO_DISPLAY *d);
   void (*clear)(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color);
   void (*draw_pixel)(ALLEGRO_DISPLAY *d, float x, float y, ALLEGRO_COLOR *color);
   void (*flip_display)(ALLEGRO_DISPLAY *d);
   void (*update_display_region)(ALLEGRO_DISPLAY *d, int x, int y,
   	int width, int height);
   bool (*acknowledge_resize)(ALLEGRO_DISPLAY *d);
   bool (*resize_display)(ALLEGRO_DISPLAY *d, int width, int height);
   void (*quick_size)(ALLEGRO_DISPLAY *d);

   ALLEGRO_BITMAP *(*create_bitmap)(ALLEGRO_DISPLAY *d,
   	int w, int h);
   
   void (*set_target_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
   ALLEGRO_BITMAP *(*get_backbuffer)(ALLEGRO_DISPLAY *d);

   bool (*is_compatible_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
   void (*switch_out)(ALLEGRO_DISPLAY *display);
   void (*switch_in)(ALLEGRO_DISPLAY *display);

   void (*draw_memory_bitmap_region)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap,
      float sx, float sy, float sw, float sh, int flags);

   ALLEGRO_BITMAP *(*create_sub_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *parent,
      int x, int y, int width, int height);

   bool (*wait_for_vsync)(ALLEGRO_DISPLAY *display);

   bool (*set_mouse_cursor)(ALLEGRO_DISPLAY *display,
      ALLEGRO_MOUSE_CURSOR *cursor);
   bool (*set_system_mouse_cursor)(ALLEGRO_DISPLAY *display,
      ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id);
   bool (*show_mouse_cursor)(ALLEGRO_DISPLAY *display);
   bool (*hide_mouse_cursor)(ALLEGRO_DISPLAY *display);

   void (*set_icons)(ALLEGRO_DISPLAY *display, int num_icons, ALLEGRO_BITMAP *bitmap[]);

   void (*set_window_position)(ALLEGRO_DISPLAY *display, int x, int y);
   void (*get_window_position)(ALLEGRO_DISPLAY *display, int *x, int *y);
   bool (*set_display_flag)(ALLEGRO_DISPLAY *display, int flag, bool onoff);
   void (*set_window_title)(ALLEGRO_DISPLAY *display, const char *title);
   
   void (*flush_vertex_cache)(ALLEGRO_DISPLAY *d);
   void* (*prepare_vertex_cache)(ALLEGRO_DISPLAY *d, int num_new_vertices);
   
   void (*update_transformation)(ALLEGRO_DISPLAY* d, ALLEGRO_BITMAP *target);

   void (*shutdown)(void);
};


struct ALLEGRO_OGL_EXTRAS;

typedef struct ALLEGRO_BLENDER
{
   int blend_op;
   int blend_source;
   int blend_dest;
   int blend_alpha_op;
   int blend_alpha_source;
   int blend_alpha_dest;
} ALLEGRO_BLENDER;

/* These are settings Allegro itself doesn't really care about on its
 * own, but which users may want to specify for a display anyway.
 */
ALLEGRO_STATIC_ASSERT(aintern_display, ALLEGRO_DISPLAY_OPTIONS_COUNT <= 32);
typedef struct
{
   int required, suggested; /* Bitfields. */
   int settings[ALLEGRO_DISPLAY_OPTIONS_COUNT];

   /* These are come in handy when creating a context. */
   void *info;
   int index, score;
} ALLEGRO_EXTRA_DISPLAY_SETTINGS;

struct ALLEGRO_DISPLAY
{
   /* Must be first, so the display can be used as event source. */
   ALLEGRO_EVENT_SOURCE es;
   ALLEGRO_DISPLAY_INTERFACE *vt;
   int refresh_rate;
   int flags;
   int w, h;
   
   int backbuffer_format; /* ALLEGRO_PIXELFORMAT */

   ALLEGRO_EXTRA_DISPLAY_SETTINGS extra_settings;
   struct ALLEGRO_OGL_EXTRAS *ogl_extras;

   _AL_VECTOR bitmaps; /* A list of bitmaps created for this display. */
   
   int num_cache_vertices;
   bool cache_enabled;
   int vertex_cache_size;
   void* vertex_cache;
   uintptr_t cache_texture;
   
   ALLEGRO_BLENDER cur_blender;
   
   void (*display_invalidated)(ALLEGRO_DISPLAY*);
};

int  _al_score_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref);
void _al_fill_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds);
void _al_set_color_components(int format, ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, int importance);
int  _al_deduce_color_format(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds);
int  _al_display_settings_sorter(const void *p0, const void *p1);

void _al_destroy_display_bitmaps(ALLEGRO_DISPLAY *d);

/* This is called from the primitives addon. */
AL_FUNC(void, _al_set_display_invalidated_callback, (ALLEGRO_DISPLAY *display,
   void (*display_invalidated)(ALLEGRO_DISPLAY*)));

/* Defined in tls.c */
bool _al_set_current_display_only(ALLEGRO_DISPLAY *display);
void _al_set_new_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *settings);
ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_get_new_display_settings(void);

#ifdef __cplusplus
}
#endif

#endif
