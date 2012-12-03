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
 *      Fullscreen mode queries.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"


/* Function: al_get_num_display_modes
 */
int al_get_num_display_modes(void)
{
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   return system->vt->get_num_display_modes();
}


/* Function: al_get_display_mode
 */
ALLEGRO_DISPLAY_MODE *al_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode)
{
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   return system->vt->get_display_mode(index, mode);
}


/* vim: set sts=3 sw=3 et: */
