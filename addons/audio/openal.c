/*
 * updated for 4.9 inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */

#include <stdio.h>
#include <string.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_audio.h"

#ifndef ALLEGRO_FORMAT_QUAD8_LOKI
#define ALLEGRO_FORMAT_QUAD8_LOKI 0x10004
#endif
#ifndef ALLEGRO_FORMAT_QUAD16_LOKI
#define ALLEGRO_FORMAT_QUAD16_LOKI 0x10005
#endif

static ALCdevice *audio_dev;
static ALCcontext *audio_context;

static int preferred_channels;
static unsigned long preferred_freq;
static char preferred_devices[1024];

static size_t preferred_frag_size = 1024;
static ALuint preferred_buf_count = 4;

static char dev_spec[1024];

/* The open method starts up the driver and should lock the device, using the
   previously set paramters, or defaults. It shouldn't need to start sending
   audio data to the device yet, however. */
static int _openal_open()
{
   const char *str;

   fprintf(stderr, "Starting OpenAL...\n");

//FIXME: on my system str == NULL
   str = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
   if(str && alcGetError(NULL) == ALC_NO_ERROR)
   {
      fprintf(stderr, "\nDetected audio devices:\n");
      while(*str)
      {
         fprintf(stderr, "  %s\n", str);
         str += strlen(str)+1;
      }
   }
   else
      fprintf(stderr, "Could not get a list of audio devices\n");

   if(preferred_channels || preferred_freq || preferred_devices[0])
   {
      int i;
      i = snprintf(dev_spec, sizeof(dev_spec), "'(");
      if(preferred_channels)
         i += snprintf(dev_spec+i, sizeof(dev_spec)-i, "(speaker-num %i) ", preferred_channels);
      if(preferred_freq)
         i += snprintf(dev_spec+i, sizeof(dev_spec)-i, "(sampling-rate %lu) ", preferred_freq);
      if(preferred_devices[0])
         i += snprintf(dev_spec+i, sizeof(dev_spec)-i, "(devices '(%s)) ", preferred_devices);
      if(i > 2) --i;
      snprintf(dev_spec+i, sizeof(dev_spec)-i, ")");

      audio_dev = alcOpenDevice(dev_spec);
   }
   else
      audio_dev = alcOpenDevice(NULL);

   if(!audio_dev)
   {
      fprintf(stderr, "Could not open audio device\n");
      return 1;
   }

   audio_context = alcCreateContext(audio_dev, NULL);
   alcMakeContextCurrent(audio_context);
   alDistanceModel(AL_NONE);

   fprintf(stderr, "\n");
   fprintf(stderr, "      Vendor: %s\n", alGetString(AL_VENDOR));
   fprintf(stderr, "     Version: %s\n", alGetString(AL_VERSION));
   fprintf(stderr, "    Renderer: %s\n", alGetString(AL_RENDERER));
   fprintf(stderr, "  Extensions: %s\n", alGetString(AL_EXTENSIONS));

   fprintf(stderr, "\n...done\n\n");

   return 0;
}

/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void _openal_close()
{
   if(audio_dev)
   {
      alcMakeContextCurrent(NULL);
      if(audio_context)
         alcDestroyContext(audio_context);
      audio_context = NULL;

      alcCloseDevice(audio_dev);
      audio_dev = NULL;
   }
}

/* Custom struct to hold voice information OpenAL needs */
typedef struct ALLEGRO_AL_DATA {
   ALuint *buffers;

   size_t num_buffers;
   ALuint buffer_size;

   ALuint source;
   ALuint format;

   _AL_THREAD thread;
   volatile bool stop_voice;
} ALLEGRO_AL_DATA;

/* Custom routine which runs in another thread to periodically check if OpenAL
   wants more data for a stream */
