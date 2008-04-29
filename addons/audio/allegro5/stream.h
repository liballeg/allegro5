/*  ALLEGRO_STREAM header file
 *  User visible stream functions
 *  by Ryan Dickie
 *  TODO: documentation
 */
#ifndef A5_STREAM_H
#define A5_STREAM_H

/* ALLEGRO_STREAM: */
typedef struct ALLEGRO_STREAM ALLEGRO_STREAM;

AL_FUNC(ALLEGRO_STREAM*,al_stream_create,(unsigned long frequency, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf, bool (*stream_update)(ALLEGRO_STREAM* stream, void* data, unsigned long samples)));
AL_FUNC(void, al_stream_destroy, (ALLEGRO_STREAM *stream));
#endif
