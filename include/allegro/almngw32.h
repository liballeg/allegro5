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
 *      Configuration defines for use with Mingw32.
 *
 *      By Michael Rickmann.
 *
 *      Native build version by Henrik Stokseth.
 *
 *      See readme.txt for copyright information.
 */


#ifndef __MINGW32__
   #error bad include
#endif


#ifndef SCAN_DEPEND
   #include <io.h>
   #include <fcntl.h>
   #include <direct.h>
   #include <malloc.h>
#endif


/* describe this platform */
#ifdef ALLEGRO_STATICLINK
   #define ALLEGRO_PLATFORM_STR  "Mingw32.s"
#else
   #define ALLEGRO_PLATFORM_STR  "Mingw32"
#endif

#define ALLEGRO_MINGW32
#define ALLEGRO_WINDOWS
#define ALLEGRO_I386
#define ALLEGRO_LITTLE_ENDIAN

#ifdef USE_CONSOLE
   #define ALLEGRO_CONSOLE_OK
#endif


/* describe how function prototypes look to MINGW32 */
#if (defined ALLEGRO_STATICLINK) || (defined ALLEGRO_SRC)
   #define _AL_DLL
#else
   #define _AL_DLL   __declspec(dllimport)
#endif

#define AL_VAR(type, name)                   extern _AL_DLL type name
#define AL_ARRAY(type, name)                 extern _AL_DLL type name[]
#define AL_FUNC(type, name, args)            type __cdecl name args
#define AL_METHOD(type, name, args)          type (*name) args
#define AL_FUNCPTR(type, name, args)         extern _AL_DLL type (*name) args


/* windows specific defines */
#define NONAMELESSUNION


/* describe the asm syntax for this platform */
#define ALLEGRO_ASM_PREFIX    "_"
#define ALLEGRO_MMX


/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/alwin.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/aintwin.h"
#define ALLEGRO_MMX_HEADER       "obj/mingw32/mmx.h"


