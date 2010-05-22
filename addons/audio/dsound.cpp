/*
 * Based on DirectSound driver by KC/Milan
 */


#include <stdio.h>
#include <string.h>


#include <windows.h>
#include <dsound.h>


/* I'm not sure what library this is supposed to be in, but I couldn't find it yet */
const IID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

#include "allegro5/allegro.h"

extern "C" {
#include "allegro5/internal/aintern_audio.h"

/* This is used to stop MinGW from complaining about type-punning */
#define MAKE_UNION(ptr, t) \
   union { \
      LPVOID *v; \
      t p; \
   } u; \
   u.p = (ptr);

/* DirectSound vars */
static IDirectSound8 *device;
static HRESULT hr;
static char ds_err_str[100];
const int BUFSIZE = 16*1024; /* FIXME */


static char *ds_get_error(HRESULT hr)
{
   switch (hr) {
      case DSERR_ALLOCATED:
         strcpy(ds_err_str, "DSERR_ALLOCATED");
         break;
      case DSERR_BADFORMAT:
         strcpy(ds_err_str, "DSERR_BADFORMAT");
         break;
      case DSERR_BUFFERTOOSMALL: 
         strcpy(ds_err_str, "DSERR_BUFFERTOOSMALL");
         break;
      case DSERR_CONTROLUNAVAIL:
         strcpy(ds_err_str, "DSERR_CONTROLUNAVAIL");
         break;
      case DSERR_DS8_REQUIRED:
         strcpy(ds_err_str, "DSERR_DS8_REQUIRED");
         break;
      case DSERR_INVALIDCALL:
         strcpy(ds_err_str, "DSERR_INVALIDCALL");
         break;
      case DSERR_INVALIDPARAM:
         strcpy(ds_err_str, "DSERR_INVALIDPARAM");
         break;
      case DSERR_NOAGGREGATION:
         strcpy(ds_err_str, "DSERR_NOAGGREGATION");
         break;
      case DSERR_OUTOFMEMORY:
         strcpy(ds_err_str, "DSERR_OUTOFMEMORY");
         break;
      case DSERR_UNINITIALIZED:
         strcpy(ds_err_str, "DSERR_UNINITIALIZED");
         break;
      case DSERR_UNSUPPORTED:
         strcpy(ds_err_str, "DSERR_UNSUPPORTED");
         break;
   }

   return ds_err_str;
}


/* Custom struct to hold voice information DirectSound needs */
/* TODO: review */
typedef struct ALLEGRO_DS_DATA {
   int bits_per_sample;
   int channels;
   DSBUFFERDESC desc;
   WAVEFORMATEX wave_fmt;
   LPDIRECTSOUNDBUFFER ds_buffer;
   LPDIRECTSOUNDBUFFER8 ds8_buffer;
   bool stop_voice;
   ALLEGRO_THREAD *thread;
} ALLEGRO_DS_DATA;


static bool _dsound_voice_is_playing(const ALLEGRO_VOICE *voice);


/* Custom routine which runs in another thread to periodically check if DirectSound
   wants more data for a stream */
static void* _dsound_update(ALLEGRO_THREAD* self, void* arg)
{
   ALLEGRO_VOICE *voice = (ALLEGRO_VOICE *)arg;
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA*)voice->extra;
   DWORD play_cursor = 0, write_cursor, saved_play_cursor = 0;
   unsigned int samples;
   int d;
   LPVOID ptr1, ptr2;
   DWORD block1_bytes, block2_bytes;
   const void *data;
   const int bytes_per_sample = ex_data->bits_per_sample / 8;

   (void)self;

   /* Fill buffer */
   hr = ex_data->ds8_buffer->Lock(0, BUFSIZE, &ptr1, &block1_bytes, &ptr2, &block2_bytes, DSBLOCK_ENTIREBUFFER);
   if (!FAILED(hr)) {
      samples = BUFSIZE / bytes_per_sample / ex_data->channels;
      data = _al_voice_update(voice, &samples);
      memcpy(ptr1, data, block1_bytes);
      memcpy(ptr2, ((unsigned char *)data)+block1_bytes, block2_bytes);
      ex_data->ds8_buffer->Unlock(ptr1, block1_bytes, ptr2, block2_bytes);
   }

   ex_data->ds8_buffer->Play(0, 0, DSBPLAY_LOOPING);

   do {
      if (!_dsound_voice_is_playing(voice)) {
         ex_data->ds8_buffer->Play(0, 0, DSBPLAY_LOOPING);
      }
      ex_data->ds8_buffer->GetCurrentPosition(&play_cursor, &write_cursor);
      if (play_cursor != saved_play_cursor) {
         d = play_cursor - saved_play_cursor;
         if (d < 0)
            d += BUFSIZE;
         samples = d / bytes_per_sample / ex_data->channels;
         data = _al_voice_update(voice, &samples);
         if (data == NULL) {
            /* FIXME: play silence */
         }
         else {
            hr = ex_data->ds8_buffer->Lock(saved_play_cursor, d, &ptr1, &block1_bytes, &ptr2, &block2_bytes, 0);
            if (!FAILED(hr)) {
               memcpy(ptr1, data, block1_bytes);
               memcpy(ptr2, ((unsigned char *)data)+block1_bytes, block2_bytes);
               hr = ex_data->ds8_buffer->Unlock(ptr1, block1_bytes, ptr2, block2_bytes);
               if (FAILED(hr)) {
                  fprintf(stderr, "Unlock failed: %s\n", ds_get_error(hr));
               }
            }
            else {
            }
            saved_play_cursor += block1_bytes + block2_bytes;
            saved_play_cursor %= BUFSIZE;
         }
      }
      al_rest(0.005);
   } while (!ex_data->stop_voice);


   ex_data->ds8_buffer->Stop();

   return NULL;
}


