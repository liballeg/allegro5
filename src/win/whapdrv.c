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
 *      List of Windows joystick drivers, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_haptic.h"

#ifndef A5O_WINDOWS
#error something is wrong with the makefile
#endif

/* Construct the DirectInput haptics driver list */
_AL_BEGIN_HAPTIC_DRIVER_LIST
_AL_HAPTIC_DRIVER_DIRECTX
_AL_END_HAPTIC_DRIVER_LIST
