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
 *      List of MacOS X drivers.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif






BEGIN_GFX_DRIVER_LIST

END_GFX_DRIVER_LIST


BEGIN_DIGI_DRIVER_LIST
{   DIGI_CORE_AUDIO,          &digi_core_audio,         TRUE  },
END_DIGI_DRIVER_LIST


BEGIN_MIDI_DRIVER_LIST
{   MIDI_CORE_AUDIO,          &midi_core_audio,         TRUE  },
END_MIDI_DRIVER_LIST


 _AL_BEGIN_JOYSTICK_DRIVER_LIST
 _AL_END_JOYSTICK_DRIVER_LIST
