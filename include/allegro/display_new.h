#ifndef ALLEGRO_DISPLAY_NEW_H
#define ALLEGRO_DISPLAY_NEW_H

typedef struct AL_COLOR AL_COLOR;

typedef struct AL_DISPLAY AL_DISPLAY;

struct AL_COLOR
{
   float r, g, b, a;
};

/* Possible bit combinations for the flags parameter of al_create_display. */

#define AL_COLOR_AUTO 0
#define AL_COLOR_32 1
#define AL_COLOR_16 2
#define AL_COLOR_8 3

#define AL_WINDOWED 4
#define AL_FULLSCREEN 8

#define AL_OPENGL 16
#define AL_DIRECT3D 32
#define AL_GENERATE_UPDATE_EVENTS 64
#define AL_RESIZABLE 128

#define AL_UPDATE_IMMEDIATE         0x0001
#define AL_UPDATE_DOUBLE_BUFFER     0x0002

#define al_color(r, g, b, a) (AL_COLOR){r, g, b, a}

void al_set_display_parameters(int format, int refresh_rate, int flags);
void al_get_display_parameters(int *format, int *refresh_rate, int *flags);
AL_DISPLAY *al_create_display(int w, int h);
void al_destroy_display(AL_DISPLAY *display);
void al_change_current_display(AL_DISPLAY *display);
void al_clear(AL_COLOR color);
void al_line(float fx, float fy, float tx, float ty, AL_COLOR color);
void al_filled_rectangle(float tlx, float tly, float brx, float bry,
    AL_COLOR color);
void al_notify_resize(void);
void al_flip_display(void);
void al_flip_display_region(int x, int y,
	int width, int height);
AL_DISPLAY *al_get_current_display(void);

#endif
