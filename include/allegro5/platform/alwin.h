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
 *      Windows-specific header defines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_WINDOWS
   #error bad include
#endif

#include <windows.h>

/*******************************************/
/********** magic main emulation ***********/
/*******************************************/
#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(int, _WinMain, (void *_main, void *hInst, void *hPrev, char *Cmd, int nShow));

#ifdef __cplusplus
}
#endif


/* The following is due to torhu from A.cc (see
 * http://www.allegro.cc/forums/thread/596872/756993#target)
 */
#ifndef ALLEGRO_NO_MAGIC_MAIN
   #if defined _MSC_VER && !defined ALLEGRO_LIB_BUILD
      #pragma comment(linker,"/ENTRY:mainCRTStartup")
   #endif
#endif


/*******************************************/
/************* system drivers **************/
/*******************************************/
#define SYSTEM_DIRECTX           AL_ID('D','X',' ',' ')




/*******************************************/
/************* mouse drivers ***************/
/*******************************************/
#define MOUSE_DIRECTX            AL_ID('D','X',' ',' ')



/*******************************************/
/*************** gfx drivers ***************/
/*******************************************/
#define GFX_DIRECTX              AL_ID('D','X','A','C')
#define GFX_DIRECTX_ACCEL        AL_ID('D','X','A','C')
#define GFX_DIRECTX_SAFE         AL_ID('D','X','S','A')
#define GFX_DIRECTX_SOFT         AL_ID('D','X','S','O')
#define GFX_DIRECTX_WIN          AL_ID('D','X','W','N')
#define GFX_DIRECTX_OVL          AL_ID('D','X','O','V')
#define GFX_GDI                  AL_ID('G','D','I','B')
/* Direct3D driver */
#define GFX_DIRECT3D_WINDOWED    AL_ID('D','3','D','W')
#define GFX_DIRECT3D_FULLSCREEN  AL_ID('D','3','D','F')
#define GFX_DIRECT3D             GFX_DIRECT3D_FULLSCREEN


/********************************************/
/*************** sound drivers **************/
/********************************************/
#define DIGI_DIRECTX(n)          AL_ID('D','X','A'+(n),' ')
#define DIGI_DIRECTAMX(n)        AL_ID('A','X','A'+(n),' ')
#define DIGI_WAVOUTID(n)         AL_ID('W','O','A'+(n),' ')
#define MIDI_WIN32MAPPER         AL_ID('W','3','2','M')
#define MIDI_WIN32(n)            AL_ID('W','3','2','A'+(n))
#define MIDI_WIN32_IN(n)         AL_ID('W','3','2','A'+(n))



/*******************************************/
/************ joystick drivers *************/
/*******************************************/
#define AL_JOY_TYPE_DIRECTX      AL_ID('D','X',' ',' ')

AL_VAR(struct ALLEGRO_JOYSTICK_DRIVER, _al_joydrv_directx);

#define _AL_JOYSTICK_DRIVER_DIRECTX                                     \
   { AL_JOY_TYPE_DIRECTX,  &_al_joydrv_directx,    true  },

