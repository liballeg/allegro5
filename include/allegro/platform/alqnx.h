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
 *      QNX specific driver definitions.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


/* magic to capture name of executable file */
extern int    __crt0_argc;
extern char **__crt0_argv;

#ifndef USE_CONSOLE
   #define main _mangled_main
   #undef END_OF_MAIN
   #define END_OF_MAIN() void *_mangled_main_address = (void*) _mangled_main;
#else
   #undef END_OF_MAIN
   #define END_OF_MAIN() void *_mangled_main_address;
#endif


/* System driver */
#define SYSTEM_QNX            AL_ID('Q','S','Y','S')
AL_VAR(SYSTEM_DRIVER, system_qnx);


/* Keyboard driver */
#define KEYBOARD_QNX          AL_ID('Q','K','E','Y')
AL_VAR(KEYBOARD_DRIVER, keyboard_qnx);


/* Mouse driver */
#define MOUSE_QNX             AL_ID('Q','M','S','E')
AL_VAR(MOUSE_DRIVER, mouse_qnx);


/* Sound driver */
#define DIGI_ALSA             AL_ID('A','L','S','A')
AL_VAR(DIGI_DRIVER, digi_alsa);


/* MIDI driver */
#define MIDI_ALSA             AL_ID('A','M','I','D')
AL_VAR(MIDI_DRIVER, midi_alsa);


/* Timer driver */
#define TIMERDRV_UNIX_PTHREADS	AL_ID('P','T','H','D')
AL_VAR(TIMER_DRIVER, timerdrv_unix_pthreads);


/* Graphics drivers */
#define GFX_PHOTON            AL_ID('Q','P','H',' ')
#define GFX_PHOTON_DIRECT     AL_ID('Q','P','H','D')
AL_VAR(GFX_DRIVER, gfx_photon);
AL_VAR(GFX_DRIVER, gfx_photon_direct);



#define GFX_SAFE_ID           GFX_PHOTON
#define GFX_SAFE_DEPTH        8
#define GFX_SAFE_W            320
#define GFX_SAFE_H            200
