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
 *      DirectSound windows sound driver
 *
 *      By Stefan Schimanski.
 *
 *      Various bugfixes by Patrick Hogan.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTSOUND_VERSION 0x0300

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"

#ifndef SCAN_DEPEND
   #include <mmsystem.h>
   #include <dsound.h>
   #include <math.h>

   #ifdef _MSC_VER
      #include <mmreg.h>
   #endif
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


static int digi_directsound_detect(int input);
static int digi_directsound_init(int input, int voices);
static void digi_directsound_exit(int input);
static int digi_directsound_mixer_volume(int volume);

static void digi_directsound_init_voice(int voice, AL_CONST SAMPLE * sample);
static void digi_directsound_release_voice(int voice);
static void digi_directsound_start_voice(int voice);
static void digi_directsound_stop_voice(int voice);
static void digi_directsound_loop_voice(int voice, int playmode);

static void *digi_directsound_lock_voice(int voice, int start, int end);
static void digi_directsound_unlock_voice(int voice);

static int digi_directsound_get_position(int voice);
static void digi_directsound_set_position(int voice, int position);

static int digi_directsound_get_volume(int voice);
static void digi_directsound_set_volume(int voice, int volume);

static int digi_directsound_get_frequency(int voice);
static void digi_directsound_set_frequency(int voice, int frequency);

static int digi_directsound_get_pan(int voice);
static void digi_directsound_set_pan(int voice, int pan);


/* template driver: will be cloned for each hardware device */
static DIGI_DRIVER digi_directx = 
{
   0,
   empty_string,
   empty_string,
   empty_string,
   0,                           /* available voices */
   0,                           /* voice number offset */
   16,                          /* maximum voices we can support */
   4,                           /* default number of voices to use */

   /* setup routines */
   digi_directsound_detect,
   digi_directsound_init,
   digi_directsound_exit,
   digi_directsound_mixer_volume,

   /* audiostream locking functions */
   digi_directsound_lock_voice,
   digi_directsound_unlock_voice,
   NULL,

   /* voice control functions */
   digi_directsound_init_voice,
   digi_directsound_release_voice,
   digi_directsound_start_voice,
   digi_directsound_stop_voice,
   digi_directsound_loop_voice,

   /* position control functions */
   digi_directsound_get_position,
   digi_directsound_set_position,

   /* volume control functions */
   digi_directsound_get_volume,
   digi_directsound_set_volume,
   NULL, NULL,

   /* pitch control functions */
   digi_directsound_get_frequency,
   digi_directsound_set_frequency,
   NULL, NULL,

   /* pan control functions */
   digi_directsound_get_pan,
   digi_directsound_set_pan,
   NULL, NULL,

   /* effect control functions */
   NULL, NULL, NULL,

   /* input functions */
   0,                           /* int rec_cap_bits; */
   0,                           /* int rec_cap_stereo; */
   NULL,                        /* AL_METHOD(int,  rec_cap_rate, (int bits, int stereo)); */
   NULL,                        /* AL_METHOD(int,  rec_cap_parm, (int rate, int bits, int stereo)); */
   NULL,                        /* AL_METHOD(int,  rec_source, (int source)); */
   NULL,                        /* AL_METHOD(int,  rec_start, (int rate, int bits, int stereo)); */
   NULL,                        /* AL_METHOD(void, rec_stop, (void)); */
   NULL                         /* AL_METHOD(int,  rec_read, (void *buf)); */
};



/* sound driver globals */
static LPDIRECTSOUND directsound = NULL;
static LPDIRECTSOUNDBUFFER prim_buf = NULL;
static int _freq, _bits, _stereo;
static unsigned char allegro_to_decibel[256];


/* internal driver representation of a voice */
struct DIRECTSOUND_VOICE {
   int bits;
   int bytes_per_sample;
   int freq, pan, vol;
   int dfreq, dpan, dvol;
   int target_freq, target_pan, target_vol;
   int looping;

   void *lock_buf_a;
   void *lock_buf_b;
   int lock_size_a, lock_size_b;
   int lock_bytes;

   LPDIRECTSOUNDBUFFER ds_buffer;
} DIRECTSOUND_VOICE;

