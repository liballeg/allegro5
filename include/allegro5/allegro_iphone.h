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
 */


#ifndef A5_IPHONE_ALLEGRO_H
#define A5_IPHONE_ALLEGRO_H

#ifdef __cplusplus
   extern "C" {
#endif

/*
 *  Public iPhone-related API
 */

AL_FUNC(void, al_iphone_program_has_halted,    (void));
AL_FUNC(void, al_iphone_override_screen_scale, (float scale));

#ifdef __cplusplus
   }
#endif

#endif /* A5_IPONE_ALLEGRO_H */
