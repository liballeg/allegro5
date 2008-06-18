/*  ALLEGRO_SAMPLE header file
 *  User visible sample functions
 *  by Ryan Dickie
 *  TODO: documentation
 */
#ifndef A5_SAMPLE_H
#define A5_SAMPLE_H

typedef struct ALLEGRO_SAMPLE ALLEGRO_SAMPLE;
A5_AUDIO_FUNC(ALLEGRO_SAMPLE*, al_sample_create, (void *buf, unsigned long samples, unsigned long frequency, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf, bool free_buffer));
A5_AUDIO_FUNC(void, al_sample_destroy, (ALLEGRO_SAMPLE *sample));
A5_AUDIO_FUNC(ALLEGRO_AUDIO_ENUM, al_sample_get_depth, (const ALLEGRO_SAMPLE *sample));
A5_AUDIO_FUNC(ALLEGRO_AUDIO_ENUM, al_sample_get_channels, (const ALLEGRO_SAMPLE *sample));
A5_AUDIO_FUNC(unsigned long, al_sample_get_frequency, (const ALLEGRO_SAMPLE* sample));
A5_AUDIO_FUNC(unsigned long, al_sample_get_length, (const ALLEGRO_SAMPLE* sample));
A5_AUDIO_FUNC(float, al_sample_get_time, (const ALLEGRO_SAMPLE* sample));
A5_AUDIO_FUNC(void*, al_sample_get_buffer, (const ALLEGRO_SAMPLE* sample));
A5_AUDIO_FUNC(void, al_sample_set_buffer, (ALLEGRO_SAMPLE* sample, void* buf));
/* FIXME: rename or make internal */
A5_AUDIO_FUNC(int,  al_audio_buffer_size, (const ALLEGRO_SAMPLE* sample));
A5_AUDIO_FUNC(int,  al_audio_size_bytes, (int samples, ALLEGRO_AUDIO_ENUM channels, ALLEGRO_AUDIO_ENUM depth));
#endif
