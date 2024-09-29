#ifndef __al_included_allegro5_display_h
#define __al_included_allegro5_display_h

#include "allegro5/bitmap.h"
#include "allegro5/color.h"
#include "allegro5/events.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Possible bit combinations for the flags parameter of al_create_display. */
enum {
   A5O_WINDOWED                    = 1 << 0,
   A5O_FULLSCREEN                  = 1 << 1,
   A5O_OPENGL                      = 1 << 2,
   A5O_DIRECT3D_INTERNAL           = 1 << 3,
   A5O_RESIZABLE                   = 1 << 4,
   A5O_FRAMELESS                   = 1 << 5,
   A5O_NOFRAME                     = A5O_FRAMELESS, /* older synonym */
   A5O_GENERATE_EXPOSE_EVENTS      = 1 << 6,
   A5O_OPENGL_3_0                  = 1 << 7,
   A5O_OPENGL_FORWARD_COMPATIBLE   = 1 << 8,
   A5O_FULLSCREEN_WINDOW           = 1 << 9,
   A5O_MINIMIZED                   = 1 << 10,
   A5O_PROGRAMMABLE_PIPELINE       = 1 << 11,
   A5O_GTK_TOPLEVEL_INTERNAL       = 1 << 12,
   A5O_MAXIMIZED                   = 1 << 13,
   A5O_OPENGL_ES_PROFILE           = 1 << 14,
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
   A5O_OPENGL_CORE_PROFILE         = 1 << 15,
   A5O_DRAG_AND_DROP               = 1 << 16,
#endif
};

/* Possible parameters for al_set_display_option.
 * Make sure to update A5O_EXTRA_DISPLAY_SETTINGS if you modify
 * anything here.
 */
enum A5O_DISPLAY_OPTIONS {
   A5O_RED_SIZE = 0,
   A5O_GREEN_SIZE = 1,
   A5O_BLUE_SIZE = 2,
   A5O_ALPHA_SIZE = 3,
   A5O_RED_SHIFT = 4,
   A5O_GREEN_SHIFT = 5,
   A5O_BLUE_SHIFT = 6,
   A5O_ALPHA_SHIFT = 7,
   A5O_ACC_RED_SIZE = 8,
   A5O_ACC_GREEN_SIZE = 9,
   A5O_ACC_BLUE_SIZE = 10,
   A5O_ACC_ALPHA_SIZE = 11,
   A5O_STEREO = 12,
   A5O_AUX_BUFFERS = 13,
   A5O_COLOR_SIZE = 14,
   A5O_DEPTH_SIZE = 15,
   A5O_STENCIL_SIZE = 16,
   A5O_SAMPLE_BUFFERS = 17,
   A5O_SAMPLES = 18,
   A5O_RENDER_METHOD = 19,
   A5O_FLOAT_COLOR = 20,
   A5O_FLOAT_DEPTH = 21,
   A5O_SINGLE_BUFFER = 22,
   A5O_SWAP_METHOD = 23,
   A5O_COMPATIBLE_DISPLAY = 24,
   A5O_UPDATE_DISPLAY_REGION = 25,
   A5O_VSYNC = 26,
   A5O_MAX_BITMAP_SIZE = 27,
   A5O_SUPPORT_NPOT_BITMAP = 28,
   A5O_CAN_DRAW_INTO_BITMAP = 29,
   A5O_SUPPORT_SEPARATE_ALPHA = 30,
   A5O_AUTO_CONVERT_BITMAPS = 31,
   A5O_SUPPORTED_ORIENTATIONS = 32,
   A5O_OPENGL_MAJOR_VERSION = 33,
   A5O_OPENGL_MINOR_VERSION = 34,
   A5O_DEFAULT_SHADER_PLATFORM = 35,
   A5O_DISPLAY_OPTIONS_COUNT
};

enum
{
   A5O_DONTCARE,
   A5O_REQUIRE,
   A5O_SUGGEST
};


/* Bitflags so they can be used for the A5O_SUPPORTED_ORIENTATIONS option. */
enum A5O_DISPLAY_ORIENTATION
{
   A5O_DISPLAY_ORIENTATION_UNKNOWN = 0,
   A5O_DISPLAY_ORIENTATION_0_DEGREES = 1,
   A5O_DISPLAY_ORIENTATION_90_DEGREES = 2,
   A5O_DISPLAY_ORIENTATION_180_DEGREES = 4,
   A5O_DISPLAY_ORIENTATION_270_DEGREES = 8,
   A5O_DISPLAY_ORIENTATION_PORTRAIT = 5,
   A5O_DISPLAY_ORIENTATION_LANDSCAPE = 10,
   A5O_DISPLAY_ORIENTATION_ALL = 15,
   A5O_DISPLAY_ORIENTATION_FACE_UP = 16,
   A5O_DISPLAY_ORIENTATION_FACE_DOWN = 32
};


/* Formally part of the primitives addon. */
enum
{
   _A5O_PRIM_MAX_USER_ATTR = 10
};


