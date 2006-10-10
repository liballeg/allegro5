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
 *      Configuration defines for use with MacOS X.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALOSXCFG_H
#define ALOSXCFG_H

/* Provide implementations of missing functions */
#define ALLEGRO_NO_STRICMP
#define ALLEGRO_NO_STRLWR
#define ALLEGRO_NO_STRUPR

/* Provide implementations of missing definitions */
#define O_BINARY     0

/* Override default definitions for this platform */
#define AL_RAND()    ((rand() >> 16) & 0x7fff)

/* A static auto config */
#define HAVE_LIBPTHREAD
#define HAVE_DIRENT_H
#define HAVE_INTTYPES_H
#define HAVE_STDINT_H
#define HAVE_SYS_DIR_H
#define HAVE_SYS_TIME_H
#define TIME_WITH_SYS_TIME
#define HAVE_SYS_STAT_H
#define HAVE_MKSTEMP

/* Describe this platform */
#define ALLEGRO_PLATFORM_STR  "MacOS X"
#define ALLEGRO_CONSOLE_OK
#define ALLEGRO_USE_CONSTRUCTOR
#define ALLEGRO_MULTITHREADED

/* Endianesse - different between Intel and PPC based Mac's */
#ifdef __LITTLE_ENDIAN__
   #define ALLEGRO_LITTLE_ENDIAN
#endif
#ifdef __BIG_ENDIAN__
   #define ALLEGRO_BIG_ENDIAN
#endif

/* Exclude ASM */

#ifndef ALLEGRO_NO_ASM
	#define ALLEGRO_NO_ASM
#endif

/* Arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/alosx.h"


#endif
