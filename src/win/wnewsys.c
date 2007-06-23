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

#include "win_new.h"

static AL_SYSTEM_INTERFACE *vt=0;

AL_SYSTEM_WIN *_al_win_system;

/* Create a new system object for the dummy D3D driver. */
static AL_SYSTEM *win_initialize(int flags)
{
	_al_win_system = _AL_MALLOC(sizeof *_al_win_system);
	memset(_al_win_system, 0, sizeof *_al_win_system);

	_al_win_init_window();
	//_al_win_init_keyboard();

	_al_vector_init(&_al_win_system->system.displays, sizeof (AL_SYSTEM_WIN *));

	_al_win_system->system.vt = vt;

	return &_al_win_system->system;
}

// FIXME: This is just for now, the real way is of course a list of
// available display drivers. Possibly such drivers can be attached at runtime
// to the system driver, so addons could provide additional drivers.
AL_DISPLAY_INTERFACE *win_get_display_driver(void)
{
   /* FIXME: should detect one */
    return _al_display_d3d_driver();
}

// FIXME: Use the list.
AL_KEYBOARD_DRIVER *win_get_keyboard_driver(void)
{
   // FIXME: i would prefer a dynamic way to list drivers, not a static list
//   return _al_xwin_keyboard_driver_list[0].driver;
	return _al_keyboard_driver_list[0].driver;
}

AL_SYSTEM_INTERFACE *_al_system_win_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->initialize = win_initialize;
   vt->get_display_driver = win_get_display_driver;
   vt->get_keyboard_driver = win_get_keyboard_driver;

   TRACE("AL_SYSTEM_INTERFACE created.\n");

   return vt;
}

void _al_register_system_interfaces()
{
   AL_SYSTEM_INTERFACE **add;

#if defined ALLEGRO_D3D
   /* This is the only system driver right now */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_win_driver();
#endif
}

/*
 * For some reason _al_vector_find_and_delete isn't working
 */
void _al_win_delete_from_vector(_AL_VECTOR *vec, void *item)
{
   unsigned int i;
   for (i = 0; i < vec->_size; i++) {
   	void **curr = _al_vector_ref(vec, i);
	if (*curr == item) {
		_al_vector_delete_at(vec, i);
		return;
	}
   }
}
