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
 *      BeOS sound driver implementation.
 *
 *      By Peter Wang.   
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/aintern.h"
#include "allegro/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif

#ifndef SCAN_DEPEND
#include <game/GameSoundDefs.h>
#endif


#ifdef ALLEGRO_BIG_ENDIAN
   #define BE_SOUND_ENDIAN	 B_MEDIA_BIG_ENDIAN
#else
   #define BE_SOUND_ENDIAN	 B_MEDIA_LITTLE_ENDIAN
#endif


#define BE_SOUND_THREAD_PERIOD	    33333   /* 1/30 second */
#define BE_SOUND_THREAD_NAME	    "sound thread"
#define BE_SOUND_THREAD_PRIORITY    80



static volatile int be_sound_thread_running = FALSE;
static thread_id be_sound_thread_id = -1;

static BLocker *locker;
static BPushGameSound *be_sound;

static int be_sound_bufsize;
static unsigned char *be_sound_bufdata;
static int be_sound_signed;

static char be_sound_desc[320] = EMPTY_STRING;

static bool be_sound_active;



/* be_sound_thread:
 *  Update data.
 */
static int32 be_sound_thread(void *sound_started)
{
   char *p;
   size_t size;
   
   release_sem(*(sem_id *)sound_started);

   while (be_sound_thread_running) {

      locker->Lock();      
      if (be_sound->LockNextPage((void **)&p, &size) >= 0) {
	 memcpy(p, be_sound_bufdata, size);
	 be_sound->UnlockPage(p);
	 if (be_sound_active)
	    _mix_some_samples((unsigned long) be_sound_bufdata, 0, be_sound_signed);
      }
      locker->Unlock();
      
      snooze(BE_SOUND_THREAD_PERIOD);
   }

   return 0;
}
      


/* be_sound_detect.
 *  Return TRUE if sound available.
 */
extern "C" int be_sound_detect(int input)
{   
   BPushGameSound *sound;
   gs_audio_format format;
   status_t status;
   
   if (input) {
      usprintf(allegro_error, get_config_text("Input is not supported"));
      return FALSE;
   }

   format.frame_rate = 11025;
   format.channel_count = 1;
   format.format = gs_audio_format::B_GS_U8;
   format.byte_order = BE_SOUND_ENDIAN;
   format.buffer_size = 0;
   
   sound = new BPushGameSound(128, &format);
   status = sound->InitCheck();
   delete sound;
   
   return (status == B_OK) ? TRUE : FALSE;
}



/* be_sound_init:
 *  Init sound driver.
 */
