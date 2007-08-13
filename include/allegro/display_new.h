#ifndef ALLEGRO_DISPLAY_NEW_H
#define ALLEGRO_DISPLAY_NEW_H

#include "allegro/color_new.h"
#include "allegro/bitmap_new.h"

/* Possible bit combinations for the flags parameter of al_create_display. */

#define _al_current_display al_get_current_display()

#define ALLEGRO_WINDOWED     1
#define ALLEGRO_FULLSCREEN   2
#define ALLEGRO_OPENGL       4
#define ALLEGRO_DIRECT3D     8
//#define ALLEGRO_GENERATE_UPDATE_EVENTS 64
#define ALLEGRO_RESIZABLE    16
#define ALLEGRO_SINGLEBUFFER 32

/* Used for display mode queries */
typedef struct ALLEGRO_DISPLAY_MODE
{
   int width;
   int height;
   int format;
   int refresh_rate;
} ALLEGRO_DISPLAY_MODE;

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

ALLEGRO_DISPLAY *al_create_display(int w, int h);
void al_destroy_display(ALLEGRO_DISPLAY *display);
void al_set_current_display(ALLEGRO_DISPLAY *display);
ALLEGRO_DISPLAY *al_get_current_display(void);
void al_set_target_bitmap(ALLEGRO_BITMAP *bitmap);
ALLEGRO_BITMAP *al_get_backbuffer(void);
ALLEGRO_BITMAP *al_get_frontbuffer(void);
ALLEGRO_BITMAP *al_get_target_bitmap(void);

bool al_acknowledge_resize(void);
bool al_resize_display(int width, int height);
void al_flip_display(void);
bool al_update_display_region(int x, int y,
	int width, int height);
ALLEGRO_DISPLAY *al_get_current_display(void);
bool al_is_compatible_bitmap(ALLEGRO_BITMAP *bitmap);

void _al_push_target_bitmap(void);
void _al_pop_target_bitmap(void);

int al_get_num_display_modes(void);
ALLEGRO_DISPLAY_MODE *al_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode);

bool al_wait_for_vsync(void);

/* Primitives */
void al_clear(ALLEGRO_COLOR *color);
void al_draw_line(float fx, float fy, float tx, float ty, ALLEGRO_COLOR *color, int flags);
void al_draw_rectangle(float tlx, float tly, float brx, float bry,
    ALLEGRO_COLOR *color, int flags);

#endif
