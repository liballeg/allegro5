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
 *      Mac sound support.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */


#include <Gestalt.h>
#include <Sound.h>

#include "allegro.h"
#include "allegro/aintmac.h"

#ifndef ALLEGRO_MPW
   #error something is wrong with the makefile
#endif

static int sound_mac_detect(int input);
static int sound_mac_init(int input, int voices);
static void sound_mac_exit(int input);
static int sound_mac_mixer_volume(int volume);
static int sound_mac_buffer_size(void);


DIGI_DRIVER digi_macos={
   DIGI_MACOS,
   empty_string,
   empty_string,
   "Apple SoundManager",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   /* setup routines */
   sound_mac_detect,
   sound_mac_init,
   sound_mac_exit,
   sound_mac_mixer_volume,

   /* for use by the audiostream functions */
   NULL,
   NULL,
   sound_mac_buffer_size,

   /* voice control functions */
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,

   /* position control functions */
   _mixer_get_position,
   _mixer_set_position,

   /* volume control functions */
   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,

   /* pitch control functions */
   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,

   /* pan control functions */
   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,

   /* effect control functions */
   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,

   /* input functions */
   0,/*int rec_cap_bits;*/
   0,/*int rec_cap_stereo;*/
   NULL,/*AL_METHOD(int,rec_cap_rate, (int bits, int stereo));*/
   NULL,/*AL_METHOD(int,rec_cap_parm, (int rate, int bits, int stereo));*/
   NULL,/*AL_METHOD(int,rec_source, (int source));*/
   NULL,/*AL_METHOD(int,rec_start, (int rate, int bits, int stereo));*/
   NULL,/*AL_METHOD(void, rec_stop, (void));*/
   NULL,/*AL_METHOD(int,rec_read, (void *buf));*/
};

static int sound_mac_in_use = 0;
static int sound_mac_stereo = 0;
static int sound_mac_16bit = 0;
static int sound_mac_buf_size = 4096;
static int sound_mac_freq=44100;
static long _mac_sound=0;
static char sound_mac_desc[256]=EMPTY_STRING;
static SndDoubleBackUPP myDBUPP=NULL;
static SndChannelPtr chan=NULL;
static SndDoubleBufferHeader myDblHeader;


#define MAC_MSG_NONE  0
#define MAC_MSG_DONE  1
#define MAC_MSG_CLOSE 2



/* sound_mac_interrupt
 *
 */
static void pascal sound_mac_interrupt(SndChannelPtr chan, SndDoubleBufferPtr doubleBuffer)
{
   if(doubleBuffer->dbUserInfo[0] == MAC_MSG_CLOSE){
      doubleBuffer->dbNumFrames = 1;
      doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
      doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbLastBuffer;
      doubleBuffer->dbUserInfo[0] = MAC_MSG_DONE;
   }
   else{
      _mix_some_samples((long)(&(doubleBuffer->dbSoundData[0])), 0, 1);
      doubleBuffer->dbNumFrames = sound_mac_buf_size;
      doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
   }
}
END_OF_STATIC_FUNCTION(sound_mac_interrupt);



/*
 * sound_mac_detect:
 */
static int sound_mac_detect(int input){
#define _sound_has_stereo()	 (_mac_sound & (1<<gestaltStereoCapability))
#define _sound_has_stereo_mix()     (_mac_sound & (1<<gestaltStereoMixing))
#define _sound_has_input()	  (_mac_sound & (1<<gestaltHasSoundInputDevice))
#define _sound_has_play_record()    (_mac_sound & (1<<gestaltPlayAndRecord))
#define _sound_has_16bit()	  (_mac_sound & (1<<gestalt16BitSoundIO))
#define _sound_has_stereo_input()   (_mac_sound & (1<<gestaltStereoInput))
#define _sound_has_double_buffer()  (_mac_sound & (1<<gestaltSndPlayDoubleBuffer))
#define _sound_has_multichannels()  (_mac_sound & (1<<gestaltMultiChannels))
#define _sound_has_16BitAudioSupport() (_mac_sound & (1<<gestalt16BitAudioSupport))

   NumVersion myVersion;
   OSErr my_err;

   if(!input){
      if(RTrapAvailable(_SoundDispatch,ToolTrap)){
	 myVersion = SndSoundManagerVersion();
	 if(myVersion.majorRev >= 3){
	    my_err = Gestalt(gestaltSoundAttr, &_mac_sound);
	    if(my_err == noErr){
	       if(_sound_has_double_buffer()){
		  sound_mac_stereo=_sound_has_stereo()?1:0;/*_sound_stereo*/
		  sound_mac_16bit=_sound_has_16bit()?1:0;/*_sound_bits*/
		  sound_mac_freq=44100;
		  _sound_freq=sound_mac_freq;
		  sound_mac_buf_size=1024;
                  uszprintf(sound_mac_desc, sizeof(sound_mac_desc),
		     get_config_text("Apple SoundManager %d.x %d hz, %d-bit, %s"),
		     myVersion.majorRev,sound_mac_freq,
		     sound_mac_16bit?16:8,sound_mac_stereo?"stereo":"mono");
		  digi_driver->desc=sound_mac_desc;
                  return 1;
	       }
	       else{
		  ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Sorry your system can't do double buffer sound"));		  
	       }
	    }
	    else{
	       ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Sorry Can't get information about sound"));
	    }
	 }
	 else{
	    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Sorry SoundManager 3.0 or later requerid"));
	 }
      }
      else{
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Sorry SoundManager  not Found"));
      }
   }
   else{
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
   }
   return 0;
}



