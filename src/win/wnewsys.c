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
 *      New Windows system driver
 *
 *      Based on the X11 OpenGL driver by Elias Pschernig.
 *      Heavily modified by Trent Gamblin.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/platform/aintwin.h"
#include "allegro/internal/aintern_system.h"
#include "allegro/internal/aintern_bitmap.h"

#include "winalleg.h"

#include "win_new.h"

static AL_SYSTEM_INTERFACE *vt = 0;

AL_SYSTEM_WIN *_al_win_system;

/* Create a new system object for the dummy D3D driver. */
static AL_SYSTEM *win_initialize(int flags)
{
   _al_win_system = _AL_MALLOC(sizeof *_al_win_system);
   memset(_al_win_system, 0, sizeof *_al_win_system);

   _al_win_init_window();

   _al_vector_init(&_al_win_system->system.displays, sizeof (AL_SYSTEM_WIN *));

   _al_win_system->system.vt = vt;

#if defined ALLEGRO_D3D
   if (_al_d3d_init_display() != true)
      return NULL;
#endif
   
   return &_al_win_system->system;
}

/* FIXME: autodetect a driver */
AL_DISPLAY_INTERFACE *win_get_display_driver(void)
{
   return _al_display_d3d_driver();
}

/* FIXME: use the list */
AL_KEYBOARD_DRIVER *win_get_keyboard_driver(void)
{
   return _al_keyboard_driver_list[0].driver;
}

int win_get_num_display_modes(void)
{
   int format = al_get_new_display_format();
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

   if (!(flags & AL_FULLSCREEN))
      return -1;

#if defined ALLEGRO_D3D
   if (flags & AL_DIRECT3D) {
      return _al_d3d_get_num_display_modes(format, refresh_rate, flags);
   }
#endif

   if (flags & AL_OPENGL) {
      /* FIXME */
   }

   return 0;
}

AL_DISPLAY_MODE *win_get_display_mode(int index, AL_DISPLAY_MODE *mode)
{
   int format = al_get_new_display_format();
   int refresh_rate = al_get_new_display_refresh_rate();
   int flags = al_get_new_display_flags();

   if (!(flags & AL_FULLSCREEN))
      return NULL;

#if defined ALLEGRO_D3D
   if (flags & AL_DIRECT3D) {
      return _al_d3d_get_display_mode(index, format, refresh_rate, flags, mode);
   }
#endif

   if (flags & AL_OPENGL) {
      /* FIXME */
   }

   return NULL;
}

AL_SYSTEM_INTERFACE *_al_system_win_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->initialize = win_initialize;
   vt->get_display_driver = win_get_display_driver;
   vt->get_keyboard_driver = win_get_keyboard_driver;
   vt->get_num_display_modes = win_get_num_display_modes;
   vt->get_display_mode = win_get_display_mode;

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