static void _openal_update(_AL_THREAD* self, void* arg)
{
   ALLEGRO_VOICE* voice = (ALLEGRO_VOICE*) arg;
   ALLEGRO_AL_DATA *ex_data = (ALLEGRO_AL_DATA*)voice->extra;
   unsigned long i, samples_per_update;
   const void *data;
   void *silence;

   silence = calloc(1, ex_data->buffer_size);
   if(ex_data->format == AL_FORMAT_QUAD8_LOKI ||
      ex_data->format == AL_FORMAT_STEREO8 ||
      ex_data->format == AL_FORMAT_MONO8)
      memset(silence, 0x80, ex_data->buffer_size);

   for(i = 0;i < ex_data->num_buffers;++i)
      alBufferData(ex_data->buffers[i], ex_data->format, silence,
                   ex_data->buffer_size, voice->frequency);

   alSourceQueueBuffers(ex_data->source, ex_data->num_buffers,
                     ex_data->buffers);
   alSourcePlay(ex_data->source);


   samples_per_update = ex_data->buffer_size /
                        ((ex_data->format==AL_FORMAT_QUAD16_LOKI) ? 8 :
                         (ex_data->format==AL_FORMAT_QUAD8_LOKI ||
                          ex_data->format==AL_FORMAT_STEREO16) ? 4 :
                          ((ex_data->format==AL_FORMAT_STEREO8 ||
                            ex_data->format==AL_FORMAT_MONO16) ? 2 : 1));

   data = silence;

   while(!ex_data->stop_voice)
   {
      ALint status = 0;

      alGetSourcei(ex_data->source, AL_BUFFERS_PROCESSED, &status);
      if(status <= 0)
      {
         //what is this for ?
         al_rest(0.001);
         continue;
      }

      while(--status >= 0)
      {
         ALuint buffer;

         data = _al_voice_update(voice, samples_per_update);
         if(data == NULL)
            data = silence;

         alSourceUnqueueBuffers(ex_data->source, 1, &buffer);
         alBufferData(buffer, ex_data->format, data, ex_data->buffer_size,
                      voice->frequency);
         alSourceQueueBuffers(ex_data->source, 1, &buffer);
      }
      alGetSourcei(ex_data->source, AL_SOURCE_STATE, &status);
      if(status == AL_STOPPED)
         alSourcePlay(ex_data->source);
   }

   alSourceStop(ex_data->source);
}


/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached sample's looping mode should be honored, and loading
   must fail if it cannot be. */
static int _openal_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   if(voice->stream->loop != ALLEGRO_AUDIO_PLAY_ONCE &&
      voice->stream->loop != ALLEGRO_AUDIO_ONE_DIR)
      return -1;

   ex_data->buffer_size = voice->buffer_size;
   if(!ex_data->buffer_size)
      return -1;
   ex_data->num_buffers = 1;

   alGenSources(1, &ex_data->source);
   if(alGetError() != AL_NO_ERROR)
      return -1;

   ex_data->buffers = malloc(sizeof(ALuint) * ex_data->num_buffers);
   if(!ex_data->buffers)
   {
      alDeleteSources(1, &ex_data->source);
      return -1;
   }

   alGenBuffers(ex_data->num_buffers, ex_data->buffers);
   if(alGetError() != AL_NO_ERROR)
   {
      alDeleteSources(1, &ex_data->source);
      free(ex_data->buffers);
      ex_data->buffers = NULL;
      return -1;
   }

   alBufferData(ex_data->buffers[0], ex_data->format,
                data, ex_data->buffer_size, voice->frequency);
   alSourcei(ex_data->source, AL_BUFFER, ex_data->buffers[0]);

      alSource3f(ex_data->source, AL_POSITION,  0.0, 0.0, 0.0);
      alSource3f(ex_data->source, AL_VELOCITY,  0.0, 0.0, 0.0);
      alSource3f(ex_data->source, AL_DIRECTION, 0.0, 0.0, 0.0);
      alSourcef(ex_data->source, AL_ROLLOFF_FACTOR, 0.0);
      alSourcei(ex_data->source, AL_SOURCE_RELATIVE, AL_TRUE);
   alSourcei(ex_data->source, AL_LOOPING, (voice->stream->loop !=
                                           ALLEGRO_AUDIO_PLAY_ONCE));
   alSourcef(ex_data->source, AL_GAIN, 1.0f);
   if(alGetError() != AL_NO_ERROR)
   {
      alDeleteSources(1, &ex_data->source);
      alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
      free(ex_data->buffers);
      ex_data->buffers = NULL;
      return -1;
   }

   return 0;
}