/* The open method starts up the driver and should lock the device, using the
   previously set paramters, or defaults. It shouldn't need to start sending
   audio data to the device yet, however. */
static int _dsound_open()
{ 
   fprintf(stderr, "Starting DirectSound...\n");

   /* FIXME: Use default device until we have device enumeration */
   hr = DirectSoundCreate8(NULL, &device, NULL);
   if (FAILED(hr)) {
      fprintf(stderr, "DirectSoundCreate8 failed\n");
      return 1;
   }

   /* FIXME: The window specified here is probably very wrong. NULL won't work either. */
   hr = device->SetCooperativeLevel(GetForegroundWindow(), DSSCL_PRIORITY);
   if (FAILED(hr)) {
      fprintf(stderr, "SetCooperativeLevel failed\n");
      return 1;
   }

   return 0;
}


/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void _dsound_close()
{
   device->Release();
}


/* The allocate_voice method should grab a voice from the system, and allocate
   any data common to streaming and non-streaming sources. */
static int _dsound_allocate_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_DS_DATA *ex_data;
   int bits_per_sample;
   int channels;

   /* openal doesn't support very much! */
   switch (voice->depth)
   {
      case ALLEGRO_AUDIO_DEPTH_UINT8:
         /* format supported */
         bits_per_sample = 8;
         break;
      case ALLEGRO_AUDIO_DEPTH_INT8:
         fprintf(stderr, "DirectSound requires 8-bit data to be unsigned\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_UINT16:
         fprintf(stderr, "DirectSound requires 16-bit data to be signed\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_INT16:
         /* format supported */
         bits_per_sample = 16;
         break;
      case ALLEGRO_AUDIO_DEPTH_UINT24:  
         fprintf(stderr, "DirectSound does not support 24-bit data\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_INT24:  
         fprintf(stderr, "DirectSound does not support 24-bit data\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         fprintf(stderr, "DirectSound does not support 32-bit floating data\n");
         return 1;
      default:
         fprintf(stderr, "Cannot allocate unknown voice depth\n");
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

   ex_data = (ALLEGRO_DS_DATA *)al_calloc(1, sizeof(*ex_data));
   if (!ex_data) {
      fprintf(stderr, "Could not allocate voice data memory\n"); 
      return 1;
   }

   ex_data->bits_per_sample = bits_per_sample;
   ex_data->channels = channels;
   ex_data->stop_voice = 1;

   voice->extra = ex_data;

   return 0;
}

/* The deallocate_voice method should free the resources for the given voice,
   but still retain a hold on the device. The voice should be stopped and
   unloaded by the time this is called */
static void _dsound_deallocate_voice(ALLEGRO_VOICE *voice)
{
   al_free(voice->extra);
   voice->extra = NULL;
}

/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached sample's looping mode should be honored, and loading
   must fail if it cannot be. */
static int _dsound_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA *)voice->extra;
   LPVOID ptr1, ptr2;
   DWORD block1_bytes, block2_bytes;
   MAKE_UNION(&ex_data->ds8_buffer, LPDIRECTSOUNDBUFFER8 *);

   ex_data->wave_fmt.wFormatTag = WAVE_FORMAT_PCM;
   ex_data->wave_fmt.nChannels = ex_data->channels;
   ex_data->wave_fmt.nSamplesPerSec = voice->frequency;
   ex_data->wave_fmt.nBlockAlign = ex_data->channels * (ex_data->bits_per_sample/8);
   ex_data->wave_fmt.nAvgBytesPerSec = ex_data->wave_fmt.nBlockAlign * voice->frequency;
   ex_data->wave_fmt.wBitsPerSample = ex_data->bits_per_sample;
   ex_data->wave_fmt.cbSize = 0;

   ex_data->desc.dwSize = sizeof(DSBUFFERDESC);
   ex_data->desc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS; /* FIXME: software mixing for now */
   ex_data->desc.dwBufferBytes = voice->buffer_size;
   ex_data->desc.dwReserved = 0;
   ex_data->desc.lpwfxFormat = &ex_data->wave_fmt;
   ex_data->desc.guid3DAlgorithm = DS3DALG_DEFAULT;

   hr = device->CreateSoundBuffer(&ex_data->desc, &ex_data->ds_buffer, NULL);
   if (FAILED(hr)) {
      al_free(ex_data);
      return 1;
   }

   ex_data->ds_buffer->QueryInterface(IID_IDirectSoundBuffer8, u.v);

   hr = ex_data->ds8_buffer->Lock(0, voice->buffer_size, &ptr1, &block1_bytes, &ptr2, &block2_bytes,
      0);
   if (FAILED(hr))
      return 1;

   memcpy(ptr1, data, block1_bytes);
   memcpy(ptr2, ((unsigned char *)data)+block1_bytes, block2_bytes);

   ex_data->ds8_buffer->Unlock(ptr1, block1_bytes, ptr2, block2_bytes);

   return 0;
}

/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void _dsound_unload_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA *)voice->extra;
   ex_data->ds8_buffer->Release();
}


