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
 *      Base header, defines basic stuff needed by pretty much
 *      everything else.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#ifndef ALLEGRO_BASE_H
#define ALLEGRO_BASE_H

#ifndef ALLEGRO_NO_STD_HEADERS
   #include <errno.h>
   #ifdef _MSC_VER
      /* enable posix for limits.h and only limits.h
         enabling it for all msvc headers will potentially
	     disable a lot of commonly used msvcrt functions */
      #define _POSIX_
      #include <limits.h>
      #undef _POSIX_
   #else
      #include <limits.h>
   #endif
   #include <stdarg.h>
   #include <stddef.h>
   #include <stdlib.h>
   #include <time.h>
   #include <string.h>
   #include <sys/types.h>
#endif

#if (defined DEBUGMODE) && (defined FORTIFY)
   #include <fortify/fortify.h>
#endif

#if (defined DEBUGMODE) && (defined DMALLOC)
   #include <dmalloc.h>
#endif

#include "allegro5/internal/alconfig.h"

#ifdef __cplusplus
   #define AL_BEGIN_EXTERN_C	extern "C" {
   #define AL_END_EXTERN_C	}
#else  /* C */
   #define AL_BEGIN_EXTERN_C
   #define AL_END_EXTERN_C
#endif

#ifdef __cplusplus
   extern "C" {
#endif

#define ALLEGRO_VERSION          4
#define ALLEGRO_SUB_VERSION      9
#define ALLEGRO_WIP_VERSION      10
#define ALLEGRO_VERSION_STR      "4.9.10 (SVN)"
#define ALLEGRO_DATE_STR         "2009"
#define ALLEGRO_DATE             20090504    /* yyyymmdd */

/*******************************************/
/************ Some global stuff ************/
/*******************************************/

/* Type: ALLEGRO_PI
 */
#define ALLEGRO_PI        3.14159265358979323846

#define AL_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

typedef struct _DRIVER_INFO         /* info about a hardware driver */
{
   int id;                          /* integer ID */
   void *driver;                    /* the driver structure */
   int autodetect;                  /* set to allow autodetection */
} _DRIVER_INFO;

       

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_BASE_H */
