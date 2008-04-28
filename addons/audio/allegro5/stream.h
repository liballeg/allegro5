/*  ALLEGRO_STREAM header file
 *  User visible stream functions
 *  by Ryan Dickie
 *  TODO: documentation
 */
#ifndef A5_STREAM_H
#define A5_STREAM_H

/* ALLEGRO_STREAM: */
typedef struct ALLEGRO_STREAM ALLEGRO_STREAM;
AL_FUNC(ALLEGRO_STREAM*, al_stream_create, (size_t buffer_count, unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf));
AL_FUNC(void, al_stream_destroy, (ALLEGRO_STREAM *stream));
/* get the sample the stream is currently playing. Call streams methods to get/set its properties */
AL_FUNC(ALLEGRO_SAMPLE*, al_stream_get_sample, (ALLEGRO_STREAM* stream));
#endif
