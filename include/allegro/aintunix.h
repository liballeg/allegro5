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

#ifdef __cplusplus
extern "C" {
#endif

   /* Asynchronous event processing with SIGALRM.  */
   AL_FUNC(int, _sigalrm_init, (AL_METHOD(void, handler, (unsigned long interval))));
   AL_FUNC(void, _sigalrm_exit, (void));
   AL_FUNC(void, _sigalrm_disable_interrupts, (void));
   AL_FUNC(void, _sigalrm_enable_interrupts, (void));
   AL_FUNC(int, _sigalrm_interrupts_disabled, (void));
   AL_FUNC(void, _sigalrm_request_abort, (void));
   AL_FUNC(void, _sigalrm_pause, (void));
   AL_FUNC(void, _sigalrm_unpause, (void));

   /* Interrupt handlers.  */
   AL_FUNCPTR(void, _sigalrm_digi_interrupt_handler, (unsigned long interval));
   AL_FUNCPTR(void, _sigalrm_midi_interrupt_handler, (unsigned long interval));

   /* macros to enable and disable interrupts */
   #define DISABLE() _sigalrm_disable_interrupts()
   #define ENABLE()  _sigalrm_enable_interrupts()


   /* Generic Unix timer driver hook, for polling through the SIGALRM handler */
   AL_FUNCPTR(void, _unix_timer_interrupt, (unsigned long interval));


   /* Helper for locating config files */
   AL_FUNC(int, _unix_find_resource, (char *dest, char *resource, int size));


   /* Helper for setting os_type */
   AL_FUNC(void, _read_os_type, (void));


   /* Helper for yield CPU */
   AL_FUNC(void, _unix_yield_timeslice, (void));


#ifdef ALLEGRO_WITH_XWINDOWS
   AL_FUNCPTR(void, _xwin_keyboard_interrupt, (int pressed, int code));
   AL_FUNCPTR(void, _xwin_keyboard_focused, (int focused));
   AL_FUNCPTR(void, _xwin_mouse_interrupt, (int x, int y, int z, int buttons));
   AL_FUNCPTR(void, _xwin_timer_interrupt, (unsigned long interval));

   AL_ARRAY(_DRIVER_INFO, _xwin_gfx_driver_list);
   AL_ARRAY(_DRIVER_INFO, _xwin_keyboard_driver_list);
   AL_ARRAY(_DRIVER_INFO, _xwin_mouse_driver_list);
   AL_ARRAY(_DRIVER_INFO, _xwin_timer_driver_list);

   AL_FUNC(void, _xwin_handle_input, (void));
#endif


#ifdef DIGI_OSS
   /* So the setup program can read what we detected */
   AL_VAR(int, _oss_fragsize);
   AL_VAR(int, _oss_numfrags);
#endif


#ifdef ALLEGRO_LINUX
   #include "aintlnx.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* ifndef AINTUNIX_H */

