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
 *      Allegro mixer to directsound driver by Robin Burrows.
 *
 *      Based on original src/win/wdsound.c by Stefan Schimanski
 *      and src/unix/oss.c by Joshua Heyer
 *
 *      See readme.txt for copyright information.
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
static int digi_directsound_buffer_size(void);


/* sound driver globals */
static LPDIRECTSOUND directsound = NULL;
static LPDIRECTSOUNDBUFFER prim_buf = NULL;
static int _freq, _bits, _stereo;
static unsigned char allegro_to_decibel[256];
static int _digidsbufsize;
static unsigned char * _digidsbufdata;
static int _digidsbufpos;
static int bufdivs = 16;

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
   0,                           /* int rec_cap_bits; */
   0,                           /* int rec_cap_stereo; */
   NULL,                        /* AL_METHOD(int,  rec_cap_rate, (int bits, int stereo)); */
   NULL,                        /* AL_METHOD(int,  rec_cap_parm, (int rate, int bits, int stereo)); */
   NULL,                        /* AL_METHOD(int,  rec_source, (int source)); */
   NULL,                        /* AL_METHOD(int,  rec_start, (int rate, int bits, int stereo)); */
   NULL,                        /* AL_METHOD(void, rec_stop, (void)); */
   NULL                         /* AL_METHOD(int,  rec_read, (void *buf)); */
};


#define MAX_DRIVERS  16

static int num_drivers = 0;

static char *driver_names[MAX_DRIVERS];
static LPGUID driver_guids[MAX_DRIVERS];

static int dwOldPos = 0;

/* This is the callback to update sound in primary buffer */
static void CALLBACK digi_directsound_mixer_callback(
  UINT uID,      
  UINT uMsg,     
  DWORD dwUser,  
  DWORD dw1,     
  DWORD dw2)
{ 
LPVOID lpvPtr1; 
DWORD dwBytes1; 
LPVOID lpvPtr2; 
DWORD dwBytes2; 
DWORD writecurs;
HRESULT hr;
int i;

	if (IDirectSoundBuffer_GetStatus (prim_buf, &dwBytes1) == DS_OK) {
		if (dwBytes1 == DSBSTATUS_BUFFERLOST) {
			IDirectSoundBuffer_Restore (prim_buf);
			IDirectSoundBuffer_Play (prim_buf, 0, 0, DSBPLAY_LOOPING);
			IDirectSoundBuffer_SetPan (prim_buf, DSBPAN_CENTER);
			IDirectSoundBuffer_SetVolume (prim_buf, DSBVOLUME_MAX);
		}
	}
	if (IDirectSoundBuffer_GetCurrentPosition (prim_buf, NULL, &writecurs) != DS_OK)
		return;

	writecurs /= (_digidsbufsize/bufdivs);
	writecurs += 8; while (writecurs > (bufdivs-1)) writecurs -= bufdivs;
	if (writecurs == _digidsbufpos) return;

	hr = IDirectSoundBuffer_Lock(prim_buf, 
							0, 0, 
							&lpvPtr1, &dwBytes1, 
							&lpvPtr2, &dwBytes2,
							DSBLOCK_FROMWRITECURSOR | DSBLOCK_ENTIREBUFFER); 
/* If DSERR_BUFFERLOST is returned, quit, we'll catch up next time */
	if (hr == DSERR_BUFFERLOST) return;

	if (hr == DS_OK) { 
		while (writecurs != _digidsbufpos) {
		if (((char *)lpvPtr2) != NULL) {
			memcpy ((char*)lpvPtr2 + (((dwBytes1+dwBytes2)/bufdivs)*_digidsbufpos),
				_digidsbufdata + (((dwBytes1+dwBytes2)/bufdivs)*_digidsbufpos),
				(dwBytes1+dwBytes2)/bufdivs);
		} else {
			memcpy ((char*)lpvPtr1 + (((dwBytes1)/bufdivs)*_digidsbufpos),
				_digidsbufdata + (((dwBytes1)/bufdivs)*_digidsbufpos),
				(dwBytes1)/bufdivs);
		}

		_digidsbufpos++; if (_digidsbufpos > (bufdivs-1)) _digidsbufpos = 0;
		_mix_some_samples((unsigned long)
			(_digidsbufdata+(((dwBytes1+dwBytes2)/bufdivs)*_digidsbufpos)), 0, 1);
		}

		hr = IDirectSoundBuffer_Unlock(prim_buf, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2); 
	} 
	return;
} 
END_OF_STATIC_FUNCTION (digi_directsound_mixer_callback);




/* _get_digi_driver_list:
 *  System driver hook for listing the available sound drivers. This 
 *  generates the device list at runtime, to match whatever DirectSound
 *  devices are available.
 */
