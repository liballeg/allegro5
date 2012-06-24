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
 *      Dynamic driver lists shared by Unixy system drivers.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_system.h"

#ifdef ALLEGRO_WITH_XWINDOWS
#include "allegro5/internal/aintern_xglx.h"
#endif

// FIXME: will need to wire this back in
#if 0

#include "allegro5/platform/aintunix.h"



_AL_DRIVER_INFO *_unix_gfx_driver_list = 0;
_AL_DRIVER_INFO *_unix_digi_driver_list = 0;
_AL_DRIVER_INFO *_unix_midi_driver_list = 0;



/* _unix_driver_lists_init:
 *  Initialise driver lists.
 */
void _unix_driver_lists_init(void)
{
   _unix_gfx_driver_list = _create_driver_list();
   if (_unix_gfx_driver_list)
      _driver_list_append_list(&_unix_gfx_driver_list, _gfx_driver_list);
    
   _unix_digi_driver_list = _create_driver_list();
   if (_unix_digi_driver_list)
      _driver_list_append_list(&_unix_digi_driver_list, _digi_driver_list);
   
   _unix_midi_driver_list = _create_driver_list();
   if (_unix_midi_driver_list)
      _driver_list_append_list(&_unix_midi_driver_list, _midi_driver_list);
}



/* _unix_driver_lists_shutdown:
 *  Free driver lists.
 */
void _unix_driver_lists_shutdown(void)
{
   if (_unix_gfx_driver_list) {
      _destroy_driver_list(_unix_gfx_driver_list);
      _unix_gfx_driver_list = 0;
   }

   if (_unix_digi_driver_list) {
      _destroy_driver_list(_unix_digi_driver_list);
      _unix_digi_driver_list = 0;
   }

   if (_unix_midi_driver_list) {
      _destroy_driver_list(_unix_midi_driver_list);
      _unix_midi_driver_list = 0;
   }
}



/* _unix_register_gfx_driver:
 *  Used by modules to register graphics drivers.
 */
void _unix_register_gfx_driver(int id, GFX_DRIVER *driver, int autodetect, int priority)
{
   if (priority)
      _driver_list_prepend_driver(&_unix_gfx_driver_list, id, driver, autodetect);
   else
      _driver_list_append_driver(&_unix_gfx_driver_list, id, driver, autodetect);
}



/* _unix_register_digi_driver:
 *  Used by modules to register digital sound drivers.
 */
void _unix_register_digi_driver(int id, DIGI_DRIVER *driver, int autodetect, int priority)
{
   if (priority)
      _driver_list_prepend_driver(&_unix_digi_driver_list, id, driver, autodetect);
   else
      _driver_list_append_driver(&_unix_digi_driver_list, id, driver, autodetect);
}



/* _unix_register_midi_driver:
 *  Used by modules to register MIDI drivers.
 */
void _unix_register_midi_driver(int id, MIDI_DRIVER *driver, int autodetect, int priority)
{
   if (priority)
      _driver_list_prepend_driver(&_unix_midi_driver_list, id, driver, autodetect);
   else
      _driver_list_append_driver(&_unix_midi_driver_list, id, driver, autodetect);
}

#endif



/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces(void)
{
#ifdef ALLEGRO_WITH_XWINDOWS
   ALLEGRO_SYSTEM_INTERFACE **add;
   /* This is the only system driver right now */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_xglx_driver();
#endif
}

