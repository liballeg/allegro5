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
 *      Destructors.
 *
 *      By Peter Wang.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_audio.h"


static _AL_DTOR_LIST *kcm_dtors = NULL;


/* _al_kcm_init_destructors:
 *  Initialise the destructor list.
 */
void _al_kcm_init_destructors(void)
{
   if (!kcm_dtors) {
      kcm_dtors = _al_init_destructors();
   }
}


/* _al_kcm_shutdown_destructors:
 *  Run all destructors and free the destructor list.
 */
void _al_kcm_shutdown_destructors(void)
{
   if (kcm_dtors) {
      _al_run_destructors(kcm_dtors);
      _al_shutdown_destructors(kcm_dtors);
      kcm_dtors = NULL;
   }
}


/* _al_kcm_register_destructor:
 *  Register an object to be destroyed.
 */
_AL_LIST_ITEM *_al_kcm_register_destructor(char const *name, void *object,
   void (*func)(void*))
{
   return _al_register_destructor(kcm_dtors, name, object, func);
}


/* _al_kcm_unregister_destructor:
 *  Unregister an object to be destroyed.
 */
void _al_kcm_unregister_destructor(_AL_LIST_ITEM *dtor_item)
{
   _al_unregister_destructor(kcm_dtors, dtor_item);
}


/* _al_kcm_foreach_destructor:
 *  Call the callback for each registered object.
 */
void _al_kcm_foreach_destructor(
   void (*callback)(void *object, void (*func)(void *), void *udata),
   void *userdata)
{
   _al_foreach_destructor(kcm_dtors, callback, userdata);
}


/* vim: set sts=3 sw=3 et: */
