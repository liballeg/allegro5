/*
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */

#ifndef __al_included_kcm_audio_h
#define __al_included_kcm_audio_h

/* user-included header */
#include "allegro5/allegro5.h"


typedef enum ALLEGRO_AUDIO_ENUM {
   /* Sample depth and type, and signedness. Mixers only use 32-bit signed
      float (-1..+1). The unsigned value is a bit-flag applied to the depth
      value */
   ALLEGRO_AUDIO_8_BIT_INT    = 0x00,
   ALLEGRO_AUDIO_16_BIT_INT   = 0x01,
   ALLEGRO_AUDIO_24_BIT_INT   = 0x02,
   ALLEGRO_AUDIO_32_BIT_FLOAT = 0x03,

   ALLEGRO_AUDIO_UNSIGNED   = 0x08,

   /* For convenience */
   ALLEGRO_AUDIO_8_BIT_UINT   = ALLEGRO_AUDIO_8_BIT_INT|ALLEGRO_AUDIO_UNSIGNED,
   ALLEGRO_AUDIO_16_BIT_UINT  = ALLEGRO_AUDIO_16_BIT_INT|ALLEGRO_AUDIO_UNSIGNED,
   ALLEGRO_AUDIO_24_BIT_UINT  = ALLEGRO_AUDIO_24_BIT_INT|ALLEGRO_AUDIO_UNSIGNED,

   /* Speaker configuration (mono, stereo, 2.1, 3, etc). With regards to
      behavior, most of this code makes no distinction between, say, 4.1 and
      5 speaker setups.. they both have 5 "channels". However, users would
      like the distinction, and later when the higher-level stuff is added,
      the differences will become more important. (v>>4)+(v&0xF) should yield
      the total channel count */
   ALLEGRO_AUDIO_1_CH   = 0x10,
   ALLEGRO_AUDIO_2_CH   = 0x20,
   ALLEGRO_AUDIO_3_CH   = 0x30,
   ALLEGRO_AUDIO_4_CH   = 0x40,
   ALLEGRO_AUDIO_5_1_CH = 0x51,
   ALLEGRO_AUDIO_6_1_CH = 0x61,
   ALLEGRO_AUDIO_7_1_CH = 0x71,
#define ALLEGRO_MAX_CHANNELS 8

   /* Sample looping mode */
   ALLEGRO_AUDIO_PLAY_ONCE = 0x100,
   ALLEGRO_AUDIO_ONE_DIR   = 0x101,
   ALLEGRO_AUDIO_BI_DIR    = 0x102,

   /* Mixer quality */
   ALLEGRO_AUDIO_POINT  = 0x110,
   ALLEGRO_AUDIO_LINEAR = 0x111,

   /* Flags to pass to the various al_*_get_* and al_*_set_* functions. Not
      all types will apply to all functions. */
   ALLEGRO_AUDIO_DEPTH     = 0x200,
   ALLEGRO_AUDIO_CHANNELS  = 0x201,
   ALLEGRO_AUDIO_FREQUENCY = 0x202,

   ALLEGRO_AUDIO_PLAYING  = 0x203,
   ALLEGRO_AUDIO_ATTACHED = 0x204,

   ALLEGRO_AUDIO_LENGTH = 0x205,
   ALLEGRO_AUDIO_BUFFER = 0x206,

   ALLEGRO_AUDIO_LOOPMODE      = 0x207,
   ALLEGRO_AUDIO_SPEED         = 0x208,
   ALLEGRO_AUDIO_POSITION      = 0x209,
   ALLEGRO_AUDIO_ORPHAN_BUFFER = 0x20A,

   ALLEGRO_AUDIO_FRAGMENTS      = 0x20B,
   ALLEGRO_AUDIO_USED_FRAGMENTS = 0x20C,

   ALLEGRO_AUDIO_QUALITY = 0x20D,

   ALLEGRO_AUDIO_SETTINGS = 0x20E,

   ALLEGRO_AUDIO_DEVICE = 0x20F,

   /* length of audio sample*/
   ALLEGRO_AUDIO_TIME   = 0x210, 

   /* Used to define driver-specific option values, not to collide with standard values. */
   ALLEGRO_AUDIO_USER_START = 0x10000,


   /* various driver modes */
   ALLEGRO_AUDIO_DRIVER_AUTODETECT = 0x20000,
   ALLEGRO_AUDIO_DRIVER_OPENAL     = 0x20001,
   ALLEGRO_AUDIO_DRIVER_ALSA       = 0x20002,
   ALLEGRO_AUDIO_DRIVER_DSOUND     = 0x20003

} ALLEGRO_AUDIO_ENUM;

