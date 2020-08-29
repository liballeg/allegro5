/* internal-only header
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */
#ifndef AINTERN_AUDIO_H
#define AINTERN_AUDIO_H

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_list.h"
#include "allegro5/internal/aintern_vector.h"
#include "../allegro_audio.h"

struct ALLEGRO_AUDIO_RECORDER {
  ALLEGRO_EVENT_SOURCE source;
  
  ALLEGRO_THREAD           *thread;
  ALLEGRO_MUTEX            *mutex;
  ALLEGRO_COND             *cond;
                           /* recording is done in its own thread as
                              implemented by the driver */
  
  ALLEGRO_AUDIO_DEPTH      depth;
  ALLEGRO_CHANNEL_CONF     chan_conf;
  unsigned int             frequency;

  void                     **fragments;
                           /* the buffers to record into */

  unsigned int             fragment_count;
                           /* the number of fragments */

  unsigned int             samples;
                           /* the number of samples returned at every FRAGMENT event */
                           
  size_t                   fragment_size;
                           /* size in bytes of each fragument */

  unsigned int             sample_size;
                           /* the size in bytes of each sample */
  
  volatile bool            is_recording; 
                           /* true if the driver should actively be updating
                              the buffer */
                              
  void                     *extra;
                           /* custom data for the driver to use as needed */
};

typedef enum ALLEGRO_AUDIO_DRIVER_ENUM
{
   /* Various driver modes. */
   ALLEGRO_AUDIO_DRIVER_AUTODETECT = 0x20000,
   ALLEGRO_AUDIO_DRIVER_OPENAL     = 0x20001,
   ALLEGRO_AUDIO_DRIVER_ALSA       = 0x20002,
   ALLEGRO_AUDIO_DRIVER_DSOUND     = 0x20003,
   ALLEGRO_AUDIO_DRIVER_OSS        = 0x20004,
   ALLEGRO_AUDIO_DRIVER_AQUEUE     = 0x20005,
   ALLEGRO_AUDIO_DRIVER_PULSEAUDIO = 0x20006,
   ALLEGRO_AUDIO_DRIVER_OPENSL     = 0x20007,
   ALLEGRO_AUDIO_DRIVER_SDL        = 0x20008
} ALLEGRO_AUDIO_DRIVER_ENUM;

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

   unsigned int   (*get_voice_position)(const ALLEGRO_VOICE*);
   int            (*set_voice_position)(ALLEGRO_VOICE*, unsigned int);
   
   
   int            (*allocate_recorder)(struct ALLEGRO_AUDIO_RECORDER *);
   void           (*deallocate_recorder)(struct ALLEGRO_AUDIO_RECORDER *);

   _AL_LIST*      (*get_devices)(void);
};

extern ALLEGRO_AUDIO_DRIVER *_al_kcm_driver;

const void *_al_voice_update(ALLEGRO_VOICE *voice, ALLEGRO_MUTEX *mutex,
   unsigned int *samples);
bool _al_kcm_set_voice_playing(ALLEGRO_VOICE *voice, ALLEGRO_MUTEX *mutex,
   bool val);

/* A voice structure that you'd attach a mixer or sample to. Ideally there
 * would be one ALLEGRO_VOICE per system/hardware voice.
 */
struct ALLEGRO_VOICE {
   ALLEGRO_AUDIO_DEPTH  depth;
   ALLEGRO_CHANNEL_CONF chan_conf;

   unsigned int         frequency;

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
   ALLEGRO_COND         *cond;

   _AL_LIST_ITEM        *dtor_item;

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
   unsigned int         frequency;
   int                  len;
   any_buffer_t         buffer;
   bool                 free_buf;
                        /* Whether `buffer' needs to be freed when the sample
                         * is destroyed, or when `buffer' changes.
                         */
   _AL_LIST_ITEM        *dtor_item;
};

/* Read some samples into a mixer buffer.
 *
 * source:
 *    The object to read samples from.  This may be one of several types.
 *
 * *vbuf: (in-out parameter)
 *    Pointer to pointer to destination buffer.
 *    (should confirm what it means to change the pointer on return)
 *
 * *samples: (in-out parameter)
 *    On input indicates the maximum number of samples that can fit into *vbuf.
 *    On output indicates the actual number of samples that were read.
 *
 * buffer_depth:
 *    The audio depth of the destination buffer.
 *
 * dest_maxc:
 *    The number of channels in the destination.
 */
typedef void (*stream_reader_t)(void *source, void **vbuf,
   unsigned int *samples, ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc);

typedef struct {
   union {
      ALLEGRO_MIXER     *mixer;
      ALLEGRO_VOICE     *voice;
      void              *ptr;
   } u;
   bool                 is_voice;
} sample_parent_t;

/* The sample struct also serves the base of ALLEGRO_AUDIO_STREAM, ALLEGRO_MIXER. */
struct ALLEGRO_SAMPLE_INSTANCE {
   /* ALLEGRO_SAMPLE_INSTANCE does not generate any events yet but ALLEGRO_AUDIO_STREAM
    * does, which can inherit only ALLEGRO_SAMPLE_INSTANCE. */
   ALLEGRO_EVENT_SOURCE es;

   ALLEGRO_SAMPLE       spl_data;

   volatile bool        is_playing;
                        /* Is this sample is playing? */

   ALLEGRO_PLAYMODE     loop;
   float                speed;
   float                gain;
   float                pan;

   /* When resampling an audio stream there will be fractional sample
    * positions due to the difference in frequencies.
    */
   int                  pos;
   int                  pos_bresenham_error;

