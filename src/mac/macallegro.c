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
 *      Mac drivers.
 *
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */

#pragma mark Top of document 

#include "allegro.h"
#include "allegro/aintern.h"
#include "macalleg.h"
#include "allegro/aintmac.h"
#include <string.h>

#pragma mark Debug control

/*
Used for Debug proposites. log the calls to stdout
0 Disable
1 Enable
*/

#define TRACE_MAC_FILE 0
#define TRACE_MAC_GFX 0
#define TRACE_MAC_KBRD 0
#define TRACE_MAC_MOUSE 0
#define TRACE_MAC_SYSTEM 0
#define TRACE_MAC_SOUND 0

/*
Macro used for detect call from routine at interrupt time <drivers callbacks>.
This can buggy a program in a mac
*/

#define CRITICAL() if(criticalarea)DebugStr("\pCritical call was encontred")

#pragma mark Constants

const RGBColor ForeDef={0,0,0};
const RGBColor BackDef={0xFFFF,0xFFFF,0xFFFF};

#pragma mark Globals Variables

/*The our QuickDraw globals */
QDGlobals qd;

/*app name*/
Str255 mname="\0";

/*main volume number*/
short MainVol;

/*main directory ID*/
long MainDir;

/*Our main display device the display which contains the menubar on macs with more one display*/
GDHandle MainGDevice;

/*Our main Color Table for indexed devices*/
CTabHandle MainCTable=NULL;

/*Our current deph*/
short DrawDepth;

/*Vsync has ocurred*/
volatile short _sync=0;

/*Vsync handler installed?*/
short _syncOk=0;

/*???Used for DrawSprocket e Vsync callback*/
const char refconst[16];

/**/
short _mackeyboardinstalled=FALSE;

/*Used for our keyboard driver*/
volatile KeyMap KeyNow;
volatile KeyMap KeyOld;

/*Used to detect interrupt time calls*/
volatile int criticalarea=0;

/**/
short _mactimerinstalled=FALSE;

/**/
short _macmouseinstalled=FALSE;


/*Our TimerManager task entry queue*/
TMTask	theTMTask;		
/*Our TM task is running*/
short _timer_running=0;

/*Number of macvoices*/
#define NMVOICES 16
/*the mac voices*/
mac_voice mac_voices[NMVOICES];
/**/
int mixer_vol=255;

/*the control State of DrawSprocket*/
short State=0;

enum{kRDDNull	=0,
	kRDDStarted	=1,
	kRDDReserved=2,
	kRDDFadeOut	=4,
	kRDDActive	=8,
	kRDDPaused	=16,
	kRDDUnder	=32,
	kRDDOver	=64,
	kRDDouble	=128,
	};
	
/*Our DrawSprocket context*/
DSpContextReference	theContext;
/*Our Buffer Underlayer Not Used Yet*/
DSpAltBufferReference	theUnderlay;


/*Our window title ???*/
char macwindowtitle[256];

/*char buffer for Trace*/
char macsecuremsg[256];


#pragma mark Key tables
/*
The LookUp table to translate AppleKeyboard scan codes to Allegro KEY constants
*/

const unsigned char key_apple_to_allegro[128]={
/*00*/	KEY_X,		KEY_Z,		KEY_G,		KEY_H,
/*04*/	KEY_F,		KEY_D,		KEY_S,		KEY_A,	
/*08*/	KEY_R,		KEY_E,		KEY_W,		KEY_Q,
/*0C*/	KEY_B,		KEY_TILDE,	KEY_V,		KEY_C,
/*10*/	KEY_5,		KEY_6,		KEY_4,		KEY_3,
/*14*/	KEY_2,		KEY_1,		KEY_T,		KEY_Y,
/*18*/	KEY_O,		KEY_CLOSEBRACE,KEY_0,	KEY_8,
/*1C*/	KEY_MINUS,	KEY_7,		KEY_9,		KEY_EQUALS,
/*20*/	KEY_QUOTE,	KEY_J,		KEY_L,		KEY_ENTER,
/*24*/	KEY_P,		KEY_I,		KEY_OPENBRACE,KEY_U,
/*28*/	KEY_STOP,	KEY_M,		KEY_N,		KEY_SLASH,
/*2C*/	KEY_COMMA,	KEY_BACKSLASH,KEY_COLON,KEY_K,
/*30*/	KEY_LWIN,	0,			KEY_ESC,	0,	
/*34*/	KEY_BACKSPACE,KEY_BACKSLASH2,KEY_SPACE,	KEY_TAB,
/*38*/	0,			0,			0,			0,	
/*3C*/	KEY_LCONTROL,KEY_ALT,	KEY_CAPSLOCK,KEY_LSHIFT,
/*40*/	KEY_NUMLOCK,0,			KEY_PLUS_PAD,0,	
/*44*/	KEY_ASTERISK,0,			0,			0,	
/*48*/	0,			KEY_MINUS_PAD,0,		0,	
/*4C*/	KEY_SLASH_PAD,0,		0,			0,	
/*50*/	KEY_5_PAD,	KEY_4_PAD,	KEY_3_PAD,	KEY_2_PAD,
/*54*/	KEY_1_PAD,	KEY_0_PAD,	0,			0,	
/*58*/	0,			0,			0,			KEY_9_PAD,
/*5C*/	KEY_8_PAD,	0,			0,			0,	
/*60*/	KEY_F11,	0,			KEY_F9,		KEY_F8,
/*64*/	KEY_F3,		KEY_F7,		KEY_F6,		KEY_F5,
/*68*/	KEY_F12,	0,			KEY_F10,	0,	
/*6C*/	KEY_SCRLOCK,0,			0,			0,	
/*70*/	KEY_END,	KEY_F4,		KEY_DEL,	KEY_PGUP,
/*74*/	KEY_HOME,	KEY_INSERT,	0,			0,	
/*78*/	0,			KEY_UP,		KEY_DOWN,	KEY_RIGHT,
/*7C*/	KEY_LEFT,	KEY_F1,		KEY_PGDN,	KEY_F2,
};

/*
The LookUp table to translate AppleKeyboard scan codes to lowcase ASCII chars
*/

const unsigned char key_apple_to_char[128]={
/*00*/	'x',		'z',		'g',		'h',
/*04*/	'f',		'd',		's',		'a',	
/*08*/	'r',		'e',		'w',		'q',
/*0C*/	'b',		'~',		'v',		'c',
/*10*/	'5',		'6',		'4',		'3',
/*14*/	'2',		'1',		't',		'y',
/*18*/	'o',		']',		'0',		'8',
/*1C*/	'-',		'7',		'9',		'=',
/*20*/	' ',		'j',		'l',		0xD,
/*24*/	'p',		'i',		'[',		'u',
/*28*/	'.',		'm',		'n',		'/',
/*2C*/	',',		'\\',		':',		'k',
/*30*/	0,			0,			27,			0,	
/*34*/	8,			'\\',		' ',		9,
/*38*/	0,			0,			0,			0,	
/*3C*/	0,			0,			0,			0,
/*40*/	0,			0,			'+',		0,	
/*44*/	'*',		0,			0,			0,	
/*48*/	0,			'-',		0,			0,	
/*4C*/	'/',		0,			0,			0,	
/*50*/	'5',		'4',		'3',		'2',
/*54*/	'1',		'0',		0,			0,	
/*58*/	0,			0,			0,			'9',
/*5C*/	'8',		0,			0,			0,	
/*60*/	0,			0,			0,			0,
/*64*/	0,			0,			0,			0,
/*68*/	0,			0,			0,			0,	
/*6C*/	0,			0,			0,			0,	
/*70*/	0,			0,			127,		0,
/*74*/	0,			0,			0,			0,	
/*78*/	0,			0,			0,			0,
/*7C*/	0,			0,			0,			0,
};

#pragma mark todo
/*
Todo LookUp table to translate AppleKeyboard scan codes to upcase ASCII chars
	and diacrilic chars
*/

#pragma mark driverlists

_DRIVER_INFO _system_driver_list[2] ={
		{SYSTEM_MACOS, &system_macos, TRUE},
		{0, NULL, 0}
	};
_DRIVER_INFO _timer_driver_list[] ={
		{TIMER_MACOS, &timer_macos, TRUE},
		{0, NULL, 0}
	};
_DRIVER_INFO _mouse_driver_list[] ={
		{MOUSE_MACOS, &mouse_macos, TRUE},
		{0, NULL, 0}
	};
_DRIVER_INFO _keyboard_driver_list[] ={
		{KEYBOARD_MACOS, &keyboard_macos, TRUE},
		{0, NULL, 0}
	};
_DRIVER_INFO _gfx_driver_list[] ={
		{GFX_DRAWSPROCKET, &gfx_drawsprocket, TRUE},
		{0, NULL, 0}
	};
_DRIVER_INFO _digi_driver_list[]={
		{DIGI_MACOS,&digi_macos,TRUE},
		{0,NULL,0}
		};
_DRIVER_INFO _midi_driver_list[]={
		{MIDI_NONE,&midi_none,TRUE},
		{0,NULL,0}};
_DRIVER_INFO _joystick_driver_list[]={
		{JOY_TYPE_NONE,&joystick_none,TRUE},
		{0,NULL,0}};

#pragma mark SYSTEM_DRIVER
SYSTEM_DRIVER system_macos ={
		SYSTEM_MACOS,
		empty_string,						/* char *name; */
		empty_string,						/* char *desc; */
		"MacOs",							/* char *ascii_name; */
		_mac_init,
		_mac_exit,
		_mac_get_executable_name,
		NULL,								/* AL_METHOD(int, find_resource, (char *dest, char *resource, int size)); */
		_mac_set_window_title,
		_mac_message,
		_mac_assert,
		NULL,//_mac_save_console_state,
		NULL,//_mac_restore_console_state,
		NULL,								/* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
		NULL,								/* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
		NULL,								/* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
		NULL,								/* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp)); */
		NULL,								/* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
		NULL,								/* AL_METHOD(void, read_hardware_palette, (void)); */
		NULL,								/* AL_METHOD(void, set_palette_range, (struct RGB *p, int from, int to, int vsync)); */
		NULL,								/* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
		NULL,								/* AL_METHOD(int, set_display_switch_mode, (int mode));*/
		NULL,								/* AL_METHOD(int, set_display_switch_callback, (int dir, AL_METHOD(void, cb, (void))));*/
		NULL,								/* AL_METHOD(void, remove_display_switch_callback, (AL_METHOD(void, cb, (void))));*/
		NULL,								/* AL_METHOD(void, display_switch_lock, (int lock, int foreground));*/
		_mac_desktop_color_depth,
		_mac_yield_timeslice,
		NULL,								/*AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void));*/
		NULL,								/*AL_METHOD(_DRIVER_INFO *, digi_drivers, (void));*/
		NULL,								/*AL_METHOD(_DRIVER_INFO *, midi_drivers, (void));*/
		NULL,								/*AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void));*/
		NULL,								/*AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void));*/
		NULL,								/*AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void));*/
		NULL,								/*AL_METHOD(_DRIVER_INFO *, timer_drivers, (void));*/
	};