/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void _openal_unload_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   alDeleteSources(1, &ex_data->source);
   alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
   free(ex_data->buffers);
   ex_data->buffers = NULL;
   alGetError();
}


/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int _openal_start_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   if(!voice->streaming)
   {
      alSourcePlay(ex_data->source);
      if(alGetError() != AL_NO_ERROR)
         return -1;
      return 0;
   }

   if(ex_data->stop_voice != 0)
   {
      ex_data->buffer_size = voice->buffer_size;
      if(!ex_data->buffer_size)
         ex_data->buffer_size = preferred_frag_size *
                          ((ex_data->format==AL_FORMAT_QUAD16_LOKI) ? 8 :
                           (ex_data->format==AL_FORMAT_QUAD8_LOKI ||
                            ex_data->format==AL_FORMAT_STEREO16) ? 4 :
                            ((ex_data->format==AL_FORMAT_STEREO8 ||
                              ex_data->format==AL_FORMAT_MONO16) ? 2 : 1));
      ex_data->num_buffers = voice->num_buffers;
      if(!ex_data->num_buffers)
         ex_data->num_buffers = preferred_buf_count;

      alGenSources(1, &ex_data->source);
      if(alGetError() != AL_NO_ERROR)
         return -1;

      ex_data->buffers = malloc(sizeof(ALuint) * ex_data->num_buffers);
      if(!ex_data->buffers)
      {
         alDeleteSources(1, &ex_data->source);
         return -1;
      }

      alGenBuffers(ex_data->num_buffers, ex_data->buffers);
      if(alGetError() != AL_NO_ERROR)
      {
         alDeleteSources(1, &ex_data->source);
         free(ex_data->buffers);
         ex_data->buffers = NULL;
         return -1;
      }

      alSource3f(ex_data->source, AL_POSITION,  0.0, 0.0, 0.0);
      alSource3f(ex_data->source, AL_VELOCITY,  0.0, 0.0, 0.0);
      alSource3f(ex_data->source, AL_DIRECTION, 0.0, 0.0, 0.0);
      alSourcef(ex_data->source, AL_ROLLOFF_FACTOR, 0.0);
      alSourcei(ex_data->source, AL_SOURCE_RELATIVE, AL_TRUE);
      alSourcef(ex_data->source, AL_GAIN, 1.0f);
      if(alGetError() != AL_NO_ERROR)
      {
         alDeleteSources(1, &ex_data->source);
         alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
         free(ex_data->buffers);
         ex_data->buffers = NULL;
         return -1;
      }

      ex_data->stop_voice = 0;
      _al_thread_create(&ex_data->thread, _openal_update, (void*) voice);

   }
      return 0;
}

/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static void _openal_stop_voice(ALLEGRO_VOICE* voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   if(!ex_data->buffers)
      return;

   if(!voice->streaming)
   {
      alSourceStop(ex_data->source);
      alGetError();
      return;
   }

   if(ex_data->stop_voice == 0)
   {
      ex_data->stop_voice = 1;
      _al_thread_join(&ex_data->thread);
   }

   alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
   free(ex_data->buffers);
   ex_data->buffers = NULL;
   alDeleteSources(1, &ex_data->source);
   alGetError();
}

/* The voice_is_playing method should only be called on non-streaming sources,
   and should return true if the voice is playing */
static bool _openal_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;
   ALint status;

   if(!ex_data)
      return false;

   alGetSourcei(ex_data->source, AL_SOURCE_STATE, &status);
   return (status == AL_PLAYING);
}

/* The allocate_voice method should grab a voice from the system, and allocate
   any data common to streaming and non-streaming sources. */