DIGI_DRIVER *_get_dsalmix_driver(char * dxname, int num)
{
DIGI_DRIVER *driver;

	driver = malloc(sizeof(DIGI_DRIVER));
	if (driver == NULL) return NULL;

	memcpy(driver, &digi_directx, sizeof(DIGI_DRIVER));

	driver->id = DIGI_DIRECTAMX(num);

	driver_names[num] = malloc (strlen(dxname)+10);
	if (driver_names != NULL) {
		strcpy (driver_names[num], "Allegmix ");
		strcpy (driver_names[num]+9, dxname);
	}
	driver->ascii_name = driver_names[num];

	return driver;
}




/* digi_directsound_init:
 *  Initialize driver                                                                
 *
 */
static int digi_directsound_init(int input, int voices)
{
LPVOID lpvPtr1; 
DWORD dwBytes1; 
LPVOID lpvPtr2; 
DWORD dwBytes2; 
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
	hr = IDirectSound_SetCooperativeLevel(directsound, allegro_wnd, DSSCL_WRITEPRIMARY);
	if (FAILED(hr))
		_TRACE("Can't set DirectSound cooperative level (%x)\n", hr);
	else
		_TRACE("DirectSound cooperation level set to DSSCL_WRITEPRIMARY\n");

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

	memset (&desc, 0, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

	hr = IDirectSound_CreateSoundBuffer(directsound, &desc, &prim_buf, NULL);
	if (hr != DS_OK) { 
		_TRACE("Can't create primary buffer\nGlobal volume control won't be available\n"); 
	}

	if (prim_buf) {
		hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
		if (FAILED(hr)) {
			_TRACE("Can't get primary buffer format\n");
		} else {
			format.nChannels = _stereo ? 2 : 1;
			format.nSamplesPerSec = _freq;
			format.wBitsPerSample = _bits;
			format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
			format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

			hr = IDirectSoundBuffer_SetFormat(prim_buf, &format);
			if (FAILED(hr)) {
				_TRACE("Can't set primary buffer format\n");
			}

			hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
			if (FAILED(hr)) {
				_TRACE("Can't get primary buffer format\n");
			} else {
				_TRACE("primary format:\n");
				_TRACE("  %u channels\n  %u Hz\n  %u AvgBytesPerSec\n  %u BlockAlign\n  %u bits\n  %u size\n",
				format.nChannels, format.nSamplesPerSec, format.nAvgBytesPerSec,
				format.nBlockAlign, format.wBitsPerSample, format.cbSize);
			}
		}
	}

	allegro_to_decibel[0] = 0;
	for (v = 1; v < 256; v++)

	/* 255 / log10(255) ~ 106 */
	allegro_to_decibel[v] = (unsigned char)(106.0 * log10(v));

	hr = IDirectSoundBuffer_Lock(prim_buf, 
						0, 0,
                                    &lpvPtr1, &dwBytes1, 
                                    &lpvPtr2, &dwBytes2,
						DSBLOCK_FROMWRITECURSOR | DSBLOCK_ENTIREBUFFER);

	if (hr != DS_OK) {
	      _TRACE("Can not lock primary buffer (init)\n");
	      return -1;
	}

	memset (lpvPtr1, 0, dwBytes1);
	if (lpvPtr2 != NULL) memset (lpvPtr2, 0, dwBytes2);

	_digidsbufsize = dwBytes1;
	if (lpvPtr2 != NULL) _digidsbufsize += dwBytes2;
	_digidsbufdata = malloc (_digidsbufsize);
	_digidsbufpos = 0;
	bufdivs = _digidsbufsize/1024;

	hr = IDirectSoundBuffer_Unlock(prim_buf, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2); 

	if (_digidsbufsize%1024) {
		_TRACE("Primary buffer is not multiple of 1024, failed\n");
		return -1;
	}

	if (_mixer_init((_digidsbufsize / (_bits /8)) / bufdivs, _freq,
		_stereo, ((_bits == 16) ? 1 : 0),
		&digi_driver->voices) != 0) {

		      _TRACE("Can not init software mixer\n");
			return -1;
	}

	_mix_some_samples((unsigned long) _digidsbufdata, 0, 1);
	LOCK_FUNCTION (digi_directsound_mixer_callback);
/*	install_int (digi_directsound_mixer_callback, BPS_TO_TIMER(50)); */
	dbgtid = timeSetEvent (20, 10, digi_directsound_mixer_callback, 0, TIME_PERIODIC);
	IDirectSoundBuffer_Play (prim_buf, 0, 0, DSBPLAY_LOOPING);
	IDirectSoundBuffer_SetPan (prim_buf, DSBPAN_CENTER);
	IDirectSoundBuffer_SetVolume (prim_buf, DSBVOLUME_MAX);
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
	if (dbgtid) timeKillEvent (dbgtid);
	if (_digidsbufdata != NULL) { free (_digidsbufdata); _digidsbufdata = NULL; }

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

/* digi_directsound_buffer_size:
 *  return size of buffer in samples
 */
static int digi_directsound_buffer_size(void)
{
	return _digidsbufsize / (_bits / 8) / (_stereo ? 2 : 1);
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

	if (input) return 0;

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
