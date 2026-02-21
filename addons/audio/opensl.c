/* OpenSL: The Standard for Embedded Audio Acceleration
 * http://www.khronos.org/opensles/
 * http://www.khronos.org/registry/sles/specs/OpenSL_ES_Specification_1.1.pdf
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"

#include <SLES/OpenSLES.h>

/* TODO:
 * If an android application goes into the suspend state then the opensl resources
 * might become 'unrealized' or 'suspended' so care needs to be taken
 * to realize them again once the application is resumed.
 */

/* Not sure if this one is needed, yet */
// #include <SLES/OpenSLES_Android.h>

/* Feel free to change MAX_FRAMES and MAX_BUFFERS if it affects
 * choppiness for your device.
 */

/* Number of samples to read in one call to al_voice_update */
static const int MAX_FRAMES = 2048;

/* Number of opensl buffers to use */
#define MAX_BUFFERS 2

ALLEGRO_DEBUG_CHANNEL("opensl")

static SLObjectItf engine;

static const char * opensl_get_error_string(SLresult result)
{
   switch (result){
      case SL_RESULT_PRECONDITIONS_VIOLATED: return "Preconditions violated";
      case SL_RESULT_PARAMETER_INVALID: return "Invalid parameter";
      case SL_RESULT_MEMORY_FAILURE: return "Memory failure";
      case SL_RESULT_RESOURCE_ERROR: return "Resource error";
      case SL_RESULT_RESOURCE_LOST: return "Resource lost";
      case SL_RESULT_IO_ERROR: return "IO error";
      case SL_RESULT_BUFFER_INSUFFICIENT: return "Insufficient buffer";
      case SL_RESULT_CONTENT_CORRUPTED: return "Content corrupted";
      case SL_RESULT_CONTENT_UNSUPPORTED: return "Content unsupported";
      case SL_RESULT_CONTENT_NOT_FOUND: return "Content not found";
      case SL_RESULT_PERMISSION_DENIED: return "Permission denied";
      case SL_RESULT_FEATURE_UNSUPPORTED: return "Feature unsupported";
      case SL_RESULT_INTERNAL_ERROR: return "Internal error";
      case SL_RESULT_UNKNOWN_ERROR: return "Unknown error";
      case SL_RESULT_OPERATION_ABORTED: return "Operation aborted";
      case SL_RESULT_CONTROL_LOST: return "Control lost";
   }
   return "Unknown OpenSL error";
}

/* Only the original 'engine' object should be passed here */
static SLEngineItf getEngine(SLObjectItf engine){
   SLresult result;
   SLEngineItf interface;
   result = (*engine)->GetInterface(engine, SL_IID_ENGINE, &interface);
   if (result == SL_RESULT_SUCCESS){
      return interface;
   } else {
      ALLEGRO_ERROR("Could not get opensl engine: %s\n", opensl_get_error_string(result));
      return NULL;
   }
}

/* Create an output mixer */
static SLObjectItf createOutputMixer(SLEngineItf engine){
   SLresult result;
   SLObjectItf output;
   SLboolean required[1];
   SLInterfaceID ids[1];

   /* Not all android devices support a mixer that you can control
    * the volume on, so just ignore it for now.
    */
   required[0] = SL_BOOLEAN_TRUE;
   ids[0] = SL_IID_VOLUME;

   result = (*engine)->CreateOutputMix(engine, &output, 0, ids, required);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not create output mix: %s\n", opensl_get_error_string(result));
      return NULL;
   }

   result = (*output)->Realize(output, SL_BOOLEAN_FALSE);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not realize the output mix: %s\n", opensl_get_error_string(result));
      (*output)->Destroy(output);
      return NULL;
   }

   return output;
}

