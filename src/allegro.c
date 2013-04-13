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
 *      Assorted routines.
 *
 *      By Shawn Hargreaves and Allegro developers.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/platform/alplatf.h"



/* Function: al_get_allegro_version
 */
uint32_t al_get_allegro_version(void)
{
   return ALLEGRO_VERSION_INT;
}



/* Function: al_run_main
 */
int al_run_main(int argc, char **argv, int (*user_main)(int, char **))
{
#ifdef ALLEGRO_MACOSX
    return _al_osx_run_main(argc, argv, user_main);
#else
    return user_main(argc, argv);
#endif
}


/* vim: set sts=3 sw=3 et: */
