#ifndef DEMO_H_INCLUDED
#define DEMO_H_INCLUDED

#define A5O_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>
#include "../speed/a4_aux.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "data.h"

extern int max_fps;
extern int cheat;
typedef struct PALETTE PALETTE;
#define PAL_SIZE 256
struct PALETTE {
    A5O_COLOR rgb[PAL_SIZE];
};
typedef A5O_FONT FONT;
typedef A5O_SAMPLE SAMPLE;
#define SCREEN_W (al_get_display_width(al_get_current_display()))
#define SCREEN_H (al_get_display_height(al_get_current_display()))


A5O_COLOR get_palette(int p);
void set_palette(PALETTE *p);
void fade_out(int divider);

#endif
