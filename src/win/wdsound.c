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
 *      Custom loop points support added by Eric Botcazou.
 *
 *      Input code by Nick Kochakian.
 *
 *      API compliance improvements, enhanced input format detection
 *      and bugfixes by Javier Gonzalez.
 *
 *      See readme.txt for copyright information.
 */

/* This file requires the DirectX 5 SDK to compile because
 * the DirectSoundCapture interface is not part of DirectX 3.
 * But the playback code is fully DirectX 3 compatible.
 */
#define DIRECTSOUND_VERSION 0x0300

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef __MINGW32__
      #undef MAKEFOURCC
   #endif

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

static int digi_directsound_rec_cap_rate(int bits, int stereo);
static int digi_directsound_rec_cap_param(int rate, int bits, int stereo);
static int digi_directsound_rec_source(int source);
static int digi_directsound_rec_start(int rate, int bits, int stereo);
static void digi_directsound_rec_stop(void);
static int digi_directsound_rec_read(void *buf);

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
   digi_directsound_rec_cap_rate,
   digi_directsound_rec_cap_param,
   digi_directsound_rec_source,
   digi_directsound_rec_start,
   digi_directsound_rec_stop,
   digi_directsound_rec_read
};


/* sound driver globals */

/* input */
static LPDIRECTSOUNDCAPTURE ds_capture = NULL;
static LPDIRECTSOUNDCAPTUREBUFFER ds_capture_buf = NULL;
static DSCBUFFERDESC dscBufDesc;
static WAVEFORMATEX dsc_buf_wfx;
static unsigned long int ds_capture_buffer_size;
static unsigned long int last_capture_pos, input_wave_bytes_read;
static unsigned char *input_wave_data = NULL;

/* output */
static LPDIRECTSOUND directsound = NULL;
static LPDIRECTSOUNDBUFFER prim_buf = NULL;

/* misc */
static long int initial_volume;
static int _freq, _bits, _stereo;
static unsigned char allegro_to_decibel[256];


/* internal driver representation of a voice */
struct DIRECTSOUND_VOICE {
   int bits;
   int bytes_per_sample;
   int freq, pan, vol;
   int dfreq, dpan, dvol;
   int target_freq, target_pan, target_vol;
   int loop_offset;
   int loop_len;
   int looping;

   void *lock_buf_a;
   void *lock_buf_b;
   int lock_size_a, lock_size_b;
   int lock_bytes;

   LPDIRECTSOUNDBUFFER ds_buffer;
   LPDIRECTSOUNDBUFFER ds_loop_buffer;
} DIRECTSOUND_VOICE;

static struct DIRECTSOUND_VOICE *ds_voices;


/* dynamically generated driver list */
static _DRIVER_INFO *driver_list = NULL;

#define MAX_DRIVERS  16

static int num_drivers = 0;

static char *driver_names[MAX_DRIVERS];
static LPGUID driver_guids[MAX_DRIVERS];



/* DSEnumCallback:
 *  DirectSound callback for enumaterating the available sound drivers
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

/* function from wdsndmix.c to get a driver */
DIGI_DRIVER * _get_dsalmix_driver (char *, int);

/* function from wsndwo.c to get a driver */
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

      /* This function has to allocate drivers for wdsndmix.c as well
       * and also, wsndwo.c ASSUMES 2 DRIVERS
       */
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
         driver = _get_dsalmix_driver(driver_names[i], i);

         driver_list[i+num_drivers].id = driver->id;
         driver_list[i+num_drivers].driver = driver;
         driver_list[i+num_drivers].autodetect = TRUE;
      }

      driver = _get_woalmix_driver(0);
      driver_list[i+num_drivers].id = driver->id;
      driver_list[i+num_drivers].driver = driver;
      driver_list[i+num_drivers].autodetect = TRUE;
      i++;

      driver = _get_woalmix_driver(1);
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
 *  returns an error string
 */
#ifdef DEBUGMODE
char *ds_err(long err)
{
   static char err_str[32];

   switch (err) {

      case DS_OK:
         strcpy(err_str, "DS_OK");
         break;

      case DSERR_ALLOCATED:
         strcpy(err_str, "DSERR_ALLOCATED");
         break;

      case DSERR_BADFORMAT:
         strcpy(err_str, "DSERR_BADFORMAT");
         break;

      case DSERR_INVALIDPARAM:
         strcpy(err_str, "DSERR_INVALIDPARAM");
         break;

      case DSERR_NOAGGREGATION:
         strcpy(err_str, "DSERR_NOAGGREGATION");
         break;

      case DSERR_OUTOFMEMORY:
         strcpy(err_str, "DSERR_OUTOFMEMORY");
         break;

      case DSERR_UNINITIALIZED:
         strcpy(err_str, "DSERR_UNINITIALIZED");
         break;

      case DSERR_UNSUPPORTED:
         strcpy(err_str, "DSERR_UNSUPPORTED");
         break;

      default:
         strcpy(err_str, "DSERR_UNKNOWN");
         break;
   }

   return err_str;
}
#else
#define ds_err(hr) "\0"
#endif