#pragma mark MOUSE_DRIVER
MOUSE_DRIVER mouse_macos ={
		MOUSE_MACOS,
		empty_string,
		empty_string,
		"MacOs Mouse",
		mouse_mac_init,
		mouse_mac_exit,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};

#pragma mark GFX_DRIVER
GFX_DRIVER gfx_drawsprocket ={
	GFX_DRAWSPROCKET,
	"",							//AL_CONST char *name;
	"",							//AL_CONST char *desc;
	"DrawSprocket",				//AL_CONST char *ascii_name;
	init_drawsprocket,			//AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
	exit_drawsprocket,			//AL_METHOD(void, exit, (struct BITMAP *b));
	NULL,						//AL_METHOD(int, scroll, (int x, int y));
	vsync_drawsprocket,			//AL_METHOD(void, vsync, (void));
	set_palette_drawsprocket,	//AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync));
	NULL,						//AL_METHOD(int, request_scroll, (int x, int y));
	NULL,						//AL_METHOD(int, poll_scroll, (void));
	NULL,						//AL_METHOD(void, enable_triple_buffer, (void));
	NULL,						//AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
	NULL,						//AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
	NULL,						//AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
	NULL,						//AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
	_mac_create_system_bitmap,	//AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
	_mac_destroy_system_bitmap,	//AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
	NULL,						//AL_METHOD(int, set_mouse_sprite, (AL_CONST struct BITMAP *sprite, int xfocus, int yfocus));
	NULL,						//AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
	NULL,						//AL_METHOD(void, hide_mouse, (void));
	NULL,						//AL_METHOD(void, move_mouse, (int x, int y));
	NULL,						//AL_METHOD(void, drawing_mode, (void));
	NULL,						//AL_METHOD(void, save_video_state, (void));
	NULL,						//AL_METHOD(void, restore_video_state, (void));
	640, 480,					// int w, h			/* physical (not virtual!) screen size */
	TRUE,						// int linear		/* true if video memory is linear */
	0,							// long bank_size;	/* bank size, in bytes */
	0,							// long bank_gran;	/* bank granularity, in bytes */
	0,							// long vid_mem;	/* video memory size, in bytes */
	0,							// long vid_phys_base;/* physical address of video memory */
};
#pragma mark KEYBOARD_DRIVER
KEYBOARD_DRIVER keyboard_macos ={
	KEYBOARD_MACOS,
	0,							//
	0,							//
	"MacOs Key",				//
	0,							//
	key_mac_init,				//
	key_mac_exit,				//
	NULL, NULL, NULL,			//
	NULL,						//
	NULL,						//
	NULL						//
	};

#pragma mark TIMER_DRIVER
TIMER_DRIVER timer_macos ={
		TIMER_MACOS,
		empty_string,
		empty_string,
		"MacOs Timer",
		mac_timer_init,
		mac_timer_exit,
		NULL,			//mac_timer_install_int,
		NULL,			//mac_timer_remove_int,
		NULL,			//mac_timer_install_param_int,
		NULL,			//mac_timer_remove_param_int,
		NULL,			//mac_timer_can_simulate_retrace,
		NULL,			//mac_timer_simulate_retrace,
		NULL,			//mac_timer_rest
	};

