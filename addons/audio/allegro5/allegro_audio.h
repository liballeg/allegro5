/*
 * Updated for 4.9 api inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */

#ifndef __al_included_allegro5_allegro_audio_h
#define __al_included_allegro5_allegro_audio_h

#ifdef __cplusplus
extern "C" {
#endif

/* Title: Audio types
 */

#include "allegro5/allegro.h"


#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
   #ifndef A5O_STATICLINK
      #ifdef A5O_KCM_AUDIO_SRC
         #define _A5O_KCM_AUDIO_DLL __declspec(dllexport)
      #else
         #define _A5O_KCM_AUDIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_KCM_AUDIO_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_KCM_AUDIO_FUNC(type, name, args)      _A5O_KCM_AUDIO_DLL type __cdecl name args
#elif defined A5O_MINGW32
   #define A5O_KCM_AUDIO_FUNC(type, name, args)            extern type name args
#elif defined A5O_BCC32
   #define A5O_KCM_AUDIO_FUNC(type, name, args)      extern _A5O_KCM_AUDIO_DLL type name args
#else
   #define A5O_KCM_AUDIO_FUNC      AL_FUNC
#endif

/* Enum: A5O_AUDIO_EVENT_TYPE
 */
enum A5O_AUDIO_EVENT_TYPE
{
   /* Internal, used to communicate with acodec. */
   /* Must be in 512 <= n < 1024 */
   _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE    = 512,
   A5O_EVENT_AUDIO_STREAM_FRAGMENT   = 513,
   A5O_EVENT_AUDIO_STREAM_FINISHED   = 514,
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_KCM_AUDIO_SRC)
   A5O_EVENT_AUDIO_RECORDER_FRAGMENT = 515,
#endif
};

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_KCM_AUDIO_SRC)
/* Type: A5O_AUDIO_RECORDER_EVENT
 */
typedef struct A5O_AUDIO_RECORDER_EVENT A5O_AUDIO_RECORDER_EVENT;
struct A5O_AUDIO_RECORDER_EVENT
{
   _AL_EVENT_HEADER(struct A5O_AUDIO_RECORDER)
   struct A5O_USER_EVENT_DESCRIPTOR *__internal__descr;
   void *buffer;
   unsigned int samples;
};
#endif


/* Enum: A5O_AUDIO_DEPTH
 */
enum A5O_AUDIO_DEPTH
{
   /* Sample depth and type, and signedness. Mixers only use 32-bit signed
    * float (-1..+1). The unsigned value is a bit-flag applied to the depth
    * value.
    */
   A5O_AUDIO_DEPTH_INT8      = 0x00,
   A5O_AUDIO_DEPTH_INT16     = 0x01,
   A5O_AUDIO_DEPTH_INT24     = 0x02,
   A5O_AUDIO_DEPTH_FLOAT32   = 0x03,

   A5O_AUDIO_DEPTH_UNSIGNED  = 0x08,

   /* For convenience */
   A5O_AUDIO_DEPTH_UINT8  = A5O_AUDIO_DEPTH_INT8 |
                                 A5O_AUDIO_DEPTH_UNSIGNED,
   A5O_AUDIO_DEPTH_UINT16 = A5O_AUDIO_DEPTH_INT16 |
                                 A5O_AUDIO_DEPTH_UNSIGNED,
   A5O_AUDIO_DEPTH_UINT24 = A5O_AUDIO_DEPTH_INT24 |
                                 A5O_AUDIO_DEPTH_UNSIGNED
};


/* Enum: A5O_CHANNEL_CONF
 */
enum A5O_CHANNEL_CONF
{
   /* Speaker configuration (mono, stereo, 2.1, 3, etc). With regards to
    * behavior, most of this code makes no distinction between, say, 4.1 and
    * 5 speaker setups.. they both have 5 "channels". However, users would
    * like the distinction, and later when the higher-level stuff is added,
    * the differences will become more important. (v>>4)+(v&0xF) should yield
    * the total channel count.
    */
   A5O_CHANNEL_CONF_1   = 0x10,
   A5O_CHANNEL_CONF_2   = 0x20,
   A5O_CHANNEL_CONF_3   = 0x30,
   A5O_CHANNEL_CONF_4   = 0x40,
   A5O_CHANNEL_CONF_5_1 = 0x51,
   A5O_CHANNEL_CONF_6_1 = 0x61,
   A5O_CHANNEL_CONF_7_1 = 0x71
#define A5O_MAX_CHANNELS 8
};


