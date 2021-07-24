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
bool _al_identify_wav(ALLEGRO_FILE *f);

/*
 * Built-in Port of A4 Creative Voice file (.voc) Loader.
 * should not implement streams since it's unlikely this container
 * will ever be used as such.
 */
ALLEGRO_SAMPLE *_al_load_voc(const char *filename);
ALLEGRO_SAMPLE *_al_load_voc_f(ALLEGRO_FILE *fp);
bool _al_identify_voc(ALLEGRO_FILE *f);


#ifdef ALLEGRO_CFG_ACODEC_FLAC
ALLEGRO_SAMPLE *_al_load_flac(const char *filename);
ALLEGRO_SAMPLE *_al_load_flac_f(ALLEGRO_FILE *f);
ALLEGRO_AUDIO_STREAM *_al_load_flac_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_flac_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples);
bool _al_identify_flac(ALLEGRO_FILE *f);
#endif

#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
bool _al_register_dumb_loaders(void);
#endif

#ifdef ALLEGRO_CFG_ACODEC_VORBIS
ALLEGRO_SAMPLE *_al_load_ogg_vorbis(const char *filename);
ALLEGRO_SAMPLE *_al_load_ogg_vorbis_f(ALLEGRO_FILE *file);
ALLEGRO_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream_f(ALLEGRO_FILE* file,
   size_t buffer_count, unsigned int samples);
bool _al_identify_ogg_vorbis(ALLEGRO_FILE *f);
#endif

#ifdef ALLEGRO_CFG_ACODEC_OPUS
ALLEGRO_SAMPLE *_al_load_ogg_opus(const char *filename);
ALLEGRO_SAMPLE *_al_load_ogg_opus_f(ALLEGRO_FILE *file);
ALLEGRO_AUDIO_STREAM *_al_load_ogg_opus_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_ogg_opus_audio_stream_f(ALLEGRO_FILE* file,
   size_t buffer_count, unsigned int samples);
bool _al_identify_ogg_opus(ALLEGRO_FILE *f);
#endif

#ifdef ALLEGRO_CFG_ACODEC_MP3
ALLEGRO_SAMPLE *_al_load_mp3(const char *filename);
ALLEGRO_SAMPLE *_al_load_mp3_f(ALLEGRO_FILE *f);
ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples);
bool _al_identify_mp3(ALLEGRO_FILE *f);
#endif

#endif
