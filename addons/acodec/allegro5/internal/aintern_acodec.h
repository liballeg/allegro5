/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 * todo: add streaming support
 */

#ifndef AINTERN_ACODEC_H
#define AINTERN_ACODEC_H

#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_acodec_cfg.h"

#if defined(ALLEGRO_CFG_ACODEC_FLAC)
   ALLEGRO_SAMPLE *al_load_sample_flac(const char *filename);
   ALLEGRO_STREAM *al_load_stream_flac(const char *filename);
#endif

#if defined(ALLEGRO_CFG_ACODEC_SNDFILE)
   ALLEGRO_SAMPLE *al_load_sample_sndfile(const char *filename);
   ALLEGRO_STREAM *al_load_stream_sndfile(const char *filename,
       size_t buffer_count, unsigned int samples);
#endif

#if defined(ALLEGRO_CFG_ACODEC_VORBIS)
   ALLEGRO_SAMPLE *al_load_sample_oggvorbis(const char *filename);
   ALLEGRO_STREAM *al_load_stream_oggvorbis(const char *filename,
	size_t buffer_count, unsigned int samples);
#endif

/* TODO: move this into audio */
ALLEGRO_CHANNEL_CONF _al_count_to_channel_conf(int num_channels);
ALLEGRO_AUDIO_DEPTH _al_word_size_to_depth_conf(int word_size);

#endif
