#ifndef __al_included_allegro5_aintern_display_h
#define __al_included_allegro5_aintern_display_h

#include "allegro5/allegro.h"
#include "allegro5/transformations.h"
#include "allegro5/bitmap.h"
#include "allegro5/display.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_primitives.h"

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
   int (*get_orientation)(ALLEGRO_DISPLAY *d);

   ALLEGRO_BITMAP *(*create_bitmap)(ALLEGRO_DISPLAY *d,
      int w, int h, int format, int flags);

   void (*set_target_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
   ALLEGRO_BITMAP *(*get_backbuffer)(ALLEGRO_DISPLAY *d);

   bool (*is_compatible_bitmap)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
   void (*switch_out)(ALLEGRO_DISPLAY *display);
   void (*switch_in)(ALLEGRO_DISPLAY *display);

   void (*draw_memory_bitmap_region)(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap,
      float sx, float sy, float sw, float sh, int flags);

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
   bool (*get_window_borders)(ALLEGRO_DISPLAY *display, int *left, int *top, int *right, int *bottom);
   bool (*set_window_constraints)(ALLEGRO_DISPLAY *display, int min_w, int min_h, int max_w, int max_h);
   bool (*get_window_constraints)(ALLEGRO_DISPLAY *display,  int *min_w, int *min_h, int *max_w, int *max_h);
   bool (*set_display_flag)(ALLEGRO_DISPLAY *display, int flag, bool onoff);
   void (*set_window_title)(ALLEGRO_DISPLAY *display, const char *title);

   /* Old batching api. */
   void (*flush_vertex_cache)(ALLEGRO_DISPLAY *d);
   void* (*prepare_vertex_cache)(ALLEGRO_DISPLAY *d, int num_new_vertices);

   /* New batching api. */
   int (*prepare_batch)(ALLEGRO_DISPLAY* d, ALLEGRO_BITMAP *bitmap, ALLEGRO_PRIM_TYPE type,
      int num_new_vertices, int num_new_indices, void **vertices, void **indices);
   void (*draw_batch)(ALLEGRO_DISPLAY *d);

   void (*update_transformation)(ALLEGRO_DISPLAY* d, ALLEGRO_BITMAP *target);

   /* Unused */
   void (*shutdown)(void);

   void (*acknowledge_drawing_halt)(ALLEGRO_DISPLAY *d);
   void (*acknowledge_drawing_resume)(ALLEGRO_DISPLAY *d);

   void (*set_display_option)(ALLEGRO_DISPLAY *display, int option, int val);

   void (*clear_depth_buffer)(ALLEGRO_DISPLAY *display, float x);
   void (*update_render_state)(ALLEGRO_DISPLAY *display);

   char *(*get_clipboard_text)(ALLEGRO_DISPLAY *display);
   bool  (*set_clipboard_text)(ALLEGRO_DISPLAY *display, const char *text);
   bool  (*has_clipboard_text)(ALLEGRO_DISPLAY *display);

   /* Issue #725 */
   void (*apply_window_constraints)(ALLEGRO_DISPLAY *display, bool onoff);

   /* Primitives */
   int (*draw_prim)(ALLEGRO_BITMAP *target, ALLEGRO_BITMAP *texture, const void *vtxs, const ALLEGRO_VERTEX_DECL *decl, int start, int end, int type);
   int (*draw_prim_indexed)(ALLEGRO_BITMAP *target, ALLEGRO_BITMAP *texture, const void *vtxs, const ALLEGRO_VERTEX_DECL *decl, const int *indices, int num_vtx, int type);

   bool  (*create_vertex_buffer)(ALLEGRO_VERTEX_BUFFER *buf, const void *initial_data, size_t num_vertices, int flags);
   void  (*destroy_vertex_buffer)(ALLEGRO_VERTEX_BUFFER *buf);
   void *(*lock_vertex_buffer)(ALLEGRO_VERTEX_BUFFER *buf);
   void  (*unlock_vertex_buffer)(ALLEGRO_VERTEX_BUFFER *buf);

   bool (*create_vertex_decl)(ALLEGRO_DISPLAY *display, ALLEGRO_VERTEX_DECL *decl);

   bool  (*create_index_buffer)(ALLEGRO_INDEX_BUFFER *buf, const void *initial_data, size_t num_indices, int flags);
   void  (*destroy_index_buffer)(ALLEGRO_INDEX_BUFFER *buf);
   void *(*lock_index_buffer)(ALLEGRO_INDEX_BUFFER *buf);
   void  (*unlock_index_buffer)(ALLEGRO_INDEX_BUFFER *buf);

   int (*draw_vertex_buffer)(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, int start, int end, int type);
   int (*draw_indexed_buffer)(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type);
};