   int                  loop_start;
   int                  loop_end;

   int                  step;
   int                  step_denom;
                        /* The numerator and denominator of the step are 
                         * stored separately. The actual step is obtained by 
                         * dividing step by step_denom */

   float                *matrix;
                        /* Used to convert from this format to the attached
                         * mixers, if any.  Otherwise is NULL.
                         * The gain is premultiplied in.
                         */

   bool                 is_mixer;
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
   _AL_LIST_ITEM        *dtor_item;
};

void _al_kcm_destroy_sample(ALLEGRO_SAMPLE_INSTANCE *sample, bool unregister);
void _al_kcm_stream_set_mutex(ALLEGRO_SAMPLE_INSTANCE *stream, ALLEGRO_MUTEX *mutex);
void _al_kcm_detach_from_parent(ALLEGRO_SAMPLE_INSTANCE *spl);


typedef size_t (*stream_callback_t)(ALLEGRO_AUDIO_STREAM *, void *, size_t);
typedef void (*unload_feeder_t)(ALLEGRO_AUDIO_STREAM *);
typedef bool (*rewind_feeder_t)(ALLEGRO_AUDIO_STREAM *);
typedef bool (*seek_feeder_t)(ALLEGRO_AUDIO_STREAM *, double);
typedef double (*get_feeder_position_t)(ALLEGRO_AUDIO_STREAM *);
typedef double (*get_feeder_length_t)(ALLEGRO_AUDIO_STREAM *);
typedef bool (*set_feeder_loop_t)(ALLEGRO_AUDIO_STREAM *, double, double);

struct ALLEGRO_AUDIO_STREAM {
   ALLEGRO_SAMPLE_INSTANCE spl;
                        /* ALLEGRO_AUDIO_STREAM is derived from
                         * ALLEGRO_SAMPLE_INSTANCE.
                         */

   unsigned int         buf_count;
                        /* The stream buffer is divided into a number of
                         * fragments; this is the number of fragments.
                         */

   void                 *main_buffer;
                        /* Pointer to a single buffer big enough to hold all
                         * the fragments. Each fragment has additional samples
                         * at the start for linear/cubic interpolation.
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

   uint64_t              consumed_fragments;
                         /* Number of complete fragment buffers consumed since
                          * the stream was started.
                          */

   ALLEGRO_THREAD        *feed_thread;
   ALLEGRO_MUTEX         *feed_thread_started_mutex;
   ALLEGRO_COND          *feed_thread_started_cond;
   bool                  feed_thread_started;
   volatile bool         quit_feed_thread;
   unload_feeder_t       unload_feeder;
   rewind_feeder_t       rewind_feeder;
   seek_feeder_t         seek_feeder;
   get_feeder_position_t get_feeder_position;
   get_feeder_length_t   get_feeder_length;
   set_feeder_loop_t     set_feeder_loop;
   stream_callback_t     feeder;
                         /* If ALLEGRO_AUDIO_STREAM has been created by
                          * al_load_audio_stream(), the stream will be fed
                          * by a thread using the 'feeder' callback. Such
                          * streams don't need to be fed by the user.
                          */

   _AL_LIST_ITEM        *dtor_item;

   void                  *extra;
                         /* Extra data for use by the flac/vorbis addons. */
};

bool _al_kcm_refill_stream(ALLEGRO_AUDIO_STREAM *stream);


typedef void (*postprocess_callback_t)(void *buf, unsigned int samples,
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
   _AL_LIST_ITEM           *dtor_item;
};

extern void _al_kcm_mixer_rejig_sample_matrix(ALLEGRO_MIXER *mixer,
   ALLEGRO_SAMPLE_INSTANCE *spl);
extern void _al_kcm_mixer_read(void *source, void **buf, unsigned int *samples,
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc);


typedef enum {
   ALLEGRO_NO_ERROR       = 0,
   ALLEGRO_INVALID_PARAM  = 1,
   ALLEGRO_INVALID_OBJECT = 2,
   ALLEGRO_GENERIC_ERROR  = 255
} AL_ERROR_ENUM;

extern void _al_set_error(int error, char* string);

struct ALLEGRO_AUDIO_DEVICE {
   char* name;
   void* identifier;
};

/* Supposedly internal */
ALLEGRO_KCM_AUDIO_FUNC(void*, _al_kcm_feed_stream, (ALLEGRO_THREAD *self, void *vstream));

/* Helper to emit an event that the stream has got a buffer ready to be refilled. */
void _al_kcm_emit_stream_events(ALLEGRO_AUDIO_STREAM *stream);

void _al_kcm_init_destructors(void);
void _al_kcm_shutdown_destructors(void);
_AL_LIST_ITEM *_al_kcm_register_destructor(char const *name, void *object,
   void (*func)(void*));
void _al_kcm_unregister_destructor(_AL_LIST_ITEM *dtor_item);
void _al_kcm_foreach_destructor(
      void (*callback)(void *object, void (*func)(void *), void *udata),
      void *userdata);

ALLEGRO_KCM_AUDIO_FUNC(void, _al_kcm_shutdown_default_mixer, (void));

ALLEGRO_KCM_AUDIO_FUNC(ALLEGRO_CHANNEL_CONF, _al_count_to_channel_conf, (int num_channels));
ALLEGRO_KCM_AUDIO_FUNC(ALLEGRO_AUDIO_DEPTH, _al_word_size_to_depth_conf, (int word_size));

ALLEGRO_KCM_AUDIO_FUNC(void, _al_emit_audio_event, (int event_type));

#endif

/* vim: set sts=3 sw=3 et: */