static int _opensl_open(void)
{
   SLresult result;
   SLuint32 state;
   SLEngineOption options[] = {
      { SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE },
      /*
      { SL_ENGINEOPTION_MAJORVERSION, (SLuint32) 1 },
      { SL_ENGINEOPTION_MINORVERSION, (SLuint32) 1 },
      */
   };

   result = slCreateEngine(&engine, 1, options, 0, NULL, NULL);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not open audio device: %s\n",
                 opensl_get_error_string(result));
      return 1;
   }

   /* Transition the engine to the realized state in synchronous mode */
   result = (*engine)->GetState(engine, &state);
   if (result == SL_RESULT_SUCCESS){
      switch (state){
         case SL_OBJECT_STATE_UNREALIZED: {
            result = (*engine)->Realize(engine, SL_BOOLEAN_FALSE);
            break;
         }
         case SL_OBJECT_STATE_REALIZED: {
            /* This is good */
            break;
         }
         case SL_OBJECT_STATE_SUSPENDED: {
            result = (*engine)->Resume(engine, SL_BOOLEAN_FALSE);
            break;
         }
      }
   } else {
      return 1;
   }

   // output = createOutputMixer(getEngine(engine));

   return 0;
}

static void _opensl_close(void)
{
   /*
   if (output != NULL){
      (*output)->Destroy(output);
      output = NULL;
   }
   */

   if (engine != NULL){
      (*engine)->Destroy(engine);
      engine = NULL;
   }
}

typedef struct OpenSLData{
   /* Output mixer */
   SLObjectItf output;
   /* Audio player */
   SLObjectItf player;
   volatile enum { PLAYING, STOPPING, STOPPED } status;

   /* load_voice stuff that isn't used, but might be someday */
   /*
   const void * data;
   int position;
   int length;
   */

   /* Size of a single sample: depth * channels */
   int frame_size;
   ALLEGRO_THREAD * poll_thread;

   /* local buffers to keep opensl fed since it doesn't copy
    * data by default.
    */
   char * buffers[MAX_BUFFERS];
} OpenSLData;

