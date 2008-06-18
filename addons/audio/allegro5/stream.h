/*  ALLEGRO_STREAM header file
 *  User visible stream functions
 *  by Ryan Dickie
 *  TODO: documentation
 */
#ifndef A5_STREAM_H
#define A5_STREAM_H

/* ALLEGRO_STREAM: */
typedef struct ALLEGRO_STREAM ALLEGRO_STREAM;
/* yes this is very long and I cannot break it into multiple lines due to AL_FUNC. I also leave named parameters for the natural docs */
A5_AUDIO_FUNC(ALLEGRO_STREAM*,al_stream_create,(unsigned long frequency, enum ALLEGRO_AUDIO_ENUM depth, enum ALLEGRO_AUDIO_ENUM chan_conf, bool (*stream_update)(ALLEGRO_STREAM* stream, void* data, unsigned long samples),void (*stream_close)(ALLEGRO_STREAM*)));
A5_AUDIO_FUNC(void, al_stream_destroy, (ALLEGRO_STREAM* stream));
A5_AUDIO_FUNC(bool, al_stream_is_dry, (ALLEGRO_STREAM* stream));
#endif
