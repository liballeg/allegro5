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
 *      Macros for generating allegro.def in a platform independent way.
 *
 *      By Michael Rickmann.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_EXPORT
   #error bad include
#endif


#undef  AL_INLINE

#define AL_VAR(type, name)                   alldllvar name##_dll
#define AL_FUNCPTR(type, name, args)         alldllfpt name##_dll
#define AL_ARRAY(type, name)                 alldllarr name##_dll
#define AL_FUNC(type, name, args)            alldllfun name##_dll
#define AL_INLINE(type, name, args, code)    alldllinl name##_dll

#define ALLEGRO_EXTRA_HEADER     "allegro/alwin.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/aintwin.h"

#define ALLEGRO_WINDOWS
#define ALLEGRO_LITTLE_ENDIAN

