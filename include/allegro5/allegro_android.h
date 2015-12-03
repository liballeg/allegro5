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


#ifndef A5_ANDROID_ALLEGRO_H
#define A5_ANDROID_ALLEGRO_H

#ifdef __cplusplus
   extern "C" {
#endif

/*
 *  Public android-related API
 */
void al_android_set_apk_file_interface(void);
const char *al_android_get_os_version(void);
void al_android_set_apk_fs_interface(void);

/* XXX decide if this should be public */
void _al_android_set_capture_volume_keys(ALLEGRO_DISPLAY *display, bool onoff);

#ifdef __cplusplus
   }
#endif

#endif /* A5_ANDROID_ALLEGRO_H */