/* struct ALLEGRO_SAMPLE:

An ALLEGRO_SAMPLE object stores the data necessary for playing pre-defined digital audio. It holds information pertaining to data length, the looping mode, loop start/end points, playing position, playing frequency, channel configuration, etc. As such, you cannot have an ALLEGRO_SAMPLE object playing multiple times simultaniously. The object holds a user-specified PCM data buffer, of the format the object is created with. Multiple objects may be created using the same data buffer.

To be played, an ALLEGRO_SAMPLE object must be attached to an ALLEGRO_VOICE object, or to an ALLEGRO_MIXER object which is itself attached to an ALLEGRO_VOICE object (or to another ALLEGRO_MIXER object which is attached to an ALLEGRO_VOICE object, etc).

An ALLEGRO_SAMPLE object uses the following fields:

* ALLEGRO_AUDIO_DEPTH (enum) - Gets the bit depth format the object was created with. This may not be changed after the object is created.

* ALLEGRO_AUDIO_CHANNELS (enum) - Gets the channel configuration the object was created with. This may not be changed after the object is created.

* ALLEGRO_AUDIO_FREQUENCY (long) - Gets the frequency (in hz) the object was created with. This may not be changed after the object is created. To change playback speed, see ALLEGRO_AUDIO_SPEED.

* ALLEGRO_AUDIO_PLAYING (bool) - Gets or sets the object's playing status. By default, it is stopped. Note that simply setting it true does not cause the object to play. It must also be attached to a voice, directly or indirectly (eg. sample->voice, sample->mixer->voice, sample->mixer->...->voice).

* ALLEGRO_AUDIO_ATTACHED (bool) - Gets the object's attachment status (true if it is attached to a something, false if not). Setting this to false detaches the object from whatever it is attached to. You may not directly set this to true.

* ALLEGRO_AUDIO_LENGTH (long) - Gets or sets the length of the object's data buffer, in samples-per-channel. When changing the length, you must make sure the current buffer is large enough. You may not change the length while the object is set to play.

* ALLEGRO_AUDIO_BUFFER (ptr) - Gets or sets the object's data buffer. You may not get or set this if the object is set to play, and you may not get this if the buffer was previously orphaned (see ALLEGRO_AUDIO_ORPHAN_BUFFER).

* ALLEGRO_AUDIO_LOOPMODE (enum) - Gets or sets the object's looping mode. Setting this may fail if the object is attached to a voice and the audio driver does not support the requested looping mode.

* ALLEGRO_AUDIO_SPEED (float) - Gets or sets the object's playing speed. Negative values will cause the object to play backwards. If the value is set too close to 0, this will fail to set.

* ALLEGRO_AUDIO_POSITION (long) - Gets or sets the object's playing position. The value is in samples-per-channel.

* ALLEGRO_AUDIO_ORPHAN_BUFFER (bool) - Setting this flag to true will cause the object's data buffer to be free()'d automatically when either the sample is destroyed, or the buffer is changed (setting an ALLEGRO_AUDIO_BUFFER value). If you set another ALLEGRO_AUDIO_BUFFER value, this will revert back to false. You may not directly set this to false. You must not orphan a buffer that is referenced outside of the object.

*/
typedef struct ALLEGRO_SAMPLE ALLEGRO_SAMPLE;