#pragma mark DIGI_DRIVER	
DIGI_DRIVER digi_macos={					/* driver for playing digital sfx */
	DIGI_MACOS,								/* driver ID code */
	empty_string,							/* driver name */
	empty_string,							/* description string */
	"MacOs Digital",						/* ASCII format name string */
	0,//intvoices;							/* available voices */
	0,//intbasevoice;						/* voice number offset */
	16,//intmax_voices;						/* maximum voices we can support */
	4,//intdef_voices;						/* default number of voices to use */

	/* setup routines */
	sound_mac_detect,/*AL_METHOD(int,detect, (int input)); */
	sound_mac_init,/*AL_METHOD(int,init, (int input, int voices)); */
	sound_mac_exit,/*AL_METHOD(void, exit, (int input)); */
	sound_mac_mixer_volume,/*AL_METHOD(int,mixer_volume, (int volume));*/

	/* for use by the audiostream functions */
	sound_mac_lock_voice,/*AL_METHOD(void *, lock_voice, (int voice, int start, int end));*/
	sound_mac_unlock_voice,/*AL_METHOD(void, unlock_voice, (int voice));*/
	NULL,/*AL_METHOD(int,buffer_size, (void));*/

	/* voice control functions */
	sound_mac_init_voice,/*	AL_METHOD(void, init_voice, (int voice, AL_CONST SAMPLE *sample));*/
	sound_mac_release_voice,/*	AL_METHOD(void, release_voice, (int voice));*/
	sound_mac_start_voice,/*	AL_METHOD(void, start_voice, (int voice));*/
	sound_mac_stop_voice,/*	AL_METHOD(void, stop_voice, (int voice));*/
	sound_mac_loop_voice,/*	AL_METHOD(void, loop_voice, (int voice, int playmode));*/

	/* position control functions */
	sound_mac_get_position,/*	AL_METHOD(int,get_position, (int voice));*/
	sound_mac_set_position,/*	AL_METHOD(void, set_position, (int voice, int position));*/

	/* volume control functions */
	sound_mac_get_volume,/*	AL_METHOD(int,get_volume, (int voice));*/
	sound_mac_set_volume,/*	AL_METHOD(void, set_volume, (int voice, int volume));*/
	NULL,/*AL_METHOD(void, ramp_volume, (int voice, int time, int endvol));*/
	NULL,/*AL_METHOD(void, stop_volume_ramp, (int voice));*/

	/* pitch control functions */
	sound_mac_get_frequency,/*	AL_METHOD(int,get_frequency, (int voice));*/
	sound_mac_set_frequency,/*	AL_METHOD(void, set_frequency, (int voice, int frequency));*/
	NULL,/*AL_METHOD(void, sweep_frequency, (int voice, int time, int endfreq));*/
	NULL,/*AL_METHOD(void, stop_frequency_sweep, (int voice));*/

	/* pan control functions */
	sound_mac_get_pan,/*AL_METHOD(int,get_pan, (int voice));*/
	sound_mac_set_pan,/*AL_METHOD(void, set_pan, (int voice, int pan));*/
	NULL,/*AL_METHOD(void, sweep_pan, (int voice, int time, int endpan));*/
	NULL,/*AL_METHOD(void, stop_pan_sweep, (int voice));*/

	/* effect control functions */
	NULL,/*AL_METHOD(void, set_echo, (int voice, int strength, int delay));*/
	NULL,/*AL_METHOD(void, set_tremolo, (int voice, int rate, int depth));*/
	NULL,/*AL_METHOD(void, set_vibrato, (int voice, int rate, int depth));*/

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

#pragma mark system bitmaps vtables

GFX_VTABLE __mac_sys_vtable8;
GFX_VTABLE __mac_sys_vtable15;
GFX_VTABLE __mac_sys_vtable24;

#pragma mark system driver routines
int _mac_init(){
	CRITICAL();
	os_type = 'MAC ';
	register_trace_handler(_mac_trace_handler);
#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"mac_init()::OK\n");
	fflush(stdout);
#endif
	_rgb_r_shift_15 = 10;			/*Ours truecolor pixel format */
	_rgb_g_shift_15 = 5;
	_rgb_b_shift_15 = 0;
	_rgb_r_shift_24 = 16;
	_rgb_g_shift_24 = 8;
	_rgb_b_shift_24 = 0;
	return 0;

Error:
#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"mac_init()::error\n");
	fflush(stdout);
#endif
	_mac_exit();
	return -1;
	}

void _mac_exit(){
	CRITICAL();
	#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"mac_exit()\n");
	fflush(stdout);
#endif
	}

void _mac_get_executable_name(char *output, int size){
	Str255 appName;
	OSErr				err;
	ProcessInfoRec		info;
	ProcessSerialNumber	curPSN;
	CRITICAL();

	err = GetCurrentProcess(&curPSN);

	info.processInfoLength = sizeof(ProcessInfoRec);	// ALWAYS USE sizeof!
	info.processName = appName;							// so it returned somewhere
	info.processAppSpec = NULL;							// I don't care!

	err = GetProcessInformation(&curPSN, &info);

	size=MIN(size,(unsigned char)appName[0]);
	strncpy(output,(char const *)appName+1,size);
	output[size]=0;
	}

void _mac_set_window_title(const char *name){
	CRITICAL();
#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"mac_set_window_title(%s)\n",name);
	fflush(stdout);
#endif
	memcpy(macwindowtitle,name,254);
	macwindowtitle[254]=0;
	}

void _mac_message(const char *msg){
	CRITICAL();
#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"mac_message(%s)\n",msg);
	fflush(stdout);
#endif
	memcpy(macsecuremsg,msg,254);
	macsecuremsg[254]=0;
	paramtext(macsecuremsg,"\0","\0","\0");
	Alert(rmac_message,nil);
	}

void _mac_assert(const char *msg){
	CRITICAL();
	debugstr(msg);
	}

void _mac_save_console_state(void){
	CRITICAL();
#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"mac_save_console_state()\n");
	fflush(stdout);
#endif
	}

void _mac_restore_console_state(void){
	CRITICAL();
#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"mac_restore_console_state()\n");
	fflush(stdout);
#endif
	}

int _mac_desktop_color_depth(void){
/*	short			thisDepth;*/
/*	char			wasState;*/
/*	CRITICAL();*/
/*#if (TRACE_MAC_SYSTEM)*/
/*	fprintf(stdout,"mac_desktop_color_depth()=%d\n",8);*/
/*	fflush(stdout);*/
/*#endif*/
/**/
/*	if (MainGDevice != 0L){							// Make sure we have device handle.*/
/*		wasState = HGetState((Handle)MainGDevice);	// Remember the handle's state.*/
/*		HLock((Handle)MainGDevice);					// Lock the device handle down.*/
/*		thisDepth = (**(**MainGDevice).gdPMap).pixelSize;// Get it's depth (pixelSize).*/
/*		HSetState((Handle)MainGDevice, wasState);	// Restore handle's state.*/
/*		}*/
/*	else thisDepth=0;*/
/*	*/
	fprintf(stdout,"depth=%d\n",0);
	fflush(stdout);
	return 0;
	}

void _mac_yield_timeslice(void){
	CRITICAL();
	SystemTask();	
	}

int _mac_trace_handler(const char *msg){
	CRITICAL();
	debugstr(msg);
	return 0;
	}

#pragma mark system bitmaps methods

void _mac_init_system_bitmap(void){
	BlockMove(&__linear_vtable8,&__mac_sys_vtable8,sizeof(GFX_VTABLE));
	BlockMove(&__linear_vtable15,&__mac_sys_vtable15,sizeof(GFX_VTABLE));
	BlockMove(&__linear_vtable24,&__mac_sys_vtable24,sizeof(GFX_VTABLE));

/*   AL_METHOD(void, set_clip, (struct BITMAP *bmp));*/
/*   AL_METHOD(void, acquire, (struct BITMAP *bmp));*/
/*   AL_METHOD(void, release, (struct BITMAP *bmp));*/
/*   AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));*/
/*   AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));*/
/*   AL_METHOD(int,  getpixel, (AL_CONST struct BITMAP *bmp, int x, int y));*/
/*   AL_METHOD(void, putpixel, (struct BITMAP *bmp, int x, int y, int color));*/
/*   AL_METHOD(void, vline, (struct BITMAP *bmp, int x, int y1, int y2, int color));*/
/*   AL_METHOD(void, hline, (struct BITMAP *bmp, int x1, int y, int x2, int color));*/
/*   AL_METHOD(void, line, (struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color));*/
/*   AL_METHOD(void, rectfill, (struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color));*/
/*   AL_METHOD(int,  triangle, (struct BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color));*/
/*   AL_METHOD(void, draw_sprite, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_256_sprite, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_sprite_v_flip, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_sprite_h_flip, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_sprite_vh_flip, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_trans_sprite, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_trans_rgba_sprite, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_lit_sprite, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y, int color));*/
/*   AL_METHOD(void, draw_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_trans_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_trans_rgba_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));*/
/*   AL_METHOD(void, draw_lit_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));*/
/*   AL_METHOD(void, draw_character, (struct BITMAP *bmp, AL_CONST struct BITMAP *sprite, int x, int y, int color));*/
/*   AL_METHOD(void, draw_glyph, (struct BITMAP *bmp, AL_CONST struct FONT_GLYPH *glyph, int x, int y, int color));*/
/*   AL_METHOD(void, blit_from_memory, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, blit_to_memory, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, blit_from_system, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, blit_to_system, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, blit_to_self, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, blit_to_self_forward, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, blit_to_self_backward, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, masked_blit, (AL_CONST struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));*/
/*   AL_METHOD(void, clear_to_color, (struct BITMAP *bitmap, int color));*/
/*   AL_METHOD(void, draw_sprite_end, (void));*/
/*   AL_METHOD(void, blit_end, (void));*/

	__mac_sys_vtable8.clear_to_color=_mac_sys_clear_to_color8;
	__mac_sys_vtable8.rectfill=_mac_sys_rectfill8;
/*	__mac_sys_vtable8.blit_from_system=_mac_sys_blit8;*/
/*	__mac_sys_vtable8.blit_to_system=_mac_sys_blit8;*/
	__mac_sys_vtable8.blit_to_self=_mac_sys_blit8;
	__mac_sys_vtable8.blit_to_self_forward=_mac_sys_selfblit8;
	__mac_sys_vtable8.blit_to_self_backward=_mac_sys_selfblit8;
	__mac_sys_vtable8.hline=_mac_sys_hline8;
	__mac_sys_vtable8.vline=_mac_sys_vline8;
/*	__mac_sys_vtable8.set_clip=_mac_sys_set_clip;*/
//	__mac_sys_vtable8.line=_mac_sys_line8;
//	__mac_sys_vtable8.triangle=_mac_sys_triangle;
	};

BITMAP *_mac_create_system_bitmap(int w, int h){
	BITMAP		*b;
	if(DrawDepth==8){
	CGrafPtr	graf;
	GDHandle	oldDevice;
	Ptr			data;
	unsigned char*theseBits;
	long		sizeOfOff, offRowBytes;
	short		thisDepth;
	Rect		bounds;
	int			size;
	mac_bitmap	*mb;
	int		i;
	SetRect(&bounds,0,0,w,h);
	oldDevice = GetGDevice();
	SetGDevice(MainGDevice);
	graf = (CGrafPtr)NewPtrClear(sizeof(CGrafPort));
	if (graf != 0L){
		OpenCPort(graf);
		thisDepth = (**(*graf).portPixMap).pixelSize;
		offRowBytes = ((((long)thisDepth *(long)w) + 255L) >> 8L) << 5L;
		sizeOfOff = (long)h * offRowBytes+32;
		
		data = NewPtrClear(sizeOfOff);
		theseBits=(unsigned char*)((long)(data+31)&(-32L));
		if (theseBits != 0L){
			(**(*graf).portPixMap).baseAddr = (char*)theseBits;
			(**(*graf).portPixMap).rowBytes = (short)offRowBytes + 0x8000;
			(**(*graf).portPixMap).bounds = bounds;

			(**(*graf).portPixMap).pmTable = MainCTable;
			ClipRect(&bounds);
			RectRgn(graf->visRgn,&bounds);
			ForeColor(blackColor);
			BackColor(whiteColor);
			EraseRect(&bounds);
			}
		else{
			CloseCPort(graf);		
			DisposePtr((Ptr)graf);
			graf = 0L;
			return NULL;
			}
		}
	SetGDevice(oldDevice);
	size = sizeof(mac_bitmap);
	mb = (mac_bitmap *)NewPtr(size);
	if (!mb)return NULL;
	mb->cg=graf;
	mb->rowBytes=offRowBytes;
	mb->data=data;
	mb->first=theseBits;
	mb->flags=1;
	if((long)theseBits&31){
		DebugStr("\palign is not 32");
		mb->cachealigned=0;
		}
	else{
		mb->cachealigned=1;
		};
	size=sizeof(BITMAP)+ sizeof(char *) * h;
	b=(BITMAP*)NewPtr(size);
	if(b==NULL){DisposePtr((Ptr)mb);return NULL;};
	b->w = b->cr = w;
	b->h = b->cb = h;
	b->clip = TRUE;
	b->cl = b->ct = 0;
	b->vtable = &__mac_sys_vtable8;
	b->write_bank = b->read_bank = _stub_bank_switch;
	b->dat = NULL;
	b->id = BMP_ID_SYSTEM;
	b->extra = (void*)mb;
	b->x_ofs = 0;
	b->y_ofs = 0;
	b->seg = _video_ds();
	b->line[0] = (unsigned char *)theseBits;
 	for (i=1; i<h; i++)b->line[i] = b->line[i-1] + offRowBytes;
	mb->last=b->line[i-1]+w;
	}
	else{
		b=create_bitmap_ex(DrawDepth, w,h);
		b->id = BMP_ID_SYSTEM;
		b->extra = NULL;
		};
	return b;
	};

void _mac_destroy_system_bitmap(BITMAP *bmp){
	mac_bitmap *mbmp;
	if(bmp){
		mbmp=GETMACBITMAP(bmp);
		if(mbmp){
			if(mbmp->flags){
				if(mbmp->data){
					CloseCPort(mbmp->cg);
					DisposePtr((Ptr)mbmp->cg);
					DisposePtr(mbmp->data);
					};
				};
			DisposePtr((Ptr)mbmp);
			}
		else{
			bmp->id&=~BMP_ID_SYSTEM;
			destroy_bitmap(bmp);
			};
		DisposePtr((Ptr)bmp);
		};
	};

void _mac_sys_set_clip(struct BITMAP *dst){
	RgnHandle rclip;
	mac_bitmap *mdst;
	mdst=GETMACBITMAP(dst);
	SetPort((GrafPtr)mdst->cg);
	rclip=NewRgn();
	MacSetRectRgn(rclip,dst->cl,dst->ct,dst->cr,dst->cb);
	SetClip(rclip);
	DisposeRgn(rclip);		
	};

void _mac_sys_clear_to_color8 (BITMAP *bmp, int color){
	register long w;
	mac_bitmap *mbmp;
	mbmp=GETMACBITMAP(bmp);
	w=((long)(bmp->w+3))&-4L;
	if(mbmp->cachealigned){
		w=((long)(bmp->w+31))&-32L;
		ClearChunk(mbmp->first,mbmp->first+w,mbmp->rowBytes-w,mbmp->rowBytes,mbmp->last);
		}
	else{
		w=((long)(bmp->w+3))&-4L;
		Clear8(0x01010101*((long)color&0xFFL),mbmp->first,w,mbmp->rowBytes,mbmp->last);
		};
	};

void _mac_sys_blit8(BITMAP *src,BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h){
	Rect rsrc,rdst;
	mac_bitmap *msrc;
	mac_bitmap *mdst;
	msrc=GETMACBITMAP(src);
	mdst=GETMACBITMAP(dst);
	SetRect(&rsrc,src_x,src_y,src_x+w,src_y+h);
	SetRect(&rdst,dst_x,dst_y,dst_x+w,dst_y+h);
	SetPort((GrafPtr)mdst->cg);
	switch (_drawing_mode){
		case DRAW_MODE_SOLID:{
			DSpBlitInfo di;
			di.completionFlag=false;
			di.filler[0]=0;
			di.filler[1]=0;
			di.filler[2]=0;
			di.filler[3]=0;
			di.completionProc=NULL;
			di.srcContext=NULL;
			di.srcBuffer=msrc->cg;
			di.srcRect=rsrc;
			di.srcKey=0;
			di.dstContext=NULL;
			di.dstBuffer=mdst->cg;
			di.dstRect=rdst;
			di.dstKey=0;
			di.mode=0;
			di.reserved[0]=0;
			di.reserved[1]=0;
			di.reserved[2]=0;
			di.reserved[3]=0;
			DSpBlit_Fastest(&di,false);
			};
			break;
		case DRAW_MODE_XOR:
			CopyBits(	&((GrafPtr)msrc->cg)->portBits,
						&((GrafPtr)mdst->cg)->portBits,
						&rsrc,&rdst,srcXor,NULL);
			break;
		default:
			_linear_blit8(src,dst,src_x,src_y,dst_x,dst_y,w,h);
		};
	};
void system_stretch_blit(BITMAP *src,BITMAP *dst,int sx,int sy,int sw,int sh,int dx,int dy,int dw,int dh){
	Rect rsrc,rdst;
	RgnHandle rclip;
	mac_bitmap *msrc;
	mac_bitmap *mdst;
	msrc=GETMACBITMAP(src);
	mdst=GETMACBITMAP(dst);
	SetRect(&rsrc,sx,sy,sx+sw,sy+sh);
	SetRect(&rdst,dx,dy,dx+dw,dy+dh);
	SetPort((GrafPtr)mdst->cg);
	rclip=NewRgn();
	MacSetRectRgn(rclip,dst->cl,dst->ct,dst->cr,dst->cb);
	SetClip(rclip);
	DisposeRgn(rclip);		
	CopyBits(	&((GrafPtr)msrc->cg)->portBits,
				&((GrafPtr)mdst->cg)->portBits,
				&rsrc,&rdst,srcCopy,NULL);
	};

void _mac_sys_selfblit8(BITMAP *src,BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h){
	Rect rsrc,rdst;
	mac_bitmap *msrc;
	mac_bitmap *mdst;
	msrc=GETMACBITMAP(src);
	mdst=GETMACBITMAP(dst);
	SetRect(&rsrc,src_x,src_y,src_x+w,src_y+h);
	SetRect(&rdst,dst_x,dst_y,dst_x+w,dst_y+h);
	SetPort((GrafPtr)mdst->cg);
	switch (_drawing_mode){
		case DRAW_MODE_SOLID:
			CopyBits(	&((GrafPtr)msrc->cg)->portBits,
						&((GrafPtr)mdst->cg)->portBits,
						&rsrc,&rdst,srcCopy,NULL);
			break;
		case DRAW_MODE_XOR:
			CopyBits(	&((GrafPtr)msrc->cg)->portBits,
						&((GrafPtr)mdst->cg)->portBits,
						&rsrc,&rdst,srcXor,NULL);
			break;
		default:
			_linear_blit8(src,dst,src_x,src_y,dst_x,dst_y,w,h);
		};
	};

int _mac_sys_triangle(struct BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color){
	int done=0;
	mac_bitmap *mbmp;
	PolyHandle triPoly;
	if(_drawing_mode==DRAW_MODE_SOLID){
		mbmp=GETMACBITMAP(bmp);
		SetPort((GrafPtr)mbmp->cg);
		RGBForeColor(&((**MainCTable).ctTable[color].rgb));
		triPoly = OpenPoly();
			MoveTo(x1,y1);
			LineTo(x2,y2);
			LineTo(x3,y3);
		ClosePoly();
		PaintPoly(triPoly);
		KillPoly(triPoly);
		done=1;
		};
	return done;
	};

void _mac_sys_rectfill8(struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color){
	mac_bitmap *mbmp;
	BITMAP *parent;
	Rect r;
	int t;
	if(x1>x2){t=x1;x1=x2;x2=t;};
	if(y1>y2){t=y1;y1=y2;y2=t;};
	if(bmp->clip) {
		if (x1 < bmp->cl)x1 = bmp->cl;
		if (x2 >= bmp->cr)x2 = bmp->cr-1;
		if (x2 < x1)return;
		if (y1 < bmp->ct)y1 = bmp->ct;
		if (y2 >= bmp->cb)y2 = bmp->cb-1;
		if (y2 < y1)return;
		}
	switch(_drawing_mode){
		case DRAW_MODE_SOLID:
			SetRect(&r,x1,y1,x2+1,y2+1);
			parent = bmp;
			mbmp=GETMACBITMAP(bmp);
			SetPort((GrafPtr)mbmp->cg);
			RGBForeColor(&((**MainCTable).ctTable[color].rgb));
			PaintRect(&r);	
			RGBForeColor(&ForeDef);
			break;
		default:
			_normal_rectfill(bmp,x1,y1,x2,y2,color);
			break;
		};
	};

void _mac_sys_hline8(struct BITMAP *bmp, int x1, int y, int x2, int color){
	unsigned long d;
	unsigned char *	p;
	unsigned char *	last;
	unsigned char * sbase;
	unsigned char * s_end;
	unsigned char * s;
	unsigned char * last1;
	unsigned char c=color&0xFF;
	long xoff;
	int t;
	if(x1>x2){t=x1;x1=x2;x2=t;};
	if(bmp->clip) {
		if ((y < bmp->ct)||(y >= bmp->cb))return;
		if (x1 < bmp->cl)x1 = bmp->cl;
		if (x2 >= bmp->cr)x2 = bmp->cr-1;
		if (x2 < x1)return;
		};
	last=bmp->line[y]+x2;
	p=bmp->line[y]+x1;
	if(_drawing_mode<DRAW_MODE_COPY_PATTERN){
		d=0x01010101*c;
		if(_drawing_mode==DRAW_MODE_SOLID){
			while((unsigned long)p&7L&&p<last)*p++=c;
			last-=7;
			while(p<last){
				*(unsigned long*)p=d;
				p+=4;
				};
			last+=7;
			while(p<last)*p++=c;
			}
		else if(_drawing_mode==DRAW_MODE_XOR){
			while((unsigned long)p&3L&&p<last){*p++^=c;};
			last-=3;
			while(p<last){*(unsigned long*)p^=d;p+=4;};
			last+=3;
			while(p<last)*p++^=c;
			}
		else if(_drawing_mode==DRAW_MODE_TRANS){
			 unsigned char * blender = color_map->data[c];
			do{*p++=blender[*p];}while(p<=last);
			};
		}
	else {
		xoff=(x1 - _drawing_x_anchor) & _drawing_x_mask;
		sbase = _drawing_pattern->line[(y - _drawing_y_anchor) & _drawing_y_mask];
		s_end = sbase+_drawing_x_mask+1;
		s = sbase+xoff;
		xoff=_drawing_x_mask+1-(((long)last-(long)p)&_drawing_x_mask)-xoff;
		if(xoff>=0)xoff-=(_drawing_x_mask+1);
		last1=last+xoff;
		if(_drawing_mode==DRAW_MODE_COPY_PATTERN){
			while(p<last1){		
				while(s<s_end){*p++=*s++;};
				s = sbase;
				};
			while(p<last){
				*p++=*s++;
				};
			}
		else if(_drawing_mode==DRAW_MODE_SOLID_PATTERN){
			while(p<last1){		
				while(s<s_end){
					if(*s++){
						*p++=c;
						}
					else{
						*p++=0;
						};
					};
				s = sbase;
				};
 				while(p<last){
				if(*s++){
					*p++=c;
					}
				else{
					*p++=0;
					};
				};
			}
		else if(_drawing_mode==DRAW_MODE_MASKED_PATTERN){
			while(p<last1){		
				while(s<s_end){
					if(*s++){
						*p=c;
						}
					p++;
					};
				s = sbase;
				};
 				while(p<last){
				if(*s++){
					*p=c;
					};
				p++;
				};
			};
		};
	};

void _mac_sys_vline8(struct BITMAP *bmp, int x, int y1, int y2, int color){
	unsigned long inc;
	unsigned char *p;
	unsigned char *last;
	int t;
	if(y1>y2){t=y1;y1=y2;y2=t;};
	if(bmp->clip) {
		if (y1 < bmp->ct)y1 = bmp->ct;
		if (y2 >= bmp->cb)y2 = bmp->cb-1;
		if (y2 < y1)return;
		if ((x < bmp->cl)||(x >= bmp->cr))return;
		};

	p=bmp->line[y1]+x;
	last=bmp->line[y2]+x;
	inc=bmp->line[1]-bmp->line[0];

	if(_drawing_mode<DRAW_MODE_COPY_PATTERN){
		switch(_drawing_mode){
			case DRAW_MODE_SOLID:
				do{*p=color;p+=inc;}while(p<=last);
				break;
			case DRAW_MODE_XOR:
				do{*p^=color;p+=inc;}while(p<=last);
				break;
			case DRAW_MODE_TRANS:{
				unsigned char * blender = color_map->data[(unsigned char)color];
				do{*p=blender[*p];p+=inc;}while(p<=last);
				};
				break;
			default:
				_linear_vline8(bmp,x,y1,y2,color);
				break;
			};
		}
	else{
		unsigned long xoff;
		unsigned long pinc;
		unsigned char * sbase;
		unsigned char * s_end;
		unsigned char * s;
		unsigned char * work;
		xoff=((x - _drawing_x_anchor) & _drawing_x_mask);
		pinc=_drawing_pattern->line[1]-_drawing_pattern->line[0];
		sbase = _drawing_pattern->line[0]+xoff;
		s_end = _drawing_pattern->line[_drawing_y_mask]+xoff;
		s = _drawing_pattern->line[((y1) - _drawing_y_anchor) & _drawing_y_mask]+xoff;
		switch(_drawing_mode){
			case DRAW_MODE_COPY_PATTERN:{
				do{
					work=s+(long)last-(long)p;
					s_end=(unsigned long)s_end<(unsigned long)work?s_end:work;
					do{*p=*s;p+=inc;s+=pinc;}while(s<=s_end);
					s = sbase;
					}while(p<=last);
				};
				break;
			case DRAW_MODE_SOLID_PATTERN:{
				do{
					work=s+(long)last-(long)p;
					s_end=(unsigned long)s_end<(unsigned long)work?s_end:work;
					do{
						if(*s) *p=color;
						else *p=0;
						p += inc; s += pinc;
						}while(s <= s_end);
					s = sbase;
					}while(p<=last);
				};
				break;
			case DRAW_MODE_MASKED_PATTERN:{
				do{
					work=s+(long)last-(long)p;
					s_end=(unsigned long)s_end<(unsigned long)work?s_end:work;
					do{
						if(*s) *p=color;
						p+=inc;s+=pinc;
						}while(s<=s_end);
					s = sbase;
					}while(p<=last);
				};
				break;
			default:
				_linear_vline8(bmp,x,y1,y2,color);
				break;
			};
		};
	};

BITMAP *_CGrafPtr_to_system_bitmap(CGrafPtr cg){
	unsigned char *theseBits;
	long		offRowBytes;
	Rect		bounds;
	int			size;
	BITMAP		*b=NULL;
	mac_bitmap	*mb;
	int			i,h,w;
	GrafPtr		svcg;
	
	bounds = (*cg).portRect;
	h=bounds.bottom-bounds.top;
	w=bounds.right-bounds.left;
	theseBits = (unsigned char *)(**(*cg).portPixMap).baseAddr;
	offRowBytes = 0x7FFF&(**(*cg).portPixMap).rowBytes;

	size = sizeof(mac_bitmap);
	mb = (mac_bitmap *)NewPtr(size);
	
	GetPort(&svcg);
	SetPort((GrafPtr)cg);
	ForeColor(blackColor);
	BackColor(whiteColor);
	SetPort(svcg);
	if (mb){
		mb->flags=0;
		mb->cg=cg;
		mb->rowBytes=offRowBytes;
		mb->data=NULL;
		mb->first=theseBits;
		if((long)theseBits&31)DebugStr("\pvmem is not 32byte aligned");
		mb->cachealigned=0;
		
		size=sizeof(BITMAP)+ sizeof(char *) * h;
		b=(BITMAP*)NewPtr(size);
		if(b==NULL){DisposePtr((Ptr)mb);return NULL;};
		b->w = b->cr = w;
		b->h = b->cb = h;
		b->clip = TRUE;
		b->cl = b->ct = 0;
		b->write_bank = b->read_bank = _stub_bank_switch;
		b->dat = NULL;
		b->id = BMP_ID_SYSTEM;
		b->extra = (void*)mb;
		b->x_ofs = 0;
		b->y_ofs = 0;
		b->seg = _video_ds();
		b->line[0] = theseBits;
		for (i=1; i<b->h; i++)b->line[i] = (theseBits += offRowBytes);
		mb->last=theseBits+w;
		switch((**(*cg).portPixMap).pixelSize){
			case 8:
				b->vtable = &__mac_sys_vtable8;
				break;
			case 16:
				b->vtable = &__linear_vtable15;
				break;
			case 32:
				b->vtable = &__linear_vtable24;
				break;
			default:
				DisposePtr((Ptr)b);
				DisposePtr((Ptr)mb);
				b=NULL;
			};
		};
	return b;
	};


#pragma mark gfx driver routines
BITMAP *init_drawsprocket(int w, int h, int v_w, int v_h, int color_depth){
	OSStatus theError;
	CGrafPtr cg;
	BITMAP* b;
	DSpContextAttributes Attr;

	CRITICAL();
#if(TRACE_MAC_GFX)
	fprintf(stdout,"init_drawsprocket(%d, %d, %d, %d, %d)\n",w,h,v_w,v_h,color_depth);
	fflush(stdout);
#endif

	if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0)) return (NULL);
	State|=kRDDStarted;
	Attr.frequency = 0;
	Attr.reserved1 = 0;
	Attr.reserved2 = 0;
	Attr.colorNeeds = kDSpColorNeeds_Require;
	Attr.colorTable = MainCTable;
	Attr.contextOptions = 0;
	Attr.gameMustConfirmSwitch = false;
	Attr.reserved3[0] = 0;
	Attr.reserved3[1] = 0;
	Attr.reserved3[2] = 0;
	Attr.reserved3[3] = 0;
	Attr.pageCount = 1;
	Attr.displayWidth = w;
	Attr.displayHeight = h;
	Attr.displayBestDepth = color_depth;
	
	_rgb_r_shift_15 = 10;
	_rgb_g_shift_15 = 5;
	_rgb_b_shift_15 = 0;
	_rgb_r_shift_16 = 10;
	_rgb_g_shift_16 = 5;
	_rgb_b_shift_16 = 0;
	_rgb_r_shift_24 = 16;
	_rgb_g_shift_24 = 8;
	_rgb_b_shift_24 = 0;
	_rgb_r_shift_32 = 16;
	_rgb_g_shift_32 = 8;
	_rgb_b_shift_32 = 0;
	
	switch(color_depth){
		case 8:
			DrawDepth=8;
			Attr.displayDepthMask = kDSpDepthMask_8;
			break;
		case 15:
			DrawDepth=15;
			Attr.displayDepthMask = kDSpDepthMask_16;
			break;
		case 24:
			DrawDepth=24;
			Attr.displayDepthMask = kDSpDepthMask_32;
			break;
		default:
			goto Error;
		};
	Attr.backBufferBestDepth = color_depth;
	Attr.backBufferDepthMask = Attr.displayDepthMask;
	
	if(DSpFindBestContext(&Attr,&theContext)!=noErr)goto Error;
	
	Attr.displayWidth = w;
	Attr.displayHeight = h;	
	
	