static int _openal_allocate_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data;

   /* OpenAL needs signed 16-bit data or unsigned 8-bit data */
   if(voice->depth == ALLEGRO_AUDIO_16_BIT_UINT)
   {
      if((voice->settings & ALLEGRO_AUDIO_REQUIRE))
         return -1;
      voice->depth &= ~ALLEGRO_AUDIO_UNSIGNED;
   }
   else if(voice->depth == ALLEGRO_AUDIO_8_BIT_INT)
   {
      if((voice->settings&ALLEGRO_AUDIO_REQUIRE))
         return -1;
      voice->depth |= ALLEGRO_AUDIO_UNSIGNED;
   }
   else if(voice->depth != ALLEGRO_AUDIO_16_BIT_INT &&
           voice->depth != ALLEGRO_AUDIO_8_BIT_UINT)
   {
      if((voice->settings&ALLEGRO_AUDIO_REQUIRE))
         return -1;
      voice->depth = ALLEGRO_AUDIO_16_BIT_INT;
   }

   if(voice->chan_conf != ALLEGRO_AUDIO_1_CH && voice->chan_conf != ALLEGRO_AUDIO_2_CH)
   {
      if(voice->chan_conf != ALLEGRO_AUDIO_4_CH ||
         !alIsExtensionPresent("AL_LOKI_quadriphonic"))
      {
         if((voice->settings&ALLEGRO_AUDIO_REQUIRE))
            return -1;
         voice->chan_conf = ALLEGRO_AUDIO_2_CH;
      }
   }

   ex_data = calloc(1, sizeof(*ex_data));
   if(!ex_data)
      return -1;

   if(voice->chan_conf == ALLEGRO_AUDIO_1_CH)
   {
      if(voice->depth == ALLEGRO_AUDIO_8_BIT_UINT)
         ex_data->format = AL_FORMAT_MONO8;
      else
         ex_data->format = AL_FORMAT_MONO16;
   }
   else if(voice->chan_conf == ALLEGRO_AUDIO_2_CH)
   {
      if(voice->depth == ALLEGRO_AUDIO_8_BIT_UINT)
         ex_data->format = AL_FORMAT_STEREO8;
      else
         ex_data->format = AL_FORMAT_STEREO16;
   }
   else
   {
      if(voice->depth == ALLEGRO_AUDIO_8_BIT_UINT)
         ex_data->format = AL_FORMAT_QUAD8_LOKI;
      else
         ex_data->format = AL_FORMAT_QUAD16_LOKI;
   }

   ex_data->stop_voice = 1;
   voice->extra = ex_data;

   return 0;
}

/* The deallocate_voice method should free the resources for the given voice,
   but still retain a hold on the device. The voice should be stopped and
   unloaded by the time this is called */
static void _openal_deallocate_voice(ALLEGRO_VOICE *voice)
{
   free(voice->extra);
   voice->extra = NULL;
}

/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned long _openal_get_voice_position(const ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;
   ALint pos;

   alGetSourcei(ex_data->source, AL_SAMPLE_OFFSET, &pos);
   if(alGetError() != AL_NO_ERROR)
      return 0;
   return pos;
}

/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int _openal_set_voice_position(ALLEGRO_VOICE *voice, unsigned long val)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   alSourcei(ex_data->source, AL_SAMPLE_OFFSET, val);
   if(alGetError() != AL_NO_ERROR)
      return -1;
   return 0;
}

/* A good portion of the get/set methods will use driver-specific option
   values, though it is strongly encouraged to support a standard set of
   options. The set methods may be called without the driver being initialized,
   and generally shouldn't take effect until the next time the driver is
   initialized. The get methods are allowed to fail (cleanly) if the driver
   isn't currently in use and should return the values being used (not
   necessarilly what was previously set) */

static int _openal_set_bool(ALLEGRO_AUDIO_ENUM setting, bool val)
{
   return 1;
   (void)setting;
   (void)val;
}

static int _openal_set_enum(ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_CHANNELS:
         if(val&0xF)
            return 1;
         preferred_channels = val>>4;
         return 0;
      default:
         break;
   }
   return 1;
}

