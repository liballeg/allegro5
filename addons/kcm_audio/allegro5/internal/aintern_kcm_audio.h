/* internal-only header
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */
#ifndef AINTERN_AUDIO_H
#define AINTERN_AUDIO_H

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_vector.h"
#include "../kcm_audio.h"

/* This can probably be set to 16, or higher, if long is 64-bit */
#define MIXER_FRAC_SHIFT  8
#define MIXER_FRAC_ONE    (1<<MIXER_FRAC_SHIFT)
#define MIXER_FRAC_MASK   (MIXER_FRAC_ONE-1)

typedef struct ALLEGRO_AUDIO_DRIVER ALLEGRO_AUDIO_DRIVER;
struct ALLEGRO_AUDIO_DRIVER {
   const char     *specifier;

   int            (*open)(void);
   void           (*close)(void);

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

const void *_al_voice_update(ALLEGRO_VOICE *voice, unsigned long *samples);

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

   ALLEGRO_SAMPLE_INSTANCE       *attached_stream;
                        /* The stream that is attached to the voice, or NULL.
                         * May be an ALLEGRO_SAMPLE_INSTANCE or ALLEGRO_MIXER object.
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

struct ALLEGRO_SAMPLE {
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

typedef void (*stream_reader_t)(void *, void **, unsigned long *,
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
struct ALLEGRO_SAMPLE_INSTANCE {
   /* ALLEGRO_SAMPLE_INSTANCE does not generate any events yet but ALLEGRO_STREAM
    * does, which can inherit only ALLEGRO_SAMPLE_INSTANCE. */
   ALLEGRO_EVENT_SOURCE es;

   ALLEGRO_SAMPLE       spl_data;

   volatile bool        is_playing;
                        /* Is this sample is playing? */

   ALLEGRO_PLAYMODE     loop;
   float                speed;
   float                gain;
   float                pan;

   unsigned long        pos;
   unsigned long        loop_start;
   unsigned long        loop_end;
   long                 step;

   float                *matrix;
                        /* Used to convert from this format to the attached
                         * mixers, if any.  Otherwise is NULL.
                         * The gain is premultiplied in.
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

void _al_kcm_destroy_sample(ALLEGRO_SAMPLE_INSTANCE *sample, bool unregister);
void _al_kcm_stream_set_mutex(ALLEGRO_SAMPLE_INSTANCE *stream, ALLEGRO_MUTEX *mutex);
void _al_kcm_detach_from_parent(ALLEGRO_SAMPLE_INSTANCE *spl);


typedef size_t (*stream_callback_t)(ALLEGRO_STREAM *, void *, size_t);
typedef void (*unload_feeder_t)(ALLEGRO_STREAM *);
typedef bool (*rewind_feeder_t)(ALLEGRO_STREAM *);
typedef bool (*seek_feeder_t)(ALLEGRO_STREAM *, double);
typedef double (*get_feeder_position_t)(ALLEGRO_STREAM *);
typedef double (*get_feeder_length_t)(ALLEGRO_STREAM *);
typedef bool (*set_feeder_loop_t)(ALLEGRO_STREAM *, double, double);

struct ALLEGRO_STREAM {
   ALLEGRO_SAMPLE_INSTANCE       spl;
                        /* ALLEGRO_STREAM is derived from ALLEGRO_SAMPLE_INSTANCE. */

   unsigned int         buf_count;
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
   volatile bool         is_draining;
                         /* Set to true if sample data is not going to be passed
                          * to the stream any more. The stream must change its
                          * playing state to false after all buffers have been
                          * played.
                          */

   ALLEGRO_THREAD        *feed_thread;
   volatile bool         quit_feed_thread;
   unload_feeder_t       unload_feeder;
   rewind_feeder_t       rewind_feeder;
   seek_feeder_t         seek_feeder;
   get_feeder_position_t get_feeder_position;
   get_feeder_length_t   get_feeder_length;
   set_feeder_loop_t     set_feeder_loop;
   stream_callback_t     feeder;
                         /* If ALLEGRO_STREAM has been created by
                          * al_stream_from_file(), acodec will be feeding the stream
                          * from a feeding thread using the 'feeder' callback. Such
                          * streams don't need to be fed by the user.
                          */

   void                  *extra;
                         /* Extra data for use by acodec. */
};

bool _al_kcm_refill_stream(ALLEGRO_STREAM *stream);


typedef void (*postprocess_callback_t)(void *buf, unsigned long samples,
   void *userdata);

/* ALLEGRO_MIXER is derived from ALLEGRO_SAMPLE_INSTANCE. Certain internal functions and
 * pointers may take either object type, and such things are explicitly noted.
 * This is never exposed to the user, though.  The sample object's read method
 * will be set to a different function that will call the read method of all
 * attached streams (which may be a sample, or another mixer).
 */
struct ALLEGRO_MIXER {
   ALLEGRO_SAMPLE_INSTANCE          ss;
                           /* ALLEGRO_MIXER is derived from ALLEGRO_SAMPLE_INSTANCE. */

   ALLEGRO_MIXER_QUALITY   quality;

   postprocess_callback_t  postprocess_callback;
   void                    *pp_callback_userdata;

   _AL_VECTOR              streams;
                           /* Vector of ALLEGRO_SAMPLE_INSTANCE*.  Holds the list of
                            * streams being mixed together.
                            */
};

extern void _al_kcm_mixer_rejig_sample_matrix(ALLEGRO_MIXER *mixer,
   ALLEGRO_SAMPLE_INSTANCE *spl);
extern void _al_kcm_mixer_read(void *source, void **buf, unsigned long *samples,
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc);


typedef enum {
   ALLEGRO_NO_ERROR       = 0,
   ALLEGRO_INVALID_PARAM  = 1,
   ALLEGRO_INVALID_OBJECT = 2,
   ALLEGRO_GENERIC_ERROR  = 255
} AL_ERROR_ENUM;

extern void _al_set_error(int error, char* string);

/* Supposedly internal */
A5_KCM_AUDIO_FUNC(int, _al_audio_get_silence, (ALLEGRO_AUDIO_DEPTH depth));
A5_KCM_AUDIO_FUNC(void*, _al_kcm_feed_stream, (ALLEGRO_THREAD *self, void *vstream));

/* Helper to emit an event that the stream has got a buffer ready to be refilled. */
bool _al_kcm_emit_stream_event(ALLEGRO_STREAM *stream, unsigned long count);

void _al_kcm_init_destructors(void);
void _al_kcm_shutdown_destructors(void);
void _al_kcm_register_destructor(void *object, void (*func)(void*));
void _al_kcm_unregister_destructor(void *object);

A5_KCM_AUDIO_FUNC(void, _al_kcm_shutdown_default_mixer, (void));

A5_KCM_AUDIO_FUNC(ALLEGRO_CHANNEL_CONF, _al_count_to_channel_conf, (int num_channels));
A5_KCM_AUDIO_FUNC(ALLEGRO_AUDIO_DEPTH, _al_word_size_to_depth_conf, (int word_size));

#endif

/* vim: set sts=3 sw=3 et: */
