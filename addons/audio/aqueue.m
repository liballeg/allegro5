/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Apple Audio Queue driver
 *
 *      By Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"
#include "allegro5/internal/aintern_vector.h"

#import <CoreAudio/CoreAudioTypes.h>
#import <AudioToolbox/AudioToolbox.h>

#ifdef ALLEGRO_IPHONE
#import <AVFoundation/AVFoundation.h>
#endif

#import <Foundation/NSAutoreleasePool.h>
#import <AudioToolbox/AudioQueue.h>

#define THREAD_BEGIN NSAutoreleasePool *___p = [[NSAutoreleasePool alloc] init];
#define THREAD_END [___p drain];

// Make configurable
#define BUFFER_SIZE 1024*2 // in samples
#define NUM_BUFFERS 4

ALLEGRO_DEBUG_CHANNEL("AudioQueue")

typedef struct ALLEGRO_AQ_DATA {
   int bits_per_sample;
   int channels;
   bool playing;
   unsigned int buffer_size;
   unsigned char *silence;
   ALLEGRO_VOICE *voice;
   AudioQueueRef queue;
   AudioQueueBufferRef buffers[NUM_BUFFERS];
} ALLEGRO_AQ_DATA;

static _AL_VECTOR saved_voices = _AL_VECTOR_INITIALIZER(ALLEGRO_VOICE*);
static _AL_LIST* output_device_list;

/* Audio queue callback */
static void handle_buffer(
   void *in_data,
   AudioQueueRef inAQ,
   AudioQueueBufferRef inBuffer)
{
   ALLEGRO_AQ_DATA *ex_data = in_data;
   const void *data;

   (void)inAQ; // unsused

   unsigned int samples = (ex_data->buffer_size / ex_data->channels) /
                          (ex_data->bits_per_sample / 8);

   data = _al_voice_update(ex_data->voice, ex_data->voice->mutex, &samples);
   if (data == NULL)
      data = ex_data->silence;

   unsigned int copy_bytes = samples * ex_data->channels *
      (ex_data->bits_per_sample / 8);
   copy_bytes = _ALLEGRO_MIN(copy_bytes, inBuffer->mAudioDataBytesCapacity);

   memcpy(inBuffer->mAudioData, data, copy_bytes);
   inBuffer->mAudioDataByteSize = copy_bytes;

   AudioQueueEnqueueBuffer(
      ex_data->queue,
      inBuffer,
      0,
      NULL
   );
}

#ifdef ALLEGRO_IPHONE
static int _aqueue_start_voice(ALLEGRO_VOICE *voice);
static int _aqueue_stop_voice(ALLEGRO_VOICE* voice);

static void interruption_callback(void *inClientData, UInt32 inInterruptionState)
{
   unsigned i;
   (void)inClientData;
   for (i = 0; i < _al_vector_size(&saved_voices); i++) {
      ALLEGRO_VOICE **voice = _al_vector_ref(&saved_voices, i);
      if (inInterruptionState == kAudioSessionBeginInterruption) {
         _aqueue_stop_voice(*voice);
      }
      else {
         _aqueue_start_voice(*voice);
      }
   }
}

// This allows plugging/unplugging of hardware/bluetooth/speakers etc while keeping the sound playing
static void property_listener(void *inClientData, AudioSessionPropertyID inID, UInt32 inDataSize, const void *inData)
{
   unsigned i;
   (void)inClientData;
   (void)inDataSize;

   if (inID == kAudioSessionProperty_AudioRouteChange) {
      for (i = 0; i < _al_vector_size(&saved_voices); i++) {
         ALLEGRO_VOICE **voice = _al_vector_ref(&saved_voices, i);
         UInt32 reason = (UInt32)inData;
         if (reason == kAudioSessionRouteChangeReason_NewDeviceAvailable) {
            _aqueue_stop_voice(*voice);
            _aqueue_start_voice(*voice);
         }
      }
   }
}
#endif

static bool _aqueue_device_has_scope(AudioObjectID deviceID, AudioObjectPropertyScope scope)
{
    AudioObjectPropertyAddress propertyAddress = {
        .mSelector = kAudioDevicePropertyStreamConfiguration,
        .mScope = scope,
        .mElement = kAudioObjectPropertyElementWildcard
    };

    UInt32 dataSize = 0;
    if (AudioObjectGetPropertyDataSize(deviceID, &propertyAddress, 0, NULL, &dataSize) != noErr) {
      return false;
    }

    AudioBufferList *bufferList = al_malloc(dataSize);
    if(bufferList == NULL) {
        return false;
    }

    if (AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, NULL, &dataSize, bufferList) != noErr) {
      al_free(bufferList);
      return false;
    }

    BOOL supportsScope = bufferList->mNumberBuffers > 0;
    al_free(bufferList);
    return supportsScope;
}

