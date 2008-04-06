/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 * todo: add streaming support
 */

#ifndef AINTERN_ACODEC_H
#define AINTERN_ACODEC_H

#include "../../../audio/allegro5/audio.h"

#cmakedefine ALLEGRO_CFG_ACODEC_FLAC
#cmakedefine ALLEGRO_CFG_ACODEC_SNDFILE
#cmakedefine ALLEGRO_CFG_ACODEC_OGG

#if defined(ALLEGRO_CFG_ACODEC_FLAC)
   ALLEGRO_SAMPLE* al_load_sample_flac(const char *filename);
#endif

#if defined(ALLEGRO_CFG_ACODEC_SNDFILE)
   ALLEGRO_SAMPLE* al_load_sample_sndfile(const char *filename);
#endif

#if defined(ALLEGRO_CFG_ACODEC_OGG)
   ALLEGRO_SAMPLE* al_load_sample_oggvorbis(const char *filename);
#endif

#endif
