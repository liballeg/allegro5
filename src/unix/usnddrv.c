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
#if (defined DIGI_ESD) && (!defined ALLEGRO_WITH_MODULES)
   DIGI_DRIVER_ESD
#endif
#if (defined DIGI_ALSA) && (!defined ALLEGRO_WITH_MODULES)
   DIGI_DRIVER_ALSA
#endif
#ifdef DIGI_OSS
   DIGI_DRIVER_OSS
#endif
END_DIGI_DRIVER_LIST


BEGIN_MIDI_DRIVER_LIST
   MIDI_DRIVER_DIGMID
#if (defined MIDI_ALSA) && (!defined ALLEGRO_WITH_MODULES)
   MIDI_DRIVER_ALSA
#endif
#ifdef MIDI_OSS
   MIDI_DRIVER_OSS
#endif
END_MIDI_DRIVER_LIST