static _AL_LIST* _aqueue_get_output_devices(void)
{
   return output_device_list;
}

static void _output_device_list_dtor(void* value, void* userdata)
{
   (void)userdata;

   ALLEGRO_AUDIO_DEVICE* device = (ALLEGRO_AUDIO_DEVICE*)value;
   al_free(device->name);
   al_free(device->identifier);
   al_free(device);
}

static void _aqueue_list_audio_output_devices(void)
{
   output_device_list = _al_list_create();

   AudioObjectPropertyAddress propertyAddress;
   AudioObjectID *deviceIDs;
   UInt32 propertySize;
   NSInteger numDevices;

   propertyAddress.mSelector = kAudioHardwarePropertyDevices;
   propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
   propertyAddress.mElement = kAudioObjectPropertyElementMaster;

   if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize) != noErr) {
      return;
   }

   numDevices = propertySize / sizeof(AudioDeviceID);
   deviceIDs = (AudioDeviceID *)al_calloc(numDevices, sizeof(AudioDeviceID));

   if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize, deviceIDs) != noErr) {
      return;
   }

   for (NSInteger idx=0; idx<numDevices; idx++) {
      AudioObjectPropertyAddress deviceAddress;
      char deviceName[64];

      propertySize = sizeof(deviceName);
      deviceAddress.mSelector = kAudioDevicePropertyDeviceName;
      deviceAddress.mScope = kAudioObjectPropertyScopeGlobal;
      #if (__MAC_OS_X_VERSION_MAX_ALLOWED >= 120000)
         deviceAddress.mElement = kAudioObjectPropertyElementMain;
      #else
         deviceAddress.mElement = kAudioObjectPropertyElementMaster;
      #endif
      if (AudioObjectGetPropertyData(deviceIDs[idx], &deviceAddress, 0, NULL, &propertySize, deviceName) != noErr) {
         continue;
      }

      if (_aqueue_device_has_scope(deviceIDs[idx], kAudioObjectPropertyScopeOutput)) {
         ALLEGRO_AUDIO_DEVICE* device = (ALLEGRO_AUDIO_DEVICE*)al_malloc(sizeof(ALLEGRO_AUDIO_DEVICE));
         device->identifier = (void*)al_malloc(sizeof(AudioDeviceID));
         device->name = (char*)al_malloc(sizeof(deviceName));

         memcpy(device->identifier, &deviceIDs[idx], sizeof(AudioDeviceID));
         memcpy(device->name, deviceName, sizeof(deviceName));

         _al_list_push_back_ex(output_device_list, device, _output_device_list_dtor);
      }
   }

   al_free(deviceIDs);
}

/* The open method starts up the driver and should lock the device, using the
   previously set paramters, or defaults. It shouldn't need to start sending
   audio data to the device yet, however. */
static int _aqueue_open(void)
{
#ifdef ALLEGRO_IPHONE
   /* These settings allow ipod music playback simultaneously with
    * our Allegro music/sfx, and also do not stop the streams when
    * a phone call comes in (it's muted for the duration of the call).
    */
   AudioSessionInitialize(NULL, NULL, interruption_callback, NULL);

   UInt32 sessionCategory = kAudioSessionCategory_AmbientSound;
   AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
      sizeof(sessionCategory), &sessionCategory);

   UInt32 mix = TRUE;
   AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryMixWithOthers,
      sizeof(mix), &mix);

   AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, property_listener, NULL);
#endif

   _aqueue_list_audio_output_devices();

   return 0;
}


/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void _aqueue_close(void)
{
   _al_list_destroy(output_device_list);
   _al_vector_free(&saved_voices);
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
      default:
         return 1;
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

   ex_data = (ALLEGRO_AQ_DATA *)al_calloc(1, sizeof(*ex_data));
   if (!ex_data) {
      fprintf(stderr, "Could not allocate voice data memory\n");
      return 1;
   }

   ex_data->bits_per_sample = bits_per_sample;
   ex_data->channels = channels;
   ex_data->buffer_size = BUFFER_SIZE*channels*(bits_per_sample/8);
   ex_data->playing = false;

   voice->extra = ex_data;
   ex_data->voice = voice;

   ex_data->silence = al_calloc(1, ex_data->buffer_size);

   return 0;
}

