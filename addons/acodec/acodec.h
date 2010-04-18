#ifndef __al_included_acodec_acodec_h
#define __al_included_acodec_acodec_h

#include "allegro5/internal/aintern_acodec_cfg.h"

ALLEGRO_SAMPLE *_al_load_wav(const char *filename);
ALLEGRO_SAMPLE *_al_load_wav_f(ALLEGRO_FILE *fp);
ALLEGRO_AUDIO_STREAM *_al_load_wav_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_wav_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples);
bool _al_save_wav(const char *filename, ALLEGRO_SAMPLE *spl);
bool _al_save_wav_f(ALLEGRO_FILE *pf, ALLEGRO_SAMPLE *spl);

#ifdef ALLEGRO_CFG_ACODEC_FLAC
ALLEGRO_SAMPLE *_al_load_flac(const char *filename);
ALLEGRO_SAMPLE *_al_load_flac_f(ALLEGRO_FILE *f);
ALLEGRO_AUDIO_STREAM *_al_load_flac_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_flac_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples);
#endif

#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
ALLEGRO_AUDIO_STREAM *_al_load_mod_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_it_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_xm_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_s3m_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_mod_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_it_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_xm_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_s3m_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples);
#endif

#ifdef ALLEGRO_CFG_ACODEC_VORBIS
ALLEGRO_SAMPLE *_al_load_ogg_vorbis(const char *filename);
ALLEGRO_SAMPLE *_al_load_ogg_vorbis_f(ALLEGRO_FILE *file);
ALLEGRO_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream_f(ALLEGRO_FILE* file,
   size_t buffer_count, unsigned int samples);
#endif

#endif
