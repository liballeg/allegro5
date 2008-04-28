/**
 * allegro image codec loader
 * author: Ryan Dickie (c) 2008
 */

#ifndef ICODEC_H
#define ICODEC_H

#include "allegro5/allegro5.h"

int al_save_image(ALLEGRO_BITMAP* bmp, const char* path);
ALLEGRO_BITMAP* al_load_image(const char* path);

#endif