/* ALLEGRO_STREAM:

An ALLEGRO_STREAM object is used to stream generated audio to the sound device, in real-time. As with ALLEGRO_SAMPLE objects, they store information necessary for playback, so you may not play one multiple times simultaniously. They also need to be attached to an ALLEGRO_VOICE object, or to an ALLEGRO_MIXER object which, eventually, reaches an ALLEGRO_VOICE object.

While playing, you must periodicly supply new buffer data by first checking ALLEGRO_AUDIO_USED_FRAGMENTS, then refilling the buffers via ALLEGRO_AUDIO_BUFFER. If you're late with supplying new data, the object will be silenced until new data is provided.

ALLEGRO_STREAM objects use the following fields:

* ALLEGRO_AUDIO_DEPTH (enum) - Same as ALLEGRO_SAMPLE

* ALLEGRO_AUDIO_CHANNELS (enum) - Same as ALLEGRO_SAMPLE

* ALLEGRO_AUDIO_FREQUENCY (enum) - Same as ALLEGRO_SAMPLE

* ALLEGRO_AUDIO_ATTACHED (bool) - Same as ALLEGRO_SAMPLE

* ALLEGRO_AUDIO_PLAYING (bool) - Same as ALLEGRO_SAMPLE, with the exception that ALLEGRO_STREAM objects are set to play by default.

* ALLEGRO_AUDIO_SPEED (float) - Same as ALLEGRO_SAMPLE, with the added caveat that negative values aren't allowed.

* ALLEGRO_AUDIO_LENGTH (long) - This gets the length, in samples-per-channel, of the individual buffer fragments. You may not set this after the object is created.

* ALLEGRO_AUDIO_BUFFER (ptr) - This gets the next buffer fragment that needs to be filled. After the buffer is filled, this field must be set to the same pointer value to let the object know the new data is ready.

* ALLEGRO_AUDIO_FRAGMENTS (long) - This gets the total number of buffer fragments the object was created with. You may not set this after the object is created.

* ALLEGRO_AUDIO_USED_FRAGMENTS (long) - This gets the number of buffer fragments that are waiting to be refilled. This value is decreased when ALLEGRO_AUDIO_BUFFER is used to retrieve a waiting buffer fragment. You may not set this value.

*/
typedef struct ALLEGRO_STREAM ALLEGRO_STREAM;

/* struct ALLEGRO_MIXER:

*/
typedef struct ALLEGRO_MIXER ALLEGRO_MIXER;

/* struct ALLEGRO_VOICE:

*/
typedef struct ALLEGRO_VOICE ALLEGRO_VOICE;

/* TODO: note how these go against allegro5 conventions. eg: it is al_sample_destroy instead of al_destroy_sample
 * i would change them quickly but wouldn't it be weird if it were al_get_long_sample ??? */


/* Sample functions (more detailed explanations below) */
AL_FUNC(ALLEGRO_SAMPLE*, al_sample_create, (void *buf, unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf));
AL_FUNC(void, al_sample_destroy, (ALLEGRO_SAMPLE *spl));
AL_FUNC(int, al_sample_get_long, (const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, unsigned long *val));
AL_FUNC(int, al_sample_get_float, (const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, float *val));
AL_FUNC(int, al_sample_get_enum, (const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val));
AL_FUNC(int, al_sample_get_bool, (const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, bool *val));
AL_FUNC(int, al_sample_get_ptr, (const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, void **ptr));
AL_FUNC(int, al_sample_set_long, (ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, unsigned long val));
AL_FUNC(int, al_sample_set_float, (ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, float val));
AL_FUNC(int, al_sample_set_enum, (ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val));
AL_FUNC(int, al_sample_set_bool, (ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, bool val));
AL_FUNC(int, al_sample_set_ptr, (ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, void *ptr));
AL_FUNC(void, al_sample_play, (ALLEGRO_SAMPLE *spl));
AL_FUNC(void, al_sample_stop, (ALLEGRO_SAMPLE *spl));

AL_FUNC(bool, al_is_channel_conf, (ALLEGRO_AUDIO_ENUM conf));
AL_FUNC(size_t, al_channel_count, (ALLEGRO_AUDIO_ENUM conf));
AL_FUNC(size_t, al_depth_size, (ALLEGRO_AUDIO_ENUM conf));