static SLDataFormat_PCM setupFormat(ALLEGRO_VOICE * voice){
   SLDataFormat_PCM format;
   format.formatType = SL_DATAFORMAT_PCM;

   format.numChannels = al_get_channel_count(voice->chan_conf);

   /* TODO: review the channelMasks */
   switch (voice->chan_conf){
      case ALLEGRO_CHANNEL_CONF_1: {
         /* Not sure if center is right.. */
         format.channelMask = SL_SPEAKER_FRONT_CENTER;
         break;
      }
      case ALLEGRO_CHANNEL_CONF_2: {
         format.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
         break;
      }
      case ALLEGRO_CHANNEL_CONF_3: {
         format.channelMask = SL_SPEAKER_FRONT_LEFT |
                         SL_SPEAKER_FRONT_RIGHT |
                         SL_SPEAKER_FRONT_CENTER;
         break;
      }
      case ALLEGRO_CHANNEL_CONF_4: {
         format.channelMask = SL_SPEAKER_FRONT_LEFT |
                         SL_SPEAKER_BACK_LEFT |
                         SL_SPEAKER_FRONT_RIGHT |
                         SL_SPEAKER_BACK_RIGHT;
         break;
      }
      case ALLEGRO_CHANNEL_CONF_5_1: {
         format.channelMask = SL_SPEAKER_FRONT_LEFT |
                         SL_SPEAKER_BACK_LEFT |
                         SL_SPEAKER_FRONT_RIGHT |
                         SL_SPEAKER_BACK_RIGHT |
                         SL_SPEAKER_FRONT_CENTER |
                         SL_SPEAKER_LOW_FREQUENCY;
         break;
      }
      case ALLEGRO_CHANNEL_CONF_6_1: {
         format.channelMask = SL_SPEAKER_FRONT_LEFT |
                         SL_SPEAKER_BACK_LEFT |
                         SL_SPEAKER_FRONT_RIGHT |
                         SL_SPEAKER_BACK_RIGHT |
                         SL_SPEAKER_FRONT_CENTER |
                         SL_SPEAKER_LOW_FREQUENCY |
                         SL_SPEAKER_SIDE_LEFT |
                         SL_SPEAKER_SIDE_RIGHT;

         break;
      }
      case ALLEGRO_CHANNEL_CONF_7_1: {
         format.channelMask = SL_SPEAKER_FRONT_LEFT |
                         SL_SPEAKER_BACK_LEFT |
                         SL_SPEAKER_FRONT_RIGHT |
                         SL_SPEAKER_BACK_RIGHT |
                         SL_SPEAKER_FRONT_CENTER |
                         SL_SPEAKER_LOW_FREQUENCY |
                         SL_SPEAKER_SIDE_LEFT |
                         SL_SPEAKER_SIDE_RIGHT |
                         SL_SPEAKER_TOP_CENTER;
         break;
      }
      default: {
         ALLEGRO_ERROR("Cannot allocate voice with unknown channel configuration\n");
      }
   }

   switch (voice->frequency){
      case 8000: format.samplesPerSec = SL_SAMPLINGRATE_8; break;
      case 11025: format.samplesPerSec = SL_SAMPLINGRATE_11_025; break;
      case 12000: format.samplesPerSec = SL_SAMPLINGRATE_12; break;
      case 16000: format.samplesPerSec = SL_SAMPLINGRATE_16; break;
      case 22050: format.samplesPerSec = SL_SAMPLINGRATE_22_05; break;
      case 24000: format.samplesPerSec = SL_SAMPLINGRATE_24; break;
      case 32000: format.samplesPerSec = SL_SAMPLINGRATE_32; break;
      case 44100: format.samplesPerSec = SL_SAMPLINGRATE_44_1; break;
      case 48000: format.samplesPerSec = SL_SAMPLINGRATE_48; break;
      case 64000: format.samplesPerSec = SL_SAMPLINGRATE_64; break;
      case 88200: format.samplesPerSec = SL_SAMPLINGRATE_88_2; break;
      case 96000: format.samplesPerSec = SL_SAMPLINGRATE_96; break;
      case 192000: format.samplesPerSec = SL_SAMPLINGRATE_192; break;
      default: {
         ALLEGRO_ERROR("Unsupported frequency %d. Using 44100 instead.\n", voice->frequency);
         format.samplesPerSec = SL_SAMPLINGRATE_44_1;
         voice->frequency = 44100;
      }
   }

   switch (voice->depth) {
      case ALLEGRO_AUDIO_DEPTH_UINT8:
      case ALLEGRO_AUDIO_DEPTH_INT8: {
         format.bitsPerSample = 8;
         format.containerSize = 8;
         break;
      }
      case ALLEGRO_AUDIO_DEPTH_UINT16:
      case ALLEGRO_AUDIO_DEPTH_INT16: {
         format.bitsPerSample = 16;
         format.containerSize = 16;
         break;
      }
      case ALLEGRO_AUDIO_DEPTH_UINT24:
      case ALLEGRO_AUDIO_DEPTH_INT24: {
         format.bitsPerSample = 24;
         format.containerSize = 32;
         break;
      }
      case ALLEGRO_AUDIO_DEPTH_FLOAT32: {
         format.bitsPerSample = 32;
         format.containerSize = 32;
         break;
      }
      default: {
         ALLEGRO_WARN("Cannot allocate unknown voice depth\n");
      }
   }

#ifdef ALLEGRO_BIG_ENDIAN
   format.endianness = SL_BYTEORDER_BIGENDIAN;
#else
   format.endianness = SL_BYTEORDER_LITTLEENDIAN;
#endif

   /* Then OpenSL spec says these values are needed for SLDataFormat_PCM_EX
    * but not all implementations define that struct. If they do then
    * the following code can be used.
    */

   /*
   switch (voice->depth){
      case ALLEGRO_AUDIO_DEPTH_UINT8:
      case ALLEGRO_AUDIO_DEPTH_UINT16:
      case ALLEGRO_AUDIO_DEPTH_UINT24: {
         format.representation = SL_PCM_REPRESENTATION_UNSIGNED_INT;
      }
      case ALLEGRO_AUDIO_DEPTH_INT8:
      case ALLEGRO_AUDIO_DEPTH_INT16:
      case ALLEGRO_AUDIO_DEPTH_INT24: {
         format.representation = SL_PCM_REPRESENTATION_SIGNED_INT;
         break;
      }
      case ALLEGRO_AUDIO_DEPTH_FLOAT32: {
         format.representation = SL_PCM_REPRESENTATION_FLOAT;
         break;
      }
   }
   */

   return format;
}

