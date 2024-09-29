#ifndef __al_included_acodec_acodec_h
#define __al_included_acodec_acodec_h

#include "allegro5/internal/aintern_acodec_cfg.h"

A5O_SAMPLE *_al_load_wav(const char *filename);
A5O_SAMPLE *_al_load_wav_f(A5O_FILE *fp);
A5O_AUDIO_STREAM *_al_load_wav_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
A5O_AUDIO_STREAM *_al_load_wav_audio_stream_f(A5O_FILE* f,
   size_t buffer_count, unsigned int samples);
bool _al_save_wav(const char *filename, A5O_SAMPLE *spl);
bool _al_save_wav_f(A5O_FILE *pf, A5O_SAMPLE *spl);
bool _al_identify_wav(A5O_FILE *f);

/*
 * Built-in Port of A4 Creative Voice file (.voc) Loader.
 * should not implement streams since it's unlikely this container
 * will ever be used as such.
 */
A5O_SAMPLE *_al_load_voc(const char *filename);
A5O_SAMPLE *_al_load_voc_f(A5O_FILE *fp);
bool _al_identify_voc(A5O_FILE *f);


#ifdef A5O_CFG_ACODEC_FLAC
A5O_SAMPLE *_al_load_flac(const char *filename);
A5O_SAMPLE *_al_load_flac_f(A5O_FILE *f);
A5O_AUDIO_STREAM *_al_load_flac_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
A5O_AUDIO_STREAM *_al_load_flac_audio_stream_f(A5O_FILE* f,
   size_t buffer_count, unsigned int samples);
bool _al_identify_flac(A5O_FILE *f);
#endif

#ifdef A5O_CFG_ACODEC_DUMB
bool _al_register_dumb_loaders(void);
#endif

#ifdef A5O_CFG_ACODEC_OPENMPT
bool _al_register_openmpt_loaders(void);
#endif

#if defined(A5O_CFG_ACODEC_DUMB) || defined(A5O_CFG_ACODEC_OPENMPT)
bool _al_identify_it(A5O_FILE *f);
bool _al_identify_669(A5O_FILE *f);
bool _al_identify_amf(A5O_FILE *f);
bool _al_identify_asy(A5O_FILE *f);
bool _al_identify_mtm(A5O_FILE *f);
bool _al_identify_okt(A5O_FILE *f);
bool _al_identify_psm(A5O_FILE *f);
bool _al_identify_ptm(A5O_FILE *f);
bool _al_identify_riff(A5O_FILE *f);
bool _al_identify_stm(A5O_FILE *f);
bool _al_identify_mod(A5O_FILE *f);
bool _al_identify_s3m(A5O_FILE *f);
bool _al_identify_xm(A5O_FILE *f);
#endif

#ifdef A5O_CFG_ACODEC_VORBIS
A5O_SAMPLE *_al_load_ogg_vorbis(const char *filename);
A5O_SAMPLE *_al_load_ogg_vorbis_f(A5O_FILE *file);
A5O_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
A5O_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream_f(A5O_FILE* file,
   size_t buffer_count, unsigned int samples);
bool _al_identify_ogg_vorbis(A5O_FILE *f);
#endif

#ifdef A5O_CFG_ACODEC_OPUS
A5O_SAMPLE *_al_load_ogg_opus(const char *filename);
A5O_SAMPLE *_al_load_ogg_opus_f(A5O_FILE *file);
A5O_AUDIO_STREAM *_al_load_ogg_opus_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
A5O_AUDIO_STREAM *_al_load_ogg_opus_audio_stream_f(A5O_FILE* file,
   size_t buffer_count, unsigned int samples);
bool _al_identify_ogg_opus(A5O_FILE *f);
#endif

#ifdef A5O_CFG_ACODEC_MP3
A5O_SAMPLE *_al_load_mp3(const char *filename);
A5O_SAMPLE *_al_load_mp3_f(A5O_FILE *f);
A5O_AUDIO_STREAM *_al_load_mp3_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
A5O_AUDIO_STREAM *_al_load_mp3_audio_stream_f(A5O_FILE* f,
   size_t buffer_count, unsigned int samples);
bool _al_identify_mp3(A5O_FILE *f);
#endif

#endif
