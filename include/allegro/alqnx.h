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
 *      QNX specific driver definitions.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __QNX__
#error Bad include!
#endif


/* System driver */
#define SYSTEM_QNX            AL_ID('Q','S','Y','S')
AL_VAR(SYSTEM_DRIVER, system_qnx);


/* Sound driver */
#define DIGI_ALSA             AL_ID('A','L','S','A')
AL_VAR(DIGI_DRIVER, digi_alsa);


/* MIDI driver */
#define MIDI_ALSA             AL_ID('A','M','I','D')
AL_VAR(MIDI_DRIVER, midi_alsa);


/* Timer driver */
#define TIMERDRV_UNIX         AL_ID('U','N','I','X')
AL_VAR(TIMER_DRIVER, timerdrv_unix);



/* #define GFX_SAFE_ID           GFX_PHOTON */
#define GFX_SAFE_DEPTH        8
#define GFX_SAFE_W            640
#define GFX_SAFE_H            480
