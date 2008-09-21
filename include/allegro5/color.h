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
 *      Color manipulation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_COLOR_H
#define ALLEGRO_COLOR_H

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

AL_VAR(int, _rgb_r_shift_15);
AL_VAR(int, _rgb_g_shift_15);
AL_VAR(int, _rgb_b_shift_15);
AL_VAR(int, _rgb_r_shift_16);
AL_VAR(int, _rgb_g_shift_16);
AL_VAR(int, _rgb_b_shift_16);
AL_VAR(int, _rgb_r_shift_24);
AL_VAR(int, _rgb_g_shift_24);
AL_VAR(int, _rgb_b_shift_24);
AL_VAR(int, _rgb_r_shift_32);
AL_VAR(int, _rgb_g_shift_32);
AL_VAR(int, _rgb_b_shift_32);
AL_VAR(int, _rgb_a_shift_32);

AL_ARRAY(int, _rgb_scale_1);
AL_ARRAY(int, _rgb_scale_4);
AL_ARRAY(int, _rgb_scale_5);
AL_ARRAY(int, _rgb_scale_6);

/*
#define MASK_COLOR_8       0
#define MASK_COLOR_15      0x7C1F
#define MASK_COLOR_16      0xF81F
#define MASK_COLOR_24      0xFF00FF
#define MASK_COLOR_32      0xFF00FF
*/

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_COLOR_H */


