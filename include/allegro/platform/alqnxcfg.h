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
 *      Configuration defines for use with QNX.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <fcntl.h>
   #include <unistd.h>
#endif


/* a static auto config */
#define HAVE_STRICMP
#define HAVE_STRLWR
#define HAVE_STRUPR
#define HAVE_MEMCMP
#define HAVE_MKSTEMP
#define HAVE_UNISTD_H
#define HAVE_FCNTL_H
#define HAVE_LIMITS_H
#define HAVE_DIRENT_H
#define HAVE_SYS_DIR_H
#define HAVE_SYS_UTSNAME_H
#define HAVE_SYS_TIME_H
#define TIME_WITH_SYS_TIME
#define HAVE_LIBPTHREAD

/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "QNX"
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_CONSOLE_OK
#define ALLEGRO_USE_SCHED_YIELD

/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/alqnx.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintqnx.h"
#define ALLEGRO_ASMCAPA_HEADER   "obj/qnx/asmcapa.h"
