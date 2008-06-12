/* internal-only header 
 * Originally done by KC/Milan
 * Rewritten by Ryan Dickie
 */
#ifndef AINTERN_AUDIO_H
#define AINTERN_AUDIO_H

//aintern.h included because aintern_thread.h is not complete!!
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "../audio.h"

typedef struct ALLEGRO_AUDIO_DRIVER {
   int  (*open)();
   void (*close)();

   int  (*allocate_voice)(ALLEGRO_VOICE*);
   void (*deallocate_voice)(ALLEGRO_VOICE*);

   int  (*start_voice)(ALLEGRO_VOICE*);
   int  (*stop_voice)(ALLEGRO_VOICE*);

   bool (*voice_is_playing)(const ALLEGRO_VOICE*);

   unsigned long (*get_voice_position)(const ALLEGRO_VOICE*);
   int (*set_voice_position)(ALLEGRO_VOICE*, unsigned long);

   int (*set_loop_mode)(ALLEGRO_VOICE*, bool loop);
   bool (*get_loop_mode)(ALLEGRO_VOICE*);
}ALLEGRO_AUDIO_DRIVER;

/* A voice structure that you'd attach a mixer or sample to. Ideally there
   would be one ALLEGRO_VOICE per system/hardware voice */
struct ALLEGRO_VOICE {
   /* the sample */
   ALLEGRO_SAMPLE* sample;
   /* the stream */
   ALLEGRO_STREAM* stream;
   /* true for streams or mixers */
   volatile bool streaming;
   /* Extra data for use by the driver */
   void *extra;
};

/* The sample struct */
struct ALLEGRO_SAMPLE {
   unsigned long frequency;
   ALLEGRO_AUDIO_ENUM depth;
   ALLEGRO_AUDIO_ENUM chan_conf;
   unsigned long length;
   void* buffer;
   bool free_buffer;
};

/* stream inherits from sample */
struct ALLEGRO_STREAM {
   unsigned long frequency;
   ALLEGRO_AUDIO_ENUM depth;
   ALLEGRO_AUDIO_ENUM chan_conf;
   bool dried_up; /* TRUE if there is nothing left to get */
   bool (*stream_update)(ALLEGRO_STREAM*, void*, unsigned long);
   void (*stream_close)(ALLEGRO_STREAM*); /* called when the stream is destroyed */
   void* ex_data; /* extra data meant for the stream readers to use */
};



extern ALLEGRO_AUDIO_DRIVER* _al_audio_driver;
extern struct ALLEGRO_AUDIO_DRIVER _al_openal_driver;
#if defined(ALLEGRO_LINUX)
   extern struct ALLEGRO_AUDIO_DRIVER _al_alsa_driver;
#endif



extern const void* _al_voice_update(ALLEGRO_VOICE* voice, unsigned long samples);



#endif
