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
 *      Configuration defines for use on Raspberry Pi platforms.
 *
 *      By Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

/* Describe this platform.  */
#define A5O_PLATFORM_STR  "RaspberryPi"

#define A5O_EXTRA_HEADER "allegro5/platform/alraspberrypi.h"
#define A5O_INTERNAL_HEADER "allegro5/platform/aintraspberrypi.h"
#define A5O_INTERNAL_THREAD_HEADER "allegro5/platform/aintuthr.h"

#define A5O_EXCLUDE_GLX
