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
#include "allegro5/internal/aintern_system.h"



/* debugging stuff */
static int debug_assert_virgin = true;
static int debug_trace_virgin = true;

static FILE *assert_file = NULL;
static FILE *trace_file = NULL;

static int (*assert_handler)(AL_CONST char *msg) = NULL;
int (*_al_trace_handler)(AL_CONST char *msg) = NULL;

struct
{
   /* 0: debug, 1: info, 2: warn, 3: error */
   int level;
   /* 1: line number, 2: function name, 4: timestamp */
   int flags;
   /* List of channels to log. NULL to log all channels. */
   ALLEGRO_USTR **channels;
   /* Whether settings have been read from allegro.cfg or not. */
   bool configured;
} _al_debug_info = {0, 7, NULL, false};

/* dynamic registration system for cleanup code */
struct al_exit_func {
   void (*funcptr)(void);
   AL_CONST char *desc;
   struct al_exit_func *next;
};

static struct al_exit_func *exit_func_list = NULL;



/* _al_add_exit_func:
 *  Adds a function to the list that need to be called on Allegro shutdown.
 *  `desc' should point to a statically allocated string to help with
 *  debugging.
 */
void _al_add_exit_func(void (*func)(void), AL_CONST char *desc)
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
	 _AL_FREE(iter);
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

   debug_assert_virgin = true;
   debug_trace_virgin = true;
}



/* al_assert:
 *  Raises an assert (uses ASCII strings).
 */
void al_assert(AL_CONST char *file, int line)
{
   static int asserted = false;
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
	 _al_add_exit_func(debug_exit, "debug_exit");

      debug_assert_virgin = false;
   }

   if (assert_file) {
      fprintf(assert_file, "%s\n", buf);
      fflush(assert_file);
   }
   else {
      asserted = true;

/*
      if ((system_driver) && (system_driver->assert)) {
	 system_driver->assert(buf);
      }
      else {
      */
//	 al_uninstall_system();
#ifndef ALLEGRO_MSVC
	 fprintf(stderr, "%s\n", buf);
#endif
	 abort();
     // }
   }

   errno = olderr;
}



bool _al_trace_prefix(char const *channel, int level,
   char const *file, int line, char const *function)
{
   int i;
   char *name;
   double t = al_current_time();

   if (!_al_debug_info.configured) {
      ALLEGRO_SYSTEM *sys = al_system_driver();
      /* Messages logged before the system driver and allegro.cfg are
       * up will always use defaults - but usually nothing is logged
       * before al_init.
       */
      if (sys && sys->config) {
         ALLEGRO_USTR *u;
         char const *v;
         bool got_all = false;
         int n = 0, pos = 0;
         v = al_get_config_value(sys->config, "trace", "channels");
         if (v) {
            ALLEGRO_USTR **channels = NULL;
            u = al_ustr_new(v);
            while (pos >= 0) {
               int comma = al_ustr_find_chr(u, pos, ',');
               ALLEGRO_USTR *u2;
               if (comma == -1)
                  u2 = al_ustr_dup_substr(u, pos, al_ustr_length(u));
               else
                  u2 = al_ustr_dup_substr(u, pos, comma);
               al_ustr_trim_ws(u2);
               n++;
               channels = _AL_REALLOC(channels, n * sizeof *channels);
               channels[n - 1] = u2;
               if (!strcmp(al_cstr(u2), "all"))
                  got_all = true;
               pos = comma;
               al_ustr_get_next(u, &pos);
            }
            n++;
            channels = _AL_REALLOC(channels, n * sizeof *channels);
            channels[n - 1] = NULL;
            al_ustr_free(u);
            _al_debug_info.channels = channels;
            
            if (got_all) {
               for (i = 0; _al_debug_info.channels[i]; i++) {
                  al_ustr_free(_al_debug_info.channels[i]);
               }
               _AL_FREE(_al_debug_info.channels);
               _al_debug_info.channels = NULL;
            }
         }
         v = al_get_config_value(sys->config, "trace", "level");
         if (v) {
            if (!strcmp(v, "error")) _al_debug_info.level = 3;
            else if (!strcmp(v, "warn")) _al_debug_info.level = 2;
            else if (!strcmp(v, "info")) _al_debug_info.level = 1;
         }
         v = al_get_config_value(sys->config, "trace", "timestamps");
         if (v && strcmp(v, "0"))
            _al_debug_info.flags |= 4;
         else
            _al_debug_info.flags &= ~4;
         v = al_get_config_value(sys->config, "trace", "functions");
         if (!v || strcmp(v, "0"))
            _al_debug_info.flags |= 2;
         else
            _al_debug_info.flags &= ~2;
         v = al_get_config_value(sys->config, "trace", "lines");
         if (!v || strcmp(v, "0"))
            _al_debug_info.flags |= 1;
         else
            _al_debug_info.flags &= ~1;
         _al_debug_info.configured = true;
      }
   }

   if (level < _al_debug_info.level) return false;
   if (_al_debug_info.channels == NULL) goto yes;
   if (_al_debug_info.channels[0] == NULL) return false;
   for (i = 0; _al_debug_info.channels[i]; i++) {
      if (!strcmp(al_cstr(_al_debug_info.channels[i]), channel))
         goto yes;
   }
   return false;
yes:
   al_trace("%-8s ", channel);
   if (level == 0) al_trace("D ");
   if (level == 1) al_trace("I ");
   if (level == 2) al_trace("W ");
   if (level == 3) al_trace("E ");

   name = strrchr(file, '/');
   if (_al_debug_info.flags & 1) al_trace("%20s:%-4d ",
      name ? name + 1 : file, line);
   if (_al_debug_info.flags & 2) al_trace("%-32s ", function);
   /* Kludge:
    * Very high timers (more than a year?) likely mean the timer
    * subsystem isn't initialized yet, so print 0.
    */
   if (t > 3600 * 24 * 365) t = 0;
   if (_al_debug_info.flags & 4) al_trace("[%10.5f] ", t);

   return true;
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
         _al_add_exit_func(debug_exit, "debug_exit");

      debug_trace_virgin = false;
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
