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
 *      Linux header file for the Allegro library.
 *
 *      This doesn't need to be included; it prototypes functions you
 *      can use to control the library more closely.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#ifndef LIN_ALLEGRO_H
#define LIN_ALLEGRO_H

#ifdef __cplusplus
   extern "C" {
#endif

#ifndef ALLEGRO_H
#error Please include allegro.h before linalleg.h!
#endif


/******************************************/
/************ Asynchronous I/O ************/
/******************************************/

#define ASYNC_OFF            0x00
#define ASYNC_DEFAULT        0x01
#define ASYNC_BSD            0x02
#define ASYNC_THREADS        0x03

typedef RETSIGTYPE (*SIGIO_HOOK)(int);

SIGIO_HOOK al_linux_install_sigio_hook (SIGIO_HOOK hook);
int al_linux_set_async_mode (unsigned type);
int al_linux_is_async_mode (void);


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef LIN_ALLEGRO_H */


