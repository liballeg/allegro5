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
 *      Configuration defines for use with BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */


#include <fcntl.h>
#include <unistd.h>

/* provide implementations of missing functions */
#define ALLEGRO_NO_STRICMP
#define ALLEGRO_NO_STRLWR
#define ALLEGRO_NO_STRUPR

/* a static auto config */
#define HAVE_DIRENT_H
#define HAVE_SYS_DIR_H
#define HAVE_SYS_TIME_H
#define TIME_WITH_SYS_TIME

/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "BeOS"
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_CONSOLE_OK

/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/albeos.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintbeos.h"
#define ALLEGRO_ASMCAPA_HEADER   "obj/beos/asmcapa.h"