static struct DIRECTSOUND_VOICE *ds_voices;


/* dynamically generated driver list */
static _DRIVER_INFO *driver_list = NULL;

#define MAX_DRIVERS  16

static int num_drivers = 0;

static char *driver_names[MAX_DRIVERS];
static LPGUID driver_guids[MAX_DRIVERS];



/* DSEnumCallback:
 *  DirectSound callback for enumaterating the available sound drivers.
 */
static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
   if (num_drivers < MAX_DRIVERS) {
      if (lpGuid) {
	 driver_guids[num_drivers] = malloc(sizeof(GUID));
	 memcpy(driver_guids[num_drivers], lpGuid, sizeof(GUID));
      }
      else
	 driver_guids[num_drivers] = NULL;

      driver_names[num_drivers] = malloc(strlen(lpcstrDescription)+1);
      strcpy(driver_names[num_drivers], lpcstrDescription);
      num_drivers++;
   }

   return (num_drivers < MAX_DRIVERS);
}

/* Function from wdsndmix.c to get a driver */
DIGI_DRIVER * _get_dsalmix_driver (char *, int);

/* Function from wsndwo.c to get a driver */
DIGI_DRIVER *_get_woalmix_driver (int);

/* _get_digi_driver_list:
 *  System driver hook for listing the available sound drivers. This 
 *  generates the device list at runtime, to match whatever DirectSound
 *  devices are available.
 */
_DRIVER_INFO *_get_digi_driver_list()
{
   DIGI_DRIVER *driver;
   int i;

   if (!driver_list) {
      DirectSoundEnumerate(DSEnumCallback, NULL);

/* This function has to allocate drivers for wdsndmix.c as well */
/* and also, wsndwo.c ASSUMES 2 DRIVERS */
      driver_list = malloc(sizeof(_DRIVER_INFO) * ((num_drivers*2)+4));

/* wdsound.c drivers */
      for (i=0; i<num_drivers; i++) {
	 driver = malloc(sizeof(DIGI_DRIVER));
	 memcpy(driver, &digi_directx, sizeof(DIGI_DRIVER));

	 driver->id = DIGI_DIRECTX(i);

	 driver->ascii_name = driver_names[i];

	 driver_list[i].id = driver->id;
	 driver_list[i].driver = driver;
	 driver_list[i].autodetect = TRUE;
      }
 
/* wdsndmix.c drivers */
      for (i=0; i<num_drivers; i++) {
         driver = _get_dsalmix_driver (driver_names[i], i);
 
         driver_list[i+num_drivers].id = driver->id;
         driver_list[i+num_drivers].driver = driver;
         driver_list[i+num_drivers].autodetect = TRUE;
      }

      driver = _get_woalmix_driver (0);
      driver_list[i+num_drivers].id = driver->id;
      driver_list[i+num_drivers].driver = driver;
      driver_list[i+num_drivers].autodetect = TRUE;
	i++;
      driver = _get_woalmix_driver (1);
      driver_list[i+num_drivers].id = driver->id;
      driver_list[i+num_drivers].driver = driver;
      driver_list[i+num_drivers].autodetect = TRUE;
	i++;

      driver_list[i+num_drivers].id = DIGI_NONE;
      driver_list[i+num_drivers].driver = &digi_none;
      driver_list[i+num_drivers].autodetect = TRUE;
      i++;
 
      driver_list[i+num_drivers].id = 0;
      driver_list[i+num_drivers].driver = NULL;
      driver_list[i+num_drivers].autodetect = FALSE;
   }

   return driver_list;
}



/* ds_err:
 *  returns error string for DirectSound error.
 */
char *ds_err(HRESULT err)
{
   char *s;

   switch (err) {
      case DS_OK:
	 s = "DS_OK";
	 break;
      case DSERR_ALLOCATED:
	 s = "DSERR_ALLOCATED";
	 break;
      case DSERR_BADFORMAT:
	 s = "DSERR_BADFORMAT";
	 break;
      case DSERR_INVALIDPARAM:
	 s = "DSERR_INVALIDPARAM";
	 break;
      case DSERR_NOAGGREGATION:
	 s = "DSERR_NOAGGREGATION";
	 break;
      case DSERR_OUTOFMEMORY:
	 s = "DSERR_OUTOFMEMORY";
	 break;
      case DSERR_UNINITIALIZED:
	 s = "DSERR_UNINITIALIZED";
	 break;
      case DSERR_UNSUPPORTED:
	 s = "DSERR_UNSUPPORTED";
	 break;
      default:
	 s = "DSERR_UNKNOWN";
   }

   return s;
}



