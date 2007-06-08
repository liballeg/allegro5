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
 */

//_DRIVER_INFO _al_d3d_keyboard_driver_list[1];

#include "allegro.h"
#include "allegro/internal/aintern_display.h"
#include "allegro/platform/ald3d.h"
#include "allegro/internal/aintern_keyboard.h"
#include "allegro/platform/aintwin.h"

#include "d3d.h"

bool _al_d3d_keyboard_initialized = false;

static bool (*orig_keyboard_init)();

static bool _d3d_keyboard_init()
{
	unsigned int i;

	orig_keyboard_init();

	if (_al_d3d_system) {
		for (i = 0; i < _al_d3d_system->system.displays._size; i++) {
			AL_DISPLAY_D3D **dptr = _al_vector_ref(&_al_d3d_system->system.displays, i);
			AL_DISPLAY_D3D *d = *dptr;
			if (d->keyboard_initialized == false) {
				d->keyboard_initialized = true;
				key_dinput_set_cooperation_level(d->window);
			}
		}
		
		if (i > 0)
			_al_d3d_win_grab_input();

		_al_d3d_keyboard_initialized = true;
	}

	return true;
}

bool _al_d3d_init_keyboard()
{
	orig_keyboard_init = ((AL_KEYBOARD_DRIVER *)_al_keyboard_driver_list[0].driver)->init_keyboard;
	((AL_KEYBOARD_DRIVER *)_al_keyboard_driver_list[0].driver)->init_keyboard = _d3d_keyboard_init;
	return true;
}

