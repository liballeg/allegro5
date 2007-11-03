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

/* Title: Joystick routines
 */

#ifndef _al_included_joystick_h
#define _al_included_joystick_h

#include "allegro/base.h"

AL_BEGIN_EXTERN_C



/* internal values */
#define _AL_MAX_JOYSTICK_AXES	   3
#define _AL_MAX_JOYSTICK_STICKS    8
#define _AL_MAX_JOYSTICK_BUTTONS   32



/* Type: ALLEGRO_JOYSTICK
 *
 *  This is an abstract data type representing a physical joystick.  Joystick
 *  objects are also event sources so can be casted to ALLEGRO_EVENT_SOURCE*.
 */
typedef struct ALLEGRO_JOYSTICK ALLEGRO_JOYSTICK;



/* Type: ALLEGRO_JOYSTATE
 *
 * This is a structure that is used to hold a "snapshot" of a
 * joystick's axes and buttons at a particular instant.
 * All fields public and read-only.
 *
 * > struct {
 * >	float axis[num_axes];		    // -1.0 to 1.0 
 * > } stick[num_sticks];
 * > int button[num_buttons];		    // 0 to 32767
 */
typedef struct ALLEGRO_JOYSTATE
{
   struct {
      float axis[_AL_MAX_JOYSTICK_AXES];        /* -1.0 to 1.0 */
   } stick[_AL_MAX_JOYSTICK_STICKS];
   int button[_AL_MAX_JOYSTICK_BUTTONS];        /* 0 to 32767 */
} ALLEGRO_JOYSTATE;



/* Flags for al_joystick_stick_flags */
typedef int ALLEGRO_JOYFLAGS;

/* Enum: ALLEGRO_JOYFLAGS
 *
 * Joystick flags.
 *
 * ALLEGRO_JOYFLAG_DIGITAL  - the stick provides digital input
 * ALLEGRO_JOYFLAG_ANALOGUE - the stick provides analogue input
 *
 * (this enum is a holdover from the old API and may be removed)
 */
enum
{
   ALLEGRO_JOYFLAG_DIGITAL  = 0x01,
   ALLEGRO_JOYFLAG_ANALOGUE = 0x02
};



AL_FUNC(bool,           al_install_joystick,    (void));
AL_FUNC(void,           al_uninstall_joystick,  (void));

AL_FUNC(int,            al_num_joysticks,       (void));
AL_FUNC(ALLEGRO_JOYSTICK*,   al_get_joystick,        (int joyn));
AL_FUNC(void,           al_release_joystick,    (ALLEGRO_JOYSTICK*));
AL_FUNC(const char*,    al_joystick_name,       (ALLEGRO_JOYSTICK*));

AL_FUNC(int,            al_joystick_num_sticks, (const ALLEGRO_JOYSTICK*));
AL_FUNC(ALLEGRO_JOYFLAGS,    al_joystick_stick_flags,(const ALLEGRO_JOYSTICK*, int stick)); /* junk? */
AL_FUNC(const char*,    al_joystick_stick_name, (const ALLEGRO_JOYSTICK*, int stick));

AL_FUNC(int,            al_joystick_num_axes,   (const ALLEGRO_JOYSTICK*, int stick));
AL_FUNC(const char*,    al_joystick_axis_name,  (const ALLEGRO_JOYSTICK*, int stick, int axis));

AL_FUNC(int,            al_joystick_num_buttons,(const ALLEGRO_JOYSTICK*));
AL_FUNC(const char*,    al_joystick_button_name,(const ALLEGRO_JOYSTICK*, int buttonn));

AL_FUNC(void,           al_get_joystick_state,  (ALLEGRO_JOYSTICK*, ALLEGRO_JOYSTATE *ret_state));



AL_END_EXTERN_C

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