/* Enum: A5O_PLAYMODE
 */
enum A5O_PLAYMODE
{
   A5O_PLAYMODE_ONCE   = 0x100,
   A5O_PLAYMODE_LOOP   = 0x101,
   A5O_PLAYMODE_BIDIR  = 0x102,
   _A5O_PLAYMODE_STREAM_ONCE   = 0x103,   /* internal */
   _A5O_PLAYMODE_STREAM_ONEDIR = 0x104,   /* internal */
   A5O_PLAYMODE_LOOP_ONCE = 0x105,
   _A5O_PLAYMODE_STREAM_LOOP_ONCE = 0x106,   /* internal */
};


/* Enum: A5O_MIXER_QUALITY
 */
enum A5O_MIXER_QUALITY
{
   A5O_MIXER_QUALITY_POINT   = 0x110,
   A5O_MIXER_QUALITY_LINEAR  = 0x111,
   A5O_MIXER_QUALITY_CUBIC   = 0x112
};


/* Enum: A5O_AUDIO_PAN_NONE
 */
#define A5O_AUDIO_PAN_NONE      (-1000.0f)

/* Type: A5O_SAMPLE
 */
typedef struct A5O_SAMPLE A5O_SAMPLE;


/* Type: A5O_SAMPLE_ID
 */
typedef struct A5O_SAMPLE_ID A5O_SAMPLE_ID;

struct A5O_SAMPLE_ID {
   int _index;
   int _id;
};


/* Type: A5O_SAMPLE_INSTANCE
 */
typedef struct A5O_SAMPLE_INSTANCE A5O_SAMPLE_INSTANCE;


/* Type: A5O_AUDIO_STREAM
 */
typedef struct A5O_AUDIO_STREAM A5O_AUDIO_STREAM;


/* Type: A5O_MIXER
 */
typedef struct A5O_MIXER A5O_MIXER;


/* Type: A5O_VOICE
 */
typedef struct A5O_VOICE A5O_VOICE;

/* Type: A5O_AUDIO_DEVICE
 */
typedef struct A5O_AUDIO_DEVICE A5O_AUDIO_DEVICE;


#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_KCM_AUDIO_SRC)
/* Type: A5O_AUDIO_RECORDER
 */
typedef struct A5O_AUDIO_RECORDER A5O_AUDIO_RECORDER;
#endif


#ifndef __cplusplus
typedef enum A5O_AUDIO_DEPTH A5O_AUDIO_DEPTH;
typedef enum A5O_CHANNEL_CONF A5O_CHANNEL_CONF;
typedef enum A5O_PLAYMODE A5O_PLAYMODE;
typedef enum A5O_MIXER_QUALITY A5O_MIXER_QUALITY;
#endif


/* Sample functions */

A5O_KCM_AUDIO_FUNC(A5O_SAMPLE *, al_create_sample, (void *buf,
      unsigned int samples, unsigned int freq, A5O_AUDIO_DEPTH depth,
      A5O_CHANNEL_CONF chan_conf, bool free_buf));
A5O_KCM_AUDIO_FUNC(void, al_destroy_sample, (A5O_SAMPLE *spl));


/* Sample instance functions */
A5O_KCM_AUDIO_FUNC(A5O_SAMPLE_INSTANCE*, al_create_sample_instance, (
      A5O_SAMPLE *data));
A5O_KCM_AUDIO_FUNC(void, al_destroy_sample_instance, (
      A5O_SAMPLE_INSTANCE *spl));

