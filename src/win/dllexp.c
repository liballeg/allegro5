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
 *      Macros for generating allegro.def in a compiler independent way.
 *
 *      By Michael Rickmann.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_EXPORT
   #error in the fixdll script
#endif


#define AL_VAR(type, name)                   alldllvar name##_dll
#define AL_FUNCPTR(type, name, args)         alldllfpt name##_dll
#define AL_ARRAY(type, name)                 alldllarr name##_dll
#define AL_FUNC(type, name, args)            alldllfun name##_dll
#define AL_INLINE(type, name, args, code)    alldllinl name##_dll


/* define the platform */
#define ALLEGRO_WINDOWS
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_COLOR8
#define ALLEGRO_COLOR16
#define ALLEGRO_COLOR24
#define ALLEGRO_COLOR32


#if defined ALLEGRO_API

   #include "allegro.h"

#elif defined ALLEGRO_WINAPI

   #define ALLEGRO_H
   #include "winalleg.h"
   #include "allegro/alwin.h"

#elif defined ALLEGRO_INTERNALS

   #define ALLEGRO_H
   #include "allegro/aintern.h"

#endif