/* digi_directsound_init:
 *  Initialize driver                                                                 *
 */
static int digi_directsound_init(int input, int voices)
{
   HRESULT hr;
   DSCAPS dscaps;
   DSBUFFERDESC desc;
   WAVEFORMATEX format;
   int v, id;

   digi_driver->voices = voices;

   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   /* initialize DirectSound interface */
   hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
   if (FAILED(hr)) { 
      _TRACE("Can't create DirectSound interface (%x)\n", hr); 
      goto Error; 
   }

   /* set cooperative level */
   hr = IDirectSound_SetCooperativeLevel(directsound, allegro_wnd, DSSCL_PRIORITY);
   if (FAILED(hr))
      _TRACE("Can't set DirectSound cooperative level (%x)\n", hr);
   else
      _TRACE("DirectSound cooperation level set to PRIORITY\n");

   /* get hardware capabilities */
   dscaps.dwSize = sizeof(DSCAPS);
   hr = IDirectSound_GetCaps(directsound, &dscaps);
   if (FAILED(hr)) { 
      _TRACE("Can't get DirectSound caps (%x)\n", hr); 
      goto Error; 
   }

   if (dscaps.dwFlags & DSCAPS_PRIMARY16BIT)
      _bits = 16;
   else
      _bits = 8;

   if (dscaps.dwFlags & DSCAPS_PRIMARYSTEREO)
      _stereo = 1;
   else
      _stereo = 0;

   if (dscaps.dwMaxSecondarySampleRate > 44000)
      _freq = 44100;
   else
      _freq = 22050;

   _TRACE("DirectSound caps: %u bits, %s, %uHz\n", _bits, _stereo ? "stereo" : "mono", _freq);

   /* create primary buffer */
   ZeroMemory(&desc, sizeof(DSBUFFERDESC));
   desc.dwSize = sizeof(DSBUFFERDESC);
   desc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;

   hr = IDirectSound_CreateSoundBuffer(directsound, &desc, &prim_buf, NULL);
   if (hr != DS_OK) { 
      _TRACE("Can't create primary buffer\nGlobal volume control won't be available\n"); 
   }

   /* get current format */
   if (prim_buf) {
      hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
      if (FAILED(hr)) {
	 _TRACE("Can't get primary buffer format\n");
      }
      else {
	 format.nChannels = _stereo ? 2 : 1;
	 format.nSamplesPerSec = _freq;
	 format.wBitsPerSample = _bits;
	 format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
	 format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	 hr = IDirectSoundBuffer_SetFormat(prim_buf, &format);
	 if (FAILED(hr)) {
	    _TRACE("Can't set primary buffer format\n");
	 }

	 /* output primary format */
	 hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
	 if (FAILED(hr)) {
	    _TRACE("Can't get primary buffer format\n");
	 }
	 else {
	    _TRACE("primary format:\n");
	    _TRACE("  %u channels\n  %u Hz\n  %u AvgBytesPerSec\n  %u BlockAlign\n  %u bits\n  %u size\n",
		  format.nChannels, format.nSamplesPerSec, format.nAvgBytesPerSec,
		  format.nBlockAlign, format.wBitsPerSample, format.cbSize);
	 }
      }
   }

   /* initialize physical voices */
   ds_voices = (struct DIRECTSOUND_VOICE *)malloc(sizeof(struct DIRECTSOUND_VOICE) * voices);
   for (v = 0; v < digi_driver->voices; v++)
      ds_voices[v].ds_buffer = NULL;

   /* setup allegro volume to decibel translation table */
   allegro_to_decibel[0] = 0;
   for (v = 1; v < 256; v++)
      allegro_to_decibel[v] = (unsigned char)(106.0 * log10(v));       /* 255 / log10(255) ~ 106 */

   return 0;

 Error:
   _TRACE("digi_directsound_init failed\n");
   digi_directsound_exit(0);
   return -1;
}



