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
 *      Keyboard routines.
 *
 *      See readme.txt for copyright information.
 */


#ifndef _al_included_keyboard_h
#define _al_included_keyboard_h

#include "allegro/base.h"
#include "allegro/keycodes.h"

AL_BEGIN_EXTERN_C



/* Abstract data type */
typedef struct AL_KEYBOARD AL_KEYBOARD;



typedef struct AL_KBDSTATE
{
   struct AL_DISPLAY *display;  /* public */
   unsigned int __key_down__internal__[4]; /* internal */
   /* Was using uint32_t, but some machines don't have stdint.h. */ 
#if AL_KEY_MAX >= (32 * 4)
# error _key_down array not big enough for AL_KEY_MAX
#endif
} AL_KBDSTATE;



AL_FUNC(bool,         al_install_keyboard,   (void));
AL_FUNC(void,         al_uninstall_keyboard, (void));

AL_FUNC(AL_KEYBOARD*, al_get_keyboard,       (void));
AL_FUNC(bool,         al_set_keyboard_leds,  (int leds));

AL_FUNC(void,         al_get_keyboard_state, (AL_KBDSTATE *ret_state));
AL_FUNC(bool,         al_key_down,           (AL_KBDSTATE *, int keycode));

/* TODO: use the config system */
AL_VAR(bool, _al_three_finger_flag);
AL_VAR(bool, _al_key_led_flag);



AL_END_EXTERN_C

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
