#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"

#define ALLEGRO_TTF_NO_KERNING 1

ALLEGRO_FONT *al_ttf_load_font(char const *filename, int size, int flags);