extern "C" int be_sound_init(int input, int voices)
{
   sem_id sound_started;
   gs_audio_format fmt;
   size_t samples;
   char tmp1[80], tmp2[80];

   if (input) {
      usprintf(allegro_error, get_config_text("Input is not supported"));
      return -1;
   }

   /* create BPushGameSound instance */
   fmt.frame_rate    = (_sound_freq > 0) ? _sound_freq : 44100;
   fmt.channel_count = (_sound_stereo) ? 2 : 1; 
   fmt.format	     = (_sound_bits == 8) ? (gs_audio_format::B_GS_U8) : (gs_audio_format::B_GS_S16);
   fmt.byte_order    = BE_SOUND_ENDIAN;  
   fmt.buffer_size   = 256;

   /* this might need to be user configurable */
   samples = (int)fmt.frame_rate * 512 / 11025;
   
   be_sound = new BPushGameSound(samples, &fmt);
   
   if (be_sound->InitCheck() != B_OK) {
      goto cleanup;
   }

   /* read the sound format back */
   fmt = be_sound->Format();

   switch (fmt.format) {
       
      case gs_audio_format::B_GS_U8:
	 _sound_bits = 8;
	 be_sound_signed = FALSE;
	 break;
	 
      case gs_audio_format::B_GS_S16:
	 _sound_bits = 16;
	 be_sound_signed = TRUE;
	 break;

      default:
	 goto cleanup;
   }
      
   _sound_stereo = (fmt.channel_count == 2) ? 1 : 0;
   _sound_freq = (int)fmt.frame_rate;

   /* allocate mixing buffer */
   be_sound_bufsize = samples * (_sound_stereo ? 2 : 1) * (_sound_bits / 8);
   be_sound_bufdata = (unsigned char *)malloc(be_sound_bufsize);

   if (!be_sound_bufdata) {
      goto cleanup;
   }

   /* start internal mixer */
   digi_beos.voices = voices;
   
   if (_mixer_init(be_sound_bufsize / (_sound_bits / 8), _sound_freq,
		   _sound_stereo, ((_sound_bits == 16) ? 1 : 0),
		   &digi_beos.voices) != 0) {
      goto cleanup;
   }

   _mix_some_samples((unsigned long) be_sound_bufdata, 0, be_sound_signed);

   /* start audio thread */
   sound_started = create_sem(0, "starting sound thread...");

   if (sound_started < B_NO_ERROR) {
      goto cleanup;
   }
   
   locker = new BLocker();
   if (!locker)
      goto cleanup;
   be_sound_active = true;

   be_sound_thread_id = spawn_thread(be_sound_thread, BE_SOUND_THREAD_NAME,
  				     BE_SOUND_THREAD_PRIORITY, &sound_started);

   if (be_sound_thread_id < 0) {
      goto cleanup;
   }

   be_sound_thread_running = TRUE;
   resume_thread(be_sound_thread_id);
   acquire_sem(sound_started);
   delete_sem(sound_started);

   be_sound->StartPlaying();

   usprintf(be_sound_desc, get_config_text("%d bits, %s, %d bps, %s"),
			      _sound_bits, uconvert_ascii((char *)(be_sound_signed ? "signed" : "unsigned"), tmp1), 
			      _sound_freq, uconvert_ascii((char *)(_sound_stereo ? "stereo" : "mono"), tmp2));

   digi_driver->desc = be_sound_desc;
   
   return 0;

   cleanup: {
      if (sound_started > 0) {
	 delete_sem(sound_started);
      }

      be_sound_exit(input);
      return -1;
   }
}



/* be_sound_exit:
 *  Shutdown sound driver.
 */
extern "C" void be_sound_exit(int input)
{
   if (input) {
      return;
   }

   if (be_sound_thread_id > 0) {
      be_sound_thread_running = FALSE;
      wait_for_thread(be_sound_thread_id, &ignore_result);
      be_sound_thread_id = -1;
   }

   if (be_sound_bufdata) {
      free(be_sound_bufdata);
      be_sound_bufdata = NULL;
   }
   
   _mixer_exit();
   
   delete be_sound;
   delete locker;
   locker = NULL;
}



/* be_sound_buffer_size:
 *  Returns the current buffer size, for use by the audiostream code.
 */
extern "C" int be_sound_buffer_size()
{
   return be_sound_bufsize / (_sound_bits / 8) / (_sound_stereo ? 2 : 1);
}



/* be_sound_mixer_volume:
 *  Set mixer volume.
 */
extern "C" int be_sound_mixer_volume(int volume)
{
   if (!be_sound)
      return -1;
   
   return (be_sound->SetGain((float)volume / 255.0) == B_OK);
}



/* be_sound_suspend:
 *  Pauses the sound thread.
 */
extern "C" void be_sound_suspend(void)
{
   if (!be_sound_thread_running)
      return;
   locker->Lock();
   be_sound_active = false;
   memset(be_sound_bufdata, 0, be_sound_bufsize);
   locker->Unlock();
}



/* be_sound_resume:
 *  Resumes the sound thread.
 */
extern "C" void be_sound_resume(void)
{
   if (!be_sound_thread_running)
      return;
   locker->Lock();
   be_sound_active = true;
   locker->Unlock();
}