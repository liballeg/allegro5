/* This is only a dummy driver, not implementing most required things,
 * it's just here to give me some understanding of the base framework of a
 * system driver.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "internal/aintern.h"
#include "platform/aintwin.h"
#include "internal/aintern_system.h"
#include "internal/aintern_bitmap.h"

#include "winalleg.h"
#include <d3d8.h>

#include "d3d.h"

static AL_SYSTEM_INTERFACE *vt=0;

AL_SYSTEM_D3D *_al_d3d_system;
LPDIRECT3D8 _al_d3d = 0;
LPDIRECT3DDEVICE8 _al_d3d_device = 0;

/* Create a new system object for the dummy D3D driver. */
static AL_SYSTEM *d3d_initialize(int flags)
{
	_al_d3d_system = _AL_MALLOC(sizeof *_al_d3d_system);
	memset(_al_d3d_system, 0, sizeof *_al_d3d_system);

	_al_d3d_init_window();
	_al_d3d_init_keyboard();

	_win_input_init(TRUE);

	if (_al_d3d_init_display() != false) {
		_AL_FREE(_al_d3d_system);
		return 0;
	}
	
	_al_vector_init(&_al_d3d_system->system.displays, sizeof (AL_SYSTEM_D3D *));

	_al_d3d_system->system.vt = vt;

	return &_al_d3d_system->system;
}

// FIXME: This is just for now, the real way is of course a list of
// available display drivers. Possibly such drivers can be attached at runtime
// to the system driver, so addons could provide additional drivers.
AL_DISPLAY_INTERFACE *get_display_driver(void)
{
    return _al_display_d3d_driver();
}

// FIXME: Use the list.
AL_KEYBOARD_DRIVER *get_keyboard_driver(void)
{
   // FIXME: i would prefer a dynamic way to list drivers, not a static list
//   return _al_xwin_keyboard_driver_list[0].driver;
	return _al_keyboard_driver_list[0].driver;
}

AL_SYSTEM_INTERFACE *_al_system_d3d_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->initialize = d3d_initialize;
   vt->get_display_driver = get_display_driver;
   vt->get_keyboard_driver = get_keyboard_driver;

   TRACE("AL_SYSTEM_INTERFACE created.\n");

   return vt;
}

