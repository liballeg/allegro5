#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"

#import <CoreAudio/CoreAudioTypes.h>
#import <AudioToolbox/AudioQueue.h>

// FIXME
#include <stdio.h>

// Make configurable
#define BUFFER_SIZE 1024*2 // in samples
#define NUM_BUFFERS 4

typedef struct ALLEGRO_AQ_DATA {
   int bits_per_sample;
   int channels;
   bool playing;
   unsigned long samples_per_update;
   unsigned int buffer_size;
   ALLEGRO_VOICE *voice;
   AudioQueueBufferRef buffers[NUM_BUFFERS];
} ALLEGRO_AQ_DATA;

static bool playing = false;
static AudioQueueRef queue;
static unsigned char *silence;

/* Audio queue callback */
static void handle_buffer(
   void *in_data,
   AudioQueueRef inAQ,
   AudioQueueBufferRef inBuffer)
{
   ALLEGRO_AQ_DATA *ex_data = in_data;
   const void *data;

   data = _al_voice_update(ex_data->voice, &ex_data->samples_per_update);
   if (data == NULL)
      data = silence;

   memcpy(inBuffer->mAudioData, data, ex_data->buffer_size);
   inBuffer->mAudioDataByteSize = ex_data->buffer_size;

   AudioQueueEnqueueBuffer(
      queue,
      inBuffer,
      0,
      NULL
   );
}


/* The open method starts up the driver and should lock the device, using the
   previously set paramters, or defaults. It shouldn't need to start sending
   audio data to the device yet, however. */
static int _aqueue_open()
{ 
   return 0;
}


/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void _aqueue_close()
{
}


/* The allocate_voice method should grab a voice from the system, and allocate
   any data common to streaming and non-streaming sources. */
static int _aqueue_allocate_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AQ_DATA *ex_data;
   int bits_per_sample;
   int channels;

   switch (voice->depth)
   {
      case ALLEGRO_AUDIO_DEPTH_UINT16:
      case ALLEGRO_AUDIO_DEPTH_INT16:
         bits_per_sample = 16;
         break;
   }

   switch (voice->chan_conf) {
      case ALLEGRO_CHANNEL_CONF_1:
         channels = 1;
         break;
      case ALLEGRO_CHANNEL_CONF_2:
         channels = 2;
         break;
      default:
         fprintf(stderr, "Unsupported number of channels\n");
         return 1;
   }

   ex_data = (ALLEGRO_AQ_DATA *)calloc(1, sizeof(*ex_data));
   if (!ex_data) {
      fprintf(stderr, "Could not allocate voice data memory\n"); 
      return 1;
   }

   ex_data->bits_per_sample = bits_per_sample;
   ex_data->channels = channels;
   ex_data->buffer_size = BUFFER_SIZE*channels*(bits_per_sample/8);
   ex_data->playing = false;
   ex_data->samples_per_update = BUFFER_SIZE;

   playing = false;

   voice->extra = ex_data;
   ex_data->voice = voice;
   
   silence = calloc(1, ex_data->buffer_size);

   return 0;
}

/* The deallocate_voice method should free the resources for the given voice,
   but still retain a hold on the device. The voice should be stopped and
   unloaded by the time this is called */
static void _aqueue_deallocate_voice(ALLEGRO_VOICE *voice)
{
   free(voice->extra);
   free(silence);
   voice->extra = NULL;
}

/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached sample's looping mode should be honored, and loading
   must fail if it cannot be. */
static int _aqueue_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   /* FIXME */
   return 1;
}

/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void _aqueue_unload_voice(ALLEGRO_VOICE *voice)
{
   /* FIXME */
}


static void *stream_proc(void *in_data)
{
   ALLEGRO_VOICE *voice = in_data;
   ALLEGRO_AQ_DATA *ex_data = voice->extra;

   AudioStreamBasicDescription desc;

   desc.mSampleRate = voice->frequency;
   desc.mFormatID = kAudioFormatLinearPCM;
   if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT16)
   	desc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |
		kLinearPCMFormatFlagIsPacked;
   else
   	desc.mFormatFlags = kLinearPCMFormatFlagIsFloat |
		kLinearPCMFormatFlagIsPacked;
   desc.mBytesPerPacket = ex_data->channels * (ex_data->bits_per_sample/8);
   desc.mFramesPerPacket = 1;
   desc.mBytesPerFrame = ex_data->channels * (ex_data->bits_per_sample/8);
   desc.mChannelsPerFrame = ex_data->channels;
   desc.mBitsPerChannel = ex_data->bits_per_sample;

   int ret = AudioQueueNewOutput(
      &desc,
      handle_buffer,
      ex_data,
      CFRunLoopGetCurrent(),
      kCFRunLoopDefaultMode,
      0,
      &queue);

   for (int i = 0; i < NUM_BUFFERS; ++i) {
      AudioQueueAllocateBuffer(
         queue,
         ex_data->buffer_size,
         &ex_data->buffers[i]
      );

      memcpy(ex_data->buffers[i]->mAudioData, silence, ex_data->buffer_size);
      ex_data->buffers[i]->mAudioDataByteSize = ex_data->buffer_size;
      ex_data->buffers[i]->mUserData = ex_data;
      ex_data->buffers[i]->mPacketDescriptionCount = 0;

      AudioQueueEnqueueBuffer(
         queue,
         ex_data->buffers[i],
         0,
         NULL
      );

   }

   AudioQueueSetParameter(
      queue,
      kAudioQueueParam_Volume,
      1.0f
   );

   ret = AudioQueueStart(
      queue,
      NULL
   );

   ex_data->playing = true;
   playing = true;

   do {
      CFRunLoopRunInMode(
         kCFRunLoopDefaultMode,
         0.01,
         false
      );
   } while (playing);
    
   return NULL;
}


/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int _aqueue_start_voice(ALLEGRO_VOICE *voice)
{
   if (voice->is_streaming) {
      al_run_detached_thread(stream_proc, voice);
      return 0;
   }

   /* FIXME */
   return 1;
}

/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int _aqueue_stop_voice(ALLEGRO_VOICE* voice)
{
   ALLEGRO_AQ_DATA *ex_data = voice->extra;
   int i;

   ex_data->playing = false;
   playing = false;

   al_rest(0.05);

   AudioQueueStop (
      queue,
      false
   );

   for (i = 0; i < NUM_BUFFERS; i++) {
   	AudioQueueFreeBuffer(queue, ex_data->buffers[i]);
   }

   AudioQueueDispose(
      queue,
      true
   );

   return 0;
}

/* The voice_is_playing method should only be called on non-streaming sources,
   and should return true if the voice is playing */
static bool _aqueue_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   ALLEGRO_AQ_DATA *ex_data = (ALLEGRO_AQ_DATA *)voice->extra;

   return ex_data->playing;
}

/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned long _aqueue_get_voice_position(const ALLEGRO_VOICE *voice)
{
   /* FIXME */
   return 0;
}

/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int _aqueue_set_voice_position(ALLEGRO_VOICE *voice, unsigned long val)
{
   /* FIXME */
   return 0;
}

ALLEGRO_AUDIO_DRIVER _al_kcm_aqueue_driver = {
   "Apple Audio Queues",

   _aqueue_open,
   _aqueue_close,

   _aqueue_allocate_voice,
   _aqueue_deallocate_voice,

   _aqueue_load_voice,
   _aqueue_unload_voice,

   _aqueue_start_voice,
   _aqueue_stop_voice,

   _aqueue_voice_is_playing,

   _aqueue_get_voice_position,
   _aqueue_set_voice_position,
};

