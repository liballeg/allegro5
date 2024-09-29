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

#ifndef __al_included_allegro5_joystick_h
#define __al_included_allegro5_joystick_h

#include "allegro5/base.h"
#include "allegro5/events.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* internal values */
#define _AL_MAX_JOYSTICK_AXES	   3
#define _AL_MAX_JOYSTICK_STICKS    16
#ifdef A5O_ANDROID
#define _AL_MAX_JOYSTICK_BUTTONS   36
#else
#define _AL_MAX_JOYSTICK_BUTTONS   32
#endif



/* Type: A5O_JOYSTICK
 */
typedef struct A5O_JOYSTICK A5O_JOYSTICK;



/* Type: A5O_JOYSTICK_STATE
 */
typedef struct A5O_JOYSTICK_STATE A5O_JOYSTICK_STATE;

struct A5O_JOYSTICK_STATE
{
   struct {
      float axis[_AL_MAX_JOYSTICK_AXES];        /* -1.0 to 1.0 */
   } stick[_AL_MAX_JOYSTICK_STICKS];
   int button[_AL_MAX_JOYSTICK_BUTTONS];        /* 0 to 32767 */
};


/* Enum: A5O_JOYFLAGS
 */
enum A5O_JOYFLAGS
{
   A5O_JOYFLAG_DIGITAL  = 0x01,
   A5O_JOYFLAG_ANALOGUE = 0x02
};



AL_FUNC(bool,           al_install_joystick,    (void));
AL_FUNC(void,           al_uninstall_joystick,  (void));
AL_FUNC(bool,           al_is_joystick_installed, (void));
AL_FUNC(bool,           al_reconfigure_joysticks, (void));

AL_FUNC(int,            al_get_num_joysticks,   (void));
AL_FUNC(A5O_JOYSTICK *, al_get_joystick,    (int joyn));
AL_FUNC(void,           al_release_joystick,    (A5O_JOYSTICK *));
AL_FUNC(bool,           al_get_joystick_active, (A5O_JOYSTICK *));
AL_FUNC(const char*,    al_get_joystick_name,   (A5O_JOYSTICK *));

AL_FUNC(int,            al_get_joystick_num_sticks, (A5O_JOYSTICK *));
AL_FUNC(int, al_get_joystick_stick_flags, (A5O_JOYSTICK *, int stick)); /* junk? */
AL_FUNC(const char*,    al_get_joystick_stick_name, (A5O_JOYSTICK *, int stick));

AL_FUNC(int,            al_get_joystick_num_axes,   (A5O_JOYSTICK *, int stick));
AL_FUNC(const char*,    al_get_joystick_axis_name,  (A5O_JOYSTICK *, int stick, int axis));

AL_FUNC(int,            al_get_joystick_num_buttons,  (A5O_JOYSTICK *));
AL_FUNC(const char*,    al_get_joystick_button_name,  (A5O_JOYSTICK *, int buttonn));

AL_FUNC(void,           al_get_joystick_state,  (A5O_JOYSTICK *, A5O_JOYSTICK_STATE *ret_state));

AL_FUNC(A5O_EVENT_SOURCE *, al_get_joystick_event_source, (void));

#ifdef __cplusplus
   }
#endif

#endif