static SLObjectItf createAudioPlayer(SLEngineItf engine, SLDataSource * source, SLDataSink * sink){
   SLresult result;
   SLObjectItf player;

   SLboolean required[1];
   SLInterfaceID ids[1];

   required[0] = SL_BOOLEAN_TRUE;
   ids[0] = SL_IID_BUFFERQUEUE;

   result = (*engine)->CreateAudioPlayer(engine, &player, source, sink, 1, ids, required);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not create audio player: %s\n", opensl_get_error_string(result));
      return NULL;
   }

   result = (*player)->Realize(player, SL_BOOLEAN_FALSE);

   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not realize audio player: %s\n", opensl_get_error_string(result));
      return NULL;
   }

   return player;
}

static SLObjectItf makeStreamingPlayer(ALLEGRO_VOICE * voice, SLObjectItf mixer){
   SLDataFormat_PCM format = setupFormat(voice);
   SLDataLocator_BufferQueue bufferQueue;
   SLDataSource audioSource;
   SLDataSink audioSink;
   SLDataLocator_OutputMix output;

   bufferQueue.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
   bufferQueue.numBuffers = MAX_BUFFERS;

   audioSource.pFormat = (void*) &format;
   audioSource.pLocator = (void*) &bufferQueue;

   output.locatorType = SL_DATALOCATOR_OUTPUTMIX;
   output.outputMix = mixer;

   audioSink.pLocator = (void*) &output;
   audioSink.pFormat = NULL;

   return createAudioPlayer(getEngine(engine), &audioSource, &audioSink);

   /*
   SLresult result;
   SLVolumeItf volume;
   result = (*extra->output)->GetInterface(extra->output, SL_IID_VOLUME, &volume);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not get volume interface: %s\n", opensl_get_error_string(result));
      return 1;
   }
   */

}

/* Number of active buffers in the queue. Will not be more than MAX_BUFFERS */
static int bufferCount(SLObjectItf player){
   SLBufferQueueItf queue;
   SLBufferQueueState state;

   (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &queue);
   (*queue)->GetState(queue, &state);
   return state.count;
}

/* Attach some data to the OpenSL player. The data is not copied */
static void enqueue(SLObjectItf player, const void * data, int bytes){
   SLresult result;
   SLBufferQueueItf queue;
   SLPlayItf play;

   result = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &queue);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not get bufferqueue interface: %s\n", opensl_get_error_string(result));
      return;
   }

   // ALLEGRO_DEBUG("Play voice data %p\n", data);

   result = (*queue)->Enqueue(queue, data, bytes);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not enqueue data: %s\n", opensl_get_error_string(result));
      return;
   }

   // result = (*volume)->SetVolumeLevel(volume, -300);

   result = (*player)->GetInterface(player, SL_IID_PLAY, &play);

   /* In case the player is not playing, make it play */
   result = (*play)->SetPlayState(play, SL_PLAYSTATE_PLAYING);

   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not set play state on OpenSL stream\n");
   }

   /*
   SLBufferQueueState state;
   result = (*queue)->GetState(queue, &state);
   if (result == SL_RESULT_SUCCESS){
      ALLEGRO_DEBUG("Buffer queue state count %d index %d\n", state.count, state.playIndex);
   }
   */
}

