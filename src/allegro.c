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
 *      Assorted globals and setup/cleanup routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/alplatf.h"
#include ALLEGRO_INTERNAL_HEADER


/* dynamic registration system for cleanup code */
struct al_exit_func {
   void (*funcptr)(void);
   const char *desc;
   struct al_exit_func *next;
};

static struct al_exit_func *exit_func_list = NULL;



/* _al_add_exit_func:
 *  Adds a function to the list that need to be called on Allegro shutdown.
 *  `desc' should point to a statically allocated string to help with
 *  debugging.
 */
void _al_add_exit_func(void (*func)(void), const char *desc)
{
   struct al_exit_func *n;

   for (n = exit_func_list; n; n = n->next)
      if (n->funcptr == func)
	 return;

   n = al_malloc(sizeof(struct al_exit_func));
   if (!n)
      return;

   n->next = exit_func_list;
   n->funcptr = func;
   n->desc = desc;
   exit_func_list = n;
}



/* _al_remove_exit_func:
 *  Removes a function from the list that need to be called on Allegro
 *  shutdown.
 */
void _al_remove_exit_func(void (*func)(void))
{
   struct al_exit_func *iter = exit_func_list, *prev = NULL;

   while (iter) {
      if (iter->funcptr == func) {
	 if (prev)
	    prev->next = iter->next;
	 else
	    exit_func_list = iter->next;
	 al_free(iter);
	 return;
      }
      prev = iter;
      iter = iter->next;
   }
}



/* _al_run_exit_funcs:
 *  Run all the functions registered with _al_add_exit_func, in reverse order of
 *  registration.
 */
void _al_run_exit_funcs(void)
{
   while (exit_func_list) {
      void (*func)(void) = exit_func_list->funcptr;
      _al_remove_exit_func(func);
      (*func)();
   }
}



/* Function: al_get_allegro_version
 */
uint32_t al_get_allegro_version(void)
{
   return ALLEGRO_VERSION_INT;
}



/* Function: al_run_main
 */
int al_run_main(int argc, char **argv, int (*user_main)(int, char **))
{
#ifdef ALLEGRO_MACOSX
    return _al_osx_run_main(argc, argv, user_main);
#else
    return user_main(argc, argv);
#endif
}


/* vim: set sts=3 sw=3 et: */
