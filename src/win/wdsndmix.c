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
 *      Allegro mixer to DirectSound driver.
 *
 *      By Robin Burrows.
 *
 *      Based on original src/win/wdsound.c by Stefan Schimanski
 *      and src/unix/oss.c by Joshua Heyer.
 *
 *      Bugfixes by Javier Gonzalez.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTSOUND_VERSION 0x0300

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
   #include <dsound.h>
   #include <math.h>

   #ifdef ALLEGRO_MSVC
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
static int digi_directsound_buffer_size(void);


/* sound driver globals */
static LPDIRECTSOUND directsound = NULL;
static LPDIRECTSOUNDBUFFER prim_buf = NULL;
static long int initial_volume;
static int _freq, _bits, _stereo;
static int linear_to_millibel[256];
static int digidsbufsize, digidsbufpos;
static unsigned char *digidsbufdata;
static int bufdivs = 16;
static int prim_buf_paused = FALSE;
static int prim_buf_vol;
static UINT dbgtid;

static DIGI_DRIVER digi_directx =
{
   0,
   empty_string,
   empty_string,
   empty_string,
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   digi_directsound_detect,
   digi_directsound_init,
   digi_directsound_exit,
   digi_directsound_mixer_volume,

   NULL,
   NULL,
   digi_directsound_buffer_size,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,

   _mixer_get_position,
   _mixer_set_position,

   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,

   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,

   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,

   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,

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


#define MAX_DRIVERS  16

static char *driver_names[MAX_DRIVERS];
static LPGUID driver_guids[MAX_DRIVERS];



/* digi_diretsound_mixer_callback:
 *  callback to update sound in primary buffer
 */
static void CALLBACK digi_directsound_mixer_callback(UINT uID, UINT uMsg, DWORD dwUser,
                                                     DWORD dw1, DWORD dw2)
{ 
   LPVOID lpvPtr1, lpvPtr2;
   DWORD dwBytes1, dwBytes2, writecurs;
   HRESULT hr;
   int i, switch_mode;

   /* handle display switchs */
   switch_mode = get_display_switch_mode();

   if (prim_buf_paused) {
      if (app_foreground ||
          (switch_mode == SWITCH_BACKGROUND) || (switch_mode == SWITCH_BACKAMNESIA)) {
         prim_buf_paused = FALSE;
         IDirectSoundBuffer_Play(prim_buf, 0, 0, DSBPLAY_LOOPING);
      }
      else
         return;
   }
   else {
      if (!app_foreground &&
          ((switch_mode == SWITCH_PAUSE) || (switch_mode == SWITCH_AMNESIA))) {
         prim_buf_paused = TRUE;
         IDirectSoundBuffer_Stop(prim_buf);
         return;
      }
   }

   /* get primary buffer current state */
   hr = IDirectSoundBuffer_GetStatus(prim_buf, &dwBytes1);
   if (hr == DS_OK) {
      if (dwBytes1 == DSBSTATUS_BUFFERLOST) {
         IDirectSoundBuffer_Restore(prim_buf);
         IDirectSoundBuffer_Play(prim_buf, 0, 0, DSBPLAY_LOOPING);
         IDirectSoundBuffer_SetVolume(prim_buf, prim_buf_vol);
      }
   }

   hr = IDirectSoundBuffer_GetCurrentPosition(prim_buf, NULL, &writecurs);
   if (FAILED(hr))
      return;

   writecurs /= (digidsbufsize/bufdivs);
   writecurs += 8;

   while (writecurs > (bufdivs-1))
      writecurs -= bufdivs;

   if (writecurs == digidsbufpos)
      return;

   hr = IDirectSoundBuffer_Lock(prim_buf, 0, 0, 
                                &lpvPtr1, &dwBytes1, 
                                &lpvPtr2, &dwBytes2,
                                DSBLOCK_FROMWRITECURSOR | DSBLOCK_ENTIREBUFFER);

   /* if DSERR_BUFFERLOST is returned, we'll catch up next time */
   if (FAILED(hr))
      return;

   /* write datas into the buffer */
   while (writecurs != digidsbufpos) {
      if (lpvPtr2) {
         memcpy((char*)lpvPtr2 + (((dwBytes1+dwBytes2)/bufdivs)*digidsbufpos),
                digidsbufdata + (((dwBytes1+dwBytes2)/bufdivs)*digidsbufpos),
                (dwBytes1+dwBytes2)/bufdivs);
      }
      else {
         memcpy((char*)lpvPtr1 + (((dwBytes1)/bufdivs)*digidsbufpos),
                digidsbufdata + (((dwBytes1)/bufdivs)*digidsbufpos),
                (dwBytes1)/bufdivs);
      }

      if (++digidsbufpos > (bufdivs-1))
         digidsbufpos = 0;

      _mix_some_samples((unsigned long)(digidsbufdata+(((dwBytes1+dwBytes2)/bufdivs)*digidsbufpos)), 0, 1);
   }

   IDirectSoundBuffer_Unlock(prim_buf, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2);
} 



/* _get_digi_driver_list:
 *  System driver hook for listing the available sound drivers. This 
 *  generates the device list at runtime, to match whatever DirectSound
 *  devices are available.
 */
DIGI_DRIVER *_get_dsalmix_driver(char *name, LPGUID guid, int num)
{
   DIGI_DRIVER *driver;

   driver = malloc(sizeof(DIGI_DRIVER));
   if (!driver)
      return NULL;

   memcpy(driver, &digi_directx, sizeof(DIGI_DRIVER));

   driver->id = DIGI_DIRECTAMX(num);

   driver_names[num] = malloc(strlen(name)+10);
   if (driver_names[num]) {
      strcpy(driver_names[num], "Allegmix ");
      strcpy(driver_names[num]+9, name);
      driver->ascii_name = driver_names[num];
   }

   driver_guids[num] = guid;

   return driver;
}



/* ds_err:
 *  returns an error string
 */
#ifdef DEBUGMODE
static char *ds_err(long err)
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



/* digi_directsound_init:
 *  initializes DirectSound
 */
static int digi_directsound_init(int input, int voices)
{
   LPVOID lpvPtr1, lpvPtr2;
   DWORD dwBytes1, dwBytes2;
   HRESULT hr;
   DSCAPS dscaps;
   DSBUFFERDESC desc;
   WAVEFORMATEX format;
   int v, id;

   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   if (input)
      return digi_directsound_capture_init(driver_guids[id]);

   digi_driver->voices = voices;

   /* initialize DirectSound interface */
   hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
   if (FAILED(hr)) { 
      _TRACE("Can't create DirectSound interface (%s)\n", ds_err(hr)); 
      goto Error; 
   }

   /* set cooperative level */
   hr = IDirectSound_SetCooperativeLevel(directsound, allegro_wnd, DSSCL_WRITEPRIMARY);
   if (FAILED(hr))
      _TRACE("Can't set DirectSound cooperative level (%s)\n", ds_err(hr));
   else
      _TRACE("DirectSound cooperation level set to DSSCL_WRITEPRIMARY\n");

   /* get hardware capabilities */
   dscaps.dwSize = sizeof(DSCAPS);
   hr = IDirectSound_GetCaps(directsound, &dscaps);
   if (FAILED(hr)) { 
      _TRACE("Can't get DirectSound caps (%s)\n", ds_err(hr)); 
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

   memset(&desc, 0, sizeof(DSBUFFERDESC));
   desc.dwSize = sizeof(DSBUFFERDESC);
   desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME | DSBCAPS_STICKYFOCUS;

   hr = IDirectSound_CreateSoundBuffer(directsound, &desc, &prim_buf, NULL);
   if (FAILED(hr))
      _TRACE("Can't create primary buffer (%s)\nGlobal volume control won't be available\n", ds_err(hr)); 

   if (prim_buf) {
      hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
      if (FAILED(hr)) {
         _TRACE("Can't get primary buffer format (%s)\n", ds_err(hr));
      }
      else {
         format.nChannels = _stereo ? 2 : 1;
         format.nSamplesPerSec = _freq;
         format.wBitsPerSample = _bits;
         format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
         format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

         hr = IDirectSoundBuffer_SetFormat(prim_buf, &format);
         if (FAILED(hr))
            _TRACE("Can't set primary buffer format (%s)\n", ds_err(hr));

         hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
         if (FAILED(hr)) {
            _TRACE("Can't get primary buffer format (%s)\n", ds_err(hr));
         }
         else {
            _TRACE("primary format:\n");
            _TRACE("  %u channels\n  %u Hz\n  %u AvgBytesPerSec\n  %u BlockAlign\n  %u bits\n  %u size\n",
                   format.nChannels, format.nSamplesPerSec, format.nAvgBytesPerSec,
                   format.nBlockAlign, format.wBitsPerSample, format.cbSize);
         }
      }
   }

   /* setup volume lookup table */
   linear_to_millibel[0] = DSBVOLUME_MIN;
   for (v = 1; v < 256; v++)
      linear_to_millibel[v] = MAX(DSBVOLUME_MIN, DSBVOLUME_MAX + 2000.0*log10(v/255.0));

   /* fill the primary buffer with zeros */
   hr = IDirectSoundBuffer_Lock(prim_buf, 0, 0,
                                &lpvPtr1, &dwBytes1, 
                                &lpvPtr2, &dwBytes2,
                                DSBLOCK_FROMWRITECURSOR | DSBLOCK_ENTIREBUFFER);

   if (FAILED(hr)) {
      _TRACE("Can't lock primary buffer (%s)\n", ds_err(hr));
      return -1;
   }

   memset(lpvPtr1, 0, dwBytes1);
   digidsbufsize = dwBytes1;

   if (lpvPtr2) {
      memset(lpvPtr2, 0, dwBytes2);
      digidsbufsize += dwBytes2;
   }

   digidsbufdata = malloc(digidsbufsize);
   digidsbufpos = 0;
   bufdivs = digidsbufsize/1024;

   IDirectSoundBuffer_Unlock(prim_buf, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2); 

   if (digidsbufsize%1024) {
      _TRACE("Primary buffer is not multiple of 1024, failed\n");
      return -1;
   }

   if (_mixer_init((digidsbufsize / (_bits /8)) / bufdivs, _freq,
                   _stereo, ((_bits == 16) ? 1 : 0), &digi_driver->voices) != 0) {
      _TRACE("Can't init software mixer\n");
      return -1;
   }

   _mix_some_samples((unsigned long)digidsbufdata, 0, 1);

   /* get primary buffer volume */
   IDirectSoundBuffer_GetVolume(prim_buf, &initial_volume);
   prim_buf_vol = initial_volume;

   /* start playing */
   /* install_int(digi_directsound_mixer_callback, BPS_TO_TIMER(50)); */
   dbgtid = timeSetEvent(20, 10, digi_directsound_mixer_callback, 0, TIME_PERIODIC);
   IDirectSoundBuffer_Play(prim_buf, 0, 0, DSBPLAY_LOOPING);
	
   return 0;

 Error:
   _TRACE("digi_directsound_init() failed\n");
   digi_directsound_exit(0);
   return -1;
}



/* digi_directsound_exit:
 *  closes DirectSound
 */
static void digi_directsound_exit(int input)
{
   if (input) {
      digi_directsound_capture_exit();
      return;
   }
	
   if (dbgtid)
      timeKillEvent(dbgtid);

   if (digidsbufdata) {
      free(digidsbufdata);
      digidsbufdata = NULL;
   }

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



/* digi_directsound_mixer_volume:
 *  sets mixer volume
 */
static int digi_directsound_mixer_volume(int volume)
{
   if (prim_buf) {
      prim_buf_vol = linear_to_millibel[MID(0, volume, 255)];
      IDirectSoundBuffer_SetVolume(prim_buf, prim_buf_vol); 
   }

   return 0;
}



/* digi_directsound_buffer_size:
 *  returns size of buffer in samples
 */
static int digi_directsound_buffer_size(void)
{
   return digidsbufsize / (_bits / 8) / (_stereo ? 2 : 1);
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

   if (input)
      return digi_directsound_capture_detect(driver_guids[id]);

   if (!directsound) {
      /* initialize DirectSound interface */
      hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
      if (FAILED(hr)) {
         _TRACE("DirectSound interface creation failed during detect (%s)\n", ds_err(hr));
         return 0;
      }

      _TRACE("DirectSound interface successfully created\n");

      /* release DirectSound */
      IDirectSound_Release(directsound);
      directsound = NULL;
   }

   return 1;
}
