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
 *      Some definitions for internal use by the Unix library code.
 *
 *      By Shawn Hargreaves.
 * 
 *      See readme.txt for copyright information.
 */

#ifndef AINTUNIX_H
#define AINTUNIX_H

#include "allegro5/path.h"

/* Need right now for XKeyEvent --pw */
#ifdef ALLEGRO_WITH_XWINDOWS
#include <X11/Xlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

   AL_FUNC(ALLEGRO_PATH *, _al_unix_get_path, (int id));

   /* Helper for locating config files */
   AL_FUNC(int, _unix_find_resource, (char *dest, AL_CONST char *resource, int size));

   /* Helper for setting os_type */
   AL_FUNC(void, _unix_read_os_type, (void));


   /* Helper for yield CPU */
   AL_FUNC(void, _unix_yield_timeslice, (void));


   /* Unix rest function */
   AL_FUNC(void, _unix_rest, (unsigned int, AL_METHOD(void, callback, (void))));


   /* Module support */
   AL_FUNC(void, _unix_load_modules, (int system_driver_id));
   AL_FUNC(void, _unix_unload_modules, (void));


   /* Dynamic driver lists, for modules */
   AL_VAR(_DRIVER_INFO *, _unix_gfx_driver_list);
   AL_VAR(_DRIVER_INFO *, _unix_digi_driver_list);
   AL_VAR(_DRIVER_INFO *, _unix_midi_driver_list);
   AL_FUNC(void, _unix_driver_lists_init, (void));
   AL_FUNC(void, _unix_driver_lists_shutdown, (void));


#ifdef ALLEGRO_WITH_XWINDOWS
   AL_ARRAY(_DRIVER_INFO, _al_xwin_keyboard_driver_list);
   AL_ARRAY(_DRIVER_INFO, _al_xwin_mouse_driver_list);
#endif


#ifdef __cplusplus
}
#endif


#ifdef ALLEGRO_LINUX
   #include "allegro5/platform/aintlnx.h"
#endif



/*----------------------------------------------------------------------*
 *									*
 *	New stuff							*
 *									*
 *----------------------------------------------------------------------*/

/* TODO: integrate this above */

#include "allegro5/platform/aintuthr.h"


AL_BEGIN_EXTERN_C

/* time */
AL_FUNC(void, _al_unix_init_time, (void));

/* fdwatch */
void _al_unix_start_watching_fd(int fd, void (*callback)(void *), void *cb_data);
void _al_unix_stop_watching_fd(int fd);

/* ljoynu.c */
/* This isn't in aintlnx.h because it's needed for the X11 port as well. */
#define _ALLEGRO_JOYDRV_LINUX    AL_ID('L','N','X','A')

#ifdef ALLEGRO_HAVE_LINUX_JOYSTICK_H
AL_VAR(struct ALLEGRO_JOYSTICK_DRIVER, _al_joydrv_linux);
#endif

AL_END_EXTERN_C


#endif /* ifndef AINTUNIX_H */
