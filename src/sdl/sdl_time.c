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
#include "allegro5/platform/allegro_sdl_thread.h"
#include "allegro5/debug.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

double al_get_time(void)
{
   return 1.0 * SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
}



void al_rest(double seconds)
{
   SDL_Delay(seconds * 1000);
}



void al_init_timeout(ALLEGRO_TIMEOUT *timeout, double seconds)
{
   ALLEGRO_TIMEOUT_SDL *timeout_sdl = (void *)timeout;
   timeout_sdl->ms = seconds * 1000;
}

/* vim: set sts=3 sw=3 et */
