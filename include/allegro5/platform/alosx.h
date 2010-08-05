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
 *      MacOS X specific header defines.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef __al_included_allegro5_alosx_h
#define __al_included_allegro5_alosx_h

#ifndef ALLEGRO_MACOSX
   #error bad include
#endif


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <signal.h>
   #include <pthread.h>
   #if defined __OBJC__ && defined ALLEGRO_SRC
      #import <mach/mach.h>
      #import <mach/mach_error.h>
      #import <AppKit/AppKit.h>
      #import <ApplicationServices/ApplicationServices.h>
      #import <Cocoa/Cocoa.h>
      #import <CoreAudio/CoreAudio.h>
      #import <AudioUnit/AudioUnit.h>
      #import <AudioToolbox/AudioToolbox.h>
      #import <QuickTime/QuickTime.h>
      #import <IOKit/IOKitLib.h>
      #import <IOKit/IOCFPlugIn.h>
      #import <IOKit/hid/IOHIDLib.h>
      #import <IOKit/hid/IOHIDKeys.h>
      #import <Kernel/IOKit/hidsystem/IOHIDUsageTables.h>
   #endif
#endif


/* The following code comes from alunix.h */
/* Magic to capture name of executable file */
extern int    __crt0_argc;
extern char **__crt0_argv;

#ifndef ALLEGRO_NO_MAGIC_MAIN
   #define ALLEGRO_MAGIC_MAIN
   #define main _al_mangled_main
   #ifdef __cplusplus
      extern "C" int _al_mangled_main(int, char **);
   #endif
#endif

/* System driver */
#define SYSTEM_MACOSX           AL_ID('O','S','X',' ')
/* Keyboard driver */
#define KEYBOARD_MACOSX         AL_ID('O','S','X','K')
/*AL_VAR(AL_KEYBOARD_DRIVER, keyboard_macosx); */

/* Mouse driver */
#define MOUSE_MACOSX            AL_ID('O','S','X','M')

/* Gfx drivers */
//#define GFX_QUARTZ_WINDOW       AL_ID('Q','Z','W','N')
#define GFX_QUARTZ_FULLSCREEN   AL_ID('Q','Z','F','L')
#define GFX_OPENGL_WINDOW       AL_ID('G','L','W','N')

/* Digital sound drivers */
#define DIGI_CORE_AUDIO         AL_ID('D','C','A',' ')
#define DIGI_SOUND_MANAGER      AL_ID('S','N','D','M')

/* MIDI music drivers */
#define MIDI_CORE_AUDIO         AL_ID('M','C','A',' ')
#define MIDI_QUICKTIME          AL_ID('Q','T','M',' ')

/* Joystick drivers */ 
#define JOYSTICK_HID            AL_ID('H','I','D','J') 

#endif

/* Local variables:       */
/* mode: objc             */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
