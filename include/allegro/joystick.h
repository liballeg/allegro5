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
 *      Joystick routines.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _al_included_joystick_h
#define _al_included_joystick_h

#include "allegro/base.h"

AL_BEGIN_EXTERN_C



/* internal values */
#define _AL_MAX_JOYSTICK_AXES	   3
#define _AL_MAX_JOYSTICK_STICKS    8
#define _AL_MAX_JOYSTICK_BUTTONS   32



/* Abstract data type */
typedef struct AL_JOYSTICK AL_JOYSTICK;



/* All fields public and read-only */
typedef struct AL_JOYSTATE
{
   struct {
      float axis[_AL_MAX_JOYSTICK_AXES];        /* -1.0 to 1.0 */
   } stick[_AL_MAX_JOYSTICK_STICKS];
   int button[_AL_MAX_JOYSTICK_BUTTONS];        /* 0 to 32767 */
} AL_JOYSTATE;



/* Flags for al_joystick_stick_flags */
enum
{
   AL_JOYFLAG_DIGITAL  = 0x01,
   AL_JOYFLAG_ANALOGUE = 0x02
};



AL_FUNC(bool,           al_install_joystick,    (void));
AL_FUNC(void,           al_uninstall_joystick,  (void));

AL_FUNC(int,            al_num_joysticks,       (void));
AL_FUNC(AL_JOYSTICK*,   al_get_joystick,        (int joyn));
AL_FUNC(void,           al_release_joystick,    (AL_JOYSTICK*));
AL_FUNC(AL_CONST char*, al_joystick_name,       (AL_JOYSTICK*));

AL_FUNC(int,            al_joystick_num_sticks, (AL_JOYSTICK*));
AL_FUNC(int,            al_joystick_stick_flags,(AL_JOYSTICK*, int stick)); /* junk? */
AL_FUNC(AL_CONST char*, al_joystick_stick_name, (AL_JOYSTICK*, int stick));

AL_FUNC(int,            al_joystick_num_axes,   (AL_JOYSTICK*, int stick));
AL_FUNC(AL_CONST char*, al_joystick_axis_name,  (AL_JOYSTICK*, int stick, int axis));

AL_FUNC(int,		al_joystick_num_buttons,(AL_JOYSTICK*));
AL_FUNC(AL_CONST char*, al_joystick_button_name,(AL_JOYSTICK*, int buttonn));

AL_FUNC(void,           al_get_joystick_state,  (AL_JOYSTICK*, AL_JOYSTATE *ret_state));



AL_END_EXTERN_C

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
