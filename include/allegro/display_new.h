#ifndef ALLEGRO_DISPLAY_NEW_H
#define ALLEGRO_DISPLAY_NEW_H

#include "allegro/color_new.h"
#include "allegro/bitmap_new.h"

/* Possible bit combinations for the flags parameter of al_create_display. */

#define _al_current_display al_get_current_display()

#define AL_WINDOWED     1
#define AL_FULLSCREEN   2
#define AL_OPENGL       4
#define AL_DIRECT3D     8
//#define AL_GENERATE_UPDATE_EVENTS 64
#define AL_RESIZABLE    16
#define AL_SINGLEBUFFER 32

/* Used for display mode queries */
typedef struct AL_DISPLAY_MODE
{
   int width;
   int height;
   int format;
   int refresh_rate;
} AL_DISPLAY_MODE;

void al_set_new_display_format(int format);
void al_set_new_display_refresh_rate(int refresh_rate);
void al_set_new_display_flags(int flags);
int al_get_new_display_format(void);
int al_get_new_display_refresh_rate(void);
int al_get_new_display_flags(void);

int al_get_display_width(void);
int al_get_display_height(void);
int al_get_display_format(void);
int al_get_display_refresh_rate(void);
int al_get_display_flags(void);

AL_DISPLAY *al_create_display(int w, int h);
void al_destroy_display(AL_DISPLAY *display);
void al_set_current_display(AL_DISPLAY *display);
AL_DISPLAY *al_get_current_display(void);
void al_set_target_bitmap(AL_BITMAP *bitmap);
AL_BITMAP *al_get_backbuffer(void);
AL_BITMAP *al_get_frontbuffer(void);
AL_BITMAP *al_get_target_bitmap(void);
void al_clear(AL_COLOR *color);
void al_draw_line(float fx, float fy, float tx, float ty, AL_COLOR *color);
void al_draw_filled_rectangle(float tlx, float tly, float brx, float bry,
    AL_COLOR *color);
bool al_acknowledge_resize(void);
bool al_resize_display(int width, int height);
void al_flip_display(void);
bool al_update_display_region(int x, int y,
	int width, int height);
AL_DISPLAY *al_get_current_display(void);
bool al_is_compatible_bitmap(AL_BITMAP *bitmap);

void _al_push_target_bitmap(void);
void _al_pop_target_bitmap(void);

int al_get_num_display_modes(void);
AL_DISPLAY_MODE *al_get_display_mode(int index, AL_DISPLAY_MODE *mode);

bool al_wait_for_vsync(void);

#endif
