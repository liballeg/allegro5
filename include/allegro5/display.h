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
   ALLEGRO_WINDOWED                    = 1 << 0,
   ALLEGRO_FULLSCREEN                  = 1 << 1,
   ALLEGRO_OPENGL                      = 1 << 2,
   ALLEGRO_DIRECT3D_INTERNAL           = 1 << 3,
   ALLEGRO_RESIZABLE                   = 1 << 4,
   ALLEGRO_FRAMELESS                   = 1 << 5,
   ALLEGRO_NOFRAME                     = ALLEGRO_FRAMELESS, /* older synonym */
   ALLEGRO_GENERATE_EXPOSE_EVENTS      = 1 << 6,
   ALLEGRO_OPENGL_3_0                  = 1 << 7,
   ALLEGRO_OPENGL_FORWARD_COMPATIBLE   = 1 << 8,
   ALLEGRO_FULLSCREEN_WINDOW           = 1 << 9,
   ALLEGRO_MINIMIZED                   = 1 << 10
};

/* Possible parameters for al_set_display_option.
 * Make sure to update ALLEGRO_EXTRA_DISPLAY_SETTINGS if you modify
 * anything here.
 */
enum ALLEGRO_DISPLAY_OPTIONS {
   ALLEGRO_RED_SIZE,
   ALLEGRO_GREEN_SIZE,
   ALLEGRO_BLUE_SIZE,
   ALLEGRO_ALPHA_SIZE,
   ALLEGRO_RED_SHIFT,
   ALLEGRO_GREEN_SHIFT,
   ALLEGRO_BLUE_SHIFT,
   ALLEGRO_ALPHA_SHIFT,
   ALLEGRO_ACC_RED_SIZE,
   ALLEGRO_ACC_GREEN_SIZE,
   ALLEGRO_ACC_BLUE_SIZE,
   ALLEGRO_ACC_ALPHA_SIZE,
   ALLEGRO_STEREO,
   ALLEGRO_AUX_BUFFERS,
   ALLEGRO_COLOR_SIZE,
   ALLEGRO_DEPTH_SIZE,
   ALLEGRO_STENCIL_SIZE,
   ALLEGRO_SAMPLE_BUFFERS,
   ALLEGRO_SAMPLES,
   ALLEGRO_RENDER_METHOD,
   ALLEGRO_FLOAT_COLOR,
   ALLEGRO_FLOAT_DEPTH,
   ALLEGRO_SINGLE_BUFFER,
   ALLEGRO_SWAP_METHOD,
   ALLEGRO_COMPATIBLE_DISPLAY,
   ALLEGRO_UPDATE_DISPLAY_REGION,
   ALLEGRO_VSYNC,
   ALLEGRO_MAX_BITMAP_SIZE,
   ALLEGRO_SUPPORT_NPOT_BITMAP,
   ALLEGRO_CAN_DRAW_INTO_BITMAP,
   ALLEGRO_SUPPORT_SEPARATE_ALPHA,
   ALLEGRO_DISPLAY_OPTIONS_COUNT
};

enum
{
   ALLEGRO_DONTCARE,
   ALLEGRO_REQUIRE,
   ALLEGRO_SUGGEST
};


enum ALLEGRO_DISPLAY_ORIENTATION
{
   ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES,
   ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES,
   ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES,
   ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES,
   ALLEGRO_DISPLAY_ORIENTATION_FACE_UP,
   ALLEGRO_DISPLAY_ORIENTATION_FACE_DOWN
};


/* Type: ALLEGRO_DISPLAY
 */
typedef struct ALLEGRO_DISPLAY ALLEGRO_DISPLAY;


AL_FUNC(void, al_set_new_display_refresh_rate, (int refresh_rate));
AL_FUNC(void, al_set_new_display_flags, (int flags));
AL_FUNC(int,  al_get_new_display_refresh_rate, (void));
AL_FUNC(int,  al_get_new_display_flags, (void));

AL_FUNC(int, al_get_display_width,  (ALLEGRO_DISPLAY *display));
AL_FUNC(int, al_get_display_height, (ALLEGRO_DISPLAY *display));
AL_FUNC(int, al_get_display_format, (ALLEGRO_DISPLAY *display));
AL_FUNC(int, al_get_display_refresh_rate, (ALLEGRO_DISPLAY *display));
AL_FUNC(int, al_get_display_flags,  (ALLEGRO_DISPLAY *display));
AL_FUNC(bool, al_set_display_flag, (ALLEGRO_DISPLAY *display, int flag, bool onoff));
AL_FUNC(bool, al_toggle_display_flag, (ALLEGRO_DISPLAY *display, int flag, bool onoff));

AL_FUNC(ALLEGRO_DISPLAY*, al_create_display, (int w, int h));
AL_FUNC(void,             al_destroy_display, (ALLEGRO_DISPLAY *display));
AL_FUNC(ALLEGRO_DISPLAY*, al_get_current_display, (void));
AL_FUNC(void,            al_set_target_bitmap, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(void,            al_set_target_backbuffer, (ALLEGRO_DISPLAY *display));
AL_FUNC(ALLEGRO_BITMAP*, al_get_backbuffer,    (ALLEGRO_DISPLAY *display));
AL_FUNC(ALLEGRO_BITMAP*, al_get_target_bitmap, (void));

AL_FUNC(bool, al_acknowledge_resize, (ALLEGRO_DISPLAY *display));
AL_FUNC(bool, al_resize_display,     (ALLEGRO_DISPLAY *display, int width, int height));
AL_FUNC(void, al_flip_display,       (void));
AL_FUNC(void, al_update_display_region, (int x, int y, int width, int height));
AL_FUNC(bool, al_is_compatible_bitmap, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(bool, al_wait_for_vsync, (void));

AL_FUNC(ALLEGRO_EVENT_SOURCE *, al_get_display_event_source, (ALLEGRO_DISPLAY *display));

AL_FUNC(void, al_set_display_icon, (ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *icon));
AL_FUNC(void, al_set_display_icons, (ALLEGRO_DISPLAY *display, int num_icons, ALLEGRO_BITMAP *icons[]));

/* Stuff for multihead/window management */
AL_FUNC(int, al_get_new_display_adapter, (void));
AL_FUNC(void, al_set_new_display_adapter, (int adapter));
AL_FUNC(void, al_set_new_window_position, (int x, int y));
AL_FUNC(void, al_get_new_window_position, (int *x, int *y));
AL_FUNC(void, al_set_window_position, (ALLEGRO_DISPLAY *display, int x, int y));
AL_FUNC(void, al_get_window_position, (ALLEGRO_DISPLAY *display, int *x, int *y));

AL_FUNC(void, al_set_window_title, (ALLEGRO_DISPLAY *display, const char *title));

/* Defined in display_settings.c */
AL_FUNC(void, al_set_new_display_option, (int option, int value, int importance));
AL_FUNC(int, al_get_new_display_option, (int option, int *importance));
AL_FUNC(void, al_reset_new_display_options, (void));
AL_FUNC(int, al_get_display_option, (ALLEGRO_DISPLAY *display, int option));

/*Deferred drawing*/
AL_FUNC(void, al_hold_bitmap_drawing, (bool hold));
AL_FUNC(bool, al_is_bitmap_drawing_held, (void));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