//	Attr.contextOptions |= kDSpContextOption_PageFlip;
	Attr.contextOptions = 0;

	
	if(DSpContext_Reserve(theContext, &Attr)!=noErr)goto Error;
	State|=kRDDReserved;

/*	theError=DSpAltBuffer_New(theContext,false,0,&theUnderlay);*/
/*	if(theError)goto Error;*/
/*	theError=DSpContext_SetUnderlayAltBuffer(theContext, theUnderlay);*/
/*	if(theError)goto Error;*/
/*	State|=kRDDUnder;*/
	
	active_drawsprocket();	

	theError=DSpContext_SetVBLProc (theContext,MyVBLTask,(void *)refconst);
	if(theError==noErr){_syncOk=1;}
	else{_syncOk=0;};

	cg=GetFrontBuffer();

	b=_CGrafPtr_to_system_bitmap(cg);
	if(b){
		gfx_drawsprocket.w=w;
		gfx_drawsprocket.h=h;
/*		mouse_mac_setpos(w/2,h/2);*/
		return b;
		};
Error: 
#if(TRACE_MAC_GFX)
	fprintf(stdout,"init_drawsprocket()failed\n");
	fflush(stdout);
#endif
	exit_drawsprocket(b);
	return NULL;
	}
void exit_drawsprocket(struct BITMAP *b){
#pragma unused b
	OSStatus theError;
	CRITICAL();
#if(TRACE_MAC_GFX)
	fprintf(stdout,"exit_drawsprocket()\n");
	fflush(stdout);
#endif
	if((State&kRDDStarted)!=0){
		if((State&kRDDReserved)!=0){
/*			if (State&kRDDUnder){*/
/*				DSpContext_SetUnderlayAltBuffer(theContext, NULL);*/
/*				DSpAltBuffer_Dispose(theUnderlay);*/
/*				};*/
			theError=DSpContext_SetState(theContext, kDSpContextState_Inactive);
			theError=DSpContext_Release(theContext);
			};
/*		SetCursor( &qd.arrow );*/
/*		ShowCursor();*/
		};
	State=0;
	gfx_drawsprocket.w=0;
	gfx_drawsprocket.h=0;
	DrawDepth=0;
	};