/* sound_mac_init:
 *  returns zero on success, -1 on failure.
 */
static int sound_mac_init(int input, int voices){
   int i;
   OSErr my_err;
   SndDoubleBufferPtr myDblBuffer;
   chan=NULL;
   digi_driver->voices = voices;
   if(input)
      return -1;
   
   my_err=SndNewChannel(&chan, sampledSynth, sound_mac_stereo?initStereo:initMono, NULL);
   if(my_err == noErr){
      myDblHeader.dbhNumChannels = sound_mac_stereo?2:1;
      myDblHeader.dbhSampleSize = sound_mac_16bit?16:8;
      myDblHeader.dbhCompressionID = 0;
      myDblHeader.dbhPacketSize = 0;
      myDblHeader.dbhSampleRate = Long2Fix(sound_mac_freq);
      myDBUPP = NewSndDoubleBackUPP (sound_mac_interrupt);
      myDblHeader.dbhDoubleBack = myDBUPP;

      if (_mixer_init(sound_mac_buf_size+(sound_mac_stereo?sound_mac_buf_size:0),
                      sound_mac_freq, sound_mac_stereo,
		      sound_mac_16bit, &digi_driver->voices) != 0){
         SndDisposeChannel(chan , true);
	 chan=NULL;
	 return -1;
      }
      _sound_freq=sound_mac_freq;
      for (i = 0; i < 2; i++){
	 myDblBuffer = (SndDoubleBufferPtr)NewPtr(sizeof(SndDoubleBuffer) + sound_mac_buf_size*(sound_mac_16bit?2:1)*(sound_mac_stereo?2:1));
	 if( myDblBuffer == NULL){
	    my_err=MemError();
	 }
	 else{
	    myDblBuffer->dbNumFrames = 0;
	    myDblBuffer->dbFlags = 0;
	    myDblBuffer->dbUserInfo[0] =MAC_MSG_NONE;
	    InvokeSndDoubleBackUPP (chan, myDblBuffer , myDBUPP);
	    myDblHeader.dbhBufferPtr[i] = myDblBuffer;
	 }
      }

      if(my_err==noErr)
	 my_err = SndPlayDoubleBuffer(chan, &myDblHeader);
      if (my_err != noErr){
	 for( i = 0 ; i < 2 ; i++ )
	    if( myDblHeader.dbhBufferPtr[i] != NULL){
	       DisposePtr((Ptr) myDblHeader.dbhBufferPtr[i]);
	       myDblHeader.dbhBufferPtr[i]=NULL;
	    }
      
	 if( myDBUPP != NULL ){
	    DisposeSndDoubleBackUPP (myDBUPP);
	    myDBUPP=NULL;
	 }
	 if( chan!=NULL ){
	    SndDisposeChannel(chan , true);
	    chan=NULL;
	 }
	 return -1;
      }
   }
   else
      return -1;
   sound_mac_mixer_volume(255);
   sound_mac_in_use = TRUE;
   return 0;
}



/* sound_mac_exit:
 * 
 */
static void sound_mac_exit(int input)
{
   if(input)return;

/*   _mixer_exit();*/

   if( chan != NULL ){
      SCStatus myStatus;
      OSErr my_err;
      int i;
      
      myDblHeader.dbhBufferPtr[0]->dbUserInfo[0] =MAC_MSG_CLOSE;
      myDblHeader.dbhBufferPtr[1]->dbUserInfo[0] =MAC_MSG_CLOSE;

      do
	 my_err = SndChannelStatus(chan, sizeof(myStatus), &myStatus);
      while(myStatus.scChannelBusy);
      
      for( i = 0 ; i < 2 ; i++ )
	 if( myDblHeader.dbhBufferPtr[i] != NULL){
	    DisposePtr( (Ptr) myDblHeader.dbhBufferPtr[i]);
	    myDblHeader.dbhBufferPtr[i]=NULL;
	 }
	 
      if( myDBUPP != NULL ){
	 DisposeSndDoubleBackUPP ( myDBUPP );   
	 myDBUPP=NULL;
      }

      SndDisposeChannel(chan , true);
      chan=NULL;
   }
   sound_mac_in_use = FALSE;
}



/*
 *
 */
static int sound_mac_buffer_size(void)
{
   return sound_mac_buf_size+(sound_mac_stereo?sound_mac_buf_size:0);
}



/*
 *
 */
static int sound_mac_mixer_volume(int volume)
{
   SndCommand	theCommand;
   OSErr	e;
   if(chan){
      theCommand.cmd = volumeCmd;
      theCommand.param1 = 0;
      theCommand.param2 = (long)(volume+(volume<<16));
      e = SndDoImmediate(chan, &theCommand);
   }
   return 0;
}