/* digi_directsound_exit:
 *  Closes DirectX
 */
static void digi_directsound_exit(int input)
{
   int v;

   /* destroy physical voices */
   for (v = 0; v < digi_driver->voices; v++) {
      digi_directsound_release_voice(v);
   }

   free(ds_voices);
   ds_voices = NULL;

   /* destroy primary buffer */
   if (prim_buf) {
      IDirectSoundBuffer_Release(prim_buf);
      prim_buf = NULL;
   }

   /* shutdown DirectSound */
   if (directsound) {
      IDirectSound_Release(directsound);
      directsound = NULL;
   }
}



/* digi_directsound_mixer_volume:
 *  Sets mixer volume
 */
static int digi_directsound_mixer_volume(int volume)
{
   int ds_vol;

   if (prim_buf) {
      volume = allegro_to_decibel[MID(0, volume, 255)];
      ds_vol = DSBVOLUME_MIN + volume * (DSBVOLUME_MAX - DSBVOLUME_MIN) / 255;

      IDirectSoundBuffer_SetVolume(prim_buf, ds_vol); 
   }

   return 0;
}



/* digi_directsound_detect:
 *  Detects DirectSound.
 */
static int digi_directsound_detect(int input)
{
   HRESULT hr;
   int id;

   if (input)
      return 0;

   if (directsound == NULL) {
      /* deduce our device number from the driver ID code */
      id = ((digi_driver->id >> 8) & 0xFF) - 'A';

      /* initialize DirectSound interface */
      hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
      if (FAILED(hr)) {
	 _TRACE("DirectSound interface creation failed during detect.\n");
	 return 0;
      }

      _TRACE("DirectSound interface successfully created.\n");

      /* release DirectSound */
      IDirectSound_Release(directsound);
      directsound = NULL;
   }

   return 1;
}