void vsync_drawsprocket(void){
/*	CRITICAL();*/
	if(_syncOk){
		_sync=0;
		while(!_sync){};
		};
	};

void init_mypalette(){
	MainCTable=GetCTable(8);
	DetachResource((Handle) MainCTable);
	};
	
void set_palette_drawsprocket(const struct RGB *p, int from, int to, int retracesync){
	int i;OSErr theError;
	CRITICAL();
#if(TRACE_MAC_GFX)
	fprintf(stdout,"set_palette");
	fflush(stdout);
#endif
	if(MainCTable==NULL){
		init_mypalette();
		};
	for(i=from;i<=to;i++){
		(**MainCTable).ctTable[i].rgb.red = p[i].r*1040;
		(**MainCTable).ctTable[i].rgb.green = p[i].g*1040;
		(**MainCTable).ctTable[i].rgb.blue = p[i].b*1040;
		};
	if(retracesync)vsync_drawsprocket();
	if(DrawDepth==8){
		theError = DSpContext_SetCLUTEntries(theContext, (**MainCTable).ctTable, from, to - from);
		if( theError ) DebugStr("\p error from CLUT ");	
		};
	};
short active_drawsprocket(){
	CRITICAL();
	if(!(State&kRDDActive)){
/*		HideCursor();*/
		if(!(State&kRDDPaused))
		if(DSpContext_SetState(theContext , kDSpContextState_Active )!=noErr){
/*			ShowCursor();*/
			return 1;
			};
		State&=(~kRDDPaused);
		State|=kRDDActive;
		};
	return 0;
	};
short pause_drawsprocket(){
	CRITICAL();
	if(!(State&kRDDPaused)){
		if(DSpContext_SetState(theContext, kDSpContextState_Paused)!=noErr)return 1;//Fatal("\pPause");
		State&=(~kRDDActive);
		State|=kRDDPaused;
		DrawMenuBar();
/*		SetCursor( &qd.arrow );*/
/*		ShowCursor();*/
		};
	return 0;
	};
short inactive_drawsprocket(){
	CRITICAL();
	if(!(State&(kRDDPaused|kRDDActive))){
		if(DSpContext_SetState(theContext, kDSpContextState_Inactive)!=noErr)return 1;//Fatal("\pInactive");
		State&=(~kRDDPaused);
		State&=(~kRDDActive);
		DrawMenuBar();
/*		SetCursor( &qd.arrow );*/
/*		ShowCursor();*/
		};
	return 0;
	};


CGrafPtr GetBackBuffer(){
	CGrafPtr theBuffer;
	DSpContext_GetBackBuffer( theContext, kDSpBufferKind_Normal, &theBuffer );
	return theBuffer;
	};

CGrafPtr GetFrontBuffer(){
	CGrafPtr theBuffer;
//	DSpContext_GetBackBuffer( theContext, kDSpBufferKind_Normal, &theBuffer );
	DSpContext_GetFrontBuffer(theContext, &theBuffer);	
	return theBuffer;
	};
void swap_drawsprocket(){
	CRITICAL();
	DSpContext_SwapBuffers(theContext, nil, nil);
	};
Boolean MyVBLTask (DSpContextReference inContext,void *inRefCon){
#pragma unused inContext,inRefCon
	_sync=1;
	return false;
	};

#pragma mark timer routines
int mac_timer_init(void){
	_mactimerinstalled=1;
	return 0;
	};
void mac_timer_exit(void){
	_mactimerinstalled=0;
	};
#pragma mark mouse routines
int mouse_mac_init(void){
#if (TRACE_MAC_MOUSE)
	CRITICAL();
	fprintf(stdout,"mouse_mac_init()\n");
	fflush(stdout);
#endif
	_macmouseinstalled=1;
	return 1;
	};
void mouse_mac_exit(void){
#if (TRACE_MAC_MOUSE)
	CRITICAL();
	fprintf(stdout,"mouse_mac_exit()\n");
	fflush(stdout);
#endif
	_macmouseinstalled=0;
	};
#pragma mark keyboard routines
int key_mac_init(void){
	CRITICAL();
#if (TRACE_MAC_KBRD)
	fprintf(stdout,"key_mac_init()\n");
	fflush(stdout);
#endif
	GetKeys(KeyNow);
	BlockMove(KeyNow,KeyOld,sizeof(KeyMap));
	_mackeyboardinstalled=1;
	return 0;
	};
void key_mac_exit(void){
	CRITICAL();
#if (TRACE_MAC_KBRD)
	fprintf(stdout,"key_mac_exit()\n");
	fflush(stdout);
#endif
	_mackeyboardinstalled=0;
	};
	
#pragma mark todo
/*EXTERN_API( SInt16 ) LMGetKeyThresh(void)													TWOWORDINLINE(0x3EB8, 0x018E);*/
/*EXTERN_API( void ) LMSetKeyThresh(SInt16 value)												TWOWORDINLINE(0x31DF, 0x018E);*/
/*EXTERN_API( SInt16 ) LMGetKeyRepThresh(void)												TWOWORDINLINE(0x3EB8, 0x0190);*/
/*EXTERN_API( void ) LMSetKeyRepThresh(SInt16 value)											TWOWORDINLINE(0x31DF, 0x0190);*/

#pragma mark sound routines



