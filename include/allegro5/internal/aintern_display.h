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

typedef struct A5O_DISPLAY_INTERFACE A5O_DISPLAY_INTERFACE;

struct A5O_DISPLAY_INTERFACE
{
   int id;
   A5O_DISPLAY *(*create_display)(int w, int h);
   void (*destroy_display)(A5O_DISPLAY *display);
   bool (*set_current_display)(A5O_DISPLAY *d);
   void (*unset_current_display)(A5O_DISPLAY *d);
   void (*clear)(A5O_DISPLAY *d, A5O_COLOR *color);
   void (*draw_pixel)(A5O_DISPLAY *d, float x, float y, A5O_COLOR *color);
   void (*flip_display)(A5O_DISPLAY *d);
   void (*update_display_region)(A5O_DISPLAY *d, int x, int y,
   	int width, int height);
   bool (*acknowledge_resize)(A5O_DISPLAY *d);
   bool (*resize_display)(A5O_DISPLAY *d, int width, int height);
   void (*quick_size)(A5O_DISPLAY *d);
   int (*get_orientation)(A5O_DISPLAY *d);

   A5O_BITMAP *(*create_bitmap)(A5O_DISPLAY *d,
      int w, int h, int format, int flags);

   void (*set_target_bitmap)(A5O_DISPLAY *display, A5O_BITMAP *bitmap);
   A5O_BITMAP *(*get_backbuffer)(A5O_DISPLAY *d);

   bool (*is_compatible_bitmap)(A5O_DISPLAY *display, A5O_BITMAP *bitmap);
   void (*switch_out)(A5O_DISPLAY *display);
   void (*switch_in)(A5O_DISPLAY *display);

   void (*draw_memory_bitmap_region)(A5O_DISPLAY *display, A5O_BITMAP *bitmap,
      float sx, float sy, float sw, float sh, int flags);

   bool (*wait_for_vsync)(A5O_DISPLAY *display);

   bool (*set_mouse_cursor)(A5O_DISPLAY *display,
      A5O_MOUSE_CURSOR *cursor);
   bool (*set_system_mouse_cursor)(A5O_DISPLAY *display,
      A5O_SYSTEM_MOUSE_CURSOR cursor_id);
   bool (*show_mouse_cursor)(A5O_DISPLAY *display);
   bool (*hide_mouse_cursor)(A5O_DISPLAY *display);

   void (*set_icons)(A5O_DISPLAY *display, int num_icons, A5O_BITMAP *bitmap[]);

   void (*set_window_position)(A5O_DISPLAY *display, int x, int y);
   void (*get_window_position)(A5O_DISPLAY *display, int *x, int *y);
   bool (*get_window_borders)(A5O_DISPLAY *display, int *left, int *top, int *right, int *bottom);
   bool (*set_window_constraints)(A5O_DISPLAY *display, int min_w, int min_h, int max_w, int max_h);
   bool (*get_window_constraints)(A5O_DISPLAY *display,  int *min_w, int *min_h, int *max_w, int *max_h);
   bool (*set_display_flag)(A5O_DISPLAY *display, int flag, bool onoff);
   void (*set_window_title)(A5O_DISPLAY *display, const char *title);
   
   void (*flush_vertex_cache)(A5O_DISPLAY *d);
   void* (*prepare_vertex_cache)(A5O_DISPLAY *d, int num_new_vertices);
   
   void (*update_transformation)(A5O_DISPLAY* d, A5O_BITMAP *target);

   /* Unused */
   void (*shutdown)(void);

   void (*acknowledge_drawing_halt)(A5O_DISPLAY *d);
   void (*acknowledge_drawing_resume)(A5O_DISPLAY *d);
      
   void (*set_display_option)(A5O_DISPLAY *display, int option, int val);

   void (*clear_depth_buffer)(A5O_DISPLAY *display, float x);
   void (*update_render_state)(A5O_DISPLAY *display);
   
   char *(*get_clipboard_text)(A5O_DISPLAY *display);
   bool  (*set_clipboard_text)(A5O_DISPLAY *display, const char *text);
   bool  (*has_clipboard_text)(A5O_DISPLAY *display);