/* create_test_capture_buffer:
 *  an internal function that tries to create a capture buffer with
 *  the specified format and deletes it immediatly.
 */
static int create_test_capture_buffer(WAVEFORMATEX *wfx)
{
   LPDIRECTSOUNDCAPTUREBUFFER dsc_trybuf;
   DSCBUFFERDESC dsc_trybuf_desc;
   HRESULT hr;

   /* create the capture buffer */
   dsc_trybuf_desc.dwSize  = sizeof(DSCBUFFERDESC);
   dsc_trybuf_desc.dwFlags = 0;
   dsc_trybuf_desc.dwBufferBytes = 1024;
   dsc_trybuf_desc.dwReserved  = 0;
   dsc_trybuf_desc.lpwfxFormat = wfx;

   hr = IDirectSoundCapture_CreateCaptureBuffer(ds_capture, &dsc_trybuf_desc, &dsc_trybuf, NULL);
   
   /* if we weren't able to create the capture buffer, return error */
   if (FAILED(hr))
      return -1;

   /* else, destroy our capture buffer and return success */
   IDirectSoundCaptureBuffer_Release(dsc_trybuf);

   return 0;
}



/* get_capture_format_support:
 *  an internal function to see if the specified input device
 *  can support a combination of capture settings.
 */
static int get_capture_format_support(int bits, int stereo, int rate,
                                      int autodetect, WAVEFORMATEX *wfx)
{
   int i;
   DSCCAPS dsCaps;
   HRESULT hr;
   WAVEFORMATEX *test_wfx;

   struct {
      unsigned long int type;
      unsigned long int freq;
      unsigned char bits;
      unsigned char channels;
      BOOL stereo;
   } ds_formats[] = {
      { WAVE_FORMAT_4S16,   44100, 16, 2, TRUE  },
      { WAVE_FORMAT_2S16,   22050, 16, 2, TRUE  },
      { WAVE_FORMAT_1S16,   11025, 16, 2, TRUE  },

      { WAVE_FORMAT_4M16,   44100, 16, 1, FALSE },
      { WAVE_FORMAT_2M16,   22050, 16, 1, FALSE },
      { WAVE_FORMAT_1M16,   11025, 16, 1, FALSE },

      { WAVE_FORMAT_4S08,   44100, 8,  2, TRUE  },
      { WAVE_FORMAT_2S08,   22050, 8,  2, TRUE  },
      { WAVE_FORMAT_1S08,   11025, 8,  2, TRUE  },

      { WAVE_FORMAT_4M08,   44100, 8,  1, FALSE },
      { WAVE_FORMAT_2M08,   22050, 8,  1, FALSE },
      { WAVE_FORMAT_1M08,   11025, 8,  1, FALSE },

      { WAVE_INVALIDFORMAT,     0,  0, 0, FALSE }
   };

   if (!ds_capture)
     return -1;

   /* if we have already a capture buffer working
    * we return its format as the only valid one
    */
   if (ds_capture_buf) {
      if (!autodetect) {
         /* we must check that everything that cares is
          * the same as in the capture buffer settings
          */
         if (((bits > 0) && (dsc_buf_wfx.wBitsPerSample != bits)) ||
             ( stereo && (dsc_buf_wfx.nChannels != 2)) ||
             (!stereo && (dsc_buf_wfx.nChannels != 1)) ||
             ((rate > 0) && (dsc_buf_wfx.nSamplesPerSec != rate)))
            return -1;
      }

      /* return the actual capture buffer settings */
      if (wfx)
         memcpy(&dsc_buf_wfx, wfx, sizeof(WAVEFORMATEX));

      return 0;
   }

   /* we use a two-level checking process:
    *  - the normal check of exposed capabilities,
    *  - the actual creation of the capture buffer,
    * because of the single frequency limitation on some
    * sound cards (e.g SB16 ISA) in full duplex mode.
    */
   dsCaps.dwSize = sizeof(DSCCAPS);
   hr = IDirectSoundCapture_GetCaps(ds_capture, &dsCaps);
   if (FAILED(hr)) {
      _TRACE("Can't get input device caps (%s).\n", ds_err(hr));
      return -1;
   }

   if (wfx)
      test_wfx = wfx;
   else
      test_wfx = malloc(sizeof(WAVEFORMATEX));

   for (i=0; ds_formats[i].type != WAVE_INVALIDFORMAT; i++)
      /* if the current format is supported */
      if (dsCaps.dwFormats & ds_formats[i].type) {
         if (!autodetect) {
            /* we must check that everything that cares is
             * the same as in the capture buffer settings
             */
            if (((bits > 0) && (ds_formats[i].bits != bits)) ||
                (stereo && !ds_formats[i].stereo) ||
                (!stereo && ds_formats[i].stereo) ||
                ((rate > 0) && (ds_formats[i].freq != (unsigned int) rate)))
               continue;  /* go to next format */
         }

         test_wfx->wFormatTag = WAVE_FORMAT_PCM;
         test_wfx->nChannels = ds_formats[i].channels;
         test_wfx->nSamplesPerSec = ds_formats[i].freq;
         test_wfx->wBitsPerSample = ds_formats[i].bits;
         test_wfx->nBlockAlign = test_wfx->nChannels * (test_wfx->wBitsPerSample / 8);
         test_wfx->nAvgBytesPerSec = test_wfx->nSamplesPerSec * test_wfx->nBlockAlign;
         test_wfx->cbSize = 0;

         if (create_test_capture_buffer(test_wfx) == 0) {
            if (!wfx)
               free(test_wfx);
            return 0;
         }
      }

   if (!wfx)
      free(test_wfx);

   _TRACE("No valid recording formats found.\n");
   return -1;
}