/* With the set_long method, the following options would be useful:
 * * ALLEGRO_AUDIO_FREQUENCY - the over-all output frequency (in hz)
 * * ALLEGRO_AUDIO_FRAGMENTS - total fragment count (# of buffers for the output
 *                        device)
 * * ALLEGRO_AUDIO_LENGTH    - the size of each fragment (in bytes)
 * The values set are merely hints and don't need to be followed precisely.
 */
static int _openal_set_long(ALLEGRO_AUDIO_ENUM setting, unsigned long val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_FREQUENCY:
         preferred_freq = val;
         return 0;

      case ALLEGRO_AUDIO_FRAGMENTS:
         if(val)
            preferred_buf_count = val;
         return 0;

      case ALLEGRO_AUDIO_LENGTH:
         if(val)
            preferred_frag_size = val;
         return 0;

      default:
         break;
   }

   return 1;
}

/* The set_ptr method should be able to take the following:
 * * ALLEGRO_AUDIO_DEVICE - The device the driver should use, represented as a
 *                     string. The pointer should not be assumed to remain
 *                     valid after the call is completed.
 */
static int _openal_set_ptr(ALLEGRO_AUDIO_ENUM setting, const void *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_DEVICE:
         if(val)
            strncpy(preferred_devices, val, sizeof(preferred_devices));
         else
            preferred_devices[0] = '\0';
         return 0;

      default:
         break;
   }

   return 1;
}


static int _openal_get_bool(ALLEGRO_AUDIO_ENUM setting, bool *val)
{
   return 1;
   (void)setting;
   (void)val;
}

static int _openal_get_enum(ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val)
{
   return 1;
   (void)setting;
   (void)val;
}

/* The get_long method would  be useful to retrieve the following options:
 * * ALLEGRO_AUDIO_FREQUENCY - The actual output frequency in use by the current
 *                        device
 * * ALLEGRO_AUDIO_LENGTH    - The length in samples of each buffer fragment
 * * ALLEGRO_AUDIO_FRAGMENTS - The number of fragments
 */
static int _openal_get_long(ALLEGRO_AUDIO_ENUM setting, unsigned long *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_FREQUENCY:
      {
         ALCenum err;
         ALCint freq = 0;
         if(!audio_dev)
            return 1;

         alcGetIntegerv(audio_dev, ALC_FREQUENCY, 1, &freq);
         if((err=alcGetError(audio_dev)) != ALC_NO_ERROR)
            return 1;
         *val = freq;
         return 0;
      }

      case ALLEGRO_AUDIO_LENGTH:
      {
         ALCint size = 0;
         if(!audio_dev)
            return 1;

         alcGetIntegerv(audio_dev, ALC_REFRESH, 1, &size);
         if(alcGetError(audio_dev) != ALC_NO_ERROR)
            return 1;
         *val = size;
         return 0;
      }

      default:
         break;
   }

   return 1;
}

/* The get_ptr method should handle the following:
 * * ALLEGRO_AUDIO_DEVICE - The name of the current device, given as a string. The
 *                     returned string will not be modified.
 */
static int _openal_get_ptr(ALLEGRO_AUDIO_ENUM setting, const void **val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_DEVICE:
         if(!audio_dev)
            return 1;
         *val = alcGetString(audio_dev, ALC_DEVICE_SPECIFIER);
         return 0;

      default:
         break;
   }

   return 1;
}


ALLEGRO_AUDIO_DRIVER _openal_driver = {
   "OpenAL",

   _openal_open,
   _openal_close,

   _openal_get_bool,
   _openal_get_enum,
   _openal_get_long,
   _openal_get_ptr,

   _openal_set_bool,
   _openal_set_enum,
   _openal_set_long,
   _openal_set_ptr,

   _openal_allocate_voice,
   _openal_deallocate_voice,

   _openal_load_voice,
   _openal_unload_voice,

   _openal_start_voice,
   _openal_stop_voice,

   _openal_voice_is_playing,

   _openal_get_voice_position,
   _openal_set_voice_position,
};