/* The deallocate_voice method should free the resources for the given voice,
   but still retain a hold on the device. The voice should be stopped and
   unloaded by the time this is called */
static void _aqueue_deallocate_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AQ_DATA *ex_data = voice->extra;
   al_free(ex_data->silence);
   al_free(ex_data);
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
   (void)voice;
   (void)data;
   return 1;
}

/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void _aqueue_unload_voice(ALLEGRO_VOICE *voice)
{
   /* FIXME */
   (void)voice;
}


static void *stream_proc(void *in_data)
{
   THREAD_BEGIN

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
#ifdef ALLEGRO_BIG_ENDIAN
   desc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
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
      kCFRunLoopCommonModes,
      0,
      &ex_data->queue);
   (void)ret; // FIXME: handle failure?

   int i;
   for (i = 0; i < NUM_BUFFERS; ++i) {
      AudioQueueAllocateBuffer(
         ex_data->queue,
         ex_data->buffer_size,
         &ex_data->buffers[i]
      );

      memcpy(ex_data->buffers[i]->mAudioData, ex_data->silence, ex_data->buffer_size);
      ex_data->buffers[i]->mAudioDataByteSize = ex_data->buffer_size;
      ex_data->buffers[i]->mUserData = ex_data;
      // FIXME: Safe to comment this out?
      //ex_data->buffers[i]->mPacketDescriptionCount = 0;

      AudioQueueEnqueueBuffer(
         ex_data->queue,
         ex_data->buffers[i],
         0,
         NULL
      );
   }

   AudioQueueSetParameter(
      ex_data->queue,
      kAudioQueueParam_Volume,
      1.0f
   );

   ret = AudioQueueStart(
      ex_data->queue,
      NULL
   );

   do {
      THREAD_BEGIN
      CFRunLoopRunInMode(
         kCFRunLoopDefaultMode,
         0.05,
         false
      );
      THREAD_END
   } while (ex_data->playing);

   THREAD_END

   return NULL;
}


