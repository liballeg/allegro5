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
 *      Dynamic driver lists shared by Unixy system drivers.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_system.h"

#if defined A5O_WITH_XWINDOWS
#ifndef A5O_RASPBERRYPI
#include "allegro5/platform/aintxglx.h"
#else
#include "allegro5/internal/aintern_raspberrypi.h"
#endif
#endif



/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces(void)
{
   A5O_SYSTEM_INTERFACE **add;
#if defined A5O_WITH_XWINDOWS && !defined A5O_RASPBERRYPI
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_xglx_driver();
#elif defined A5O_RASPBERRYPI
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_raspberrypi_driver();
#endif
}

