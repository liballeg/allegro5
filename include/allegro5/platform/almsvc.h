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
 *      Configuration defines for use with MSVC.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <malloc.h>

#include "allegro5/platform/alplatf.h"

#pragma warning (disable: 4200 4244 4305 4800)


/* describe this platform */
#ifdef A5O_STATICLINK
   #define A5O_PLATFORM_STR  "MSVC.s"
#else
   #define A5O_PLATFORM_STR  "MSVC"
#endif

#define A5O_WINDOWS
#define A5O_I386
#define A5O_LITTLE_ENDIAN
#define A5O_GUESS_INTTYPES_OK

#ifdef A5O_USE_CONSOLE
   #define A5O_NO_MAGIC_MAIN
#endif
  

/* describe how function prototypes look to MSVC */
#ifndef A5O_STATICLINK
   #ifdef A5O_SRC
      #define _AL_DLL   __declspec(dllexport)
   #else
      #define _AL_DLL   __declspec(dllimport)
   #endif
#else
   #define _AL_DLL
#endif

#define AL_VAR(type, name)             extern _AL_DLL type name
#define AL_ARRAY(type, name)           extern _AL_DLL type name[]
#define AL_FUNC(type, name, args)      _AL_DLL type __cdecl name args
#define AL_METHOD(type, name, args)    type (__cdecl *name) args
#define AL_FUNCPTR(type, name, args)   extern _AL_DLL type (__cdecl *name) args

#ifdef AL_INLINE
   #define END_OF_INLINE(name)         void *_force_instantiate_##name = name;
#else
   #define END_OF_INLINE(name)
#endif

#undef AL_INLINE
#undef AL_INLINE_STATIC

#define AL_INLINE(type, name, args, code)        __inline _AL_DLL type __cdecl name args code END_OF_INLINE(name)
#define AL_INLINE_STATIC(type, name, args, code) __inline type __cdecl name args code END_OF_INLINE(name)

#define INLINE       __inline

/* VC10 is the first version to define int64_t and uint64_t */
#if _MSC_VER < 1600
#define int64_t      signed __int64
#define uint64_t     unsigned __int64
#endif

/* __func__ is C99 */
#ifndef __func__
   /* MSVC versions before VC7 don't have __FUNCTION__ */
   #if _MSC_VER < 1300
      #define __func__ "???"
   #else
      #define __func__ __FUNCTION__
   #endif
#endif


/* life would be so easy if compilers would all use the same names! */
#if (!defined S_IRUSR)
   #define S_IRUSR   S_IREAD
   #define S_IWUSR   S_IWRITE
   #define S_IXUSR   S_IEXEC
#endif


/* arrange for other headers to be included later on */
#define A5O_EXTRA_HEADER     "allegro5/platform/alwin.h"
#define A5O_INTERNAL_HEADER  "allegro5/platform/aintwin.h"
#define A5O_INTERNAL_THREAD_HEADER "allegro5/platform/aintwthr.h"