/* digi_directsound_capture_init:
 *  inits DirectSoundCapture stuff.
 */
static int digi_directsound_capture_init(int id)
{
   DSCCAPS dsCaps;
   WAVEFORMATEX wfx;
   HRESULT hr;

   /* The DirectSoundCapture interface is not part of DirectX 3 */
   if (_dx_ver < 0x500)
      return -1;  

   /* create the device:
    *  we use CoCreateInstance() instead of DirectSoundCaptureCreate() to avoid
    *  the dll loader blocking the start of Allegro under DirectX 3.
    */
   hr = CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IDirectSoundCapture, (LPVOID *)&ds_capture);

   if (FAILED(hr)) {
      _TRACE("Can't create DirectSoundCapture interface (%s).\n", ds_err(hr));
      goto Error;
   }

   /* initialize the device */
   hr = IDirectSoundCapture_Initialize(ds_capture, driver_guids[id]);

   if (FAILED(hr)) {
      hr = IDirectSoundCapture_Initialize(ds_capture, NULL);
      if (FAILED(hr)) {
         _TRACE("Can't initialize DirectSoundCapture interface (%s).\n", ds_err(hr));
         goto Error;
      }
   }

   /* get the device caps */
   dsCaps.dwSize = sizeof(DSCCAPS);
   hr = IDirectSoundCapture_GetCaps(ds_capture, &dsCaps);
   if (FAILED(hr)) {
      _TRACE("Can't get input device caps (%s).\n", ds_err(hr));
      goto Error;
   }

   /* cool little 'autodetection' process :) */
   if (get_capture_format_support(0, FALSE, 0, TRUE, &wfx) != 0) {
      _TRACE("The DirectSound hardware doesn't support any capture types.\n");
      goto Error;
   }

   /* this was added for compatibility */
   digi_driver->rec_cap_bits = wfx.wBitsPerSample;
   digi_driver->rec_cap_stereo = (wfx.nChannels == 2) ? 1 : 0;

   return 0;

 Error:
   /* shutdown DirectSoundCapture */
   digi_directsound_exit(TRUE);

   return -1;
}



/* digi_directsound_init:
 *  initializes DirectSound
 */
