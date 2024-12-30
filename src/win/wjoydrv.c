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
#include "allegro5/internal/aintern_joystick.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif



_AL_BEGIN_JOYSTICK_DRIVER_LIST
_AL_JOYSTICK_DRIVER_DIRECTX
#ifdef ALLEGRO_CFG_XINPUT
_AL_JOYSTICK_DRIVER_XINPUT
_AL_JOYSTICK_DRIVER_WINDOWS_ALL
#endif
_AL_END_JOYSTICK_DRIVER_LIST
