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
 *      BeOS display switching routines.
 *
 *      By Angelo Mottola.
 *
 *      Based on similar Windows routines by Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif

#define MAX_SWITCH_CALLBACKS		8

int _be_switch_mode = SWITCH_PAUSE;

static void (*_be_switch_in_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static void (*_be_switch_out_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };



/* be_sys_set_display_switch_mode:
 *  Initializes new display switching mode.
 */
extern "C" int be_sys_set_display_switch_mode(int mode)
{
   int i;
   
   /* Fullscreen only supports SWITCH_AMNESIA */
   if ((_be_allegro_screen) && (mode != SWITCH_AMNESIA))
         return -1;

   _be_switch_mode = mode;

   /* Clear callbacks and return success */
   for (i=0; i<MAX_SWITCH_CALLBACKS; i++)
      _be_switch_in_cb[i] = _be_switch_out_cb[i] = NULL;

   return 0;
}



/* be_sys_set_display_switch_cb:
 *  Adds a display switch callback routines.
 */
extern "C" int be_sys_set_display_switch_cb(int dir, void (*cb)(void))
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (dir == SWITCH_IN) {
	 if (!_be_switch_in_cb[i]) {
	    _be_switch_in_cb[i] = cb;
	    return 0;
	 }
      }
      else {
	 if (!_be_switch_out_cb[i]) {
	    _be_switch_out_cb[i] = cb;
	    return 0;
	 }
      }
   }

   return -1;
}



/* be_sys_remove_display_switch_cb:
 *  Removes a display switch callback routine.
 */
extern "C" void be_sys_remove_display_switch_cb(void (*cb)(void))
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (_be_switch_in_cb[i] == cb)
	 _be_switch_in_cb[i] = NULL;

      if (_be_switch_out_cb[i] == cb)
	 _be_switch_out_cb[i] = NULL;
   }
}



/* be_display_switch_callback:
 *  Calls callback functions when switching in/out of the app.
 */
extern "C" void be_display_switch_callback(int dir)
{
   int i;
   
   if (dir == SWITCH_IN) {
      for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
         if (_be_switch_in_cb[i])
            _be_switch_in_cb[i]();
      }
   }
   else {
      for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
         if (_be_switch_out_cb[i])
            _be_switch_out_cb[i]();
      }
   }
}
