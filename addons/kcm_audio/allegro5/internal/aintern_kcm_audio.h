/* internal-only header
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */
#ifndef AINTERN_AUDIO_H
#define AINTERN_AUDIO_H

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_vector.h"
#include "../kcm_audio.h"

/* This can probably be set to 16, or higher, if long is 64-bit */
#define MIXER_FRAC_SHIFT  8
#define MIXER_FRAC_ONE    (1<<MIXER_FRAC_SHIFT)
#define MIXER_FRAC_MASK   (MIXER_FRAC_ONE-1)

typedef struct ALLEGRO_AUDIO_DRIVER ALLEGRO_AUDIO_DRIVER;
struct ALLEGRO_AUDIO_DRIVER {
   const char     *specifier;

   int            (*open)();
   void           (*close)();

   int            (*allocate_voice)(ALLEGRO_VOICE*);
   void           (*deallocate_voice)(ALLEGRO_VOICE*);

   int            (*load_voice)(ALLEGRO_VOICE*, const void*);
   void           (*unload_voice)(ALLEGRO_VOICE*);

   int            (*start_voice)(ALLEGRO_VOICE*);
   int            (*stop_voice)(ALLEGRO_VOICE*);

   bool           (*voice_is_playing)(const ALLEGRO_VOICE*);

   unsigned long  (*get_voice_position)(const ALLEGRO_VOICE*);
   int            (*set_voice_position)(ALLEGRO_VOICE*, unsigned long);
};

extern ALLEGRO_AUDIO_DRIVER *_al_kcm_driver;

const void *_al_voice_update(ALLEGRO_VOICE *voice, unsigned long samples);

/* A voice structure that you'd attach a mixer or sample to. Ideally there
 * would be one ALLEGRO_VOICE per system/hardware voice.
 */
struct ALLEGRO_VOICE {
   ALLEGRO_AUDIO_DEPTH  depth;
   ALLEGRO_CHANNEL_CONF chan_conf;

   unsigned long        frequency;

   size_t               buffer_size;
   size_t               num_buffers;
                        /* If non-0, they must be honored by the driver. */

   ALLEGRO_SAMPLE       *attached_stream;
                        /* The stream that is attached to the voice, or NULL.
                         * May be an ALLEGRO_SAMPLE or ALLEGRO_MIXER object.
                         */

   bool                 is_streaming;
                        /* True for voices with an attached mixer. */

   ALLEGRO_MUTEX        *mutex;

   ALLEGRO_AUDIO_DRIVER *driver;
                        /* XXX shouldn't there only be one audio driver active
                         * at a time?
                         */

   void                 *extra;
                        /* Extra data for use by the driver. */
};


typedef union {
   float    *f32;
   uint32_t *u24;
   int32_t  *s24;
   uint16_t *u16;
   int16_t  *s16;
   uint8_t  *u8;
   int8_t   *s8;
   void     *ptr;
} any_buffer_t;

struct ALLEGRO_SAMPLE_DATA {
   ALLEGRO_AUDIO_DEPTH  depth;
   ALLEGRO_CHANNEL_CONF chan_conf;
   unsigned long        frequency;
   unsigned long        len;
   any_buffer_t         buffer;
   bool                 free_buf;
                        /* Whether `buffer' needs to be freed when the sample
                         * is destroyed, or when `buffer' changes.
                         */
};


typedef void (*stream_reader_t)(void *, void **, unsigned long,
   ALLEGRO_AUDIO_DEPTH, size_t);

typedef struct {
   union {
      ALLEGRO_MIXER     *mixer;
      ALLEGRO_VOICE     *voice;
      void              *ptr;
   } u;
   bool                 is_voice;
} sample_parent_t;

/* The sample struct also serves the base of ALLEGRO_STREAM, ALLEGRO_MIXER. */
struct ALLEGRO_SAMPLE {
   ALLEGRO_SAMPLE_DATA  spl_data;

   volatile bool        is_playing;
                        /* Is this sample is playing? */

   volatile bool        is_stream;
                        /* Is this sample is part of a ALLEGRO_STREAM object?
                         * XXX this should be accounted for in the loop mode
                         */

   ALLEGRO_PLAYMODE     loop;
   float                speed;

   unsigned long        pos;
   unsigned long        loop_start;
   unsigned long        loop_end;
   long                 step;

   float                *matrix;
                        /* Used to convert from this format to the attached
                         * mixers, if any.  Otherwise is NULL.
                         */

   stream_reader_t      spl_read;
                        /* Reads sample data into the provided buffer, using
                         * the specified format, converting as necessary.
                         */

   ALLEGRO_MUTEX        *mutex;
                        /* Points to the parent object's mutex.  It is NULL if
                         * the sample is not directly or indirectly attached
                         * to a voice.
                         */

   sample_parent_t      parent;
                        /* The object that this sample is attached to, if any.
                         */
};

void _al_kcm_stream_set_mutex(ALLEGRO_SAMPLE *stream, ALLEGRO_MUTEX *mutex);
void _al_kcm_detach_from_parent(ALLEGRO_SAMPLE *spl);


struct ALLEGRO_STREAM {
   ALLEGRO_SAMPLE       spl;
                        /* ALLEGRO_STREAM is derived from ALLEGRO_SAMPLE. */

   size_t               buf_count;
                        /* The stream buffer is divided into a number of
                         * fragments; this is the number of fragments.
                         */

   void                 *main_buffer;
                        /* Pointer to a single buffer big enough to hold all
                         * the fragments.
                         */

   void                 **pending_bufs;
   void                 **used_bufs;
                        /* Arrays of offsets into the main_buffer.
                         * The arrays are each 'buf_count' long.
                         *
                         * 'pending_bufs' holds pointers to fragments supplied
                         * by the user which are yet to be handed off to the
                         * audio driver.
                         *
                         * 'used_bufs' holds pointers to fragments which
                         * have been sent to the audio driver and so are
                         * ready to receive new data.
                         */
};


typedef void (*postprocess_callback_t)(void *buf, unsigned long samples,
   void *userdata);

/* ALLEGRO_MIXER is derived from ALLEGRO_SAMPLE. Certain internal functions and
 * pointers may take either object type, and such things are explicitly noted.
 * This is never exposed to the user, though.  The sample object's read method
 * will be set to a different function that will call the read method of all
 * attached streams (which may be a sample, or another mixer).
 */
struct ALLEGRO_MIXER {
   ALLEGRO_SAMPLE          ss;
                           /* ALLEGRO_MIXER is derived from ALLEGRO_SAMPLE. */

   ALLEGRO_MIXER_QUALITY   quality;

   postprocess_callback_t  postprocess_callback;
   void                    *pp_callback_userdata;

   _AL_VECTOR              streams;
                           /* Vector of ALLEGRO_SAMPLE*.  Holds the list of
                            * streams being mixed together.
                            */
};


typedef enum {
   ALLEGRO_NO_ERROR       = 0,
   ALLEGRO_INVALID_PARAM  = 1,
   ALLEGRO_INVALID_OBJECT = 2,
   ALLEGRO_GENERIC_ERROR  = 255
} AL_ERROR_ENUM;

extern void _al_set_error(int error, char* string);
extern int _al_audio_get_silence(ALLEGRO_AUDIO_DEPTH depth);

#endif

/* vim: set sts=3 sw=3 et: */
