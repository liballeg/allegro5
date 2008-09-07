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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_memory.h"



#define PREFIX_I                "al-main INFO: "
#define PREFIX_W                "al-main WARNING: "
#define PREFIX_E                "al-main ERROR: "


/* debugging stuff */
static int debug_assert_virgin = TRUE;
static int debug_trace_virgin = TRUE;

static FILE *assert_file = NULL;
static FILE *trace_file = NULL;

static int (*assert_handler)(AL_CONST char *msg) = NULL;
int (*_al_trace_handler)(AL_CONST char *msg) = NULL;


/* dynamic registration system for cleanup code */
struct al_exit_func {
   void (*funcptr)(void);
   AL_CONST char *desc;
   struct al_exit_func *next;
};

static struct al_exit_func *exit_func_list = NULL;



/* _add_exit_func:
 *  Adds a function to the list that need to be called on Allegro shutdown.
 *  `desc' should point to a statically allocated string to help with
 *  debugging.
 */
void _add_exit_func(void (*func)(void), AL_CONST char *desc)
{
   struct al_exit_func *n;

   for (n = exit_func_list; n; n = n->next)
      if (n->funcptr == func)
	 return;

   n = _AL_MALLOC(sizeof(struct al_exit_func));
   if (!n)
      return;

   n->next = exit_func_list;
   n->funcptr = func;
   n->desc = desc;
   exit_func_list = n;
}



/* _remove_exit_func:
 *  Removes a function from the list that need to be called on Allegro
 *  shutdown.
 */
void _remove_exit_func(void (*func)(void))
{
   struct al_exit_func *iter = exit_func_list, *prev = NULL;

   while (iter) {
      if (iter->funcptr == func) {
	 if (prev)
	    prev->next = iter->next;
	 else
	    exit_func_list = iter->next;
	 _AL_FREE(iter);
	 return;
      }
      prev = iter;
      iter = iter->next;
   }
}



/* _al_run_exit_funcs:
 *  Run all the functions registered with _add_exit_func, in reverse order of
 *  registration.
 */
void _al_run_exit_funcs(void)
{
   while (exit_func_list) {
      void (*func)(void) = exit_func_list->funcptr;
      _remove_exit_func(func);
      (*func)();
   }
}



/* debug_exit:
 *  Closes the debugging output files.
 */
static void debug_exit(void)
{
   if (assert_file) {
      fclose(assert_file);
      assert_file = NULL;
   }

   if (trace_file) {
      fclose(trace_file);
      trace_file = NULL;
   }

   debug_assert_virgin = TRUE;
   debug_trace_virgin = TRUE;

   _remove_exit_func(debug_exit);
}



/* al_assert:
 *  Raises an assert (uses ASCII strings).
 */
void al_assert(AL_CONST char *file, int line)
{
   static int asserted = FALSE;
   int olderr = errno;
   char buf[128];
   char *s;

   if (asserted)
      return;

   /* todo, some day: use snprintf (C99) */
   sprintf(buf, "Assert failed at line %d of %s", line, file);

   if (assert_handler) {
      if (assert_handler(buf))
	 return;
   }

   if (debug_assert_virgin) {
      s = getenv("ALLEGRO_ASSERT");

      if (s)
	 assert_file = fopen(s, "w");
      else
	 assert_file = NULL;

      if (debug_trace_virgin)
	 _add_exit_func(debug_exit, "debug_exit");

      debug_assert_virgin = FALSE;
   }

   if (assert_file) {
      fprintf(assert_file, "%s\n", buf);
      fflush(assert_file);
   }
   else {
      asserted = TRUE;

/*
      if ((system_driver) && (system_driver->assert)) {
	 system_driver->assert(buf);
      }
      else {
      */
//	 al_uninstall_system();
	 fprintf(stderr, "%s\n", buf);
	 abort();
     // }
   }

   errno = olderr;
}



/* al_trace:
 *  Outputs a trace message (uses ASCII strings).
 */
void al_trace(AL_CONST char *msg, ...)
{
   int olderr = errno;
   char buf[512];
   char *s;

   /* todo, some day: use vsnprintf (C99) */
   va_list ap;
   va_start(ap, msg);
   vsprintf(buf, msg, ap);
   va_end(ap);

   if (_al_trace_handler) {
      if (_al_trace_handler(buf))
	 return;
   }

   if (debug_trace_virgin) {
      s = getenv("ALLEGRO_TRACE");

      if (s)
	 trace_file = fopen(s, "w");
      else
	 trace_file = fopen("allegro.log", "w");

      if (debug_assert_virgin)
	 _add_exit_func(debug_exit, "debug_exit");

      debug_trace_virgin = FALSE;
   }

   if (trace_file) {
      fwrite(buf, sizeof(char), strlen(buf), trace_file);
      fflush(trace_file);
   }

   errno = olderr;
}



/* register_assert_handler:
 *  Installs a user handler for assert failures.
 */
void register_assert_handler(int (*handler)(AL_CONST char *msg))
{
   assert_handler = handler;
}



/* register_trace_handler:
 *  Installs a user handler for trace output.
 */
void register_trace_handler(int (*handler)(AL_CONST char *msg))
{
   _al_trace_handler = handler;
}


