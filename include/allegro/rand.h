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
 *      Private random number generator.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_RAND_H
#define ALLEGRO_RAND_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define _AL_RAND_MAX          0xffff

AL_FUNC(void, _al_srand, (int seed));
AL_FUNC(int, _al_rand, (void));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_RAND_H */