   /* Issue #725 */
   void (*apply_window_constraints)(A5O_DISPLAY *display, bool onoff);
};


struct A5O_OGL_EXTRAS;

typedef struct A5O_BLENDER
{
   int blend_op;
   int blend_source;
   int blend_dest;
   int blend_alpha_op;
   int blend_alpha_source;
   int blend_alpha_dest;
   A5O_COLOR blend_color;
} A5O_BLENDER;

typedef struct _A5O_RENDER_STATE {
   int write_mask;
   int depth_test, depth_function;
   int alpha_test, alpha_function, alpha_test_value;
} _A5O_RENDER_STATE;


/* These are settings Allegro itself doesn't really care about on its
 * own, but which users may want to specify for a display anyway.
 */
A5O_STATIC_ASSERT(aintern_display, A5O_DISPLAY_OPTIONS_COUNT <= 64);
typedef struct
{
   int64_t required, suggested; /* Bitfields. */
   int settings[A5O_DISPLAY_OPTIONS_COUNT];

   /* These are come in handy when creating a context. */
   void *info;
   int index, score;
} A5O_EXTRA_DISPLAY_SETTINGS;

struct A5O_DISPLAY
{
   /* Must be first, so the display can be used as event source. */
   A5O_EVENT_SOURCE es;
   A5O_DISPLAY_INTERFACE *vt;
   int refresh_rate;
   int flags;
   int w, h;
   int min_w, min_h;
   int max_w, max_h;

   int backbuffer_format; /* A5O_PIXELFORMAT */

   A5O_EXTRA_DISPLAY_SETTINGS extra_settings;
   struct A5O_OGL_EXTRAS *ogl_extras;

   /* A list of bitmaps created for this display, sub-bitmaps not included. */
   _AL_VECTOR bitmaps;

   int num_cache_vertices;
   bool cache_enabled;
   int vertex_cache_size;
   void* vertex_cache;
   uintptr_t cache_texture;

   A5O_BLENDER cur_blender;

   A5O_SHADER* default_shader;

   A5O_TRANSFORM projview_transform;

   _A5O_RENDER_STATE render_state;

   _AL_VECTOR display_invalidated_callbacks;
   _AL_VECTOR display_validated_callbacks;

   /* Issue #725 */
   bool use_constraints;
};

int  _al_score_display_settings(A5O_EXTRA_DISPLAY_SETTINGS *eds, A5O_EXTRA_DISPLAY_SETTINGS *ref);
void _al_fill_display_settings(A5O_EXTRA_DISPLAY_SETTINGS *eds);
void _al_set_color_components(int format, A5O_EXTRA_DISPLAY_SETTINGS *eds, int importance);
int  _al_deduce_color_format(A5O_EXTRA_DISPLAY_SETTINGS *eds);
int  _al_display_settings_sorter(const void *p0, const void *p1);

int _al_get_suggested_display_option(A5O_DISPLAY *d,
   int option, int default_value);

/* This is called from the primitives addon and for shaders. */
AL_FUNC(void, _al_add_display_invalidated_callback, (A5O_DISPLAY *display,
   void (*display_invalidated)(A5O_DISPLAY*)));
AL_FUNC(void, _al_add_display_validated_callback, (A5O_DISPLAY *display,
   void (*display_validated)(A5O_DISPLAY*)));
AL_FUNC(void, _al_remove_display_invalidated_callback, (A5O_DISPLAY *display,
   void (*display_invalidated)(A5O_DISPLAY*)));
AL_FUNC(void, _al_remove_display_validated_callback, (A5O_DISPLAY *display,
   void (*display_validated)(A5O_DISPLAY*)));

/* Defined in tls.c */
bool _al_set_current_display_only(A5O_DISPLAY *display);
void _al_set_new_display_settings(A5O_EXTRA_DISPLAY_SETTINGS *settings);
A5O_EXTRA_DISPLAY_SETTINGS *_al_get_new_display_settings(void);


#ifdef __cplusplus
}
#endif

#endif