int sound_mac_detect(int input){/*AL_METHOD(int,detect, (int input)); */
	int det=0;
	if(!input)det=1;
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"snddetect = %d",det);
#endif
	return det;
	};
int sound_mac_init(int input, int voices){/*AL_METHOD(int,init, (int input, int voices)); */
	short count;
	short soundOk=true;
	if(input)soundOk=false;
	if(voices>NMVOICES)soundOk=false;
	for(count=0;count<NMVOICES;count++){
		mac_voices[count].channel=NULL;
		mac_voices[count].sample=NULL;
		mac_voices[count].vol=255;
		mac_voices[count].frequency=22050;
		mac_voices[count].pan=128;
		mac_voices[count].loop=0;
		};
	for( count=0; count<(voices-1) && soundOk; count++ ){
		if( SndNewChannel( &mac_voices[count].channel, sampledSynth, initMono, nil ) != noErr ){
			soundOk = false;
			};
		};
	
	digi_macos.voices=voices;
//	if(!soundOk)sound_mac_exit(input);
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndinit(%d,%d) = %d",input,voices,count);
#endif
	return !count;
	};
void sound_mac_exit(int input){/*AL_METHOD(void, exit, (int input)); */
	short count;
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndexit(%d)",input);
#endif
	for(count=0;count<NMVOICES;count++){
		if (mac_voices[count].channel){
			sound_mac_stop_voice(count);
			SndDisposeChannel(mac_voices[count].channel, true);
			mac_voices[count].channel = NULL;
			};
		};
	};
int sound_mac_mixer_volume(int volume){/*AL_METHOD(int,mixer_volume, (int volume));*/
	int vol=mixer_vol;
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndvol(%d)",volume);
#endif
	mixer_vol=volume;
	return vol;
	};
/* for use by the audiostream functions */
void * sound_mac_lock_voice(int voice, int start, int end){/*AL_METHOD(void *, lock_voice, (int voice, int start, int end));*/
	void * p=NULL;
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndlock(%d,%d,%d)",voice,start,end);
#endif
	if(mac_voices[voice].sample){
		HLock((Handle)mac_voices[voice].sample);
		p=(void *)((*(Handle)(mac_voices[voice].sample))+mac_voices[voice].headsize);
		};
	return p;
	};
void sound_mac_unlock_voice(int voice){/*AL_METHOD(void, unlock_voice, (int voice));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndunlock()");
#endif
	if(mac_voices[voice].sample){
		HUnlock((Handle)mac_voices[voice].sample);
		};
	};
/*AL_METHOD(int,buffer_size, (void));*/

/* voice control functions */
void sound_mac_init_voice(int voice, AL_CONST SAMPLE *sample){/*	AL_METHOD(void, init_voice, (int voice, AL_CONST SAMPLE *sample));*/
	OSErr err;
	SndListHandle msample;
	char a,b,*src,*dst;
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndinitv(%d,...)",voice);
#endif
	if(mac_voices[voice].channel){
		
		msample=(SndListHandle)NewHandleClear(100+sample->len);
		if(msample){
#if (TRACE_MAC_SOUND)
			fprintf(stdout,"*SAMPLE*\nbits=%d\nfreq=%d\nlen=%d\n",sample->bits,sample->freq,sample->len);
#endif
			HLock((Handle)msample);
			err=SetupSndHeader(msample,
				sample->stereo?2:1,Long2Fix(sample->freq),
				sample->bits,'NONE',kMiddleC,
				sample->len,&(mac_voices[voice].headsize));
#if (TRACE_MAC_SOUND)
			fprintf(stdout,"the error is %d\n",err);
#endif
			switch(sample->bits){
				case 8:
					BlockMoveData(sample->data,(*(Handle)msample)+mac_voices[voice].headsize,sample->len);
					break;
				case 16:
					for(src=sample->data,dst=(*(Handle)msample)+mac_voices[voice].headsize;
						src<((char *)sample->data)+sample->len;
						){
						a=*src++;
						b=*src++;
						*dst++=a;
						*dst++=b;
						};
				default:
					fprintf(stdout,"a unknow depth %d",sample->bits);
					break;
				};
			sound_mac_stop_voice(voice);
			if(mac_voices[voice].sample)DisposeHandle((Handle)mac_voices[voice].sample);
			mac_voices[voice].sample=(SndListHandle)msample;
			mac_voices[voice].frequency=sample->freq;
			mac_voices[voice].loop_start=sample->loop_start;
			mac_voices[voice].loop_end=sample->loop_end;
			GetSoundHeaderOffset((SndListHandle)msample, &mac_voices[voice].headeroff);
			};
		};
	};

void sound_mac_release_voice(int voice){/*	AL_METHOD(void, release_voice, (int voice));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndrelv(%d)",voice);
#endif
	if(mac_voices[voice].channel){
		sound_mac_stop_voice(voice);
		if((Handle)mac_voices[voice].sample){
			HUnlock((Handle)mac_voices[voice].sample);
			DisposeHandle((Handle)mac_voices[voice].sample);
			mac_voices[voice].sample=NULL;
			};
		};
	};
void sound_mac_start_voice(int voice){/*	AL_METHOD(void, start_voice, (int voice));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndstartv(%d)",voice);
#endif
	if( mac_voices[voice].channel&&mac_voices[voice].sample){
		SndCommand	theCommand;
		OSErr		theErr;

		SoundHeader *sh;
		sh=getsoundheader(voice);
		sh->sampleRate=Long2Fix(mac_voices[voice].frequency);
		if(mac_voices[voice].loop){
			sh->loopStart=mac_voices[voice].loop_start;
			sh->loopEnd=mac_voices[voice].loop_end;
			
			theCommand.cmd = flushCmd;
			theCommand.param1 = 0;
			theCommand.param2 = 0L;
			theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);

			theCommand.cmd = quietCmd;
			theCommand.param1 = 0;
			theCommand.param2 = 0L;
			theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);

			theCommand.cmd = soundCmd;
			theCommand.param1 = 0;
			theCommand.param2 = (long)sh;
			theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);
			
			theCommand.cmd = freqDurationCmd;
			theCommand.param1 = 32767;
			theCommand.param2 = kMiddleC;
			theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);			
			}
		else{
			theCommand.cmd = flushCmd;
			theCommand.param1 = 0;
			theCommand.param2 = 0L;
			theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);

			theCommand.cmd = quietCmd;
			theCommand.param1 = 0;
			theCommand.param2 = 0L;
			theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);

			sh->loopStart=0;
			sh->loopEnd=0;

			SndPlay(mac_voices[voice].channel, mac_voices[voice].sample, true );
			};
		};
	};
void sound_mac_stop_voice(int voice){/*	AL_METHOD(void, stop_voice, (int voice));*/
	SndCommand	theCommand;
	OSErr		theErr;
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndstopv(%d)",voice);
#endif
	if(mac_voices[voice].channel){
		theCommand.cmd = flushCmd;
		theCommand.param1 = 0;
		theCommand.param2 = 0L;
		theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);

		theCommand.cmd = quietCmd;
		theCommand.param1 = 0;
		theCommand.param2 = 0L;
		theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);

		};
	};
