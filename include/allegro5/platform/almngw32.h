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


#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <malloc.h>

#include "allegro5/platform/alplatf.h"


/* describe this platform */
#ifdef A5O_STATICLINK
   #define A5O_PLATFORM_STR  "MinGW32.s"
#else
   #define A5O_PLATFORM_STR  "MinGW32"
#endif

#define A5O_WINDOWS
#define A5O_I386
#define A5O_LITTLE_ENDIAN

#ifdef A5O_USE_CONSOLE
   #define A5O_NO_MAGIC_MAIN
#endif


/* describe how function prototypes look to MINGW32 */
#if (defined A5O_STATICLINK) || (defined A5O_SRC)
   #define _AL_DLL
#else
   #define _AL_DLL   __declspec(dllimport)
#endif

#define AL_VAR(type, name)                   extern _AL_DLL type name
#define AL_ARRAY(type, name)                 extern _AL_DLL type name[]
#define AL_FUNC(type, name, args)            extern type name args
#define AL_METHOD(type, name, args)          type (*name) args
#define AL_FUNCPTR(type, name, args)         extern _AL_DLL type (*name) args


/* windows specific defines */

#if (defined A5O_SRC)
/* pathches to handle DX7 headers on a win9x system */

/* should WINNT be defined on win9x systems? */
#ifdef WINNT
   #undef WINNT
#endif

/* defined in windef.h */
#ifndef HMONITOR_DECLARED
   #define HMONITOR_DECLARED 1
#endif

#endif /* A5O_SRC */

/* another instance of missing constants in the mingw32 headers */
#ifndef ENUM_CURRENT_SETTINGS
   #define ENUM_CURRENT_SETTINGS       ((DWORD)-1)
#endif

/* arrange for other headers to be included later on */
#define A5O_EXTRA_HEADER     "allegro5/platform/alwin.h"
#define A5O_INTERNAL_HEADER  "allegro5/platform/aintwin.h"
#define A5O_INTERNAL_THREAD_HEADER "allegro5/platform/aintwthr.h"
