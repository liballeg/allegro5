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

#ifndef __al_included_allegro5_keyboard_h
#define __al_included_allegro5_keyboard_h

#include "allegro5/base.h"
#include "allegro5/events.h"
#include "allegro5/keycodes.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct A5O_KEYBOARD A5O_KEYBOARD;



/* Type: A5O_KEYBOARD_STATE
 */
typedef struct A5O_KEYBOARD_STATE A5O_KEYBOARD_STATE;

struct A5O_KEYBOARD_STATE
{
   struct A5O_DISPLAY *display;  /* public */
   /* internal */
   unsigned int __key_down__internal__[(A5O_KEY_MAX + 31) / 32];
};


AL_FUNC(bool,         al_is_keyboard_installed,   (void));
AL_FUNC(bool,         al_install_keyboard,   (void));
AL_FUNC(void,         al_uninstall_keyboard, (void));

AL_FUNC(bool,         al_can_set_keyboard_leds,  (void));
AL_FUNC(bool,         al_set_keyboard_leds,  (int leds));

AL_FUNC(const char *, al_keycode_to_name, (int keycode));

AL_FUNC(void,         al_get_keyboard_state, (A5O_KEYBOARD_STATE *ret_state));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(void,         al_clear_keyboard_state, (A5O_DISPLAY *display));
#endif
AL_FUNC(bool,         al_key_down,           (const A5O_KEYBOARD_STATE *, int keycode));

AL_FUNC(A5O_EVENT_SOURCE *, al_get_keyboard_event_source, (void));

#ifdef __cplusplus
   }
#endif

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
