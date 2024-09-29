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


/*
#ifdef A5O_SRC
   #error Currently BCC32 cannot build the library
#endif
*/


#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <malloc.h>


#pragma warn -8004  /* unused assigned value         */
#pragma warn -8008  /* condition always met          */
#pragma warn -8057  /* unused parameter              */
#pragma warn -8066  /* unreachable code              */


/* describe this platform */
#define A5O_PLATFORM_STR  "BCC32"
#define A5O_WINDOWS
#define A5O_I386
#define A5O_LITTLE_ENDIAN
#define A5O_GUESS_INTTYPES_OK
   /* TODO: check if BCC has inttypes.h and/or stdint.h */

#ifdef A5O_USE_CONSOLE
   #define A5O_NO_MAGIC_MAIN
#endif


/* describe how function prototypes look to BCC32 */
#if (defined A5O_STATICLINK)
   #define _AL_DLL
#elif (defined A5O_SRC)
   #define _AL_DLL   __declspec(dllexport)
#else
   #define _AL_DLL   __declspec(dllimport)
#endif

#define AL_VAR(type, name)             extern _AL_DLL type name
#define AL_ARRAY(type, name)           extern _AL_DLL type name[]
#define AL_FUNC(type, name, args)      extern _AL_DLL type name args
#define AL_METHOD(type, name, args)    type (*name) args
#define AL_FUNCPTR(type, name, args)   extern _AL_DLL type (*name) args


#define END_OF_INLINE(name)
//#define AL_INLINE(type, name, args, code)    extern __inline type name args code END_OF_INLINE(name)

#define INLINE       __inline

#undef AL_INLINE
#undef AL_INLINE_STATIC

#define AL_INLINE(type, name, args, code)        extern __inline type __cdecl name args code END_OF_INLINE(name)
#define AL_INLINE_STATIC(type, name, args, code) static __inline type name args code END_OF_INLINE(name)

#define int64_t      signed __int64
#define uint64_t     unsigned __int64

#define __func__  "FIXME"

#define _wfindfirst __wfindfirst
#define _wfindnext __wfindnext

#define WinMain _main

/* windows specific defines */
#ifdef NONAMELESSUNION
   #undef NONAMELESSUNION
#endif
/* This fixes 99.999999% of Borland C++Builder's problems with structs. */

/* arrange for other headers to be included later on */
#define A5O_EXTRA_HEADER     "allegro5/platform/alwin.h"
#define A5O_INTERNAL_HEADER  "allegro5/platform/aintwin.h"
#define A5O_INTERNAL_THREAD_HEADER "allegro5/platform/aintwthr.h"
