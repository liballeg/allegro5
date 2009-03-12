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

/* Title: Keyboard types
 */

#ifndef _al_included_keyboard_h
#define _al_included_keyboard_h

#include "allegro5/base.h"
#include "allegro5/keycodes.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: ALLEGRO_KEYBOARD
 */
typedef struct ALLEGRO_KEYBOARD ALLEGRO_KEYBOARD;



/* Type: ALLEGRO_KEYBOARD_STATE
 *		the state was saved.  If no display was focused, this points
 *		to NULL.
 */
typedef struct ALLEGRO_KEYBOARD_STATE
{
   struct ALLEGRO_DISPLAY *display;  /* public */
   /* internal */
   unsigned int __key_down__internal__[(ALLEGRO_KEY_MAX + 31) / 32)]; 
} ALLEGRO_KEYBOARD_STATE;


AL_FUNC(bool,         al_is_keyboard_installed,   (void));
AL_FUNC(bool,         al_install_keyboard,   (void));
AL_FUNC(void,         al_uninstall_keyboard, (void));

AL_FUNC(ALLEGRO_KEYBOARD*, al_get_keyboard,       (void));
AL_FUNC(bool,         al_set_keyboard_leds,  (int leds));

AL_FUNC(AL_CONST char *, al_keycode_to_name, (int keycode));

AL_FUNC(void,         al_get_keyboard_state, (ALLEGRO_KEYBOARD_STATE *ret_state));
AL_FUNC(bool,         al_key_down,           (const ALLEGRO_KEYBOARD_STATE *, int keycode));

/* TODO: use the config system */
AL_VAR(bool, _al_three_finger_flag);
AL_VAR(bool, _al_key_led_flag);

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