static int digi_directsound_init(int input, int voices)
{
   HRESULT hr;
   DSCAPS dscaps;
   DSBUFFERDESC desc;
   WAVEFORMATEX format;
   int v, id;

   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   if (input)
      return digi_directsound_capture_init(id);

   digi_driver->voices = voices;

   /* initialize DirectSound interface */
   hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
   if (FAILED(hr)) { 
      _TRACE("Can't create DirectSound interface (%s).\n", ds_err(hr)); 
      goto Error; 
   }

   /* set cooperative level */
   hr = IDirectSound_SetCooperativeLevel(directsound, allegro_wnd, DSSCL_PRIORITY);
   if (FAILED(hr))
      _TRACE("Can't set DirectSound cooperative level (%s).\n", ds_err(hr));
   else
      _TRACE("DirectSound cooperation level set to PRIORITY.\n");

   /* get hardware capabilities */
   dscaps.dwSize = sizeof(DSCAPS);
   hr = IDirectSound_GetCaps(directsound, &dscaps);
   if (FAILED(hr)) { 
      _TRACE("Can't get DirectSound caps (%s).\n", ds_err(hr)); 
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
   memset(&desc, 0, sizeof(DSBUFFERDESC));
   desc.dwSize = sizeof(DSBUFFERDESC);
   desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;

   hr = IDirectSound_CreateSoundBuffer(directsound, &desc, &prim_buf, NULL);
   if (hr != DS_OK) { 
      _TRACE("Can't create primary buffer (%s)\nGlobal volume control won't be available.\n", ds_err(hr)); 
   }

   /* get current format */
   if (prim_buf) {
      hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
      if (FAILED(hr)) {
         _TRACE("Can't get primary buffer format (%s).\n", ds_err(hr));
      }
      else {
         format.nChannels = _stereo ? 2 : 1;
         format.nSamplesPerSec = _freq;
         format.wBitsPerSample = _bits;
         format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
         format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

         hr = IDirectSoundBuffer_SetFormat(prim_buf, &format);
         if (FAILED(hr)) {
            _TRACE("Can't set primary buffer format (%s).\n", ds_err(hr));
         }

         /* output primary format */
         hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
         if (FAILED(hr)) {
            _TRACE("Can't get primary buffer format (%s).\n", ds_err(hr));
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
   for (v = 0; v < digi_driver->voices; v++) {
      ds_voices[v].ds_buffer = NULL;
      ds_voices[v].ds_loop_buffer = NULL;
   }

   /* setup allegro volume to decibel translation table */
   allegro_to_decibel[0] = 0;
   for (v = 1; v < 256; v++)
      allegro_to_decibel[v] = (unsigned char)(106.0 * log10(v));       /* 255 / log10(255) ~ 106 */

   /* get primary buffer (global) volume */
   IDirectSoundBuffer_GetVolume(prim_buf, &initial_volume); 

   return 0;

 Error:
   _TRACE("digi_directsound_init() failed.\n");
   digi_directsound_exit(0);
   return -1;
}



/* digi_directsound_exit:
 *  closes DirectSound or DirectSoundCapture if input is true.
 */
static void digi_directsound_exit(int input)
{
   int v;

   if (input) {
      /* destroy capture buffer */
      digi_directsound_rec_stop();

      /* shutdown DirectSoundCapture */
      if (ds_capture) {
         IDirectSoundCapture_Release(ds_capture);
         ds_capture = NULL;
      }
   }
   else {
      /* destroy physical voices */
      for (v = 0; v < digi_driver->voices; v++)
         digi_directsound_release_voice(v);

      free(ds_voices);
      ds_voices = NULL;

      /* destroy primary buffer */
      if (prim_buf) {
         /* restore primary buffer initial volume */
         IDirectSoundBuffer_SetVolume(prim_buf, initial_volume);

         IDirectSoundBuffer_Release(prim_buf);
         prim_buf = NULL;
      }

      /* shutdown DirectSound */
      if (directsound) {
         IDirectSound_Release(directsound);
         directsound = NULL;
      }
   }
}



/* digi_directsound_mixer_volume:
 *  sets mixer volume
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
 *  detects DirectSound
 */
static int digi_directsound_detect(int input)
{
   HRESULT hr;
   int id;

   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   if (input) {
      /* The DirectSoundCapture interface is not part of DirectX 3 */
      if (_dx_ver < 0x500)
         return 0;  

      if (!ds_capture) {
         /* create the device:
          *  we use CoCreateInstance() instead of DirectSoundCaptureCreate() to avoid
          *  the dll loader blocking the start of Allegro under DirectX 3.
          */
         hr = CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                               &IID_IDirectSoundCapture, (LPVOID *)&ds_capture);

         if (FAILED(hr)) {
            _TRACE("DirectSoundCapture interface creation failed during detect (%s).\n", ds_err(hr));
            return 0;
         }

         /* initialize the device */
         hr = IDirectSoundCapture_Initialize(ds_capture, driver_guids[id]);

         if (FAILED(hr)) {
            hr = IDirectSoundCapture_Initialize(ds_capture, NULL);
            if (FAILED(hr)) {
               _TRACE("DirectSoundCapture interface initialization failed during detect (%s).\n", ds_err(hr));
               return 0;
            }
         }

         _TRACE("DirectSoundCapture interface successfully created.\n");

         /* release DirectSoundCapture interface */
         IDirectSoundCapture_Release(ds_capture);
         ds_capture = NULL;
      }

      return 1;
   }
   else {
      if (!directsound) {
         /* initialize DirectSound interface */
         hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
         if (FAILED(hr)) {
            _TRACE("DirectSound interface creation failed during detect (%s).\n", ds_err(hr));
            return 0;
         }

         _TRACE("DirectSound interface successfully created.\n");

         /* release DirectSound interface */
         IDirectSound_Release(directsound);
         directsound = NULL;
      }

      return 1;
   }
}



/********************************************************/
/*********** voice management functions *****************/
/********************************************************/


/* create_directsound_buffer:
 *  worker function for creating a DS buffer from a sample
 */
static int create_directsound_buffer(LPDIRECTSOUNDBUFFER *snd_buf, AL_CONST SAMPLE *sample, int loop_buffer)
{
   PCMWAVEFORMAT pcmwf;
   DSBUFFERDESC dsbdesc;
   HRESULT hr;
   LPVOID buf_a;
   DWORD size_a;
   LPVOID buf_b;
   DWORD size_b;
   DWORD pos;
   int offset, len;

   if (loop_buffer) {
      offset = sample->loop_start;
      len = sample->loop_end - sample->loop_start;
   }
   else {
      offset = 0;
      len = sample->len;
   }

   /* setup wave format structure */
   memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
   pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
   pcmwf.wf.nChannels = sample->stereo ? 2 : 1;
   pcmwf.wf.nSamplesPerSec = sample->freq;
   pcmwf.wBitsPerSample = sample->bits;
   pcmwf.wf.nBlockAlign = sample->bits * (sample->stereo ? 2 : 1) / 8;
   pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;

   /* setup DSBUFFERDESC structure */
   memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
   dsbdesc.dwSize = sizeof(DSBUFFERDESC);

   /* need default controls (pan, volume, frequency) */
   dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;
   switch (get_display_switch_mode()) {

      case SWITCH_BACKAMNESIA:
      case SWITCH_BACKGROUND:
         dsbdesc.dwFlags |= DSBCAPS_STICKYFOCUS;
         break;
   }

   dsbdesc.dwBufferBytes = len * (sample->bits / 8) * (sample->stereo ? 2 : 1);
   dsbdesc.lpwfxFormat = (LPWAVEFORMATEX) & pcmwf;

   /* create buffer */
   hr = IDirectSound_CreateSoundBuffer(directsound, &dsbdesc, snd_buf, NULL);
   if (FAILED(hr)) {
      _TRACE("create_directsound_buffer() failed (%s).\n", ds_err(hr));
      _TRACE(" - %d Hz, %s, %d bits\n", sample->freq, sample->stereo ? "stereo" : "mono", sample->bits);
      return -1;
   }

   /* copy sample contents to buffer */
   hr = IDirectSoundBuffer_Lock(*snd_buf, 0,
                                len * (sample->bits / 8) * (sample->stereo ? 2 : 1),
                                &buf_a, &size_a, &buf_b, &size_b, 0);

   /* if DSERR_BUFFERLOST is returned, restore and retry lock */
   if (hr == DSERR_BUFFERLOST) {
      _TRACE("Locking sound buffer failed (DSERR_BUFFERLOST).\n");
      IDirectSoundBuffer_Restore(*snd_buf);
      hr = IDirectSoundBuffer_Lock(*snd_buf, 0,
                                   len * (sample->bits / 8) * (sample->stereo ? 2 : 1),
                                   &buf_a, &size_a, &buf_b, &size_b, 0);
   }

   if (FAILED(hr)) {
      _TRACE("create_directsound_buffer() failed: can't lock sound buffer.\n");
      return -1;
   }

   if ((sample->bits == 16) && ((int) sample->param != -2)) {
      /* convert 16-bit samples from Allegro format to DS */
      for (pos = 0; pos < size_a / 2; pos++)
         ((unsigned short *)buf_a)[pos] = ((unsigned short *)(sample->data))[offset + pos] - 32768;

      if (buf_b) {
         for (pos = 0; pos < size_b / 2; pos++)
            ((unsigned short *)buf_b)[pos] = ((unsigned short *)(sample->data))[offset + pos + size_a / 2] - 32768;
      }
   }
   else {
      memcpy(buf_a, (char *)(sample->data) + offset, size_a);
      if (buf_b != NULL)
         memcpy(buf_b, (char *)(sample->data) + offset+ size_a, size_b);
   }

   /* release the data back to DirectSound */
   hr = IDirectSoundBuffer_Unlock(*snd_buf, buf_a, size_a, buf_b, size_b);
   if (hr != DS_OK) {
      _TRACE("Can't unlock sound buffer.\n");
   }

   return 0;
}



/* digi_directsound_init_voice:
 */
static void digi_directsound_init_voice(int voice, AL_CONST SAMPLE *sample)
{
   LPDIRECTSOUNDBUFFER snd_buf;

   ds_voices[voice].bits = sample->bits;
   ds_voices[voice].bytes_per_sample = (sample->bits/8) * (sample->stereo ? 2 : 1);
   ds_voices[voice].vol = 255;
   ds_voices[voice].pan = 128;
   ds_voices[voice].freq = sample->freq;
   ds_voices[voice].dfreq = 0;
   ds_voices[voice].dpan = 0;
   ds_voices[voice].dvol = 0;
   ds_voices[voice].loop_offset = 0;
   ds_voices[voice].loop_len = 0;
   ds_voices[voice].looping = 0;

   if (create_directsound_buffer(&snd_buf, sample, FALSE) != 0)
      return;

   ds_voices[voice].ds_buffer = snd_buf;

   /* custom loop points support */
   if ((sample->loop_start != 0) || (sample->loop_end != sample->len)) {
      _TRACE("Secondary loop buffer needed.\n");
      if (create_directsound_buffer(&snd_buf, sample, TRUE) != 0)
         return;

      ds_voices[voice].loop_offset = sample->loop_start;
      ds_voices[voice].loop_len = sample->loop_end - sample->loop_start;
      ds_voices[voice].ds_loop_buffer = snd_buf;
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

   if (ds_voices[voice].ds_loop_buffer) {
      IDirectSoundBuffer_Release(ds_voices[voice].ds_loop_buffer);
      ds_voices[voice].ds_loop_buffer = NULL;
   }
}



/* digi_directsound_start_voice:
 */
static void digi_directsound_start_voice(int voice)
{
   if (ds_voices[voice].looping && ds_voices[voice].ds_loop_buffer) {
      IDirectSoundBuffer_Play(ds_voices[voice].ds_loop_buffer, 0, 0, DSBPLAY_LOOPING);
   }
   else if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Play(ds_voices[voice].ds_buffer, 0, 0,
                              ds_voices[voice].looping ? DSBPLAY_LOOPING : 0);
   }
}



/* digi_directsound_stop_voice:
 */
static void digi_directsound_stop_voice(int voice)
{
   if (ds_voices[voice].looping && ds_voices[voice].ds_loop_buffer) {
      IDirectSoundBuffer_Stop(ds_voices[voice].ds_loop_buffer);
   }
   else if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Stop(ds_voices[voice].ds_buffer);
   }
}



/* digi_directsound_loop_voice:
 */
static void digi_directsound_loop_voice(int voice, int playmode)
{
   switch (playmode) {

      case PLAYMODE_LOOP:
         if (!ds_voices[voice].looping) {
            ds_voices[voice].looping = 1;

            if (ds_voices[voice].ds_loop_buffer)
               IDirectSoundBuffer_SetCurrentPosition(ds_voices[voice].ds_loop_buffer, 0);
         }
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
      _TRACE("digi_directsound_lock_voice() failed (%s).\n", ds_err(hr));
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
         _TRACE("digi_directsound_unlock_voice() failed (%s).\n", ds_err(hr));
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

   if (ds_voices[voice].looping && ds_voices[voice].ds_loop_buffer) {
      /* is buffer playing */
      hr = IDirectSoundBuffer_GetStatus(ds_voices[voice].ds_loop_buffer,  &status);
      if (FAILED(hr))
         return -1;

      if ((status & DSBSTATUS_PLAYING) == 0)
         return -1;

      hr = IDirectSoundBuffer_GetCurrentPosition(ds_voices[voice].ds_loop_buffer,
                                                 &play_cursor, &write_cursor);
      if (FAILED(hr)) {
         _TRACE("digi_directsound_get_position() failed (%s).\n", ds_err(hr));
         return -1;
      }

      return ds_voices[voice].loop_offset + play_cursor / ds_voices[voice].bytes_per_sample;
   }
   else if (ds_voices[voice].ds_buffer) {
      /* is buffer playing */
      hr = IDirectSoundBuffer_GetStatus(ds_voices[voice].ds_buffer,  &status);
      if (FAILED(hr))
         return -1;

      if ((status & DSBSTATUS_PLAYING) == 0)
         return -1;

      hr = IDirectSoundBuffer_GetCurrentPosition(ds_voices[voice].ds_buffer,
                                                 &play_cursor, &write_cursor);
      if (FAILED(hr)) {
         _TRACE("digi_directsound_get_position() failed (%s).\n", ds_err(hr));
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

   if (ds_voices[voice].ds_loop_buffer) {
      position = MID(0, position - ds_voices[voice].loop_offset, ds_voices[voice].loop_len-1);

      hr = IDirectSoundBuffer_SetCurrentPosition(ds_voices[voice].ds_loop_buffer,
                                                 position * ds_voices[voice].bytes_per_sample);
      if (FAILED(hr)) {
         _TRACE("digi_directsound_set_position() failed (%s).\n", ds_err(hr));
      }
   }

   if (ds_voices[voice].ds_buffer) {
      hr = IDirectSoundBuffer_SetCurrentPosition(ds_voices[voice].ds_buffer,
                                                 position * ds_voices[voice].bytes_per_sample);
      if (FAILED(hr)) {
         _TRACE("digi_directsound_set_position() failed (%s).\n", ds_err(hr));
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

      if (ds_voices[voice].ds_loop_buffer)
         IDirectSoundBuffer_SetVolume(ds_voices[voice].ds_loop_buffer, ds_vol);
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

      if (ds_voices[voice].ds_loop_buffer)
         IDirectSoundBuffer_SetFrequency(ds_voices[voice].ds_loop_buffer, frequency);
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

      if (ds_voices[voice].ds_loop_buffer)
         IDirectSoundBuffer_SetPan(ds_voices[voice].ds_loop_buffer, ds_pan);
   }
}



/********************************************/
/*********** input functions ****************/
/********************************************/


/* digi_directsound_rec_cap_rate:
 *  gets the maximum input frequency for the specified parameters.
 */
static int digi_directsound_rec_cap_rate(int bits, int stereo)
{
   WAVEFORMATEX wfx;

   if (get_capture_format_support(bits, stereo, 0, FALSE, &wfx) != 0)
      return 0;

   return wfx.nSamplesPerSec;
}



/* digi_directsound_rec_cap_param:
 *  determines if the combination of provided parameters can be
 *  used for recording.
 */
static int digi_directsound_rec_cap_param(int rate, int bits, int stereo)
{
   if (get_capture_format_support(bits, stereo, rate, FALSE, NULL) == 0)
      return 2;

   if (get_capture_format_support(bits, stereo, 44100, FALSE, NULL) == 0)
      return -44100;

   if (get_capture_format_support(bits, stereo, 22050, FALSE, NULL) == 0)
      return -22050;

   if (get_capture_format_support(bits, stereo, 11025, FALSE, NULL) == 0)
      return -11025;

   return 0;
}



/* digi_directsound_rec_source:
 *  sets the source for the audio recording.
 */
static int digi_directsound_rec_source(int source)
{
   /* since DirectSoundCapture doesn't allow us to
    * select a input source manually, we return -1
    */
   return -1;
}



/* digi_directsound_rec_start:
 *  attempts to start recording with the specified parameters.
 */
static int digi_directsound_rec_start(int rate, int bits, int stereo)
{
   DSCBUFFERDESC dscBufDesc;
   HRESULT hr;

   if (!ds_capture || ds_capture_buf)
      return 0;

   /* check if we support the desired format */
   if ((bits <= 0) || (rate <= 0))
      return 0;

   if (get_capture_format_support(bits, stereo, rate, FALSE, &dsc_buf_wfx) != 0)
      return 0;

   digi_driver->rec_cap_bits = dsc_buf_wfx.wBitsPerSample;
   digi_driver->rec_cap_stereo = (dsc_buf_wfx.nChannels == 2) ? 1 : 0;

   /* create the capture buffer */
   dscBufDesc.dwSize  = sizeof(DSCBUFFERDESC);
   dscBufDesc.dwFlags = 0;
   dscBufDesc.dwBufferBytes = dsc_buf_wfx.nAvgBytesPerSec;
   dscBufDesc.dwReserved  = 0;
   dscBufDesc.lpwfxFormat = &dsc_buf_wfx;
   ds_capture_buffer_size = dscBufDesc.dwBufferBytes;

   hr = IDirectSoundCapture_CreateCaptureBuffer(ds_capture, &dscBufDesc, &ds_capture_buf, NULL);
   if (FAILED(hr)) {
      _TRACE("Can't create the DirectSound capture buffer (%s).\n", ds_err(hr));
      return 0;
   }

   hr = IDirectSoundCaptureBuffer_Start(ds_capture_buf, DSCBSTART_LOOPING);
   if (FAILED(hr)) {
      IDirectSoundCaptureBuffer_Release(ds_capture_buf);
      ds_capture_buf = NULL;

      _TRACE("Can't start the DirectSound capture buffer (%s).\n", ds_err(hr));
      return 0;
   }

   last_capture_pos = 0;
   input_wave_data = malloc(ds_capture_buffer_size);
   input_wave_bytes_read = 0;

   return ds_capture_buffer_size;
}



/* digi_directsound_rec_stop:
 *  stops recording.
 */
static void digi_directsound_rec_stop(void)
{
   if (ds_capture_buf) {
      if (input_wave_data) {
         free(input_wave_data);
         input_wave_data = NULL;
      }

      IDirectSoundCaptureBuffer_Stop(ds_capture_buf);
      IDirectSoundCaptureBuffer_Release(ds_capture_buf);
      ds_capture_buf = NULL;
   }
}



/* digi_directsound_rec_read:
 *  reads the input buffer.
 */
static int digi_directsound_rec_read(void *buf)
{
   unsigned char *input_ptr1, *input_ptr2, *linear_input_ptr;
   unsigned long int input_bytes1, input_bytes2, bytes_to_lock;
   unsigned long int capture_pos;
   HRESULT hr;
   BOOL buffer_filled = FALSE;

   if (!ds_capture || !ds_capture_buf || !input_wave_data)
      return 0;

   IDirectSoundCaptureBuffer_GetCurrentPosition(ds_capture_buf, &capture_pos, NULL);

   /* check if we are not still in the same capture position */
   if (last_capture_pos == capture_pos)
      return 0;

   /* check how many bytes we need to lock since last check */
   if (capture_pos > last_capture_pos) {
      bytes_to_lock = capture_pos - last_capture_pos;
   }
   else {
      bytes_to_lock = ds_capture_buffer_size - last_capture_pos;
      bytes_to_lock += capture_pos;
   }

   hr = IDirectSoundCaptureBuffer_Lock(ds_capture_buf, last_capture_pos,
                                       bytes_to_lock, (LPVOID *)&input_ptr1,
                                       &input_bytes1, (LPVOID *)&input_ptr2,
                                       &input_bytes2, 0);
   if (FAILED(hr))
      return 0;

   /* let's get the data aligned linearly */
   linear_input_ptr = malloc(bytes_to_lock);
   memcpy(linear_input_ptr, input_ptr1, input_bytes1);

   if (input_ptr2)
      memcpy(linear_input_ptr + input_bytes1, input_ptr2, input_bytes2);

   IDirectSoundCaptureBuffer_Unlock(ds_capture_buf, input_ptr1,
                                    input_bytes1, input_ptr2, input_bytes2);

   if ((input_wave_bytes_read + bytes_to_lock) >= ds_capture_buffer_size) {
      /* we fill the buffer */
      long int bytes_left_to_fill = ds_capture_buffer_size - input_wave_bytes_read;
      long int bytes_to_internal = bytes_to_lock - bytes_left_to_fill;

      /* yes, copy old buffer to user buffer */
      memcpy(buf, input_wave_data, input_wave_bytes_read);

      /* and the rest of bytes we would need to fill in the buffer */
      memcpy(buf + input_wave_bytes_read, linear_input_ptr, bytes_left_to_fill);

      /* and the rest of the data to the internal buffer */
      input_wave_bytes_read = bytes_to_internal;
      memcpy(input_wave_data, linear_input_ptr + bytes_left_to_fill, bytes_to_internal);

      buffer_filled = TRUE;
  
      /* if we are using 16-bit data, we need to convert it to unsigned format */
      if (digi_driver->rec_cap_bits == 16) {
         int i;
         unsigned short *buf16 = (unsigned short *)buf;

         for (i = 0; i < ds_capture_buffer_size/2; i++)
            buf16[i] ^= 0x8000;
      }
   }
   else {
      /* we won't fill the buffer */
      memcpy(input_wave_data + input_wave_bytes_read, linear_input_ptr, bytes_to_lock);
      input_wave_bytes_read += bytes_to_lock;
   }

   free(linear_input_ptr);

   last_capture_pos = capture_pos;

   if (buffer_filled)
      return 1;
   else
      return 0;
}
