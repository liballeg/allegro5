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
 *      Configuration defines for use with Borland C++Builder.
 *
 *      By Greg Hackmann.
 *
 *      See readme.txt for copyright information.
 */


#ifdef ALLEGRO_SRC
   #error Currently BCC32 should only use the DLL
#endif

#ifndef SCAN_DEPEND
   #include <io.h>
   #include <fcntl.h>
   #include <direct.h>
   #include <malloc.h>
#endif

/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "BCC32"
#define ALLEGRO_WINDOWS
#define ALLEGRO_I386
#define ALLEGRO_LITTLE_ENDIAN

#ifdef USE_CONSOLE
   #define ALLEGRO_CONSOLE_OK
   #define ALLEGRO_NO_MAGIC_MAIN
#endif


#define AL_VAR(type, name)             extern _AL_DLL type name
#define AL_ARRAY(type, name)           extern _AL_DLL type name[]
#define AL_FUNC(type, name, args)      _AL_DLL type __cdecl name args
#define AL_METHOD(type, name, args)    type (__cdecl *name) args
#define AL_FUNCPTR(type, name, args)   extern _AL_DLL type (__cdecl *name) args

#if (defined ALLEGRO_STATICLINK) || (defined ALLEGRO_SRC)
   #define _AL_DLL
#else
   #define _AL_DLL   __declspec(dllimport)
#endif

#define END_OF_INLINE(name)
#define AL_INLINE(type, name, args, code)    extern __inline type __cdecl name args code END_OF_INLINE(name)

#define INLINE       __inline

#define LONG_LONG    __int64

/* windows specific defines */
#ifdef NONAMELESSUNION
   #undef NONAMELESSUNION
#endif
/* This fixes 99.999999% of Borland C++Builder's problems with structs. */

/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/alwin.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/aintwin.h"
