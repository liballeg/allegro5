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

#define _al_current_display al_get_current_display()

#define ALLEGRO_WINDOWED     1
#define ALLEGRO_FULLSCREEN   2
#define ALLEGRO_OPENGL       4
#define ALLEGRO_DIRECT3D     8
//#define ALLEGRO_GENERATE_UPDATE_EVENTS 64
#define ALLEGRO_RESIZABLE    16
#define ALLEGRO_SINGLEBUFFER 32

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


// FIXME: document
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
AL_FUNC(bool, al_update_display_region, (int x, int y, int width, int height));
AL_FUNC(bool, al_is_compatible_bitmap, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(void, _al_push_target_bitmap, (void));
AL_FUNC(void, _al_pop_target_bitmap, (void));

AL_FUNC(int, al_get_num_display_modes, (void));
AL_FUNC(ALLEGRO_DISPLAY_MODE*, al_get_display_mode, (int index,
        ALLEGRO_DISPLAY_MODE *mode));

AL_FUNC(bool, al_wait_for_vsync, (void));

/* Primitives */
AL_FUNC(void, al_clear, (ALLEGRO_COLOR color));
AL_FUNC(void, al_draw_line, (float fx, float fy, float tx, float ty, ALLEGRO_COLOR color));
AL_FUNC(void, al_draw_rectangle, (float tlx, float tly, float brx, float bry,
   ALLEGRO_COLOR color, int flags));
AL_FUNC(void, al_draw_pixel, (float x, float y, ALLEGRO_COLOR color));

AL_FUNC(void, al_set_display_icon, (ALLEGRO_BITMAP *icon));

AL_FUNC(int, al_get_num_video_adapters, (void));
AL_FUNC(void, al_get_monitor_info, (int adapter, ALLEGRO_MONITOR_INFO *info));
AL_FUNC(int, al_get_current_video_adapter, (void));
AL_FUNC(void, al_set_current_video_adapter, (int adapter));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
