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
 *      SDL time driver.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "SDL.h"

#include "allegro5/altime.h"
#include "allegro5/platform/allegro_internal_sdl.h"
#include "allegro5/debug.h"

A5O_DEBUG_CHANNEL("SDL")

double _al_sdl_get_time(void)
{
   return 1.0 * SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
}



void _al_sdl_rest(double seconds)
{
   SDL_Delay(seconds * 1000);
}



void _al_sdl_init_timeout(A5O_TIMEOUT *timeout, double seconds)
{
   A5O_TIMEOUT_SDL *timeout_sdl = (void *)timeout;
   timeout_sdl->ms = seconds * 1000;
}

/* vim: set sts=3 sw=3 et */
