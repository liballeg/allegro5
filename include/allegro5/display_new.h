/* Title: Display types
 */

#ifndef ALLEGRO_DISPLAY_NEW_H
#define ALLEGRO_DISPLAY_NEW_H

#include "allegro5/color_new.h"
#include "allegro5/bitmap_new.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Possible bit combinations for the flags parameter of al_create_display. */

#define ALLEGRO_WINDOWED     1
#define ALLEGRO_FULLSCREEN   2
// moved to a5_opengl.h
//#define ALLEGRO_OPENGL       4
// moved to a5_direct3d.h
//#define ALLEGRO_DIRECT3D     8
#define ALLEGRO_RESIZABLE    16
//#define ALLEGRO_SINGLEBUFFER 32
#define ALLEGRO_NOFRAME      64
#define ALLEGRO_GENERATE_EXPOSE_EVENTS 128


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
   ALLEGRO_DISPLAY_OPTIONS_COUNT
};

enum
{
   ALLEGRO_DONTCARE,
   ALLEGRO_REQUIRE,
   ALLEGRO_SUGGEST
};



/* Type: ALLEGRO_DISPLAY_MODE
 *
 * Used for display mode queries. Contains information
 * about a supported fullscreen display mode.
 *
 * >
 * > typedef struct ALLEGRO_DISPLAY_MODE {
 * >        int width;          // Screen width
 * >        int height;         // Screen height
 * >        int format;         // The pixel format of the mode
 * >        int refresh_rate;   // The refresh rate of the mode
 * > } ALLEGRO_DISPLAY_MODE;
 */
typedef struct ALLEGRO_DISPLAY_MODE
{
   int width;
   int height;
   int format;
   int refresh_rate;
} ALLEGRO_DISPLAY_MODE;


/* Type: ALLEGRO_MONITOR_INFO
 *
 * Describes a monitors size and position relative to other
 * monitors. x1, y1 will be 0, 0 on the primary display.
 * Other monitors can have negative values if they are to the
 * left or above the primary display.
 *
 * > typedef struct ALLEGRO_MONITOR_INFO
 * > {
 * >    int x1;
 * >    int y1;
 * >    int x2;
 * >    int y2;
 * > } ALLEGRO_MONITOR_INFO;
 */
typedef struct ALLEGRO_MONITOR_INFO
{
   int x1;
   int y1;
   int x2;
   int y2;
} ALLEGRO_MONITOR_INFO;

AL_FUNC(void, al_set_new_display_format, (int format));
AL_FUNC(void, al_set_new_display_refresh_rate, (int refresh_rate));
AL_FUNC(void, al_set_new_display_flags, (int flags));
AL_FUNC(int,  al_get_new_display_format, (void));
AL_FUNC(int,  al_get_new_display_refresh_rate, (void));
AL_FUNC(int,  al_get_new_display_flags, (void));

AL_FUNC(int, al_get_display_width,  (void));
AL_FUNC(int, al_get_display_height, (void));
AL_FUNC(int, al_get_display_format, (void));
AL_FUNC(int, al_get_display_refresh_rate, (void));
AL_FUNC(int, al_get_display_flags,  (void));

AL_FUNC(ALLEGRO_DISPLAY*, al_create_display, (int w, int h));
AL_FUNC(void,             al_destroy_display, (ALLEGRO_DISPLAY *display));
AL_FUNC(bool,             al_set_current_display, (ALLEGRO_DISPLAY *display));
AL_FUNC(ALLEGRO_DISPLAY*, al_get_current_display, (void));
AL_FUNC(void,            al_set_target_bitmap, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(ALLEGRO_BITMAP*, al_get_backbuffer,    (void));
AL_FUNC(ALLEGRO_BITMAP*, al_get_frontbuffer,   (void));
AL_FUNC(ALLEGRO_BITMAP*, al_get_target_bitmap, (void));

AL_FUNC(bool, al_acknowledge_resize, (ALLEGRO_DISPLAY *display));
AL_FUNC(bool, al_resize_display,     (int width, int height));
AL_FUNC(void, al_flip_display,       (void));
AL_FUNC(void, al_update_display_region, (int x, int y, int width, int height));
AL_FUNC(bool, al_is_compatible_bitmap, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(int, al_get_num_display_modes, (void));
AL_FUNC(ALLEGRO_DISPLAY_MODE*, al_get_display_mode, (int index,
        ALLEGRO_DISPLAY_MODE *mode));

AL_FUNC(bool, al_wait_for_vsync, (void));

/* Primitives */
AL_FUNC(void, al_clear, (ALLEGRO_COLOR color));
AL_FUNC(void, al_draw_pixel, (float x, float y, ALLEGRO_COLOR color));

AL_FUNC(void, al_set_display_icon, (ALLEGRO_BITMAP *icon));

/* Stuff for multihead/window management */
AL_FUNC(int, al_get_num_video_adapters, (void));
AL_FUNC(void, al_get_monitor_info, (int adapter, ALLEGRO_MONITOR_INFO *info));
AL_FUNC(int, al_get_current_video_adapter, (void));
AL_FUNC(void, al_set_current_video_adapter, (int adapter));
AL_FUNC(void, al_set_new_window_position, (int x, int y));
AL_FUNC(void, al_get_new_window_position, (int *x, int *y));
AL_FUNC(void, al_set_window_position, (ALLEGRO_DISPLAY *display, int x, int y));
AL_FUNC(void, al_get_window_position, (ALLEGRO_DISPLAY *display, int *x, int *y));
AL_FUNC(void, al_toggle_window_frame, (ALLEGRO_DISPLAY *display, bool onoff));
AL_FUNC(void, al_set_window_title, (AL_CONST char *title));

/* Defined in display_settings.c */
AL_FUNC(void, al_set_new_display_option, (int option, int value, int importance));
AL_FUNC(int, al_get_new_display_option, (int option, int *importance));
AL_FUNC(int, al_get_display_option, (int option));
AL_FUNC(void, al_clear_display_options, (void));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
