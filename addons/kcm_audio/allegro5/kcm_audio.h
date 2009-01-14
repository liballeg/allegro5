/*
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */

#ifndef __al_included_kcm_audio_h
#define __al_included_kcm_audio_h

#ifdef __cplusplus
extern "C" {
#endif

/* Title: Audio types
 */

#include "allegro5/allegro5.h"


#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_KCM_AUDIO_SRC
         #define _A5_KCM_AUDIO_DLL __declspec(dllexport)
      #else
         #define _A5_KCM_AUDIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_KCM_AUDIO_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_KCM_AUDIO_FUNC(type, name, args)      _A5_KCM_AUDIO_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_KCM_AUDIO_FUNC(type, name, args)            extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_KCM_AUDIO_FUNC(type, name, args)      extern _A5_KCM_AUDIO_DLL type name args
#else
   #define A5_KCM_AUDIO_FUNC      AL_FUNC
#endif


/* Internal, used to communicate with acodec. */
#define _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE   1234
/* User event type emitted when a stream fragment is ready to be
 * refilled with more audio data.*/
#define ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT  1235


#ifndef __cplusplus
typedef enum ALLEGRO_AUDIO_DEPTH ALLEGRO_AUDIO_DEPTH;
typedef enum ALLEGRO_CHANNEL_CONF ALLEGRO_CHANNEL_CONF;
typedef enum ALLEGRO_PLAYMODE ALLEGRO_PLAYMODE;
typedef enum ALLEGRO_MIXER_QUALITY ALLEGRO_MIXER_QUALITY;
typedef enum ALLEGRO_AUDIO_PROPERTY ALLEGRO_AUDIO_PROPERTY;
typedef enum ALLEGRO_AUDIO_DRIVER_ENUM ALLEGRO_AUDIO_DRIVER_ENUM;
#endif

/* Enum: ALLEGRO_AUDIO_DEPTH
 */
enum ALLEGRO_AUDIO_DEPTH {
   /* Sample depth and type, and signedness. Mixers only use 32-bit signed
    * float (-1..+1). The unsigned value is a bit-flag applied to the depth
    * value.
    */
   ALLEGRO_AUDIO_DEPTH_INT8      = 0x00,
   ALLEGRO_AUDIO_DEPTH_INT16     = 0x01,
   ALLEGRO_AUDIO_DEPTH_INT24     = 0x02,
   ALLEGRO_AUDIO_DEPTH_FLOAT32   = 0x03,

   ALLEGRO_AUDIO_DEPTH_UNSIGNED  = 0x08,

   /* For convenience */
   ALLEGRO_AUDIO_DEPTH_UINT8  = ALLEGRO_AUDIO_DEPTH_INT8 |
                                 ALLEGRO_AUDIO_DEPTH_UNSIGNED,
   ALLEGRO_AUDIO_DEPTH_UINT16 = ALLEGRO_AUDIO_DEPTH_INT16 |
                                 ALLEGRO_AUDIO_DEPTH_UNSIGNED,
   ALLEGRO_AUDIO_DEPTH_UINT24 = ALLEGRO_AUDIO_DEPTH_INT24 |
                                 ALLEGRO_AUDIO_DEPTH_UNSIGNED
};


/* Enum: ALLEGRO_CHANNEL_CONF
 */
enum ALLEGRO_CHANNEL_CONF {
   /* Speaker configuration (mono, stereo, 2.1, 3, etc). With regards to
    * behavior, most of this code makes no distinction between, say, 4.1 and
    * 5 speaker setups.. they both have 5 "channels". However, users would
    * like the distinction, and later when the higher-level stuff is added,
    * the differences will become more important. (v>>4)+(v&0xF) should yield
    * the total channel count.
    */
   ALLEGRO_CHANNEL_CONF_1   = 0x10,
   ALLEGRO_CHANNEL_CONF_2   = 0x20,
   ALLEGRO_CHANNEL_CONF_3   = 0x30,
   ALLEGRO_CHANNEL_CONF_4   = 0x40,
   ALLEGRO_CHANNEL_CONF_5_1 = 0x51,
   ALLEGRO_CHANNEL_CONF_6_1 = 0x61,
   ALLEGRO_CHANNEL_CONF_7_1 = 0x71
#define ALLEGRO_MAX_CHANNELS 8
};


/* Enum: ALLEGRO_PLAYMODE
 *  Sample and stream looping mode.
 */
enum ALLEGRO_PLAYMODE {
   ALLEGRO_PLAYMODE_ONCE   = 0x100,
   ALLEGRO_PLAYMODE_ONEDIR = 0x101,
   ALLEGRO_PLAYMODE_BIDIR  = 0x102,
   _ALLEGRO_PLAYMODE_STREAM_ONCE   = 0x103,   /* internal */
   _ALLEGRO_PLAYMODE_STREAM_ONEDIR = 0x104    /* internal */
};


/* Enum: ALLEGRO_MIXER_QUALITY
 */
enum ALLEGRO_MIXER_QUALITY {
   ALLEGRO_MIXER_QUALITY_POINT   = 0x110,
   ALLEGRO_MIXER_QUALITY_LINEAR  = 0x111,
};


/* Enum: ALLEGRO_AUDIO_PROPERTY
 *  Flags to pass to the various al_*_get_* and al_*_set_* functions. Not
 *  all types will apply to all functions.
 */
enum ALLEGRO_AUDIO_PROPERTY {
   ALLEGRO_AUDIOPROP_DEPTH          = 0x200,
   ALLEGRO_AUDIOPROP_CHANNELS       = 0x201,
   ALLEGRO_AUDIOPROP_FREQUENCY      = 0x202,

   ALLEGRO_AUDIOPROP_PLAYING        = 0x203,
   ALLEGRO_AUDIOPROP_ATTACHED       = 0x204,

   ALLEGRO_AUDIOPROP_LENGTH         = 0x205,
   ALLEGRO_AUDIOPROP_BUFFER         = 0x206,

   ALLEGRO_AUDIOPROP_LOOPMODE       = 0x207,
   ALLEGRO_AUDIOPROP_SPEED          = 0x208,
   ALLEGRO_AUDIOPROP_POSITION       = 0x209,
   ALLEGRO_AUDIOPROP_GAIN           = 0x20A,

   ALLEGRO_AUDIOPROP_FRAGMENTS      = 0x20B,
   ALLEGRO_AUDIOPROP_USED_FRAGMENTS = 0x20C,

   ALLEGRO_AUDIOPROP_QUALITY        = 0x20D,

   /* Length of audio sample. */
   ALLEGRO_AUDIOPROP_TIME           = 0x210
};


/* Enum: ALLEGRO_AUDIO_DRIVER_ENUM
 */
enum ALLEGRO_AUDIO_DRIVER_ENUM {
   /* Various driver modes. */
   ALLEGRO_AUDIO_DRIVER_AUTODETECT = 0x20000,
   ALLEGRO_AUDIO_DRIVER_OPENAL     = 0x20001,
   ALLEGRO_AUDIO_DRIVER_ALSA       = 0x20002,
   ALLEGRO_AUDIO_DRIVER_DSOUND     = 0x20003,
   ALLEGRO_AUDIO_DRIVER_OSS        = 0x20004
};


/* Type: ALLEGRO_SAMPLE_DATA
 *
 * An ALLEGRO_SAMPLE_DATA object stores the data necessary for playing
 * pre-defined digital audio. It holds information pertaining to data length,
 * frequency, channel configuration, etc.  You can have an ALLEGRO_SAMPLE_DATA
 * objects playing multiple times simultaneously.  The object holds a
 * user-specified PCM data buffer, of the format the object is created with.
 */
typedef struct ALLEGRO_SAMPLE_DATA ALLEGRO_SAMPLE_DATA;


/* Type: ALLEGRO_SAMPLE
 *
 * An ALLEGRO_SAMPLE object represents a playable instance of a predefined
 * sound effect. It holds information pertaining to the looping mode, loop
 * start/end points, playing position, etc.  A ALLEGRO_SAMPLE uses the data
 * from an ALLEGRO_SAMPLE_DATA object.  Multiple ALLEGRO_SAMPLEs may be created
 * from the same ALLEGRO_SAMPLE_DATA. An ALLEGRO_SAMPLE_DATA must not be
 * destroyed while there are ALLEGRO_SAMPLEs which reference it.
 *
 * To be played, an ALLEGRO_SAMPLE object must be attached to an ALLEGRO_VOICE
 * object, or to an ALLEGRO_MIXER object which is itself attached to an
 * ALLEGRO_VOICE object (or to another ALLEGRO_MIXER object which is attached
 * to an ALLEGRO_VOICE object, etc).
 *
 * An ALLEGRO_SAMPLE object uses the following fields:
 * XXX much of this will probably change soon
 *
 * ALLEGRO_AUDIOPROP_DEPTH (enum) -
 *    Gets the bit depth format the object was created with. This may not be
 *    changed after the object is created.
 *
 * ALLEGRO_AUDIOPROP_CHANNELS (enum) -
 *    Gets the channel configuration the object was created with. This may not
 *    be changed after the object is created.
 *
 * ALLEGRO_AUDIOPROP_FREQUENCY (long) -
 *    Gets the frequency (in hz) the object was created with. This may not be
 *    changed after the object is created. To change playback speed, see
 *    ALLEGRO_AUDIOPROP_SPEED.
 *
 * ALLEGRO_AUDIOPROP_PLAYING (bool) -
 *    Gets or sets the object's playing status. By default, it is stopped.
 *    Note that simply setting it true does not cause the object to play. It
 *    must also be attached to a voice, directly or indirectly (eg.
 *    sample->voice, sample->mixer->voice, sample->mixer->...->voice).
 *
 * ALLEGRO_AUDIOPROP_ATTACHED (bool) -
 *    Gets the object's attachment status (true if it is attached to a
 *    something, false if not). Setting this to false detaches the object from
 *    whatever it is attached to. You may not directly set this to true.
 *
 * ALLEGRO_AUDIOPROP_LENGTH (long) -
 *    Gets or sets the length of the object's data buffer, in
 *    samples-per-channel. When changing the length, you must make sure the
 *    current buffer is large enough. You may not change the length while the
 *    object is set to play.
 *
 * ALLEGRO_AUDIOPROP_BUFFER (ptr) -
 *    Gets or sets the object's data buffer. You may not get or set this if the
 *    object is set to play.
 *
 * ALLEGRO_AUDIOPROP_LOOPMODE (enum) -
 *    Gets or sets the object's looping mode. Setting this may fail if the
 *    object is attached to a voice and the audio driver does not support the
 *    requested looping mode.
 *
 * ALLEGRO_AUDIOPROP_SPEED (float) -
 *    Gets or sets the object's playing speed. Negative values will cause the
 *    object to play backwards. If the value is set too close to 0, this will
 *    fail to set.
 *
 * ALLEGRO_AUDIOPROP_POSITION (long) -
 *    Gets or sets the object's playing position. The value is in
 *    samples-per-channel.
 *
 * ALLEGRO_AUDIOPROP_GAIN (float) -
 *    Gets or sets the object's gain. The gain is only applied when mixing the
 *    sample into a parent mixer. Has no effect if the object is attached
 *    directly to a voice.
 */
typedef struct ALLEGRO_SAMPLE ALLEGRO_SAMPLE;


/* Type: ALLEGRO_STREAM
 *
 * An ALLEGRO_STREAM object is used to stream generated audio to the sound
 * device, in real-time. As with ALLEGRO_SAMPLE objects, they store information
 * necessary for playback, so you may not play one multiple times
 * simultaneously. They also need to be attached to an ALLEGRO_VOICE object, or
 * to an ALLEGRO_MIXER object which, eventually, reaches an ALLEGRO_VOICE
 * object.
 *
 * While playing, you must periodically supply new buffer data by first
 * checking ALLEGRO_AUDIOPROP_USED_FRAGMENTS, then refilling the buffers via
 * ALLEGRO_AUDIOPROP_BUFFER. If you're late with supplying new data, the object
 * will be silenced until new data is provided. You must call al_drain_stream()
 * when you're finished supplying the stream.
 *
 * ALLEGRO_STREAM objects use the following fields:
 *
 * ALLEGRO_AUDIOPROP_DEPTH (enum) -
 *    Same as ALLEGRO_SAMPLE
 *
 * ALLEGRO_AUDIOPROP_CHANNELS (enum) -
 *    Same as ALLEGRO_SAMPLE
 *
 * ALLEGRO_AUDIOPROP_FREQUENCY (enum) -
 *    Same as ALLEGRO_SAMPLE
 *
 * ALLEGRO_AUDIOPROP_ATTACHED (bool) -
 *    Same as ALLEGRO_SAMPLE
 *
 * ALLEGRO_AUDIOPROP_PLAYING (bool) -
 *    Same as ALLEGRO_SAMPLE, with the exception that ALLEGRO_STREAM objects
 *    are set to play by default.
 *
 * ALLEGRO_AUDIOPROP_LOOPMODE (enum) -
 *    Same as ALLEGRO_SAMPLE
 *
 * ALLEGRO_AUDIOPROP_SPEED (float) -
 *    Same as ALLEGRO_SAMPLE, with the added caveat that negative values aren't
 *    allowed.
 *
 * ALLEGRO_AUDIOPROP_GAIN (float) -
 *    Same as ALLEGRO_SAMPLE.
 *
 * ALLEGRO_AUDIOPROP_LENGTH (long) -
 *    This gets the length, in samples-per-channel, of the individual buffer
 *    fragments. You may not set this after the object is created.
 *
 * ALLEGRO_AUDIOPROP_BUFFER (ptr) -
 *    This gets the next buffer fragment that needs to be filled. After the
 *    buffer is filled, this field must be set to the same pointer value to let
 *    the object know the new data is ready.
 *
 * ALLEGRO_AUDIOPROP_FRAGMENTS (long) -
 *    This gets the total number of buffer fragments the object was created
 *    with. You may not set this after the object is created.
 *
 * ALLEGRO_AUDIOPROP_USED_FRAGMENTS (long) -
 *    This gets the number of buffer fragments that are waiting to be refilled.
 *    This value is decreased when ALLEGRO_AUDIOPROP_BUFFER is used to retrieve a
 *    waiting buffer fragment. You may not set this value.
 */
typedef struct ALLEGRO_STREAM ALLEGRO_STREAM;


/* Type: ALLEGRO_MIXER
 */
typedef struct ALLEGRO_MIXER ALLEGRO_MIXER;


/* Type: ALLEGRO_VOICE
 */
typedef struct ALLEGRO_VOICE ALLEGRO_VOICE;


/* Sample data functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_SAMPLE_DATA *, al_create_sample_data, (void *buf,
      unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_DEPTH depth,
      ALLEGRO_CHANNEL_CONF chan_conf, bool free_buf));
A5_KCM_AUDIO_FUNC(void, al_destroy_sample_data, (ALLEGRO_SAMPLE_DATA *data));

/* Sample functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_SAMPLE*, al_create_sample, (
      ALLEGRO_SAMPLE_DATA *data));
A5_KCM_AUDIO_FUNC(void, al_destroy_sample, (ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(int, al_get_sample_long, (const ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val));
A5_KCM_AUDIO_FUNC(int, al_get_sample_float, (const ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, float *val));
A5_KCM_AUDIO_FUNC(int, al_get_sample_enum, (const ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, int *val));
A5_KCM_AUDIO_FUNC(int, al_get_sample_bool, (const ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, bool *val));
A5_KCM_AUDIO_FUNC(int, al_get_sample_ptr, (const ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, void **ptr));
A5_KCM_AUDIO_FUNC(int, al_set_sample_long, (ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long val));
A5_KCM_AUDIO_FUNC(int, al_set_sample_float, (ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, float val));
A5_KCM_AUDIO_FUNC(int, al_set_sample_enum, (ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, int val));
A5_KCM_AUDIO_FUNC(int, al_set_sample_bool, (ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, bool val));
A5_KCM_AUDIO_FUNC(int, al_set_sample_ptr, (ALLEGRO_SAMPLE *spl,
      ALLEGRO_AUDIO_PROPERTY setting, void *ptr));
A5_KCM_AUDIO_FUNC(int, al_set_sample_data, (ALLEGRO_SAMPLE *spl,
      ALLEGRO_SAMPLE_DATA *data));
A5_KCM_AUDIO_FUNC(int, al_play_sample, (ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(int, al_stop_sample, (ALLEGRO_SAMPLE *spl));

/* Stream functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_STREAM*, al_create_stream, (size_t buffer_count,
      unsigned long samples, unsigned long freq,
      ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf));
A5_KCM_AUDIO_FUNC(void, al_destroy_stream, (ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(void, al_drain_stream, (ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(int, al_get_stream_long, (const ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val));
A5_KCM_AUDIO_FUNC(int, al_get_stream_float, (const ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, float *val));
A5_KCM_AUDIO_FUNC(int, al_get_stream_enum, (const ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, int *val));
A5_KCM_AUDIO_FUNC(int, al_get_stream_bool, (const ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, bool *val));
A5_KCM_AUDIO_FUNC(int, al_get_stream_ptr, (const ALLEGRO_STREAM *spl,
      ALLEGRO_AUDIO_PROPERTY setting, void **ptr));
A5_KCM_AUDIO_FUNC(int, al_set_stream_long, (ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long val));
A5_KCM_AUDIO_FUNC(int, al_set_stream_float, (ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, float val));
A5_KCM_AUDIO_FUNC(int, al_set_stream_enum, (ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, int val));
A5_KCM_AUDIO_FUNC(int, al_set_stream_bool, (ALLEGRO_STREAM *stream,
      ALLEGRO_AUDIO_PROPERTY setting, bool val));
A5_KCM_AUDIO_FUNC(int, al_set_stream_ptr, (ALLEGRO_STREAM *spl,
      ALLEGRO_AUDIO_PROPERTY setting, void *ptr));
A5_KCM_AUDIO_FUNC(bool, al_rewind_stream, (ALLEGRO_STREAM *stream));

/* Mixer functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_MIXER*, al_create_mixer, (unsigned long freq,
      ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf));
A5_KCM_AUDIO_FUNC(void, al_destroy_mixer, (ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(int, al_attach_sample_to_mixer, (ALLEGRO_MIXER *mixer,
      ALLEGRO_SAMPLE *stream));
A5_KCM_AUDIO_FUNC(int, al_attach_stream_to_mixer, (ALLEGRO_MIXER *mixer,
      ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(int, al_attach_mixer_to_mixer, (ALLEGRO_MIXER *mixer,
      ALLEGRO_MIXER *stream));
A5_KCM_AUDIO_FUNC(int, al_mixer_set_postprocess_callback, (
      ALLEGRO_MIXER *mixer,
      void (*cb)(void *buf, unsigned long samples, void *data),
      void *data));
A5_KCM_AUDIO_FUNC(int, al_get_mixer_long, (const ALLEGRO_MIXER *mixer,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val));
A5_KCM_AUDIO_FUNC(int, al_get_mixer_enum, (const ALLEGRO_MIXER *mixer,
      ALLEGRO_AUDIO_PROPERTY setting, int *val));
A5_KCM_AUDIO_FUNC(int, al_get_mixer_bool, (const ALLEGRO_MIXER *mixer,
      ALLEGRO_AUDIO_PROPERTY setting, bool *val));
A5_KCM_AUDIO_FUNC(int, al_set_mixer_long, (ALLEGRO_MIXER *mixer,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long val));
A5_KCM_AUDIO_FUNC(int, al_set_mixer_enum, (ALLEGRO_MIXER *mixer,
      ALLEGRO_AUDIO_PROPERTY setting, int val));
A5_KCM_AUDIO_FUNC(int, al_set_mixer_bool, (ALLEGRO_MIXER *mixer,
      ALLEGRO_AUDIO_PROPERTY setting, bool val));

/* Voice functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_VOICE*, al_create_voice, (unsigned long freq,
      ALLEGRO_AUDIO_DEPTH depth,
      ALLEGRO_CHANNEL_CONF chan_conf));
A5_KCM_AUDIO_FUNC(void, al_destroy_voice, (ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(int, al_attach_sample_to_voice, (ALLEGRO_VOICE *voice,
      ALLEGRO_SAMPLE *stream));
A5_KCM_AUDIO_FUNC(int, al_attach_stream_to_voice, (ALLEGRO_VOICE *voice,
      ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(int, al_attach_mixer_to_voice, (ALLEGRO_VOICE *voice,
      ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(void, al_detach_voice, (ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(int, al_get_voice_long, (const ALLEGRO_VOICE *voice,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val));
A5_KCM_AUDIO_FUNC(int, al_get_voice_enum, (const ALLEGRO_VOICE *voice,
      ALLEGRO_AUDIO_PROPERTY setting, int *val));
A5_KCM_AUDIO_FUNC(int, al_get_voice_bool, (const ALLEGRO_VOICE *voice,
      ALLEGRO_AUDIO_PROPERTY setting, bool *val));
A5_KCM_AUDIO_FUNC(int, al_set_voice_long, (ALLEGRO_VOICE *voice,
      ALLEGRO_AUDIO_PROPERTY setting, unsigned long val));
A5_KCM_AUDIO_FUNC(int, al_set_voice_enum, (ALLEGRO_VOICE *voice,
      ALLEGRO_AUDIO_PROPERTY setting, int val));
A5_KCM_AUDIO_FUNC(int, al_set_voice_bool, (ALLEGRO_VOICE *voice,
      ALLEGRO_AUDIO_PROPERTY setting, bool val));

/* Misc. audio functions */
A5_KCM_AUDIO_FUNC(int,  al_install_audio, (ALLEGRO_AUDIO_DRIVER_ENUM mode));
A5_KCM_AUDIO_FUNC(void, al_uninstall_audio, (void));

A5_KCM_AUDIO_FUNC(bool, al_is_channel_conf, (ALLEGRO_CHANNEL_CONF conf));
A5_KCM_AUDIO_FUNC(size_t, al_get_channel_count, (ALLEGRO_CHANNEL_CONF conf));
A5_KCM_AUDIO_FUNC(size_t, al_get_depth_size, (ALLEGRO_AUDIO_DEPTH conf));

/* Simple audio layer */
A5_KCM_AUDIO_FUNC(bool, al_setup_simple_audio, (int reserve_samples));
A5_KCM_AUDIO_FUNC(void, al_shutdown_simple_audio, (void));
A5_KCM_AUDIO_FUNC(ALLEGRO_VOICE *, al_get_simple_audio_voice, (void));
A5_KCM_AUDIO_FUNC(ALLEGRO_MIXER *, al_get_simple_audio_mixer, (void));
A5_KCM_AUDIO_FUNC(bool, al_play_sample_data, (ALLEGRO_SAMPLE_DATA *data));
A5_KCM_AUDIO_FUNC(void, al_stop_all_simple_samples, (void));

#ifdef __cplusplus
} /* End extern "C" */
#endif


#endif  /* __al_included_kcm_audio_h */


/* vim: set sts=3 sw=3 et: */
