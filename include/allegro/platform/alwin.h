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



/*******************************************/
/********** magic main emulation ***********/
/*******************************************/
AL_FUNC(int, _WinMain, (void *_main, void *hInst, void *hPrev, char *Cmd, int nShow));


#if (!defined ALLEGRO_NO_MAGIC_MAIN) && (!defined ALLEGRO_SRC)

   #define main _mangled_main

   #undef END_OF_MAIN

   #ifdef ALLEGRO_GCC
   #ifdef __cplusplus

      /* GCC version for C++ programs, using __attribute__ ((stdcall)) */
      #define END_OF_MAIN()                                                  \
									     \
	 extern "C" {                                                        \
	    int __attribute__ ((stdcall)) WinMain(void *hI, void *hP, char *Cmd, int nShow); \
									     \
	 }                                                                   \
									     \
	 int __attribute__ ((stdcall)) WinMain(void *hI, void *hP, char *Cmd, int nShow) \
	 {                                                                   \
	    return _WinMain((void *)_mangled_main, hI, hP, Cmd, nShow);      \
	 }

   #else    /* not C++ */

      /* GCC version for C programs, using __attribute__ ((stdcall)) */
      #define END_OF_MAIN()                                                  \
									     \
	 int __attribute__ ((stdcall)) WinMain(void *hI, void *hP, char *Cmd, int nShow); \
									     \
	 int __attribute__ ((stdcall)) WinMain(void *hI, void *hP, char *Cmd, int nShow) \
	 {                                                                   \
	    return _WinMain((void *)_mangled_main, hI, hP, Cmd, nShow);      \
	 }

   #endif   /* end of not C++ */
   #else    /* end of GCC */

      /* MSVC version, using __stdcall */
      #define END_OF_MAIN()                                                  \
									     \
	 int __stdcall WinMain(void *hI, void *hP, char *Cmd, int nShow)     \
	 {                                                                   \
	    return _WinMain((void *)_mangled_main, hI, hP, Cmd, nShow);      \
	 }

   #endif

#endif



/*******************************************/
/************* system drivers **************/
/*******************************************/
#define SYSTEM_DIRECTX           AL_ID('D','X',' ',' ')

AL_VAR(SYSTEM_DRIVER, system_directx);



/*******************************************/
/************** timer drivers **************/
/*******************************************/
#define TIMER_WIN32_HIGH_PERF    AL_ID('W','3','2','H')
#define TIMER_WIN32_LOW_PERF     AL_ID('W','3','2','L')

AL_VAR(TIMER_DRIVER, timer_win32_high_perf);
AL_VAR(TIMER_DRIVER, timer_win32_low_perf);



/*******************************************/
/************ keyboard drivers *************/
/*******************************************/
#define KEYBOARD_DIRECTX         AL_ID('D','X',' ',' ')

AL_VAR(KEYBOARD_DRIVER, keyboard_directx);



/*******************************************/
/************* mouse drivers ***************/
/*******************************************/
#define MOUSE_DIRECTX            AL_ID('D','X',' ',' ')

AL_VAR(MOUSE_DRIVER, mouse_directx);



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

AL_VAR(GFX_DRIVER, gfx_directx_accel);
AL_VAR(GFX_DRIVER, gfx_directx_safe);
AL_VAR(GFX_DRIVER, gfx_directx_soft);
AL_VAR(GFX_DRIVER, gfx_directx_win);
AL_VAR(GFX_DRIVER, gfx_directx_ovl);
AL_VAR(GFX_DRIVER, gfx_gdi);

#define GFX_DRIVER_DIRECTX                                              \
   {  GFX_DIRECTX_ACCEL,   &gfx_directx_accel,     TRUE  },             \
   {  GFX_DIRECTX_SOFT,    &gfx_directx_soft,      TRUE  },             \
   {  GFX_DIRECTX_SAFE,    &gfx_directx_safe,      TRUE  },             \
   {  GFX_DIRECTX_WIN,     &gfx_directx_win,       TRUE  },             \
   {  GFX_DIRECTX_OVL,     &gfx_directx_ovl,       TRUE  },             \
   {  GFX_GDI,             &gfx_gdi,               FALSE },

#define GFX_SAFE_ID              GFX_DIRECTX_SAFE
#define GFX_SAFE_DEPTH           8
#define GFX_SAFE_W               640
#define GFX_SAFE_H               480



/********************************************/
/*************** sound drivers **************/
/********************************************/
#define DIGI_DIRECTX(n)          AL_ID('D','X','A'+(n),' ')
#define DIGI_DIRECTAMX(n)        AL_ID('A','X','A'+(n),' ')
#define DIGI_WAVOUTID(n)         AL_ID('W','O','A'+(n),' ')
#define MIDI_WIN32MAPPER         AL_ID('W','3','2','M')
#define MIDI_WIN32(n)            AL_ID('W','3','2','A'+(n))



/*******************************************/
/************ joystick drivers *************/
/*******************************************/
#define JOY_TYPE_WIN32           AL_ID('W','3','2',' ')

AL_VAR(JOYSTICK_DRIVER, joystick_win32);

#define JOYSTICK_DRIVER_WIN32                                     \
      { JOY_TYPE_WIN32,          &joystick_win32,  TRUE  },