/********************************************************/
static void digi_directsound_init_voice(int voice, AL_CONST SAMPLE * sample)
{
   PCMWAVEFORMAT pcmwf;
   DSBUFFERDESC dsbdesc;
   HRESULT hr;
   LPVOID buf_a;
   DWORD size_a;
   LPVOID buf_b;
   DWORD size_b;
   LPDIRECTSOUNDBUFFER snd_buf;
   DWORD pos;

   ds_voices[voice].bits = sample->bits;
   ds_voices[voice].bytes_per_sample = (sample->bits/8) * (sample->stereo ? 2 : 1);
   ds_voices[voice].vol = 255;
   ds_voices[voice].pan = 128;
   ds_voices[voice].freq = sample->freq;
   ds_voices[voice].dfreq = 0;
   ds_voices[voice].dpan = 0;
   ds_voices[voice].dvol = 0;
   ds_voices[voice].looping = 0;

   /* set up wave format structure */
   memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
   pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
   pcmwf.wf.nChannels = sample->stereo ? 2 : 1;
   pcmwf.wf.nSamplesPerSec = sample->freq;
   pcmwf.wBitsPerSample = sample->bits;
   pcmwf.wf.nBlockAlign = sample->bits * (sample->stereo ? 2 : 1) / 8;
   pcmwf.wf.nAvgBytesPerSec =
       pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;


   /* setup DSBUFFERDESC structure */
   memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));   // Zero it out. 

   dsbdesc.dwSize = sizeof(DSBUFFERDESC);

   /* need default controls (pan, volume, frequency) */
   dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;

   dsbdesc.dwBufferBytes = sample->len * (sample->bits / 8) * (sample->stereo ? 2 : 1);
   dsbdesc.lpwfxFormat = (LPWAVEFORMATEX) & pcmwf;

   /* create buffer */
   hr = IDirectSound_CreateSoundBuffer(directsound, &dsbdesc, &snd_buf, NULL);
   if (FAILED(hr)) {
      _TRACE("digi_directsound_init_voice failed (%x)\n", hr);
      _TRACE(" - %d Hz, %s, %d bits\n", sample->freq, sample->stereo ? "stereo" : "mono", sample->bits);
      return;
   }

   ds_voices[voice].ds_buffer = snd_buf;

   /* copy sample contents to buffer */
   hr = IDirectSoundBuffer_Lock(snd_buf, 0,
	       sample->len * (sample->bits / 8) * (sample->stereo ? 2 : 1),
	       &buf_a, &size_a, &buf_b, &size_b, 0);

   /* if DSERR_BUFFERLOST is returned, restore and retry lock */
   if (hr == DSERR_BUFFERLOST) {
      _TRACE("Locking sound buffer failed (DSERR_BUFFERLOST)\n");
      IDirectSoundBuffer_Restore(snd_buf);
      hr = IDirectSoundBuffer_Lock(snd_buf, 0,
	       sample->len * (sample->bits / 8) * (sample->stereo ? 2 : 1),
	       &buf_a, &size_a, &buf_b, &size_b, 0);
   }

   if (FAILED(hr)) {
      _TRACE("digi_directsound_init_voice failed. Can't lock sound buffer.\n");
      digi_directsound_release_voice(voice);
      return;
   }

   /* convert 16-bit samples from Allegro format to DS */
   if ((sample->bits == 16) && ((int) sample->param != -2)) {
      for (pos = 0; pos < size_a / 2; pos++)
	 ((unsigned short *)buf_a)[pos] = ((unsigned short *)(sample->data))[pos] - 32768;

      if (buf_b) {
	 for (pos = 0; pos < size_b / 2; pos++)
	    ((unsigned short *)buf_b)[pos] = ((unsigned short *)(sample->data))[pos + size_a / 2] - 32768;
      }
   }
   else {
      CopyMemory(buf_a, sample->data, size_a);
      if (buf_b != NULL)
	 CopyMemory(buf_b, (char *)(sample->data) + size_a, size_b);
   }

   /* release the data back to DirectSound */
   hr = IDirectSoundBuffer_Unlock(snd_buf, buf_a, size_a, buf_b, size_b);
   if (hr != DS_OK) {
      _TRACE("digi_directsound_init_voice failed. Can't unlock\n");
   }
}



/* digi_directsound_release_voice:
 */
static void digi_directsound_release_voice(int voice)
{
   if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Release(ds_voices[voice].ds_buffer);
      ds_voices[voice].ds_buffer = NULL;
   }
}



/* digi_directsound_start_voice:
 */
static void digi_directsound_start_voice(int voice)
{
   if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Play(ds_voices[voice].ds_buffer,
		   0, 0, ds_voices[voice].looping ? DSBPLAY_LOOPING : 0);
   }
}



/* digi_directsound_stop_voice:
 */
static void digi_directsound_stop_voice(int voice)
{
   if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Stop(ds_voices[voice].ds_buffer);
   }
}



/* digi_directsound_loop_voice:
 */
static void digi_directsound_loop_voice(int voice, int playmode)
{
   switch (playmode) {
      case PLAYMODE_LOOP:
	 ds_voices[voice].looping = 1;
	 break;

      case PLAYMODE_PLAY:
	 ds_voices[voice].looping = 0;
	 break;
   }
}



/* digi_directsound_lock_voice:
 */
static void *digi_directsound_lock_voice(int voice, int start, int end)
{
   void *buf_a;
   void *buf_b;
   long int size_a, size_b;
   HRESULT hr;

   start = start * ds_voices[voice].bytes_per_sample;
   end = end * ds_voices[voice].bytes_per_sample;

   hr = IDirectSoundBuffer_Lock(ds_voices[voice].ds_buffer,
		start, end - start, &buf_a, &size_a, &buf_b, &size_b, 0);
   if (FAILED(hr)) {
      _TRACE("digi_directsound_lock_voice failed (%s)", ds_err(hr));
      return NULL;
   }

   ds_voices[voice].lock_buf_a = buf_a;
   ds_voices[voice].lock_size_a = size_a;
   ds_voices[voice].lock_buf_b = buf_b;
   ds_voices[voice].lock_size_b = size_b;
   ds_voices[voice].lock_bytes = end - start;

   if (buf_b != NULL) {
      digi_directsound_unlock_voice(voice);
      return NULL;
   }

   return buf_a;
}



