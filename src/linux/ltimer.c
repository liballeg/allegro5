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
 *      Linux timer driver list.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintunix.h"


/* list the available drivers */
_DRIVER_INFO _linux_timer_driver_list[] =
{
   {  TIMERDRV_UNIX,     &timerdrv_unix,     TRUE  },
   {  0,                 NULL,               0     }
};