struct ALLEGRO_OGL_EXTRAS;

typedef struct _ALLEGRO_RENDER_STATE {
   int write_mask;
   int depth_test, depth_function;
   int alpha_test, alpha_function, alpha_test_value;
} _ALLEGRO_RENDER_STATE;


/* These are settings Allegro itself doesn't really care about on its
 * own, but which users may want to specify for a display anyway.
 */
ALLEGRO_STATIC_ASSERT(aintern_display, ALLEGRO_DISPLAY_OPTIONS_COUNT <= 64);
typedef struct
{
   int64_t required, suggested; /* Bitfields. */
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
   int min_w, min_h;
   int max_w, max_h;

   int backbuffer_format; /* ALLEGRO_PIXELFORMAT */

   ALLEGRO_EXTRA_DISPLAY_SETTINGS extra_settings;
   struct ALLEGRO_OGL_EXTRAS *ogl_extras;

   /* A list of bitmaps created for this display, sub-bitmaps not included. */
   _AL_VECTOR bitmaps;

   /* Old batching api. */
   int num_cache_vertices;
   bool cache_enabled;
   int vertex_cache_size;
   void* vertex_cache;
   uintptr_t cache_texture;

   /* New batching api.*/
   ALLEGRO_VERTEX_DECL *batch_vertex_decl;
   int index_size;
   ALLEGRO_VERTEX_BUFFER *batch_vertex_buffer;
   ALLEGRO_INDEX_BUFFER *batch_index_buffer;
   void *batch_vertices;
   void *batch_indices;
   int batch_vertices_length;
   int batch_indices_length;
   int batch_vertices_capacity;
   int batch_indices_capacity;
   bool batch_enabled;
   /* Wait, is this right? How does this work for sub-bitmaps? */
   ALLEGRO_BITMAP *batch_bitmap;
   ALLEGRO_PRIM_TYPE batch_type;

   ALLEGRO_BLENDER cur_blender;

   ALLEGRO_SHADER* default_shader;

   ALLEGRO_TRANSFORM projview_transform;

   _ALLEGRO_RENDER_STATE render_state;

   _AL_VECTOR display_invalidated_callbacks;
   _AL_VECTOR display_validated_callbacks;

   /* Issue #725 */
   bool use_constraints;

   bool use_legacy_drawing_api;
};

#ifdef ALLEGRO_CFG_OPENGLES2
   typedef uint16_t _AL_BATCH_INDEX_TYPE;
#else
   typedef int _AL_BATCH_INDEX_TYPE;
#endif

int  _al_score_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref);
void _al_fill_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds);
void _al_set_color_components(int format, ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, int importance);
int  _al_deduce_color_format(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds);
int  _al_display_settings_sorter(const void *p0, const void *p1);
/* This two are internal implementation, call _al_prepare_batch/_al_draw_batch instead in non-implementation code. */
int  _al_default_prepare_batch(ALLEGRO_DISPLAY *disp, ALLEGRO_BITMAP *bitmap,
   ALLEGRO_PRIM_TYPE type, int num_new_vertices, int num_new_indices, void **vertices, void **indices);
void _al_default_draw_batch(ALLEGRO_DISPLAY *disp);

int _al_get_suggested_display_option(ALLEGRO_DISPLAY *d,
   int option, int default_value);

/* This is called from the primitives addon and for shaders. */
AL_FUNC(void, _al_add_display_invalidated_callback, (ALLEGRO_DISPLAY *display,
   void (*display_invalidated)(ALLEGRO_DISPLAY*)));
AL_FUNC(void, _al_add_display_validated_callback, (ALLEGRO_DISPLAY *display,
   void (*display_validated)(ALLEGRO_DISPLAY*)));
AL_FUNC(void, _al_remove_display_invalidated_callback, (ALLEGRO_DISPLAY *display,
   void (*display_invalidated)(ALLEGRO_DISPLAY*)));
AL_FUNC(void, _al_remove_display_validated_callback, (ALLEGRO_DISPLAY *display,
   void (*display_validated)(ALLEGRO_DISPLAY*)));

/* Defined in tls.c */
bool _al_set_current_display_only(ALLEGRO_DISPLAY *display);
void _al_set_new_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *settings);
ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_get_new_display_settings(void);


#ifdef __cplusplus
}
#endif

#endif