/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int _aqueue_start_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AQ_DATA *ex_data = voice->extra;

   if (voice->is_streaming && !ex_data->playing) {
      *(ALLEGRO_VOICE**)_al_vector_alloc_back(&saved_voices) = voice;
      ex_data->playing = true;
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

   if (ex_data->playing) {
      _al_vector_find_and_delete(&saved_voices, &voice);
      ex_data->playing = false;

      AudioQueueDispose(
         ex_data->queue,
         false /* Do it asynchronously so the feeder thread can exit. */
      );
   }

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
static unsigned int _aqueue_get_voice_position(const ALLEGRO_VOICE *voice)
{
   /* FIXME */
   (void)voice;
   return 0;
}

/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int _aqueue_set_voice_position(ALLEGRO_VOICE *voice, unsigned int val)
{
   /* FIXME */
   (void)voice;
   (void)val;
   return 0;
}

typedef struct RECORDER_DATA
{
   AudioStreamBasicDescription data_format;
   AudioQueueRef queue;

   /* AudioQueue buffer information */
   AudioQueueBufferRef *buffers;
   int buffer_count;
   uint32_t buffer_size;

   /* User fragment buffer information.
    *   Which buffer are we writing into, and
    *   how much data has already been written?
    */
   unsigned int fragment_i;
   unsigned int samples_written;

} RECORDER_DATA;

static void _aqueue_recording_callback(void *user_data, AudioQueueRef aq,
                                       AudioQueueBufferRef aq_buffer, const AudioTimeStamp *start_time,
                                       UInt32 sample_count, const AudioStreamPacketDescription *descs)
{
   ALLEGRO_AUDIO_RECORDER *recorder = (ALLEGRO_AUDIO_RECORDER *) user_data;
   RECORDER_DATA *data = (RECORDER_DATA *) recorder->extra;
   char *input = aq_buffer->mAudioData;

   (void) aq;
   (void) start_time;
   (void) descs;

   if (sample_count == 0 && data->data_format.mBytesPerPacket != 0) {
      sample_count = aq_buffer->mAudioDataByteSize / data->data_format.mBytesPerPacket;
   }

   al_lock_mutex(recorder->mutex);

   if (recorder->is_recording) {
      /* Process the full buffer, putting it into as many user fragments as it needs.
       * Send an event for every full user fragment.
       */
      while (sample_count > 0) {
         unsigned int samples_left = recorder->samples - data->samples_written;
         unsigned int samples_to_write = sample_count < samples_left ? sample_count : samples_left;

         /* Copy the incoming data into the user's fragment buffer */
         memcpy((char*)recorder->fragments[data->fragment_i] + data->samples_written * recorder->sample_size,
                input, samples_to_write * recorder->sample_size);

         input += samples_to_write * recorder->sample_size;
         data->samples_written += samples_to_write;
         sample_count -= samples_to_write;

         /* We should have never written more samples than the user asked for */
         ALLEGRO_ASSERT(recorder->samples >= data->samples_written);

         if (data->samples_written == recorder->samples) {
            ALLEGRO_EVENT user_event;
            ALLEGRO_AUDIO_RECORDER_EVENT *e;
            user_event.user.type = ALLEGRO_EVENT_AUDIO_RECORDER_FRAGMENT;
            e = al_get_audio_recorder_event(&user_event);
            e->buffer = recorder->fragments[data->fragment_i];
            e->samples = recorder->samples;
            al_emit_user_event(&recorder->source, &user_event, NULL);

            if (++data->fragment_i == recorder->fragment_count) {
               data->fragment_i = 0;
            }

            data->samples_written = 0;
         }
      }
   }

   al_unlock_mutex(recorder->mutex);


   AudioQueueEnqueueBuffer(data->queue, aq_buffer, 0, NULL);
}

static int _aqueue_allocate_recorder(ALLEGRO_AUDIO_RECORDER *recorder)
{
   RECORDER_DATA *data;
   int i;
   int ret;

   data = al_calloc(1, sizeof(*data));
   if (!data) return 1;

   data->buffer_count = 3;
   data->buffers = al_calloc(data->buffer_count, sizeof(AudioQueueBufferRef));
   if (!data->buffers) {
      al_free(data);
      return 1;
   }

   data->data_format.mFormatID = kAudioFormatLinearPCM;
   data->data_format.mSampleRate = recorder->frequency;
   data->data_format.mChannelsPerFrame = al_get_channel_count(recorder->chan_conf);
   data->data_format.mFramesPerPacket = 1;
   data->data_format.mBitsPerChannel = al_get_audio_depth_size(recorder->depth) * 8;
   data->data_format.mBytesPerFrame =
   data->data_format.mBytesPerPacket = data->data_format.mChannelsPerFrame * al_get_audio_depth_size(recorder->depth);

   data->data_format.mFormatFlags = kLinearPCMFormatFlagIsPacked;
#ifdef ALLEGRO_BIG_ENDIAN
   data->data_format.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
   if (recorder->depth == ALLEGRO_AUDIO_DEPTH_FLOAT32)
      data->data_format.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
   else if (!(recorder->depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED))
      data->data_format.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;

   data->buffer_size = 2048; // in bytes

   ret = AudioQueueNewInput(
      &data->data_format,
      _aqueue_recording_callback,
      recorder,
      NULL,
      kCFRunLoopCommonModes,
      0,
      &data->queue
   );

   if (ret) {
      ALLEGRO_ERROR("AudioQueueNewInput failed (%d)\n", ret);
      al_free(data->buffers);
      al_free(data);
      return 1;
   }

   /* Create the buffers */
   for (i = 0; i < data->buffer_count; ++i) {
      AudioQueueAllocateBuffer(data->queue, data->buffer_size, &data->buffers[i]);
      AudioQueueEnqueueBuffer(data->queue, data->buffers[i], 0, NULL);
   }

   AudioQueueStart(data->queue, NULL);

   recorder->extra = data;

   /* No thread is created. */

   return 0;
}

static void _aqueue_deallocate_recorder(ALLEGRO_AUDIO_RECORDER *recorder)
{
   RECORDER_DATA *data = (RECORDER_DATA *) recorder->extra;

   AudioQueueStop(data->queue, true);
   AudioQueueDispose(data->queue, true);

   al_free(data->buffers);
   al_free(data);
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

   _aqueue_allocate_recorder,
   _aqueue_deallocate_recorder,

   _aqueue_get_output_devices,
};