/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int _dsound_start_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA *)voice->extra;
   MAKE_UNION(&ex_data->ds8_buffer, LPDIRECTSOUNDBUFFER8 *);

   if (!voice->is_streaming) {
      ex_data->ds8_buffer->SetCurrentPosition(0);
      hr = ex_data->ds8_buffer->Play(0, 0, 0);
      if (FAILED(hr))
         return 1;
      return 0;
   }

   if (ex_data->stop_voice != 0) {
      ex_data->wave_fmt.wFormatTag = WAVE_FORMAT_PCM;
      ex_data->wave_fmt.nChannels = ex_data->channels;
      ex_data->wave_fmt.nSamplesPerSec = voice->frequency;
      ex_data->wave_fmt.nBlockAlign = ex_data->channels * (ex_data->bits_per_sample/8);
      ex_data->wave_fmt.nAvgBytesPerSec = ex_data->wave_fmt.nBlockAlign * voice->frequency;
      ex_data->wave_fmt.wBitsPerSample = ex_data->bits_per_sample;
      ex_data->wave_fmt.cbSize = 0;

      ex_data->desc.dwSize = sizeof(DSBUFFERDESC);
      ex_data->desc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS; /* FIXME: software mixing for now */
      ex_data->desc.dwBufferBytes = BUFSIZE;
      ex_data->desc.dwReserved = 0;
      ex_data->desc.lpwfxFormat = &ex_data->wave_fmt;
      ex_data->desc.guid3DAlgorithm = DS3DALG_DEFAULT;

      hr = device->CreateSoundBuffer(&ex_data->desc, &ex_data->ds_buffer, NULL);
      if (FAILED(hr)) {
         fprintf(stderr, "CreateSoundBuffer failed: %s\n", ds_get_error(hr));
         al_free(ex_data);
         return 1;
      }

      ex_data->ds_buffer->QueryInterface(IID_IDirectSoundBuffer8, u.v);

      ex_data->ds8_buffer->SetVolume(DSBVOLUME_MAX);

      ex_data->stop_voice = 0;
      ex_data->thread = al_create_thread(_dsound_update, (void*) voice);
      al_start_thread(ex_data->thread);
   }

   return 0;
}

/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int _dsound_stop_voice(ALLEGRO_VOICE* voice)
{
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA *)voice->extra;

   if (!ex_data->ds8_buffer) {
      fprintf(stderr, "Trying to stop empty voice buffer\n"); 
      return 1;
   }

   /* if playing a sample */
   if (!voice->is_streaming) {
      ex_data->ds8_buffer->Stop();
      return 0;
   }

   if (ex_data->stop_voice == 0) {
      ex_data->stop_voice = 1;
      al_join_thread(ex_data->thread, NULL);
      al_destroy_thread(ex_data->thread);
   }

   ex_data->ds8_buffer->Release();
   ex_data->ds8_buffer = NULL;

   return 0;
}

/* The voice_is_playing method should only be called on non-streaming sources,
   and should return true if the voice is playing */
static bool _dsound_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA *)voice->extra;
   DWORD status;

   if (!ex_data)
      return false;

   ex_data->ds8_buffer->GetStatus(&status);

   return (status & DSBSTATUS_PLAYING);
}

/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned int _dsound_get_voice_position(const ALLEGRO_VOICE *voice)
{
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA *)voice->extra;
   DWORD play_pos;

   hr = ex_data->ds8_buffer->GetCurrentPosition(&play_pos, NULL);
   if (FAILED(hr))
      return 0;

   return play_pos / (ex_data->channels / (ex_data->bits_per_sample/8));
}

/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int _dsound_set_voice_position(ALLEGRO_VOICE *voice, unsigned int val)
{
   ALLEGRO_DS_DATA *ex_data = (ALLEGRO_DS_DATA *)voice->extra;

   val *= ex_data->channels * (ex_data->bits_per_sample/8);

   hr = ex_data->ds8_buffer->SetCurrentPosition(val);
   if (FAILED(hr))
      return 1;

   return 0;
}

ALLEGRO_AUDIO_DRIVER _al_kcm_dsound_driver = {
   "DirectSound",

   _dsound_open,
   _dsound_close,

   _dsound_allocate_voice,
   _dsound_deallocate_voice,

   _dsound_load_voice,
   _dsound_unload_voice,

   _dsound_start_voice,
   _dsound_stop_voice,

   _dsound_voice_is_playing,

   _dsound_get_voice_position,
   _dsound_set_voice_position,
};

} /* End extern "C" */

