/* internal-only header 
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */
#ifndef AINTERN_AUDIO_H
#define AINTERN_AUDIO_H

//aintern.h included because aintern_thread.h is not complete!!
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "../audio.h"

struct ALLEGRO_AUDIO_DRIVER {
   const char *specifier;

   int  (*open)();
   void (*close)();

   int (*get_bool)(ALLEGRO_AUDIO_ENUM, bool*);
   int (*get_enum)(ALLEGRO_AUDIO_ENUM, ALLEGRO_AUDIO_ENUM*);
   int (*get_long)(ALLEGRO_AUDIO_ENUM, unsigned long*);
   int (*get_ptr)(ALLEGRO_AUDIO_ENUM, const void**);

   int (*set_bool)(ALLEGRO_AUDIO_ENUM, bool);
   int (*set_enum)(ALLEGRO_AUDIO_ENUM, ALLEGRO_AUDIO_ENUM);
   int (*set_long)(ALLEGRO_AUDIO_ENUM, unsigned long);
   int (*set_ptr)(ALLEGRO_AUDIO_ENUM, const void*);

   int  (*allocate_voice)(ALLEGRO_VOICE*);
   void (*deallocate_voice)(ALLEGRO_VOICE*);

   int (*load_voice)(ALLEGRO_VOICE*, const void*);
   void (*unload_voice)(ALLEGRO_VOICE*);

   int  (*start_voice)(ALLEGRO_VOICE*);
   void (*stop_voice)(ALLEGRO_VOICE*);

   bool (*voice_is_playing)(const ALLEGRO_VOICE*);

   unsigned long (*get_voice_position)(const ALLEGRO_VOICE*);
   int (*set_voice_position)(ALLEGRO_VOICE*, unsigned long);
};

typedef struct ALLEGRO_AUDIO_DRIVER ALLEGRO_AUDIO_DRIVER;

int _al_cache_sample(ALLEGRO_SAMPLE *sample);
void _al_decache_sample(int i);
void _al_read_sample_cache(int i, unsigned int pos, bool reverse, void **buf, unsigned int *len);

const void *_al_voice_update(ALLEGRO_VOICE *voice, unsigned long samples);

/* A voice structure that you'd attach a mixer or sample to. Ideally there
   would be one ALLEGRO_VOICE per system/hardware voice */
struct ALLEGRO_VOICE {
   ALLEGRO_AUDIO_ENUM depth;
   ALLEGRO_AUDIO_ENUM chan_conf;
   ALLEGRO_AUDIO_ENUM settings;

   unsigned long frequency;

   /* If set to non-0, they must be honored by the driver */
   size_t buffer_size;
   size_t num_buffers;

   /* This may be an ALLEGRO_SAMPLE or ALLEGRO_MIXER object */
   ALLEGRO_SAMPLE *stream;
   /* True for voices with an attached mixer */
   bool streaming;

   _AL_MUTEX mutex;

   ALLEGRO_AUDIO_DRIVER *driver;

   /* Extra data for use by the driver */
   void *extra;
};


typedef void (*stream_reader)(void*,void**,unsigned long,ALLEGRO_AUDIO_ENUM,size_t);

/* The sample struct */
struct ALLEGRO_SAMPLE {
   volatile bool playing;
   volatile bool is_stream;

   ALLEGRO_AUDIO_ENUM loop;
   ALLEGRO_AUDIO_ENUM depth;
   ALLEGRO_AUDIO_ENUM chan_conf;
   unsigned long frequency;
   float speed;

   union {
      float    *f32;
      uint32_t *u24;
      int32_t  *s24;
      uint16_t *u16;
      int16_t  *s16;
      uint8_t  *u8;
      int8_t   *s8;
      void *ptr;
   } buffer;
   bool orphan_buffer;
   unsigned long pos, len;
   unsigned long loop_start, loop_end;
   long step;

   /* Used to convert from this format to the attached mixer's */
   float *matrix;

   /* Reads sample data into the provided buffer, using the specified format,
      converting as necessary */
   stream_reader read;

   /* The mutex is shared with the parent object. It is NULL if it is not
      directly or indirectly attached to a voice. */
   _AL_MUTEX *mutex;

   union {
      ALLEGRO_MIXER *mixer;
      ALLEGRO_VOICE *voice;
      void *ptr;
   } parent;
   bool parent_is_voice;
};


struct ALLEGRO_STREAM {
   ALLEGRO_SAMPLE spl;

   size_t buf_count;

   void *main_buffer;

   void **pending_bufs;
   void **used_bufs;
};


typedef void (*pp_callback)(void*,unsigned long,void*);

/* Slight evilness. ALLEGRO_MIXER is derived from ALLEGRO_SAMPLE. Certain internal
   functions and pointers may take either object type, and such things are
   explicitly noted. This is never exposed to the user, though.
   The sample object's read method will be set to a different function
   that will call the read method of all attached streams (which may be a
   sample, or another mixer) */
struct ALLEGRO_MIXER {
   ALLEGRO_SAMPLE ss;

   ALLEGRO_AUDIO_ENUM quality;

   pp_callback post_process;
   void *cb_ptr;

   ALLEGRO_SAMPLE **streams;
};

#endif
