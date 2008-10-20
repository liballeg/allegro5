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

/* Include configuration information.  */
#include "allegro5/platform/alplatf.h"

#define ALLEGRO_INTERNAL_THREAD_HEADER "allegro5/platform/aintuthr.h"

/* Describe this platform */
#define ALLEGRO_PLATFORM_STR  "MacOS X"
#define ALLEGRO_CONSOLE_OK
#define ALLEGRO_USE_CONSTRUCTOR
#define ALLEGRO_MULTITHREADED

/* Provide implementations of missing functions.  */
#ifndef ALLEGRO_HAVE_STRICMP
#define ALLEGRO_NO_STRICMP
#endif

#ifndef ALLEGRO_HAVE_STRLWR
#define ALLEGRO_NO_STRLWR
#endif

#ifndef ALLEGRO_HAVE_STRUPR
#define ALLEGRO_NO_STRUPR
#endif

#ifndef ALLEGRO_HAVE_MEMCMP
#define ALLEGRO_NO_MEMCMP
#endif

/* Arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro5/platform/alosx.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro5/platform/aintosx.h"


#endif
