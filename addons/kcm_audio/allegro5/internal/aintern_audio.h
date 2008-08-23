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

const void* _al_voice_update(ALLEGRO_VOICE* voice, unsigned long samples);

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
   /* the stream or sample */
   ALLEGRO_SAMPLE* sample;
   /* true for streams or mixers */
   bool streaming;
   /* Extra data for use by the driver */
   void *extra;
};

/* The sample struct */
struct ALLEGRO_SAMPLE {
   ALLEGRO_AUDIO_ENUM depth;
   ALLEGRO_AUDIO_ENUM chan_conf;
   unsigned long frequency;
   unsigned long length;
   void* buffer;
   bool free_buffer;
};

/* stream inherits from sample */
struct ALLEGRO_STREAM {
   ALLEGRO_SAMPLE sample;

   size_t buf_count;

   void *main_buffer;

   void **pending_bufs;
   void **used_bufs;
};

#endif
