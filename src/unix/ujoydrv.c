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
 *      List of Unix joystick drivers.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_joystick.h"



_AL_BEGIN_JOYSTICK_DRIVER_LIST
#ifdef ALLEGRO_GP2XWIZ
   _AL_JOYSTICK_DRIVER_GP2XWIZ
#endif
#if defined ALLEGRO_HAVE_LINUX_JOYSTICK_H && defined ALLEGRO_WITH_XWINDOWS
   { _ALLEGRO_JOYDRV_LINUX,   &_al_joydrv_linux,   true  },
#endif
_AL_END_JOYSTICK_DRIVER_LIST