/* Stream functions */
AL_FUNC(ALLEGRO_STREAM*, al_stream_create, (size_t buffer_count, unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf));
AL_FUNC(void, al_stream_destroy, (ALLEGRO_STREAM *stream));
AL_FUNC(int, al_stream_get_long, (const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, unsigned long *val));
AL_FUNC(int, al_stream_get_float, (const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, float *val));
AL_FUNC(int, al_stream_get_enum, (const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val));
AL_FUNC(int, al_stream_get_bool, (const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, bool *val));
AL_FUNC(int, al_stream_get_ptr, (const ALLEGRO_STREAM *spl, ALLEGRO_AUDIO_ENUM setting, void **ptr));
AL_FUNC(int, al_stream_set_long, (ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, unsigned long val));
AL_FUNC(int, al_stream_set_float, (ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, float val));
AL_FUNC(int, al_stream_set_enum, (ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val));
AL_FUNC(int, al_stream_set_bool, (ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, bool val));
AL_FUNC(int, al_stream_set_ptr, (ALLEGRO_STREAM *spl, ALLEGRO_AUDIO_ENUM setting, void *ptr));
/* Mixer functions */
AL_FUNC(ALLEGRO_MIXER*, al_mixer_create, (unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf));
AL_FUNC(void, al_mixer_destroy, (ALLEGRO_MIXER *mixer));
AL_FUNC(int, al_mixer_attach_sample, (ALLEGRO_MIXER *mixer, ALLEGRO_SAMPLE *stream));
AL_FUNC(int, al_mixer_attach_stream, (ALLEGRO_MIXER *mixer, ALLEGRO_STREAM *stream));
AL_FUNC(int, al_mixer_attach_mixer, (ALLEGRO_MIXER *mixer, ALLEGRO_MIXER *stream));
AL_FUNC(int, al_mixer_set_postprocess_callback, (ALLEGRO_MIXER *mixer, void (*cb)(void *buf, unsigned long samples, void *data), void *data));
AL_FUNC(int, al_mixer_get_long, (const ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, unsigned long *val));
AL_FUNC(int, al_mixer_get_enum, (const ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val));
AL_FUNC(int, al_mixer_get_bool, (const ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, bool *val));
AL_FUNC(int, al_mixer_set_long, (ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, unsigned long val));
AL_FUNC(int, al_mixer_set_enum, (ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val));
AL_FUNC(int, al_mixer_set_bool, (ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, bool val));
AL_FUNC(void, mixer_read, (void*, void**, unsigned long, ALLEGRO_AUDIO_ENUM, size_t));
/* Voice functions */
AL_FUNC(ALLEGRO_VOICE*, al_voice_create, (unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf));
AL_FUNC(void, al_voice_destroy, (ALLEGRO_VOICE *voice));
AL_FUNC(int, al_voice_attach_sample, (ALLEGRO_VOICE *voice, ALLEGRO_SAMPLE *stream));
AL_FUNC(int, al_voice_attach_stream, (ALLEGRO_VOICE *voice, ALLEGRO_STREAM *stream));
AL_FUNC(int, al_voice_attach_mixer, (ALLEGRO_VOICE *voice, ALLEGRO_MIXER *mixer));
AL_FUNC(void, al_voice_detach, (ALLEGRO_VOICE *voice));
AL_FUNC(int, al_voice_get_long, (const ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, unsigned long *val));
AL_FUNC(int, al_voice_get_enum, (const ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val));
AL_FUNC(int, al_voice_get_bool, (const ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, bool *val));
AL_FUNC(int, al_voice_set_long, (ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, unsigned long val));
AL_FUNC(int, al_voice_set_enum, (ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val));
AL_FUNC(int, al_voice_set_bool, (ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, bool val));
/* Misc. audio functions */
AL_FUNC(int,  al_audio_init, (ALLEGRO_AUDIO_ENUM mode));
AL_FUNC(void, al_audio_deinit, (void));

#endif	/* __al_included_kcm_audio_h */

