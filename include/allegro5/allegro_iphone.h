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


#ifndef A5_IPHONE_A5O_H
#define A5_IPHONE_A5O_H

#ifdef __cplusplus
   extern "C" {
#endif

/*
 *  Public iPhone-related API
 */

enum A5O_IPHONE_STATUSBAR_ORIENTATION {
	A5O_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT = 0,
	A5O_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT_UPSIDE_DOWN,
	A5O_IPHONE_STATUSBAR_ORIENTATION_LANDSCAPE_RIGHT,
	A5O_IPHONE_STATUSBAR_ORIENTATION_LANDSCAPE_LEFT
};

AL_FUNC(void,   al_iphone_set_statusbar_orientation, (int orientation));
AL_FUNC(double, al_iphone_get_last_shake_time,       (void));
AL_FUNC(float,  al_iphone_get_battery_level,         (void));

#ifdef __cplusplus
   }
#endif

#endif /* A5_IPONE_A5O_H */
