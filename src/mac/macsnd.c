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
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>
#include <Gestalt.h>

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintmac.h"

#ifndef ALLEGRO_MPW
   #error something is wrong with the makefile
#endif

static int sound_mac_detect(int input);
static int sound_mac_init(int input, int voices);
static void sound_mac_exit(int input);
static int sound_mac_mixer_volume(int volume);
static int sound_mac_buffer_size(void);

static char sb_desc[160] = EMPTY_STRING;

/*difine as nothing to remove trace*/

DIGI_DRIVER digi_macos={               /* driver for playing digital sfx */
   DIGI_MACOS,                        /* driver ID code */
   empty_string,                     /* driver name */
   empty_string,                     /* description string */
   "MacOs Digital",                  /* ASCII format name string */
   0,//intvoices;                     /* available voices */
   0,//intbasevoice;                  /* voice number offset */
   MIXER_MAX_SFX,                  /* maximum voices we can support */
   MIXER_DEF_SFX,                  /* default number of voices to use */

   /* setup routines */
   sound_mac_detect,/*AL_METHOD(int,detect, (int input)); */
   sound_mac_init,/*AL_METHOD(int,init, (int input, int voices)); */
   sound_mac_exit,/*AL_METHOD(void, exit, (int input)); */
   sound_mac_mixer_volume,/*AL_METHOD(int,mixer_volume, (int volume));*/

   /* for use by the audiostream functions */
   NULL,//sound_mac_lock_voice,/*AL_METHOD(void *, lock_voice, (int voice, int start, int end));*/
   NULL,//sound_mac_unlock_voice,/*AL_METHOD(void, unlock_voice, (int voice));*/
   sound_mac_buffer_size,/*AL_METHOD(int,buffer_size, (void));*/

   /* voice control functions */
   _mixer_init_voice,/*   AL_METHOD(void, init_voice, (int voice, AL_CONST SAMPLE *sample));*/
   _mixer_release_voice,/*   AL_METHOD(void, release_voice, (int voice));*/
   _mixer_start_voice,/*   AL_METHOD(void, start_voice, (int voice));*/
   _mixer_stop_voice,/*   AL_METHOD(void, stop_voice, (int voice));*/
   _mixer_loop_voice,/*   AL_METHOD(void, loop_voice, (int voice, int playmode));*/

   /* position control functions */
   _mixer_get_position,/*   AL_METHOD(int,get_position, (int voice));*/
   _mixer_set_position,/*   AL_METHOD(void, set_position, (int voice, int position));*/

   /* volume control functions */
   _mixer_get_volume,/*   AL_METHOD(int,get_volume, (int voice));*/
   _mixer_set_volume,/*   AL_METHOD(void, set_volume, (int voice, int volume));*/
   _mixer_ramp_volume,/*AL_METHOD(void, ramp_volume, (int voice, int time, int endvol));*/
   _mixer_stop_volume_ramp,/*AL_METHOD(void, stop_volume_ramp, (int voice));*/

   /* pitch control functions */
   _mixer_get_frequency,/*   AL_METHOD(int,get_frequency, (int voice));*/
   _mixer_set_frequency,/*   AL_METHOD(void, set_frequency, (int voice, int frequency));*/
   _mixer_sweep_frequency,/*AL_METHOD(void, sweep_frequency, (int voice, int time, int endfreq));*/
   _mixer_stop_frequency_sweep,/*AL_METHOD(void, stop_frequency_sweep, (int voice));*/

   /* pan control functions */
   _mixer_get_pan,/*AL_METHOD(int,get_pan, (int voice));*/
   _mixer_set_pan,/*AL_METHOD(void, set_pan, (int voice, int pan));*/
   _mixer_sweep_pan,/*AL_METHOD(void, sweep_pan, (int voice, int time, int endpan));*/
   _mixer_stop_pan_sweep,/*AL_METHOD(void, stop_pan_sweep, (int voice));*/

   /* effect control functions */
   _mixer_set_echo,/*AL_METHOD(void, set_echo, (int voice, int strength, int delay));*/
   _mixer_set_tremolo,/*AL_METHOD(void, set_tremolo, (int voice, int rate, int depth));*/
   _mixer_set_vibrato,/*AL_METHOD(void, set_vibrato, (int voice, int rate, int depth));*/

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

static int sound_mac_in_use = 0;             /*  */
static int sound_mac_stereo = 0;             /*  */
static int sound_mac_16bit = 0;              /*  */
static int sound_mac_buf_size = 4096;           /*  */
static int sound_mac_freq=44100;
static long _mac_sound=0;
static SndDoubleBackUPP myDBUPP=NULL;
static SndChannelPtr chan=NULL;
static SndDoubleBufferHeader myDblHeader;


#define MAC_MSG_NONE  0
#define MAC_MSG_DONE  1
#define MAC_MSG_CLOSE 2


/* sound_mac_interrupt
 *
 */

static void pascal sound_mac_interrupt( SndChannelPtr chan , SndDoubleBufferPtr doubleBuffer)
{
	if( doubleBuffer->dbUserInfo[0] == MAC_MSG_CLOSE)
	{
		doubleBuffer->dbNumFrames = 1;
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbLastBuffer;
		doubleBuffer->dbUserInfo[0] = MAC_MSG_DONE;
	//	SysBeep(1);
	}
	else
	{
		_mix_some_samples( (long)(&(doubleBuffer->dbSoundData[0])) , 0 , 1);
		doubleBuffer->dbNumFrames = sound_mac_buf_size;
		doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
	}

}
END_OF_STATIC_FUNCTION(sound_mac_interrupt);


/*
 * sound_mac_detect:
 */
static int sound_mac_detect(int input)
{
#define _sound_has_stereo()         (_mac_sound & (1<<gestaltStereoCapability))
#define _sound_has_stereo_mix()     (_mac_sound & (1<<gestaltStereoMixing))
#define _sound_has_input()          (_mac_sound & (1<<gestaltHasSoundInputDevice))
#define _sound_has_play_record()    (_mac_sound & (1<<gestaltPlayAndRecord))
#define _sound_has_16bit()          (_mac_sound & (1<<gestalt16BitSoundIO))
#define _sound_has_stereo_input()   (_mac_sound & (1<<gestaltStereoInput))
#define _sound_has_double_buffer()  (_mac_sound & (1<<gestaltSndPlayDoubleBuffer))
#define _sound_has_multichannels()  (_mac_sound & (1<<gestaltMultiChannels))
#define _sound_has_16BitAudioSupport() (_mac_sound & (1<<gestalt16BitAudioSupport))

	NumVersion myVersion;
	OSErr my_err;
/*    SoundTrace("sound_mac_detect(%d) stereo=%d 16bit=%d freq=&d\n",input,_sound_stereo,_sound_bits,_sound_freq);*/

	if(!input)
	{
		if(RTrapAvailable(_SoundDispatch,ToolTrap)){
			myVersion = SndSoundManagerVersion();
			if(myVersion.majorRev >= 3)
			{
				my_err = Gestalt(gestaltSoundAttr, &_mac_sound);
				if(my_err == noErr)
				{
					if(_sound_has_double_buffer()){
						sound_mac_stereo=_sound_has_stereo()?1:0;/*_sound_stereo*/
						sound_mac_16bit=_sound_has_16bit()?1:0;/*_sound_bits*/
						sound_mac_freq=44100;
						sound_mac_buf_size=1024;
						
/*						SoundTrace("detect sound OK\n stereo=%d 16bit=%d freq=&d\n",sound_mac_stereo,sound_mac_16bit,sound_mac_freq);*/
						return 1;
					}
					else
					{
						usprintf(allegro_error, get_config_text("Sorry your system can't do double buffer sound"));						
					}
				}
				else
				{
					usprintf(allegro_error, get_config_text("Sorry Can't get information about sound"));
				}
			}
			else
			{
				usprintf(allegro_error, get_config_text("Sorry SoundManager 3.0 or later requerid"));
			}
		}
		else
		{
			usprintf(allegro_error, get_config_text("Sorry SoundManager  not Found"));
		}
	}
	else
	{
		usprintf(allegro_error, get_config_text("Input is not supported"));
	}
/*    SoundTrace("Failed \n");*/
	return 0;
}


/* sound_mac_init:
 *  returns zero on success, -1 on failure.
 */
static int sound_mac_init(int input, int voices)
{
	int i;
	OSErr my_err;
	SndDoubleBufferPtr myDblBuffer;
	chan=NULL;

/*	SoundTrace("sound_mac_init(%d,%d)",input,voices);*/
	digi_driver->voices = voices;
	
	if (input)
		return -1;
	
	my_err=SndNewChannel( &chan, sampledSynth , sound_mac_stereo?initStereo:initMono , NULL );
	if( my_err == noErr )
	{
		myDblHeader.dbhNumChannels = sound_mac_stereo?2:1;
		myDblHeader.dbhSampleSize = sound_mac_16bit?16:8;
		myDblHeader.dbhCompressionID = 0;
		myDblHeader.dbhPacketSize = 0;
		myDblHeader.dbhSampleRate = Long2Fix(sound_mac_freq);
		myDBUPP = NewSndDoubleBackUPP ( sound_mac_interrupt );
		myDblHeader.dbhDoubleBack = myDBUPP;

/*		SoundTrace("try 16bit=%d stereo=%d",sound_mac_16bit,sound_mac_stereo);*/

		if (_mixer_init(sound_mac_buf_size+(sound_mac_stereo?sound_mac_buf_size:0) , 44100 , sound_mac_stereo, sound_mac_16bit, &digi_driver->voices) != 0)
			return -1;
			
		for ( i = 0 ; i < 2 ; i++ )
		{
			myDblBuffer = (SndDoubleBufferPtr) NewPtr (sizeof(SndDoubleBuffer) + sound_mac_buf_size*(sound_mac_16bit?2:1)*(sound_mac_stereo?2:1));
			if( myDblBuffer == NULL)
			{
				my_err=MemError();
			}
			else
			{
				myDblBuffer->dbNumFrames = 0;
				myDblBuffer->dbFlags = 0;
				myDblBuffer->dbUserInfo[0] =MAC_MSG_NONE;
				InvokeSndDoubleBackUPP (chan, myDblBuffer , myDBUPP);
				myDblHeader.dbhBufferPtr[i] = myDblBuffer;
			}
		}

		if(my_err==noErr)
		{
		//	SysBeep(1);
			my_err = SndPlayDoubleBuffer(chan, &myDblHeader);

		}
		if (my_err != noErr)
		{
			for( i = 0 ; i < 2 ; i++ )
				if( myDblHeader.dbhBufferPtr[i] != NULL)
				{
					DisposePtr( (Ptr) myDblHeader.dbhBufferPtr[i]);
					myDblHeader.dbhBufferPtr[i]=NULL;
				}
		
			if( myDBUPP != NULL )
			{
				DisposeSndDoubleBackUPP ( myDBUPP );
				myDBUPP=NULL;	
			}
			if( chan!=NULL )
			{
				SndDisposeChannel(chan , true);
				chan=NULL;
			}
			return -1;
		}
		else
		{
/*			SoundTrace("init OK\n");*/
		}
	}
	else
	{
		return -1;
	}
   sound_mac_in_use = TRUE;
   return 0;
}

/* sound_mac_exit:
 * 
 */
static void sound_mac_exit(int input)
{
	if(input)return;

/*    SoundTrace("sound_mac_exit(%d)",input);	*/
/*	_mixer_exit();*/

	if( chan != NULL )
	{
		SCStatus myStatus;
		OSErr my_err;
		int i;
		
		myDblHeader.dbhBufferPtr[0]->dbUserInfo[0] =MAC_MSG_CLOSE;
		myDblHeader.dbhBufferPtr[1]->dbUserInfo[0] =MAC_MSG_CLOSE;

		do
		{
			my_err = SndChannelStatus(chan, sizeof(myStatus), &myStatus);
		}while(myStatus.scChannelBusy);
		
		for( i = 0 ; i < 2 ; i++ )
			if( myDblHeader.dbhBufferPtr[i] != NULL)
			{
				DisposePtr( (Ptr) myDblHeader.dbhBufferPtr[i]);
				myDblHeader.dbhBufferPtr[i]=NULL;
			}
			
		if( myDBUPP != NULL )
		{
			DisposeSndDoubleBackUPP ( myDBUPP );	
			myDBUPP=NULL;
		}

		SndDisposeChannel(chan , true);
		chan=NULL;
	}
   sound_mac_in_use = FALSE;
}

static int sound_mac_buffer_size(){
	return sound_mac_buf_size+(sound_mac_stereo?sound_mac_buf_size:0);
	};

static int sound_mac_mixer_volume(int volume){
/*   SoundTrace("sound_mac_mixer_volume(%d)",volume);*/
   return volume;
};
