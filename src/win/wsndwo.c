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
 *      Allegro mixer to WaveOut driver by Robin Burrows.
 *
 *      Based on original src/unix/uoss.c by Joshua Heyer
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTSOUND_VERSION 0x0300

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"

#ifndef SCAN_DEPEND
   #include <mmsystem.h>
   #include <math.h>

   #ifdef _MSC_VER
      #include <mmreg.h>
   #endif
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


static int digi_waveout_detect(int input);
static int digi_waveout_init(int input, int voices);
static void digi_waveout_exit(int input);
static int digi_waveout_mixer_volume(int volume);
static int digi_waveout_buffer_size(void);


/* sound driver globals */
static HANDLE hWaveOut = NULL;
static LPWAVEHDR lpWaveHdr = NULL;
static int digiwobufsize, digiwobufdivs, digiwobufpos;
static unsigned char * digiwobufdata = NULL;
static int _freq, _bits, _stereo;

static UINT dbgtid;

static DIGI_DRIVER digi_waveout =
{
   0,
   empty_string,
   empty_string,
   empty_string,
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   digi_waveout_detect,
   digi_waveout_init,
   digi_waveout_exit,
   digi_waveout_mixer_volume,

   NULL,
   NULL,
   digi_waveout_buffer_size,
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


/* This is the callback to update sound in waveout buffer */
static void CALLBACK digi_waveout_mixer_callback(
  UINT uID,      
  UINT uMsg,     
  DWORD dwUser,  
  DWORD dw1,     
  DWORD dw2)
{ 
MMTIME mmt;
int writecurs;

	memset (&mmt, 0, sizeof (MMTIME));
	mmt.wType = TIME_BYTES;
	if (waveOutGetPosition (hWaveOut, &mmt, sizeof(MMTIME)) == MMSYSERR_NOERROR) {
		writecurs = (int) mmt.u.cb;
		writecurs /= (digiwobufsize/digiwobufdivs);
		writecurs += 8; while (writecurs > (digiwobufdivs-1)) writecurs -= digiwobufdivs;

		while (writecurs != digiwobufpos) {
			_mix_some_samples((unsigned long)
				(digiwobufdata+((digiwobufsize/digiwobufdivs)*digiwobufpos)), 0, 1);
			digiwobufpos++; if (digiwobufpos > (digiwobufdivs-1)) digiwobufpos = 0;
		}
	}
	return;
} 
END_OF_STATIC_FUNCTION (digi_waveout_mixer_callback);



/* _get_digi_driver_list:
 *  System driver hook for listing the available sound drivers. This 
 *  generates the device list at runtime, to match whatever DirectSound
 *  devices are available.
 */
DIGI_DRIVER *_get_woalmix_driver(int num)
{
DIGI_DRIVER *driver;

	driver = malloc(sizeof(DIGI_DRIVER));
	if (driver == NULL) return NULL;

	memcpy(driver, &digi_waveout, sizeof(DIGI_DRIVER));

	driver->id = DIGI_WAVOUTID(num);

	if (num == 0) driver->ascii_name = "WaveOut 44100khz 16bit stereo";
	else driver->ascii_name = "WaveOut 22050khz 8bit mono";

	return driver;
}




/* digi_waveout_init:
 *  Initialize driver                                                                
 *
 */
static int digi_waveout_init(int input, int voices)
{
MMRESULT mmres;
WAVEFORMATEX format;
int v, id;

	digi_driver->voices = voices;

	/* deduce our device number from the driver ID code */
	id = ((digi_driver->id >> 8) & 0xFF) - 'A';

	digiwobufdivs = 32; digiwobufpos = 0;
	/* get hardware capabilities */
	if (id == 0) {
		_bits = 16; _freq = 44100; _stereo = 1;
		digiwobufsize = digiwobufdivs*1024;
	} else {
		_bits = 8; _freq = 22050; _stereo = 0;
		digiwobufsize = digiwobufdivs*512;
	}

	digiwobufdata = malloc (digiwobufsize);
	if (digiwobufdata == NULL) {
		_TRACE ("malloc failed\n");
		return -1;
	}

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = _stereo ? 2 : 1;
	format.nSamplesPerSec = _freq;
	format.wBitsPerSample = _bits;
	format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	mmres = waveOutOpen (&hWaveOut, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL );
	if (mmres != MMSYSERR_NOERROR) {
		_TRACE ("Can't open WaveOut\n");
		return -1;
	}

	lpWaveHdr = malloc (sizeof (WAVEHDR));
	lpWaveHdr->lpData = digiwobufdata;;
	lpWaveHdr->dwBufferLength = digiwobufsize;
	lpWaveHdr->dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
	lpWaveHdr->dwLoops = 0x7fffffffL;
	if (waveOutPrepareHeader (hWaveOut, lpWaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		_TRACE ("waveOutPrepare failed\n" );
	} 

	if (waveOutWrite (hWaveOut, lpWaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		_TRACE ("waveOutWrite failed\n" );
	} 

	if (_mixer_init((digiwobufsize / (_bits /8)) / digiwobufdivs, _freq,
		_stereo, ((_bits == 16) ? 1 : 0),
		&digi_driver->voices) != 0) {
		      _TRACE("Can not init software mixer\n");
			return -1;
	}

	_mix_some_samples((unsigned long) digiwobufdata, 0, 1);
	LOCK_FUNCTION (digi_waveout_mixer_callback);
/*	install_int (digi_waveout_mixer_callback, BPS_TO_TIMER(50)); */
	dbgtid = timeSetEvent (20, 10, digi_waveout_mixer_callback, 0, TIME_PERIODIC);

	return 0;
}



/* digi_waveout_exit:
 *  Closes WaveOut
 */
static void digi_waveout_exit(int input)
{
	if (dbgtid) timeKillEvent (dbgtid);

	if (hWaveOut != NULL) {
		waveOutReset (hWaveOut);
		waveOutUnprepareHeader (hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
		waveOutClose (hWaveOut);
		hWaveOut = NULL;
	}

	if (lpWaveHdr != NULL) { free (lpWaveHdr); lpWaveHdr = NULL; }
	if (digiwobufdata != NULL) { free (digiwobufdata); digiwobufdata = NULL; }
}

/* digi_waveout_buffer_size:
 *  return size of buffer in samples
 */
static int digi_waveout_buffer_size(void)
{
	return digiwobufsize / (_bits / 8) / (_stereo ? 2 : 1);
}


/* digi_directsound_mixer_volume:
 *  Sets mixer volume
 */
static int digi_waveout_mixer_volume(int volume)
{
DWORD realvol;

	if (hWaveOut != NULL) {
		realvol = (DWORD) volume;
		realvol |= realvol<<8;
		realvol |= realvol<<16;
		waveOutSetVolume (hWaveOut, realvol);
	}
	return 0;
}



/* digi_waveout_detect:
 *  Detects Waveout.
 */
static int digi_waveout_detect(int input)
{
	if (input) return 0;

	return 1;
}
