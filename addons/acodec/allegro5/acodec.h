/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 * todo: add streaming support
 */

#ifndef ACODEC_H
#define ACODEC_H

#include "../../audio/allegro5/audio.h"

ALLEGRO_SAMPLE* al_load_sample_flac(const char *filename);
ALLEGRO_SAMPLE* al_load_sample_sndfile(const char *filename);
ALLEGRO_SAMPLE* al_load_sample_oggvorbis(const char *filename);
ALLEGRO_SAMPLE* al_load_sample(const char* filename);

#endif