A5O_KCM_AUDIO_FUNC(unsigned int, al_get_sample_frequency, (const A5O_SAMPLE *spl));
A5O_KCM_AUDIO_FUNC(unsigned int, al_get_sample_length, (const A5O_SAMPLE *spl));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_DEPTH, al_get_sample_depth, (const A5O_SAMPLE *spl));
A5O_KCM_AUDIO_FUNC(A5O_CHANNEL_CONF, al_get_sample_channels, (const A5O_SAMPLE *spl));
A5O_KCM_AUDIO_FUNC(void *, al_get_sample_data, (const A5O_SAMPLE *spl));

A5O_KCM_AUDIO_FUNC(unsigned int, al_get_sample_instance_frequency, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(unsigned int, al_get_sample_instance_length, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(unsigned int, al_get_sample_instance_position, (const A5O_SAMPLE_INSTANCE *spl));

A5O_KCM_AUDIO_FUNC(float, al_get_sample_instance_speed, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(float, al_get_sample_instance_gain, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(float, al_get_sample_instance_pan, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(float, al_get_sample_instance_time, (const A5O_SAMPLE_INSTANCE *spl));

A5O_KCM_AUDIO_FUNC(A5O_AUDIO_DEPTH, al_get_sample_instance_depth, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(A5O_CHANNEL_CONF, al_get_sample_instance_channels, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(A5O_PLAYMODE, al_get_sample_instance_playmode, (const A5O_SAMPLE_INSTANCE *spl));

A5O_KCM_AUDIO_FUNC(bool, al_get_sample_instance_playing, (const A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(bool, al_get_sample_instance_attached, (const A5O_SAMPLE_INSTANCE *spl));

A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_position, (A5O_SAMPLE_INSTANCE *spl, unsigned int val));
A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_length, (A5O_SAMPLE_INSTANCE *spl, unsigned int val));

A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_speed, (A5O_SAMPLE_INSTANCE *spl, float val));
A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_gain, (A5O_SAMPLE_INSTANCE *spl, float val));
A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_pan, (A5O_SAMPLE_INSTANCE *spl, float val));

A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_playmode, (A5O_SAMPLE_INSTANCE *spl, A5O_PLAYMODE val));

A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_playing, (A5O_SAMPLE_INSTANCE *spl, bool val));
A5O_KCM_AUDIO_FUNC(bool, al_detach_sample_instance, (A5O_SAMPLE_INSTANCE *spl));

A5O_KCM_AUDIO_FUNC(bool, al_set_sample, (A5O_SAMPLE_INSTANCE *spl, A5O_SAMPLE *data));
A5O_KCM_AUDIO_FUNC(A5O_SAMPLE *, al_get_sample, (A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(bool, al_play_sample_instance, (A5O_SAMPLE_INSTANCE *spl));
A5O_KCM_AUDIO_FUNC(bool, al_stop_sample_instance, (A5O_SAMPLE_INSTANCE *spl));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_KCM_AUDIO_SRC)
A5O_KCM_AUDIO_FUNC(bool, al_set_sample_instance_channel_matrix, (A5O_SAMPLE_INSTANCE *spl, const float *matrix));
#endif


/* Stream functions */
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_STREAM*, al_create_audio_stream, (size_t buffer_count,
      unsigned int samples, unsigned int freq,
      A5O_AUDIO_DEPTH depth, A5O_CHANNEL_CONF chan_conf));
A5O_KCM_AUDIO_FUNC(void, al_destroy_audio_stream, (A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(void, al_drain_audio_stream, (A5O_AUDIO_STREAM *stream));

A5O_KCM_AUDIO_FUNC(unsigned int, al_get_audio_stream_frequency, (const A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(unsigned int, al_get_audio_stream_length, (const A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(unsigned int, al_get_audio_stream_fragments, (const A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(unsigned int, al_get_available_audio_stream_fragments, (const A5O_AUDIO_STREAM *stream));

A5O_KCM_AUDIO_FUNC(float, al_get_audio_stream_speed, (const A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(float, al_get_audio_stream_gain, (const A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(float, al_get_audio_stream_pan, (const A5O_AUDIO_STREAM *stream));

A5O_KCM_AUDIO_FUNC(A5O_CHANNEL_CONF, al_get_audio_stream_channels, (const A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_DEPTH, al_get_audio_stream_depth, (const A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(A5O_PLAYMODE, al_get_audio_stream_playmode, (const A5O_AUDIO_STREAM *stream));

A5O_KCM_AUDIO_FUNC(bool, al_get_audio_stream_playing, (const A5O_AUDIO_STREAM *spl));
A5O_KCM_AUDIO_FUNC(bool, al_get_audio_stream_attached, (const A5O_AUDIO_STREAM *spl));
A5O_KCM_AUDIO_FUNC(uint64_t, al_get_audio_stream_played_samples, (const A5O_AUDIO_STREAM *stream));

A5O_KCM_AUDIO_FUNC(void *, al_get_audio_stream_fragment, (const A5O_AUDIO_STREAM *stream));

A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_speed, (A5O_AUDIO_STREAM *stream, float val));
A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_gain, (A5O_AUDIO_STREAM *stream, float val));
A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_pan, (A5O_AUDIO_STREAM *stream, float val));

A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_playmode, (A5O_AUDIO_STREAM *stream, A5O_PLAYMODE val));

A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_playing, (A5O_AUDIO_STREAM *stream, bool val));
A5O_KCM_AUDIO_FUNC(bool, al_detach_audio_stream, (A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_fragment, (A5O_AUDIO_STREAM *stream, void *val));

A5O_KCM_AUDIO_FUNC(bool, al_rewind_audio_stream, (A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(bool, al_seek_audio_stream_secs, (A5O_AUDIO_STREAM *stream, double time));
A5O_KCM_AUDIO_FUNC(double, al_get_audio_stream_position_secs, (A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(double, al_get_audio_stream_length_secs, (A5O_AUDIO_STREAM *stream));
A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_loop_secs, (A5O_AUDIO_STREAM *stream, double start, double end));

A5O_KCM_AUDIO_FUNC(A5O_EVENT_SOURCE *, al_get_audio_stream_event_source, (A5O_AUDIO_STREAM *stream));

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_KCM_AUDIO_SRC)
A5O_KCM_AUDIO_FUNC(bool, al_set_audio_stream_channel_matrix, (A5O_AUDIO_STREAM *stream, const float *matrix));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_STREAM *, al_play_audio_stream, (const char *filename));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_STREAM *, al_play_audio_stream_f, (A5O_FILE *fp, const char *ident));
#endif

/* Mixer functions */
A5O_KCM_AUDIO_FUNC(A5O_MIXER*, al_create_mixer, (unsigned int freq,
      A5O_AUDIO_DEPTH depth, A5O_CHANNEL_CONF chan_conf));
A5O_KCM_AUDIO_FUNC(void, al_destroy_mixer, (A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_attach_sample_instance_to_mixer, (
   A5O_SAMPLE_INSTANCE *stream, A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_attach_audio_stream_to_mixer, (A5O_AUDIO_STREAM *stream,
   A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_attach_mixer_to_mixer, (A5O_MIXER *stream,
   A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_set_mixer_postprocess_callback, (
      A5O_MIXER *mixer,
      void (*cb)(void *buf, unsigned int samples, void *data),
      void *data));

A5O_KCM_AUDIO_FUNC(unsigned int, al_get_mixer_frequency, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(A5O_CHANNEL_CONF, al_get_mixer_channels, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_DEPTH, al_get_mixer_depth, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(A5O_MIXER_QUALITY, al_get_mixer_quality, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(float, al_get_mixer_gain, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_get_mixer_playing, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_get_mixer_attached, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_mixer_has_attachments, (const A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_set_mixer_frequency, (A5O_MIXER *mixer, unsigned int val));
A5O_KCM_AUDIO_FUNC(bool, al_set_mixer_quality, (A5O_MIXER *mixer, A5O_MIXER_QUALITY val));
A5O_KCM_AUDIO_FUNC(bool, al_set_mixer_gain, (A5O_MIXER *mixer, float gain));
A5O_KCM_AUDIO_FUNC(bool, al_set_mixer_playing, (A5O_MIXER *mixer, bool val));
A5O_KCM_AUDIO_FUNC(bool, al_detach_mixer, (A5O_MIXER *mixer));

/* Voice functions */
A5O_KCM_AUDIO_FUNC(A5O_VOICE*, al_create_voice, (unsigned int freq,
      A5O_AUDIO_DEPTH depth,
      A5O_CHANNEL_CONF chan_conf));
A5O_KCM_AUDIO_FUNC(void, al_destroy_voice, (A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(bool, al_attach_sample_instance_to_voice, (
   A5O_SAMPLE_INSTANCE *stream, A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(bool, al_attach_audio_stream_to_voice, (
   A5O_AUDIO_STREAM *stream, A5O_VOICE *voice ));
A5O_KCM_AUDIO_FUNC(bool, al_attach_mixer_to_voice, (A5O_MIXER *mixer,
   A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(void, al_detach_voice, (A5O_VOICE *voice));

A5O_KCM_AUDIO_FUNC(unsigned int, al_get_voice_frequency, (const A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(unsigned int, al_get_voice_position, (const A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(A5O_CHANNEL_CONF, al_get_voice_channels, (const A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_DEPTH, al_get_voice_depth, (const A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(bool, al_get_voice_playing, (const A5O_VOICE *voice));
A5O_KCM_AUDIO_FUNC(bool, al_voice_has_attachments, (const A5O_VOICE* voice));
A5O_KCM_AUDIO_FUNC(bool, al_set_voice_position, (A5O_VOICE *voice, unsigned int val));
A5O_KCM_AUDIO_FUNC(bool, al_set_voice_playing, (A5O_VOICE *voice, bool val));

/* Misc. audio functions */
A5O_KCM_AUDIO_FUNC(bool, al_install_audio, (void));
A5O_KCM_AUDIO_FUNC(void, al_uninstall_audio, (void));
A5O_KCM_AUDIO_FUNC(bool, al_is_audio_installed, (void));
A5O_KCM_AUDIO_FUNC(uint32_t, al_get_allegro_audio_version, (void));

A5O_KCM_AUDIO_FUNC(size_t, al_get_channel_count, (A5O_CHANNEL_CONF conf));
A5O_KCM_AUDIO_FUNC(size_t, al_get_audio_depth_size, (A5O_AUDIO_DEPTH conf));

A5O_KCM_AUDIO_FUNC(void, al_fill_silence, (void *buf, unsigned int samples,
      A5O_AUDIO_DEPTH depth, A5O_CHANNEL_CONF chan_conf));

A5O_KCM_AUDIO_FUNC(int, al_get_num_audio_output_devices, (void));
A5O_KCM_AUDIO_FUNC(const A5O_AUDIO_DEVICE *, al_get_audio_output_device, (int index));
A5O_KCM_AUDIO_FUNC(const char *, al_get_audio_device_name, (const A5O_AUDIO_DEVICE * device));

/* Simple audio layer */
A5O_KCM_AUDIO_FUNC(bool, al_reserve_samples, (int reserve_samples));
A5O_KCM_AUDIO_FUNC(A5O_MIXER *, al_get_default_mixer, (void));
A5O_KCM_AUDIO_FUNC(bool, al_set_default_mixer, (A5O_MIXER *mixer));
A5O_KCM_AUDIO_FUNC(bool, al_restore_default_mixer, (void));
A5O_KCM_AUDIO_FUNC(bool, al_play_sample, (A5O_SAMPLE *data,
      float gain, float pan, float speed, A5O_PLAYMODE loop, A5O_SAMPLE_ID *ret_id));
A5O_KCM_AUDIO_FUNC(void, al_stop_sample, (A5O_SAMPLE_ID *spl_id));
A5O_KCM_AUDIO_FUNC(void, al_stop_samples, (void));
A5O_KCM_AUDIO_FUNC(A5O_VOICE *, al_get_default_voice, (void));
A5O_KCM_AUDIO_FUNC(void, al_set_default_voice, (A5O_VOICE *voice));

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_KCM_AUDIO_SRC)
A5O_KCM_AUDIO_FUNC(A5O_SAMPLE_INSTANCE*, al_lock_sample_id, (A5O_SAMPLE_ID *spl_id));
A5O_KCM_AUDIO_FUNC(void, al_unlock_sample_id, (A5O_SAMPLE_ID *spl_id));
#endif

/* File type handlers */
A5O_KCM_AUDIO_FUNC(bool, al_register_sample_loader, (const char *ext,
	A5O_SAMPLE *(*loader)(const char *filename)));
A5O_KCM_AUDIO_FUNC(bool, al_register_sample_saver, (const char *ext,
	bool (*saver)(const char *filename, A5O_SAMPLE *spl)));
A5O_KCM_AUDIO_FUNC(bool, al_register_audio_stream_loader, (const char *ext,
	A5O_AUDIO_STREAM *(*stream_loader)(const char *filename,
	    size_t buffer_count, unsigned int samples)));
       
A5O_KCM_AUDIO_FUNC(bool, al_register_sample_loader_f, (const char *ext,
	A5O_SAMPLE *(*loader)(A5O_FILE *fp)));
A5O_KCM_AUDIO_FUNC(bool, al_register_sample_saver_f, (const char *ext,
	bool (*saver)(A5O_FILE *fp, A5O_SAMPLE *spl)));
A5O_KCM_AUDIO_FUNC(bool, al_register_audio_stream_loader_f, (const char *ext,
	A5O_AUDIO_STREAM *(*stream_loader)(A5O_FILE *fp,
	    size_t buffer_count, unsigned int samples)));

A5O_KCM_AUDIO_FUNC(bool, al_register_sample_identifier, (const char *ext,
	bool (*identifier)(A5O_FILE *fp)));

A5O_KCM_AUDIO_FUNC(A5O_SAMPLE *, al_load_sample, (const char *filename));
A5O_KCM_AUDIO_FUNC(bool, al_save_sample, (const char *filename,
	A5O_SAMPLE *spl));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_STREAM *, al_load_audio_stream, (const char *filename,
	size_t buffer_count, unsigned int samples));
   
A5O_KCM_AUDIO_FUNC(A5O_SAMPLE *, al_load_sample_f, (A5O_FILE* fp, const char *ident));
A5O_KCM_AUDIO_FUNC(bool, al_save_sample_f, (A5O_FILE* fp, const char *ident,
	A5O_SAMPLE *spl));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_STREAM *, al_load_audio_stream_f, (A5O_FILE* fp, const char *ident,
	size_t buffer_count, unsigned int samples));

A5O_KCM_AUDIO_FUNC(char const *, al_identify_sample_f, (A5O_FILE *fp));
A5O_KCM_AUDIO_FUNC(char const *, al_identify_sample, (char const *filename));

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_KCM_AUDIO_SRC)

/* Recording functions */
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_RECORDER *, al_create_audio_recorder, (size_t fragment_count,
   unsigned int samples, unsigned int freq, A5O_AUDIO_DEPTH depth, A5O_CHANNEL_CONF chan_conf));
A5O_KCM_AUDIO_FUNC(bool, al_start_audio_recorder, (A5O_AUDIO_RECORDER *r));
A5O_KCM_AUDIO_FUNC(void, al_stop_audio_recorder, (A5O_AUDIO_RECORDER *r));
A5O_KCM_AUDIO_FUNC(bool, al_is_audio_recorder_recording, (A5O_AUDIO_RECORDER *r));
A5O_KCM_AUDIO_FUNC(A5O_EVENT_SOURCE *, al_get_audio_recorder_event_source,
   (A5O_AUDIO_RECORDER *r));
A5O_KCM_AUDIO_FUNC(A5O_AUDIO_RECORDER_EVENT *, al_get_audio_recorder_event, (A5O_EVENT *event));
A5O_KCM_AUDIO_FUNC(void, al_destroy_audio_recorder, (A5O_AUDIO_RECORDER *r));

#endif
   
#ifdef __cplusplus
} /* End extern "C" */
#endif

#endif


/* vim: set sts=3 sw=3 et: */
