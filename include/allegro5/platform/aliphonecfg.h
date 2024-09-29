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
 *      Configuration defines for use on iOS.
 *
 *      See readme.txt for copyright information.
 */


#include <fcntl.h>
#include <unistd.h>

/* Describe this platform.  */
#define A5O_PLATFORM_STR  "IPHONE"

#define A5O_EXTRA_HEADER "allegro5/platform/aliphone.h"
#define A5O_INTERNAL_HEADER "allegro5/platform/aintiphone.h"
#define A5O_INTERNAL_THREAD_HEADER "allegro5/platform/aintuthr.h"

#define A5O_EXCLUDE_GLX

#ifndef AL_INLINE
#define AL_INLINE(type, name, args, code)    \
static __inline__ type name args;         \
static __inline__ type name args code
#endif