static void * opensl_update(ALLEGRO_THREAD * self, void * data){
   ALLEGRO_VOICE *voice = data;
   OpenSLData * opensl = voice->extra;

   int bufferIndex = 0;
   while (!al_get_thread_should_stop(self)) {
      if (opensl->status == PLAYING) {
         // unsigned int frames = 4096;
         unsigned int frames = MAX_FRAMES;
         if (voice->is_streaming) {
            // streaming audio
            if (bufferCount(opensl->player) < MAX_BUFFERS){
               const void * data = _al_voice_update(voice, voice->mutex, &frames);
               if (data){
                  /* Copy the data to a local buffer because a call to enqueue
                   * will use the memory in place and al_voice_update will
                   * re-use the same buffer for each call so we don't want
                   * to corrupt memory when the next call to al_voice_update
                   * is made.
                   */
                  char * buffer = opensl->buffers[bufferIndex];
                  memcpy(buffer, data, frames * opensl->frame_size);
                  enqueue(opensl->player, buffer, frames * opensl->frame_size);

                  bufferIndex = (bufferIndex + 1) % MAX_BUFFERS;
               }
            } else {
               al_rest(0.001);
            }
         } else {
            ALLEGRO_ERROR("Unimplemented direct audio\n");
            /*
            // direct buffer audio
            al_lock_mutex(pv->buffer_mutex);
            const char *data = pv->buffer;
            unsigned int len = frames * pv->frame_size;
            pv->buffer += frames * pv->frame_size;
            if (pv->buffer > pv->buffer_end) {
               len = pv->buffer_end - data;
               pv->buffer = voice->attached_stream->spl_data.buffer.ptr;
               voice->attached_stream->pos = 0;
               if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_ONCE) {
                  pv->status = PV_STOPPING;
               }
            } else {
               voice->attached_stream->pos += frames;
            }
            al_unlock_mutex(pv->buffer_mutex);

            pa_simple_write(pv->s, data, len, NULL);
            */
         }
      } else if (opensl->status == STOPPING){
         if (bufferCount(opensl->player) == 0){
            /* When the buffer count is 0 the opensl buffer queue should
             * transition to the SL_PLAYSTATE_STOPPED state automatically.
             */
            opensl->status = STOPPED;
         }
      } else if (opensl->status == STOPPED){
         al_rest(0.001);
      }
   }

   return NULL;
}

static int _opensl_allocate_voice(ALLEGRO_VOICE *voice)
{
   OpenSLData * data;
   int i;

   data = al_calloc(1, sizeof(*data));
   voice->extra = data;

   data->output = createOutputMixer(getEngine(engine));
   if (data->output == NULL){
      al_free(data);
      return 1;
   }

   data->player = NULL;
   /*
   data->data = NULL;
   data->position = 0;
   data->length = voice->buffer_size;
   */
   data->frame_size = al_get_channel_count(voice->chan_conf) * al_get_audio_depth_size(voice->depth);
   data->status = STOPPED;
   data->poll_thread = NULL;
   for (i = 0; i < MAX_BUFFERS; i++){
      data->buffers[i] = al_malloc(data->frame_size * MAX_FRAMES);
   }

   data->player = makeStreamingPlayer(voice, data->output);
   if (data->player == NULL){
      return 1;
   }
   data->poll_thread = al_create_thread(opensl_update, (void*)voice);
   al_start_thread(data->poll_thread);

   return 0;
}

static void _opensl_deallocate_voice(ALLEGRO_VOICE *voice)
{
   OpenSLData * data = (OpenSLData*) voice->extra;
   int i;
   if (data->poll_thread != NULL){
      al_set_thread_should_stop(data->poll_thread);
      al_join_thread(data->poll_thread, NULL);
      al_destroy_thread(data->poll_thread);
      data->poll_thread = NULL;
   }

   if (data->player != NULL){
      (*data->player)->Destroy(data->player);
      data->player = NULL;
   }

   if (data->output != NULL){
      (*data->output)->Destroy(data->output);
      data->output = NULL;
   }

   for (i = 0; i < MAX_BUFFERS; i++){
      al_free(data->buffers[i]);
   }
   al_free(voice->extra);
   voice->extra = NULL;
}

/* load_voice is only called by attach_sample_instance_to_voice which
 * isn't really used, so we leave it unimplemented for now.
 */
