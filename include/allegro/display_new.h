#ifndef ALLEGRO_DISPLAY_NEW_H
#define ALLEGRO_DISPLAY_NEW_H

#include "allegro/color_new.h"
#include "allegro/bitmap_new.h"

/* Possible bit combinations for the flags parameter of al_create_display. */

#define AL_WINDOWED     1
#define AL_FULLSCREEN   2
#define AL_OPENGL       4
#define AL_DIRECT3D     8
//#define AL_GENERATE_UPDATE_EVENTS 64
#define AL_RESIZABLE    16
#define AL_SINGLEBUFFER 32

#define al_color(r, g, b, a) (AL_COLOR){r, g, b, a}

void al_set_display_parameters(int format, int refresh_rate, int flags);
void al_get_display_parameters(int *format, int *refresh_rate, int *flags);
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
void al_notify_resize(void);
void al_flip_display(void);
void al_flip_display_region(int x, int y,
	int width, int height);
AL_DISPLAY *al_get_current_display(void);

#endif