/* Type: A5O_DISPLAY
 */
typedef struct A5O_DISPLAY A5O_DISPLAY;


/* Enum: A5O_NEW_WINDOW_TITLE_MAX_SIZE
*/
#define A5O_NEW_WINDOW_TITLE_MAX_SIZE 255

AL_FUNC(void, al_set_new_display_refresh_rate, (int refresh_rate));
AL_FUNC(void, al_set_new_display_flags, (int flags));
AL_FUNC(int,  al_get_new_display_refresh_rate, (void));
AL_FUNC(int,  al_get_new_display_flags, (void));

AL_FUNC(void, al_set_new_window_title, (const char *title));
AL_FUNC(const char *, al_get_new_window_title, (void));

AL_FUNC(int, al_get_display_width,  (A5O_DISPLAY *display));
AL_FUNC(int, al_get_display_height, (A5O_DISPLAY *display));
AL_FUNC(int, al_get_display_format, (A5O_DISPLAY *display));
AL_FUNC(int, al_get_display_refresh_rate, (A5O_DISPLAY *display));
AL_FUNC(int, al_get_display_flags,  (A5O_DISPLAY *display));
AL_FUNC(int, al_get_display_orientation, (A5O_DISPLAY* display));
AL_FUNC(bool, al_set_display_flag, (A5O_DISPLAY *display, int flag, bool onoff));

AL_FUNC(A5O_DISPLAY*, al_create_display, (int w, int h));
AL_FUNC(void,             al_destroy_display, (A5O_DISPLAY *display));
AL_FUNC(A5O_DISPLAY*, al_get_current_display, (void));
AL_FUNC(void,            al_set_target_bitmap, (A5O_BITMAP *bitmap));
AL_FUNC(void,            al_set_target_backbuffer, (A5O_DISPLAY *display));
AL_FUNC(A5O_BITMAP*, al_get_backbuffer,    (A5O_DISPLAY *display));
AL_FUNC(A5O_BITMAP*, al_get_target_bitmap, (void));

AL_FUNC(bool, al_acknowledge_resize, (A5O_DISPLAY *display));
AL_FUNC(bool, al_resize_display,     (A5O_DISPLAY *display, int width, int height));
AL_FUNC(void, al_flip_display,       (void));
AL_FUNC(void, al_update_display_region, (int x, int y, int width, int height));
AL_FUNC(bool, al_is_compatible_bitmap, (A5O_BITMAP *bitmap));

AL_FUNC(bool, al_wait_for_vsync, (void));

AL_FUNC(A5O_EVENT_SOURCE *, al_get_display_event_source, (A5O_DISPLAY *display));

AL_FUNC(void, al_set_display_icon, (A5O_DISPLAY *display, A5O_BITMAP *icon));
AL_FUNC(void, al_set_display_icons, (A5O_DISPLAY *display, int num_icons, A5O_BITMAP *icons[]));

/* Stuff for multihead/window management */
AL_FUNC(int, al_get_new_display_adapter, (void));
AL_FUNC(void, al_set_new_display_adapter, (int adapter));
AL_FUNC(void, al_set_new_window_position, (int x, int y));
AL_FUNC(void, al_get_new_window_position, (int *x, int *y));
AL_FUNC(void, al_set_window_position, (A5O_DISPLAY *display, int x, int y));
AL_FUNC(void, al_get_window_position, (A5O_DISPLAY *display, int *x, int *y));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(bool, al_get_window_borders, (A5O_DISPLAY *display, int *left, int *top, int *right, int *bottom));
#endif
AL_FUNC(bool, al_set_window_constraints, (A5O_DISPLAY *display, int min_w, int min_h, int max_w, int max_h));
AL_FUNC(bool, al_get_window_constraints, (A5O_DISPLAY *display, int *min_w, int *min_h, int *max_w, int *max_h));
AL_FUNC(void, al_apply_window_constraints, (A5O_DISPLAY *display, bool onoff));

AL_FUNC(void, al_set_window_title, (A5O_DISPLAY *display, const char *title));

/* Defined in display_settings.c */
AL_FUNC(void, al_set_new_display_option, (int option, int value, int importance));
AL_FUNC(int, al_get_new_display_option, (int option, int *importance));
AL_FUNC(void, al_reset_new_display_options, (void));
AL_FUNC(void, al_set_display_option, (A5O_DISPLAY *display, int option, int value));
AL_FUNC(int, al_get_display_option, (A5O_DISPLAY *display, int option));

/*Deferred drawing*/
AL_FUNC(void, al_hold_bitmap_drawing, (bool hold));
AL_FUNC(bool, al_is_bitmap_drawing_held, (void));

/* Miscellaneous */
AL_FUNC(void, al_acknowledge_drawing_halt, (A5O_DISPLAY *display));
AL_FUNC(void, al_acknowledge_drawing_resume, (A5O_DISPLAY *display));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(void, al_backup_dirty_bitmaps, (A5O_DISPLAY *display));
#endif

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