void sound_mac_loop_voice(int voice, int playmode){/*	AL_METHOD(void, loop_voice, (int voice, int playmode));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndloopv(%d)",voice);
#endif
	mac_voices[voice].loop=playmode;
	};

/* position control functions */
int sound_mac_get_position(int voice){/*	AL_METHOD(int,get_position, (int voice));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"macgetpos(%d)",voice);
#endif
	return 0;
	};
void sound_mac_set_position(int voice,int position){/*	AL_METHOD(void, set_position, (int voice, int position));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"macsetpos(%d,%d)",voice,position);	
#endif
	};
/* volume control functions */
int sound_mac_get_volume(int voice){/*	AL_METHOD(int,get_volume, (int voice));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndgetvol(%d)=%d",voice,mac_voices[voice].vol);
#endif
	return mac_voices[voice].vol;
	};
void sound_mac_set_volume(int voice,int vol){/*	AL_METHOD(void, set_volume, (int voice, int volume));*/
	SndCommand	theCommand;
	OSErr		theErr;
	unsigned long Rvol,Lvol;
	Rvol=(((unsigned)vol*(unsigned)mac_voices[voice].pan)>>8)<<16;
	Lvol=((unsigned)vol*(unsigned)(256-mac_voices[voice].pan))>>8;
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndsetvol(%d,%d)",voice,vol);
#endif
	if(mac_voices[voice].channel){
		theCommand.cmd = volumeCmd;
		theCommand.param1 = 0;
		theCommand.param2 = (long)(Rvol+Lvol);
		theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);
		};
	mac_voices[voice].vol=vol;
	};
/*AL_METHOD(void, ramp_volume, (int voice, int time, int endvol));*/
/*AL_METHOD(void, stop_volume_ramp, (int voice));*/

/* pitch control functions */
int sound_mac_get_frequency(int voice){/*	AL_METHOD(int,get_frequency, (int voice));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndgetfr(%d)=%d",voice,mac_voices[voice].frequency);
#endif
	return mac_voices[voice].frequency;
	};
void sound_mac_set_frequency(int voice,int frequency){/*	AL_METHOD(void, set_frequency, (int voice, int frequency));*/
/*	SndCommand	theCommand;*/
/*	OSErr		theErr;*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndsetfr(%d,%d)",voice,frequency);
#endif
/*	if(mac_voices[voice].channel){*/
/*		Fixed f = Long2Fix(frequency);*/
/*		theCommand.cmd = rateCmd;*/
/*		theCommand.param1 = 0;*/
/*		theCommand.param2 = (long)f;*/
/*		theErr = SndDoImmediate(mac_voices[voice].channel, &theCommand);*/
/*		};*/
	mac_voices[voice].frequency=frequency;
	};
/*AL_METHOD(void, sweep_frequency, (int voice, int time, int endfreq));*/
/*AL_METHOD(void, stop_frequency_sweep, (int voice));*/

/* pan control functions */
int sound_mac_get_pan(int voice){/*AL_METHOD(int,get_pan, (int voice));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndgetpan(%d)=%d",voice,mac_voices[voice].pan);
#endif
	return mac_voices[voice].pan;
	};
void sound_mac_set_pan(int voice,int pan){/*AL_METHOD(void, set_pan, (int voice, int pan));*/
#if (TRACE_MAC_SOUND)
	fprintf(stdout,"sndsetpan(%d,%d)",voice,pan);
#endif
	sound_mac_set_volume(voice,mac_voices[voice].vol);
	mac_voices[voice].pan=pan;
	};
/*AL_METHOD(void, sweep_pan, (int voice, int time, int endpan));*/
/*AL_METHOD(void, stop_pan_sweep, (int voice));*/

SoundHeader * getsoundheader(int voice){
	SndListHandle sl;
	sl=mac_voices[voice].sample;
	return  (SoundHeader*) (((char*)*sl) + ((long)mac_voices[voice].headeroff));
	};

#pragma mark Mac Suport routines
/*this routine get the parent id of dir or file by ours id*/
short parent(short vRefNum,long dirID){
	FSSpec spec;
	FSMakeFSSpec(vRefNum,dirID,NULL,&spec);
	return spec.parID;
	};

#pragma mark Find First
/*code modifyed from allegro/src/libc.c*/
struct ff_match_data{
	int type;
	const char *s1;
	const char *s2;
	};
typedef struct ff_match_data ff_match_data;
typedef ff_match_data *ffmatchdataPtr;
#define FF_MATCH_TRY 0
#define FF_MATCH_ONE 1
#define FF_MATCH_ANY 2
/* ff_match:
 *Match two strings ('*' matches any number of characters,
 *'?' matches any character).
 */
	static int ff_match(const char *s1, const char *s2){
	static int size = 0;
	static ffmatchdataPtr data = 0;
	const char *s1end = s1 + strlen(s1);
	int index, c1, c2;
	/* allocate larger working area if necessary */
	if ((data != 0) && (size < strlen(s2))) {
		free(data);
		data = 0;
	}
	if (data == 0) {
		size = strlen(s2);
		data = (ffmatchdataPtr)malloc(sizeof(ff_match_data)* size * 2 + 1);
		if (data == 0)
	return 0;
	}

	index = 0;
	data[0].s1 = s1;
	data[0].s2 = s2;
	data[0].type = FF_MATCH_TRY;

	while (index >= 0) {
		s1 = data[index].s1;
		s2 = data[index].s2;
		c1 = *s1;
		c2 = *s2;

		switch (data[index].type) {

		case FF_MATCH_TRY:
	if (c2 == 0) {
		/* pattern exhausted */
		if (c1 == 0)
			return 1;
		else
			index--;
	}
	else if (c1 == 0) {
		/* string exhausted */
		while (*s2 == '*')
			s2++;
		if (*s2 == 0)
			return 1;
		else
			index--;
	}
	else if (c2 == '*') {
		/* try to match the rest of pattern with empty string */
		data[index++].type = FF_MATCH_ANY;
		data[index].s1 = s1end;
		data[index].s2 = s2 + 1;
		data[index].type = FF_MATCH_TRY;
	}
	else if ((c2 == '?') || (c1 == c2)) {
		/* try to match the rest */
		data[index++].type = FF_MATCH_ONE;
		data[index].s1 = s1 + 1;
		data[index].s2 = s2 + 1;
		data[index].type = FF_MATCH_TRY;
	}
	else
		index--;
	break;

		case FF_MATCH_ONE:
	/* the rest of string did not match, try earlier */
	index--;
	break;

		case FF_MATCH_ANY:
	/* rest of string did not match, try add more chars to string tail */
	if (--data[index + 1].s1 >= s1) {
		data[index + 1].type = FF_MATCH_TRY;
		index++;
	}
	else
		index--;
	break;

		default:
	/* this is a bird? This is a plane? No it's a bug!!! */
	return 0;
		}
	}

	return 0;
}

char *getfilename(const char *fullpath){
	char *p = (char*)fullpath + strlen(fullpath);
	while ((p > fullpath) && (*(p - 1) != '/')&&(*(p - 1) != '\\')&&(*(p - 1) != ':'))p--;
	return p;
	};
void getpath(char *filename){
	char *temp,*d,*s,*last;
	int div;
	temp=malloc(strlen(filename)+1);
	if(temp!=NULL){
/*		if(filename[0]!=':'&&filename[0]!='/'&&filename[0]!='\\'){*/
/*			temp[0]=':';temp[1]=0;*/
/*			}*/
/*		else{*/
/*			temp[0]=0;*/
/*			};*/
/*		strcat(temp,filename);*/
/*		for(p=temp;(*p)!=0;){if((*p == '/')||(*p== '\\'))*p=':';p++;};*/
/*		if(*(--p)!=':'){*/
/*			while(*p!=':')p--;*/
/*			*(++p)=0;*/
/*			};*/
		d=temp;
		div=1;
		last=d;
		for(s=(char *)filename;*s;s++){
			if(*s==':'||*s=='/'||*s=='\\'){
				if(!div){
					*d++='\\';
					last=d;
					div=1;
					};
				}
			else{
				*d++=*s;
				div=0;
				};
			};
		*d=0;
		*last=0;
		
		strcpy(filename,temp);
		free(temp);
		};
	};

#define FF_MAXPATHLEN 1024
struct ffblk{
	char dirname[FF_MAXPATHLEN];
	char pattern[FF_MAXPATHLEN];
	int	dattr;
	long dirbase;
	long attrib;
	CInfoPBRec cpb;
	};
typedef struct ffblk ffblk;
typedef ffblk *ffblkPtr;


long finddirect(void *pdta){
	ffblkPtr dta=(ffblkPtr)pdta;	
	char *p,cname[256];
	OSErr err=noErr;						
	int done;
	short buffersize=strlen(dta->dirname);
	char *buffer=malloc(buffersize+1);

	if(!buffer)return 0;
	strcpy(buffer,dta->dirname);
	UppercaseStripDiacritics(buffer,buffersize,smSystemScript);
	p=strtok(buffer,"\\/");
		
	while(p!=NULL&&err==noErr&&(*p!=0)){
		if(strcmp(p,".")==0){
			}
		else if(strcmp(p,"..")==0){
			dta->dirbase=parent(dta->cpb.dirInfo.ioVRefNum,dta->dirbase);
			}
		else if(strcmp(p,"...")==0){
			dta->dirbase=parent(dta->cpb.dirInfo.ioVRefNum,parent(dta->cpb.dirInfo.ioVRefNum,dta->dirbase));
			}
		else{
			for(dta->cpb.dirInfo.ioFDirIndex=1,done=false;
				err == noErr&& done != true;
				dta->cpb.dirInfo.ioFDirIndex++){
				dta->cpb.dirInfo.ioDrDirID=dta->dirbase;
				dta->cpb.dirInfo.ioACUser = 0;
				if ((err = PBGetCatInfoSync(&dta->cpb)) == noErr){
					if (dta->cpb.dirInfo.ioFlAttrib&ioDirMask){					
						ptoc(dta->cpb.dirInfo.ioNamePtr,cname);
						UppercaseStripDiacritics(cname,strlen(cname),smSystemScript);
						if(strcmp(p,cname)==0){
							dta->dirbase=dta->cpb.dirInfo.ioDrDirID;
							done=true;
							};
						};
					};
				};
			};
		p=strtok(NULL,"\\/");
		};
	dta->cpb.dirInfo.ioFDirIndex=1;
	free(buffer);
	if(err)dta->dirbase=0;
	return dta->dirbase;
	};
void _al_findclose(void *dta){
	CRITICAL();
	if(dta)free(dta);
	};
int _al_findnext(void *pdta, char *nameret, int *aret){
	ffblkPtr dta=(ffblkPtr)pdta;
	char cname[256];
	OSErr err=noErr;
	int curattr;
	int list;
	CRITICAL();
#if (TRACE_MAC_FILE)
	fprintf(stdout,"_al_findnext(...)\n");
	fflush(stdout);
#endif
	if(aret)*aret=0;
	for(;err == noErr;dta->cpb.hFileInfo.ioFDirIndex++){
		dta->cpb.hFileInfo.ioDirID = dta->dirbase;
		dta->cpb.hFileInfo.ioACUser = 0;
		if ((err = PBGetCatInfoSync(&dta->cpb)) == noErr){
			ptoc(dta->cpb.hFileInfo.ioNamePtr,cname);
			UppercaseStripDiacritics(cname,strlen(cname),smSystemScript);
			list=true;
			curattr=(dta->cpb.dirInfo.ioFlAttrib&ioDirMask)?FA_DIREC:0;
			if((dta->dattr&FA_DIREC)&&!(curattr&FA_DIREC))list=false;
			if(list){
				if(ff_match(cname, dta->pattern)){
					if(nameret){
						strcpy(nameret,dta->dirname);
						strcat(nameret,cname);
						};
					if(aret)*aret=curattr;
					err='OK';
					}
				};
			};
		};
	if(err=='OK'){
		return (errno=0);
		};
	return (errno=ENOENT);
	}

void * _al_findfirst(const char *pattern,int attrib, char *nameret, int *aret){
	ffblkPtr dta=(ffblkPtr)malloc(sizeof(ffblk));
	CRITICAL();
#if (TRACE_MAC_FILE)
	fprintf(stdout,"_al_findfirst(%s, %d, .,.)\n",pattern,attrib);
	fflush(stdout);
#endif
	if (dta){	
		dta->attrib = attrib;
		strcpy(dta->dirname, pattern);
		getpath(dta->dirname);
		strcpy(dta->pattern, getfilename(pattern));	
		if (dta->dirname[0] == 0)strcpy(dta->dirname, ".\\");
		if (strcmp(dta->pattern, "*.*") == 0)strcpy(dta->pattern, "*");
		UppercaseStripDiacritics(dta->pattern,strlen(dta->pattern),smSystemScript);
		dta->cpb.dirInfo.ioCompletion=NULL;
		dta->cpb.dirInfo.ioVRefNum=MainVol;
		dta->cpb.dirInfo.ioNamePtr=mname;
		dta->dirbase=MainDir;
#if (TRACE_MAC_FILE)
		fprintf(stdout,"directory name %s\n",dta->dirname);
		fflush(stdout);
#endif
		if((finddirect(dta))!=0){
			dta->dattr=attrib;
			if ((_al_findnext(dta,nameret,aret))!=0){
#if (TRACE_MAC_FILE)
				fprintf(stdout,"_al_findfirst failed no files found\n");
				fflush(stdout);
#endif
				_al_findclose(dta);
				dta=NULL;
				};
			}
		else{
#if (TRACE_MAC_FILE)
			fprintf(stdout,"_al_findfirst failed direct not found\n");
			fflush(stdout);
#endif
			_al_findclose(dta);
			dta=NULL;
			};
		}
#if (TRACE_MAC_FILE)
	else {fprintf(stdout,"_al_findfirst lowmem\n");
	fflush(stdout);};
#endif
	
#if (TRACE_MAC_FILE)
	if(dta==NULL)fprintf(stdout,"_al_findfirst failed\n");
#endif
	return dta;
	}
int _al_file_isok(const char *filename){
	CRITICAL();
#if (TRACE_MAC_FILE)
	fprintf(stdout,"_al_file_isok(%s)\n",filename);
	fflush(stdout);
#else
#endif
	return true;
	};
int _al_file_exists(const char *filename, int attrib, int *aret){
	void *dta;
	int x=false;
	CRITICAL();
	dta=_al_findfirst(filename,attrib,NULL,aret);
	if(dta){
		_al_findclose(dta);
		x=true;
		};
#if (TRACE_MAC_FILE)
	fprintf(stdout,"fileexists(%s)=%d\n",filename,x);
	fflush(stdout);
#endif
	return x;
	}
long _al_file_size(const char *filename){
	void *dta;
	long fs;
	CRITICAL();
	dta = _al_findfirst(filename,0/*FA_RDONLY | FA_HIDDEN | FA_ARCH*/,NULL,NULL);
	if (dta){
		fs=((ffblkPtr)dta)->cpb.hFileInfo.ioFlLgLen;	
		_al_findclose(dta);
		}
	else {fs=0;};
#if (TRACE_MAC_FILE)
	fprintf(stdout,"filesize(%s)=%d\n",filename,fs);	
	fflush(stdout);
#endif
	return fs;
	}
long _al_file_time(const char *filename){
	void *dta;
	long ft;
	CRITICAL();
	dta = _al_findfirst(filename,0/*FA_RDONLY | FA_HIDDEN | FA_ARCH*/,NULL,NULL);
	if (dta){
		ft=((ffblkPtr)dta)->cpb.hFileInfo.ioFlMdDat;	
		_al_findclose(dta);
		}
	else {ft=0;};
#if (TRACE_MAC_FILE)
	fprintf(stdout,"filetime(%s)=%d\n",filename,ft);
	fflush(stdout);
#endif
	return ft;
	}
int _al_getdrive(void){
	CRITICAL();
#if (TRACE_MAC_FILE)
	fprintf(stdout,"_al_getdrive()\n");
	fflush(stdout);
#endif
	return 2;/*drive c*/
	};
void _al_getdcwd(int drive, char *buf, int size){
	CRITICAL();
#if (TRACE_MAC_FILE)
	fprintf(stdout,"_al_getdcwd(%d, ...,&d)\n",drive,size);
	fflush(stdout);
#else
#endif
	strcpy(buf,":");
	};


int _al_open(const char *filename,int mode){
	char *path;
	char *s,*d;
	int handle=-1;
	int div;
	CRITICAL();
	path=malloc(strlen(filename)+1);
	if(path!=NULL){
		div=1;
		d=path;
		*d++=':';
		s=(char *)filename;
		if(*s=='.')s++;
		while(*s=='.'){*d++=':';s++;};
		for(;*s;s++){
			if(*s==':'||*s=='/'||*s=='\\'){
				if(!div){
					*d++=':';
					div=1;
					};
				}
			else{
				*d++=*s;
				div=0;
				};
			};
		*d=0;
/*		*/
/*		*/
/*		if(filename[0]!=':'&&filename[0]!='/'&&filename[0]!='\\'){*/
/*			path[0]=':';path[1]=0;*/
/*			}*/
/*		else{*/
/*			path[0]=0;*/
/*			};*/
/*		strcat(path,filename);*/
/*		for(p=path;(*p)!=0;p++)if((*p == '/')||(*p== '\\'))*p=':';*/
/*		*/
#if (TRACE_MAC_FILE)
		fprintf(stdout,"_al_open(%s\n",path);
		fflush(stdout);
#endif
		
		handle=open(path,mode);
		free(path);
		};
#if (TRACE_MAC_FILE)
	fprintf(stdout,"_al_open(%s ,%d)=%d\n",filename,mode,handle);
	fflush(stdout);
#endif
	return handle;
	};
#pragma mark mac suport
char *strdup(const char *p){
	char *tmp=malloc(strlen(p));
	CRITICAL();
	if(tmp){
		strcpy(tmp,p);
		};
	return tmp;
	};
void getcwd(char* p,int size){
	strcpy(p,"");
	};
long GetLogicalDrives(){
	return 1;
	};


//--------------------------------------------------------------DoWeHaveColor

// Simple function that returns TRUE if we're running on a Mac thatÉ
// is running Color Quickdraw.

Boolean DoWeHaveColor (void)
{
	SysEnvRec		thisWorld;
	
	SysEnvirons(2, &thisWorld);		// Call the old SysEnvirons() function.
	return (thisWorld.hasColorQD);	// And return whether it has Color QuickDraw.
}

//--------------------------------------------------------------DoWeHaveSystem605

// Another simple "environment" function.Returns TRUE if the Mac we're runningÉ
// on has System 6.0.5 or more recent.(6.0.5 introduced the ability to switchÉ
// color depths.)

Boolean DoWeHaveSystem605 (void)
{
	SysEnvRec		thisWorld;
	Boolean			haveIt;
	
	SysEnvirons(2, &thisWorld);		// Call the old SysEnvirons() function.
	if (thisWorld.systemVersion >= 0x0605)
		haveIt = TRUE;				// Check the System version for 6.0.5É
	else							// or more recent
		haveIt = FALSE;
	return (haveIt);
}


void ptoc(StringPtr pstr,char *cstr){
	BlockMove(pstr+1,cstr,pstr[0]);
	cstr[pstr[0]]='\0';
	};
short FindMainDevice (void){
	MainGDevice = GetMainDevice();
	if (MainGDevice == 0L)return 1;
	return 0;
	}
Boolean RTrapAvailable(short tNumber,TrapType tType){
	return NGetTrapAddress( tNumber, tType ) != NGetTrapAddress( _Unimplemented, ToolTrap);		
	};

pascal void	MyTask(TMTaskPtr tmTaskPtr){
	criticalarea=1;
	if(_mactimerinstalled)_handle_timer_tick(MSEC_TO_TIMER(50));
	if(_macmouseinstalled){
		Point pt;
		pt=(*((volatile Point *)0x082C));/*LMGetMouseLocation();*/
		_mouse_b = (*((volatile UInt8 *)0x0172))&0x80?0:1;
		_mouse_x = pt.h;
		_mouse_y = pt.v;
		_handle_mouse_input();
		};
	if(_mackeyboardinstalled){
		UInt32 xKeys;
		int a,row,base,keycode;
		KeyNow[0]=(*(UInt32*)0x174);//GetKeys cannot be called at interrupt time
		KeyNow[1]=(*(UInt32*)0x178);
		KeyNow[2]=(*(UInt32*)0x17C);
		KeyNow[3]=(*(UInt32*)0x180);
		for(row=0,base=0;row<4;row++,base+=32){
			xKeys=KeyOld[row]^KeyNow[row];
			if(xKeys){
				for(a=0;a<32;a++){
					if(BitTst(&xKeys,a)){
						keycode=key_apple_to_allegro[a+base];
						if(keycode){
							if(BitTst(&KeyNow[row],a)){
								_handle_key_press(key_apple_to_char[a+base],keycode);
								}
							else{
								_handle_key_release(keycode);
								};
							};
						};
					};
				};
			KeyOld[row]=KeyNow[row];
			};		
		};
	criticalarea=0;
	PrimeTime((QElemPtr)tmTaskPtr, 50);
	};

/*
 *Executs the needed clean before return to system
 */
void maccleanup(){
	if(_timer_running)RmvTime((QElemPtr)&theTMTask);
	
	DSpShutdown();
	
	FlushEvents(nullEvent|mouseDown|mouseUp|keyDown|keyUp|autoKey|updateEvt,0);
	
	ShowCursor();
	
	};
	
/*
 *
 *The entry point for MacAllegro programs setup mac toolbox application memory 
 *This routine should be called before main() this is do by -main MacEntry option in final Link
 *if not called then the program can crash!!!
 *
 */

void MacEntry(){
	char argbuf[256];
	char *argv[64];
	int argc;
	int i, q;
	SysEnvRec envRec;long	heapSize;
	Str255 name;
	OSErr theError;

	InitGraf((Ptr) &qd.thePort);		/*init the normal mac toolbox*/
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	InitCursor();
	InitPalettes();

/*memory setup*/
	(void) SysEnvirons( curSysEnvVers, &envRec );if (envRec.machineType < 0 )ExitToShell();
	if (kStackNeeded > StackSpace()){
		SetApplLimit((Ptr) ((long) GetApplLimit() - kStackNeeded + StackSpace()));
		}
	heapSize = (long) GetApplLimit() - (long) ApplicationZone();
	if ( heapSize < kHeapNeeded){
		_mac_message("no enough memory");
		ExitToShell();
		};
	MaxApplZone();
	if(!(RTrapAvailable(_WaitNextEvent, ToolTrap)))ExitToShell();

/*	gDone			= false;*/
/*	gOApped			= false;	// probably not since users are supposed to DROP things!*/
/*	gHasAppleEvents	= Gestalt ( gestaltAppleEventsAttr, &aLong ) == noErr;*/

/*	if(!gHasAppleEvents)return;*/
/**/
/*	InitAEVTStuff ();*/

	if ((Ptr) DSpStartup == (Ptr) kUnresolvedCFragSymbolAddress)return;
	theError=DSpStartup();
	if(theError!=noErr)return;
	State=0;

	GetDateTime((unsigned long*) &qd.randSeed);
	
	HGetVol(name,&MainVol,&MainDir);	/*inicializa volume e directory*/

	if(MainCTable==NULL){				/*get our ColorTable*/
		init_mypalette();
		};
	
	_mac_init_system_bitmap();			/*init our system bitmap vtable*/

	FindMainDevice ();					/*get our main display*/

	atexit(&maccleanup);				/**/

	/* can't use parameter because it doesn't include the executable name */
	_mac_get_executable_name(argbuf,254);/*get command line*/

	argc = 0;
	i = 0;

	/* parse commandline into argc/argv format */
	while (argbuf[i]) {
		while ((argbuf[i]) && (uisspace(argbuf[i])))i++;
		if (argbuf[i]) {
			if ((argbuf[i] == '\'') || (argbuf[i] == '"')){
				q = argbuf[i++];
				if (!argbuf[i])break;
				}
			else q = 0;
			argv[argc++] = &argbuf[i];
			while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!uisspace(argbuf[i]))))i++;
			if (argbuf[i]){
				argbuf[i] = 0;
				i++;
				}
			}
		}
	theTMTask.tmAddr = NewTimerProc(MyTask); /*install our TMtask where are hooked the mouse,keyboard,timer polls*/
	theTMTask.tmWakeUp = 0;
	theTMTask.tmReserved = 0;
	InsTime((QElemPtr)&theTMTask);
	PrimeTime((QElemPtr)&theTMTask, 40);
	_timer_running=TRUE;

	HideCursor();							/*Uses Allegro Cursor*/
#if (TRACE_MAC_SYSTEM)
	fprintf(stdout,"MacEntry Done\n");
	fflush(stdout);
#endif
	main(argc, argv);						/*call the normal main*/
	FlushEvents(everyEvent,0);
	}