static int _opensl_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   (void) voice;
   (void) data;
   /*
   OpenSLData * extra = (OpenSLData*) voice->extra;
   ALLEGRO_DEBUG("Load voice data %p\n", data);
   extra->data = data;
   extra->position = 0;
   */

   return 1;
}

static void _opensl_unload_voice(ALLEGRO_VOICE *voice)
{
   (void) voice;
   /*
   OpenSLData * extra = (OpenSLData*) voice->extra;
   extra->data = NULL;
   extra->position = 0;
   */
}

/*
static void updateQueue(SLBufferQueueItf queue, void * context){
   OpenSLData * data = (OpenSLData*) context;
   if (data->position < data->length){
      int bytes = data->frame_size * 1024;
      if (data->position + bytes > data->length){
         bytes = ((data->length - data->position) / data->frame_size) * data->frame_size;
      }

      SLresult result;
      ALLEGRO_DEBUG("Enqueue %d bytes\n", bytes);
      result = (*queue)->Enqueue(queue, (char*) data->data + data->position, bytes);
      data->position += bytes;
   }
}
*/

static int _opensl_start_voice(ALLEGRO_VOICE *voice)
{
   OpenSLData * extra = (OpenSLData*) voice->extra;

   extra->status = PLAYING;

   /*
   result = (*extra->player)->GetInterface(extra->player, SL_IID_BUFFERQUEUE, &queue);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not get bufferqueue interface: %s\n", opensl_get_error_string(result));
      return 1;
   }

   ALLEGRO_DEBUG("Start playing voice data %p\n", extra->data);

   result = (*queue)->Enqueue(queue, (char*) extra->data + extra->position, extra->frame_size * 32);
   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not enqueue data: %s\n", opensl_get_error_string(result));
   }
   extra->position += extra->frame_size * 32;

   result = (*queue)->RegisterCallback(queue, updateQueue, extra);

   if (result != SL_RESULT_SUCCESS){
      ALLEGRO_ERROR("Could not register callback: %s\n", opensl_get_error_string(result));
   }

   // result = (*volume)->SetVolumeLevel(volume, -300);

   result = (*extra->player)->GetInterface(extra->player, SL_IID_PLAY, &play);
   result = (*play)->SetPlayState(play, SL_PLAYSTATE_PLAYING);

   if (result == SL_RESULT_SUCCESS){
      ALLEGRO_DEBUG("Started new OpenSL stream\n");
   }

   result = (*queue)->GetState(queue, &state);
   if (result == SL_RESULT_SUCCESS){
      ALLEGRO_DEBUG("Buffer queue state count %d index %d\n", state.count, state.playIndex);
   }
   */

   return 0;
}

static int _opensl_stop_voice(ALLEGRO_VOICE* voice)
{
   OpenSLData * data = (OpenSLData*) voice->extra;
   if (data->status == PLAYING){
      data->status = STOPPING;
   }

   while (data->status != STOPPED){
      al_rest(0.001);
   }

   return 0;
}

static bool _opensl_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   OpenSLData * extra = (OpenSLData*) voice->extra;
   return extra->status == PLAYING;
}

static unsigned int _opensl_get_voice_position(const ALLEGRO_VOICE *voice)
{
   /* TODO */
   (void) voice;
   ALLEGRO_ERROR("Unimplemented: _opensl_get_voice_position\n");
   return 0;
}

static int _opensl_set_voice_position(ALLEGRO_VOICE *voice, unsigned int val)
{
   /* TODO */
   (void) voice;
   (void) val;
   ALLEGRO_ERROR("Unimplemented: _opensl_set_voice_position\n");
   return 1;
}

ALLEGRO_AUDIO_DRIVER _al_kcm_opensl_driver = {
   "OpenSL",

   _opensl_open,
   _opensl_close,

   _opensl_allocate_voice,
   _opensl_deallocate_voice,

   _opensl_load_voice,
   _opensl_unload_voice,

   _opensl_start_voice,
   _opensl_stop_voice,

   _opensl_voice_is_playing,

   _opensl_get_voice_position,
   _opensl_set_voice_position,

   NULL,
   NULL,

   NULL,
   NULL,
};