/* digi_directsound_unlock_voice:
 */
static void digi_directsound_unlock_voice(int voice)
{
   HRESULT hr;

   if (ds_voices[voice].ds_buffer && ds_voices[voice].lock_buf_a) {
      if (ds_voices[voice].bits == 16) {
	 int pos;

	 for (pos=0; pos < ds_voices[voice].lock_bytes/2; pos++)
	     ((unsigned short *)ds_voices[voice].lock_buf_a)[pos] ^= 0x8000;
      }

      hr = IDirectSoundBuffer_Unlock(ds_voices[voice].ds_buffer,
				     ds_voices[voice].lock_buf_a,
				     ds_voices[voice].lock_size_a,
				     ds_voices[voice].lock_buf_b,
				     ds_voices[voice].lock_size_b);

      if (FAILED(hr)) {
	 _TRACE("digi_directsound_unlock_voice failed (%s)", ds_err(hr));
      }
   }
}



/* digi_directsound_get_position:
 */
static int digi_directsound_get_position(int voice)
{
   HRESULT hr;
   long int play_cursor;
   long int write_cursor;
   long int status;

   if (ds_voices[voice].ds_buffer) {
      /* is buffer playing */
      hr = IDirectSoundBuffer_GetStatus(ds_voices[voice].ds_buffer,  &status);
      if (FAILED(hr))
	 return -1;

      if ((status & DSBSTATUS_PLAYING) == 0)
	 return -1;

      hr = IDirectSoundBuffer_GetCurrentPosition(ds_voices[voice].ds_buffer,
					    &play_cursor, &write_cursor);
      if (FAILED(hr)) {
	 _TRACE("digi_directsound_get_position failed (%s)\n", ds_err(hr));
	 return -1;
      }

      return play_cursor / ds_voices[voice].bytes_per_sample;
   }
   else
      return -1;
}



/* digi_directsound_set_position:
 */
static void digi_directsound_set_position(int voice, int position)
{
   HRESULT hr;

   if (ds_voices[voice].ds_buffer) {
      hr = IDirectSoundBuffer_SetCurrentPosition(ds_voices[voice].ds_buffer,
						 position * ds_voices[voice].bytes_per_sample);
      if (FAILED(hr)) {
	 _TRACE("digi_directsound_set_position failed (%s)\n", ds_err(hr));
      }
   }
}



/* digi_directsound_get_volume:
 */
static int digi_directsound_get_volume(int voice)
{
   return ds_voices[voice].vol;
}



/* digi_directsound_set_volume:
 */
static void digi_directsound_set_volume(int voice, int volume)
{
   int ds_vol;

   ds_voices[voice].vol = volume;

   if (ds_voices[voice].ds_buffer) {
      volume = allegro_to_decibel[MID(0, volume, 255)];
      ds_vol = DSBVOLUME_MIN + volume * (DSBVOLUME_MAX - DSBVOLUME_MIN) / 255;

      IDirectSoundBuffer_SetVolume(ds_voices[voice].ds_buffer, ds_vol);
   }
}



/* digi_directsound_get_frequency:
 */
static int digi_directsound_get_frequency(int voice)
{
   return ds_voices[voice].freq;
}



/* digi_directsound_set_frequency:
 */
static void digi_directsound_set_frequency(int voice, int frequency)
{
   ds_voices[voice].freq = frequency;

   if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_SetFrequency(ds_voices[voice].ds_buffer, frequency);
   }
}



/* digi_directsound_get_pan:
 */
static int digi_directsound_get_pan(int voice)
{
   return ds_voices[voice].pan;
}



/* digi_directsound_set_pan:
 */
static void digi_directsound_set_pan(int voice, int pan)
{
   ds_voices[voice].pan = pan;

   if (ds_voices[voice].ds_buffer) {
      int ds_pan = DSBPAN_LEFT + pan * (DSBPAN_RIGHT - DSBPAN_LEFT) / 255;

      IDirectSoundBuffer_SetPan(ds_voices[voice].ds_buffer, ds_pan);
   }
}
