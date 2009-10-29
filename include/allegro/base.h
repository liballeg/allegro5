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
   #include <limits.h>
   #include <stdarg.h>
   #include <stddef.h>
   #include <stdlib.h>
   #include <time.h>
   #include <string.h>
#endif

#if (defined DEBUGMODE) && (defined FORTIFY)
   #include <fortify/fortify.h>
#endif

#if (defined DEBUGMODE) && (defined DMALLOC)
   #include <dmalloc.h>
#endif

#include "internal/alconfig.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define ALLEGRO_VERSION          4
#define ALLEGRO_SUB_VERSION      2
#define ALLEGRO_WIP_VERSION      3
#define ALLEGRO_VERSION_STR      "4.2.3"
#define ALLEGRO_DATE_STR         "2009"
#define ALLEGRO_DATE             20091029    /* yyyymmdd */

/*******************************************/
/************ Some global stuff ************/
/*******************************************/

#ifndef TRUE 
   #define TRUE         -1
   #define FALSE        0
#endif

#undef MIN
#undef MAX
#undef MID
     
#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))
     
#undef ABS
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))
     
#undef SGN
#define SGN(x)       (((x) >= 0) ? 1 : -1)

#define AL_PI        3.14159265358979323846

#define AL_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

AL_VAR(int *, allegro_errno);

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


