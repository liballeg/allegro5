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
 *      List of Unix sound drivers.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



BEGIN_DIGI_DRIVER_LIST
#ifdef DIGI_ESD
   DIGI_DRIVER_ESD
#endif
#ifdef DIGI_ALSA
   DIGI_DRIVER_ALSA
#endif
#ifdef DIGI_OSS
   DIGI_DRIVER_OSS
#endif
END_DIGI_DRIVER_LIST


BEGIN_MIDI_DRIVER_LIST
   MIDI_DRIVER_DIGMID
#ifdef MIDI_OSS
   MIDI_DRIVER_OSS
#endif
END_MIDI_DRIVER_LIST

