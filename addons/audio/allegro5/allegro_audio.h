/*
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */

#ifndef __al_included_allegro_audio_h
#define __al_included_allegro_audio_h

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
/* Must be in 512 <= n < 1024 */
#define _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE   (512)

/* User event type emitted when a stream fragment is ready to be
 * refilled with more audio data.
 * Must be in 512 <= n < 1024
 */
#define ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT  (513)


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
enum ALLEGRO_AUDIO_DEPTH
{
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
enum ALLEGRO_CHANNEL_CONF
{
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
 */
enum ALLEGRO_PLAYMODE
{
   ALLEGRO_PLAYMODE_ONCE   = 0x100,
   ALLEGRO_PLAYMODE_LOOP   = 0x101,
   ALLEGRO_PLAYMODE_BIDIR  = 0x102,
   _ALLEGRO_PLAYMODE_STREAM_ONCE   = 0x103,   /* internal */
   _ALLEGRO_PLAYMODE_STREAM_ONEDIR = 0x104    /* internal */
};


/* Enum: ALLEGRO_MIXER_QUALITY
 */
enum ALLEGRO_MIXER_QUALITY
{
   ALLEGRO_MIXER_QUALITY_POINT   = 0x110,
   ALLEGRO_MIXER_QUALITY_LINEAR  = 0x111,
};


/* Enum: ALLEGRO_AUDIO_DRIVER_ENUM
 */
enum ALLEGRO_AUDIO_DRIVER_ENUM
{
   /* Various driver modes. */
   ALLEGRO_AUDIO_DRIVER_AUTODETECT = 0x20000,
   ALLEGRO_AUDIO_DRIVER_OPENAL     = 0x20001,
   ALLEGRO_AUDIO_DRIVER_ALSA       = 0x20002,
   ALLEGRO_AUDIO_DRIVER_DSOUND     = 0x20003,
   ALLEGRO_AUDIO_DRIVER_OSS        = 0x20004,
   ALLEGRO_AUDIO_DRIVER_AQUEUE     = 0x20005
};


/* Enum: ALLEGRO_AUDIO_PAN_NONE
 */
#define ALLEGRO_AUDIO_PAN_NONE      (-1000.0f)


/* Type: ALLEGRO_SAMPLE
 */
typedef struct ALLEGRO_SAMPLE ALLEGRO_SAMPLE;


/* Type: ALLEGRO_SAMPLE_ID
 */
typedef struct ALLEGRO_SAMPLE_ID ALLEGRO_SAMPLE_ID;

struct ALLEGRO_SAMPLE_ID {
   int _index;
   int _id;
};


/* Type: ALLEGRO_SAMPLE_INSTANCE
 */
typedef struct ALLEGRO_SAMPLE_INSTANCE ALLEGRO_SAMPLE_INSTANCE;


/* Type: ALLEGRO_STREAM
 */
typedef struct ALLEGRO_STREAM ALLEGRO_STREAM;


/* Type: ALLEGRO_MIXER
 */
typedef struct ALLEGRO_MIXER ALLEGRO_MIXER;


/* Type: ALLEGRO_VOICE
 */
typedef struct ALLEGRO_VOICE ALLEGRO_VOICE;


/* Sample functions */

A5_KCM_AUDIO_FUNC(ALLEGRO_SAMPLE *, al_create_sample, (void *buf,
      unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_DEPTH depth,
      ALLEGRO_CHANNEL_CONF chan_conf, bool free_buf));
A5_KCM_AUDIO_FUNC(void, al_destroy_sample, (ALLEGRO_SAMPLE *spl));


/* Sample instance functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_SAMPLE_INSTANCE*, al_create_sample_instance, (
      ALLEGRO_SAMPLE *data));
A5_KCM_AUDIO_FUNC(void, al_destroy_sample_instance, (
      ALLEGRO_SAMPLE_INSTANCE *spl));

A5_KCM_AUDIO_FUNC(unsigned int, al_get_sample_frequency, (const ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(unsigned long, al_get_sample_length, (const ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(ALLEGRO_AUDIO_DEPTH, al_get_sample_depth, (const ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(ALLEGRO_CHANNEL_CONF, al_get_sample_channels, (const ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(void *, al_get_sample_data, (const ALLEGRO_SAMPLE *spl));

A5_KCM_AUDIO_FUNC(unsigned int, al_get_sample_instance_frequency, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(unsigned long, al_get_sample_instance_length, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(unsigned long, al_get_sample_instance_position, (const ALLEGRO_SAMPLE_INSTANCE *spl));

A5_KCM_AUDIO_FUNC(float, al_get_sample_instance_speed, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(float, al_get_sample_instance_gain, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(float, al_get_sample_instance_pan, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(float, al_get_sample_instance_time, (const ALLEGRO_SAMPLE_INSTANCE *spl));

A5_KCM_AUDIO_FUNC(ALLEGRO_AUDIO_DEPTH, al_get_sample_instance_depth, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(ALLEGRO_CHANNEL_CONF, al_get_sample_instance_channels, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(ALLEGRO_PLAYMODE, al_get_sample_instance_playmode, (const ALLEGRO_SAMPLE_INSTANCE *spl));

A5_KCM_AUDIO_FUNC(bool, al_get_sample_instance_playing, (const ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(bool, al_get_sample_instance_attached, (const ALLEGRO_SAMPLE_INSTANCE *spl));

A5_KCM_AUDIO_FUNC(bool, al_set_sample_instance_position, (ALLEGRO_SAMPLE_INSTANCE *spl, unsigned long val));
A5_KCM_AUDIO_FUNC(bool, al_set_sample_instance_length, (ALLEGRO_SAMPLE_INSTANCE *spl, unsigned long val));

A5_KCM_AUDIO_FUNC(bool, al_set_sample_instance_speed, (ALLEGRO_SAMPLE_INSTANCE *spl, float val));
A5_KCM_AUDIO_FUNC(bool, al_set_sample_instance_gain, (ALLEGRO_SAMPLE_INSTANCE *spl, float val));
A5_KCM_AUDIO_FUNC(bool, al_set_sample_instance_pan, (ALLEGRO_SAMPLE_INSTANCE *spl, float val));

A5_KCM_AUDIO_FUNC(bool, al_set_sample_instance_playmode, (ALLEGRO_SAMPLE_INSTANCE *spl, ALLEGRO_PLAYMODE val));

A5_KCM_AUDIO_FUNC(bool, al_set_sample_instance_playing, (ALLEGRO_SAMPLE_INSTANCE *spl, bool val));
A5_KCM_AUDIO_FUNC(bool, al_detach_sample_instance, (ALLEGRO_SAMPLE_INSTANCE *spl));

A5_KCM_AUDIO_FUNC(bool, al_set_sample, (ALLEGRO_SAMPLE_INSTANCE *spl, ALLEGRO_SAMPLE *data));
A5_KCM_AUDIO_FUNC(ALLEGRO_SAMPLE *, al_get_sample, (ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(bool, al_play_sample_instance, (ALLEGRO_SAMPLE_INSTANCE *spl));
A5_KCM_AUDIO_FUNC(bool, al_stop_sample_instance, (ALLEGRO_SAMPLE_INSTANCE *spl));


/* Stream functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_STREAM*, al_create_stream, (size_t buffer_count,
      unsigned long samples, unsigned long freq,
      ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf));
A5_KCM_AUDIO_FUNC(void, al_destroy_stream, (ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(void, al_drain_stream, (ALLEGRO_STREAM *stream));

A5_KCM_AUDIO_FUNC(unsigned int, al_get_stream_frequency, (const ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(unsigned long, al_get_stream_length, (const ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(unsigned int, al_get_stream_fragments, (const ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(unsigned int, al_get_available_stream_fragments, (const ALLEGRO_STREAM *stream));

A5_KCM_AUDIO_FUNC(float, al_get_stream_speed, (const ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(float, al_get_stream_gain, (const ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(float, al_get_stream_pan, (const ALLEGRO_STREAM *stream));

A5_KCM_AUDIO_FUNC(ALLEGRO_CHANNEL_CONF, al_get_stream_channels, (const ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(ALLEGRO_AUDIO_DEPTH, al_get_stream_depth, (const ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(ALLEGRO_PLAYMODE, al_get_stream_playmode, (const ALLEGRO_STREAM *stream));

A5_KCM_AUDIO_FUNC(bool, al_get_stream_playing, (const ALLEGRO_STREAM *spl));
A5_KCM_AUDIO_FUNC(bool, al_get_stream_attached, (const ALLEGRO_STREAM *spl));

A5_KCM_AUDIO_FUNC(bool, al_get_stream_fragment, (const ALLEGRO_STREAM *stream, void **val));

A5_KCM_AUDIO_FUNC(bool, al_set_stream_speed, (ALLEGRO_STREAM *stream, float val));
A5_KCM_AUDIO_FUNC(bool, al_set_stream_gain, (ALLEGRO_STREAM *stream, float val));
A5_KCM_AUDIO_FUNC(bool, al_set_stream_pan, (ALLEGRO_STREAM *stream, float val));

A5_KCM_AUDIO_FUNC(bool, al_set_stream_playmode, (ALLEGRO_STREAM *stream, ALLEGRO_PLAYMODE val));

A5_KCM_AUDIO_FUNC(bool, al_set_stream_playing, (ALLEGRO_STREAM *stream, bool val));
A5_KCM_AUDIO_FUNC(bool, al_detach_stream, (ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(bool, al_set_stream_fragment, (ALLEGRO_STREAM *stream, void *val));

A5_KCM_AUDIO_FUNC(bool, al_rewind_stream, (ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(bool, al_seek_stream_secs, (ALLEGRO_STREAM *stream, double time));
A5_KCM_AUDIO_FUNC(double, al_get_stream_position_secs, (ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(double, al_get_stream_length_secs, (ALLEGRO_STREAM *stream));
A5_KCM_AUDIO_FUNC(bool, al_set_stream_loop_secs, (ALLEGRO_STREAM *stream, double start, double end));

A5_KCM_AUDIO_FUNC(ALLEGRO_EVENT_SOURCE *, al_get_stream_event_source, (ALLEGRO_STREAM *stream));

/* Mixer functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_MIXER*, al_create_mixer, (unsigned long freq,
      ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf));
A5_KCM_AUDIO_FUNC(void, al_destroy_mixer, (ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_attach_sample_to_mixer, (
   ALLEGRO_SAMPLE_INSTANCE *stream, ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_attach_stream_to_mixer, (ALLEGRO_STREAM *stream,
   ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_attach_mixer_to_mixer, (ALLEGRO_MIXER *stream,
   ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_set_mixer_postprocess_callback, (
      ALLEGRO_MIXER *mixer,
      void (*cb)(void *buf, unsigned long samples, void *data),
      void *data));

A5_KCM_AUDIO_FUNC(unsigned int, al_get_mixer_frequency, (const ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(ALLEGRO_CHANNEL_CONF, al_get_mixer_channels, (const ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(ALLEGRO_AUDIO_DEPTH, al_get_mixer_depth, (const ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(ALLEGRO_MIXER_QUALITY, al_get_mixer_quality, (const ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_get_mixer_playing, (const ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_get_mixer_attached, (const ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_set_mixer_frequency, (ALLEGRO_MIXER *mixer, unsigned long val));
A5_KCM_AUDIO_FUNC(bool, al_set_mixer_quality, (ALLEGRO_MIXER *mixer, ALLEGRO_MIXER_QUALITY val));
A5_KCM_AUDIO_FUNC(bool, al_set_mixer_playing, (ALLEGRO_MIXER *mixer, bool val));
A5_KCM_AUDIO_FUNC(bool, al_detach_mixer, (ALLEGRO_MIXER *mixer));

/* Voice functions */
A5_KCM_AUDIO_FUNC(ALLEGRO_VOICE*, al_create_voice, (unsigned long freq,
      ALLEGRO_AUDIO_DEPTH depth,
      ALLEGRO_CHANNEL_CONF chan_conf));
A5_KCM_AUDIO_FUNC(void, al_destroy_voice, (ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(bool, al_attach_sample_to_voice, (
   ALLEGRO_SAMPLE_INSTANCE *stream, ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(bool, al_attach_stream_to_voice, (
   ALLEGRO_STREAM *stream, ALLEGRO_VOICE *voice ));
A5_KCM_AUDIO_FUNC(bool, al_attach_mixer_to_voice, (ALLEGRO_MIXER *mixer,
   ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(void, al_detach_voice, (ALLEGRO_VOICE *voice));

A5_KCM_AUDIO_FUNC(unsigned int, al_get_voice_frequency, (const ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(unsigned long, al_get_voice_position, (const ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(ALLEGRO_CHANNEL_CONF, al_get_voice_channels, (const ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(ALLEGRO_AUDIO_DEPTH, al_get_voice_depth, (const ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(bool, al_get_voice_playing, (const ALLEGRO_VOICE *voice));
A5_KCM_AUDIO_FUNC(bool, al_set_voice_position, (ALLEGRO_VOICE *voice, unsigned long val));
A5_KCM_AUDIO_FUNC(bool, al_set_voice_playing, (ALLEGRO_VOICE *voice, bool val));

/* Misc. audio functions */
A5_KCM_AUDIO_FUNC(bool, al_install_audio, (ALLEGRO_AUDIO_DRIVER_ENUM mode));
A5_KCM_AUDIO_FUNC(void, al_uninstall_audio, (void));
A5_KCM_AUDIO_FUNC(uint32_t, al_get_allegro_audio_version, (void));

A5_KCM_AUDIO_FUNC(bool, al_is_channel_conf, (ALLEGRO_CHANNEL_CONF conf));
A5_KCM_AUDIO_FUNC(size_t, al_get_channel_count, (ALLEGRO_CHANNEL_CONF conf));
A5_KCM_AUDIO_FUNC(size_t, al_get_depth_size, (ALLEGRO_AUDIO_DEPTH conf));

/* Simple audio layer */
A5_KCM_AUDIO_FUNC(bool, al_reserve_samples, (int reserve_samples));
A5_KCM_AUDIO_FUNC(ALLEGRO_MIXER *, al_get_default_mixer, (void));
A5_KCM_AUDIO_FUNC(bool, al_set_default_mixer, (ALLEGRO_MIXER *mixer));
A5_KCM_AUDIO_FUNC(bool, al_restore_default_mixer, (void));
A5_KCM_AUDIO_FUNC(bool, al_play_sample, (ALLEGRO_SAMPLE *data,
      float gain, float pan, float speed, int loop, ALLEGRO_SAMPLE_ID *ret_id));
A5_KCM_AUDIO_FUNC(void, al_stop_sample, (ALLEGRO_SAMPLE_ID *spl_id));
A5_KCM_AUDIO_FUNC(void, al_stop_samples, (void));

/* File type handlers */
A5_KCM_AUDIO_FUNC(bool, al_register_sample_loader, (const char *ext,
	ALLEGRO_SAMPLE *(*loader)(const char *filename)));
A5_KCM_AUDIO_FUNC(bool, al_register_sample_saver, (const char *ext,
	bool (*saver)(const char *filename, ALLEGRO_SAMPLE *spl)));
A5_KCM_AUDIO_FUNC(bool, al_register_stream_loader, (const char *ext,
	ALLEGRO_STREAM *(*stream_loader)(const char *filename,
	    size_t buffer_count, unsigned int samples)));

A5_KCM_AUDIO_FUNC(ALLEGRO_SAMPLE *, al_load_sample, (const char *filename));
A5_KCM_AUDIO_FUNC(bool, al_save_sample, (const char *filename,
	ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(ALLEGRO_STREAM *, al_stream_from_file, (const char *filename,
	size_t buffer_count, unsigned int samples));

/* WAV handlers */
A5_KCM_AUDIO_FUNC(ALLEGRO_SAMPLE *, al_load_sample_wav, (
      const char *filename));
A5_KCM_AUDIO_FUNC(bool, al_save_sample_wav, (const char *filename,
      ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(bool, al_save_sample_wav_pf, (ALLEGRO_FILE *pf,
      ALLEGRO_SAMPLE *spl));
A5_KCM_AUDIO_FUNC(ALLEGRO_STREAM *, al_load_stream_wav, (const char *filename,
      size_t buffer_count, unsigned int samples));


#ifdef __cplusplus
} /* End extern "C" */
#endif


#endif  /* __al_included_allegro_audio_h */


/* vim: set sts=3 sw=3 et: */
