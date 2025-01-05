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
 *      Logging and assertion handlers.
 *
 *      See LICENSE.txt for copyright information.
 */


#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_debug.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_vector.h"

#ifdef ALLEGRO_WINDOWS
#include "allegro5/internal/aintern_wunicode.h"
#endif

#ifdef ALLEGRO_ANDROID
#include <unistd.h>
#include <android/log.h>
#endif


/* tracing */
typedef struct TRACE_INFO
{
   bool trace_virgin;
   FILE *trace_file;
   bool need_close;
   _AL_MUTEX trace_mutex;

   /* 0: debug, 1: info, 2: warn, 3: error */
   int level;
   /* 1: line number, 2: function name, 4: timestamp */
   int flags;
   /* List of channels to log. NULL to log all channels. */
   _AL_VECTOR channels;
   _AL_VECTOR excluded;
   /* Whether settings have been read from allegro5.cfg or not. */
   bool configured;
   bool win_use_output_debug_string;
} TRACE_INFO;


static TRACE_INFO trace_info =
{
   true,
   NULL,
   true,
   _AL_MUTEX_UNINITED,
   0,
   7,
   _AL_VECTOR_INITIALIZER(ALLEGRO_USTR *),
   _AL_VECTOR_INITIALIZER(ALLEGRO_USTR *),
   false,
   false
};

static char static_trace_buffer[2048];

/* run-time assertions */
void (*_al_user_assert_handler)(char const *expr, char const *file,
   int line, char const *func);

void (*_al_user_trace_handler)(char const *message);


static void delete_string_list(_AL_VECTOR *v)
{
   while (_al_vector_is_nonempty(v)) {
      int i = _al_vector_size(v) - 1;
      ALLEGRO_USTR **iter = _al_vector_ref(v, i);
      al_ustr_free(*iter);
      _al_vector_delete_at(v, i);
   }
   _al_vector_free(v);
}


void _al_configure_logging(void)
{
   ALLEGRO_CONFIG *config;
   char const *v;
   bool got_all = false;

   config = al_get_system_config();
   v = al_get_config_value(config, "trace", "channels");
   if (v) {
      ALLEGRO_USTR_INFO uinfo;
      const ALLEGRO_USTR *u = al_ref_cstr(&uinfo, v);
      int pos = 0;

      while (pos >= 0) {
         int comma = al_ustr_find_chr(u, pos, ',');
         int first;
         ALLEGRO_USTR *u2, **iter;
         if (comma == -1)
            u2 = al_ustr_dup_substr(u, pos, al_ustr_length(u));
         else
            u2 = al_ustr_dup_substr(u, pos, comma);
         al_ustr_trim_ws(u2);
         first = al_ustr_get(u2, 0);

         if (first == '-') {
            al_ustr_remove_chr(u2, 0);
            iter = _al_vector_alloc_back(&trace_info.excluded);
            *iter = u2;
         }
         else {
            if (first == '+')
               al_ustr_remove_chr(u2, 0);
            iter = _al_vector_alloc_back(&trace_info.channels);
            *iter = u2;
            if (!strcmp(al_cstr(u2), "all"))
               got_all = true;
         }
         pos = comma;
         al_ustr_get_next(u, &pos);
      }

      if (got_all)
         delete_string_list(&trace_info.channels);
   }

#ifdef DEBUGMODE
   trace_info.level = 0;
#else
   trace_info.level = 9999;
#endif

   v = getenv("ALLEGRO_TRACE_LEVEL");
   if (!v)
      v = al_get_config_value(config, "trace", "level");
   if (v) {
      if (!strcmp(v, "error")) trace_info.level = 3;
      else if (!strcmp(v, "warn")) trace_info.level = 2;
      else if (!strcmp(v, "info")) trace_info.level = 1;
      else if (!strcmp(v, "debug")) trace_info.level = 0;
      else if (!strcmp(v, "none")) trace_info.level = 9999;
   }

   v = al_get_config_value(config, "trace", "timestamps");
   if (!v || strcmp(v, "0"))
      trace_info.flags |= 4;
   else
      trace_info.flags &= ~4;

   v = al_get_config_value(config, "trace", "functions");
   if (!v || strcmp(v, "0"))
      trace_info.flags |= 2;
   else
      trace_info.flags &= ~2;

   v = al_get_config_value(config, "trace", "lines");
   if (!v || strcmp(v, "0"))
      trace_info.flags |= 1;
   else
      trace_info.flags &= ~1;

   v = al_get_config_value(config, "trace", "win_use_output_debug_string");
   if (!v || strcmp(v, "true"))
      trace_info.win_use_output_debug_string = false;
   else
      trace_info.win_use_output_debug_string = true;

   if (!trace_info.configured)
      _al_mutex_init(&trace_info.trace_mutex);

   trace_info.configured = true;
}


static void open_trace_file(void)
{
   if (trace_info.trace_virgin) {
      const char *s = getenv("ALLEGRO_TRACE");

      if (s) {
         if (!strcmp(s, "-")) {
            trace_info.trace_file = stdout;
            trace_info.need_close = false;
         }
         else {
            trace_info.trace_file = fopen(s, "w");
         }
      }
      else
#if defined(ALLEGRO_IPHONE) || defined(ALLEGRO_ANDROID) || defined(__EMSCRIPTEN__)
         /* iPhone and Android don't like us writing files, so we'll be doing
          * something else there by default. */
         trace_info.trace_file = NULL;
#else
         trace_info.trace_file = fopen("allegro.log", "w");
#endif

      trace_info.trace_virgin = false;
   }
}


static void do_trace(const char *msg, ...)
{
   va_list ap;
   int s = strlen(static_trace_buffer);
   va_start(ap, msg);
   vsnprintf(static_trace_buffer + s, sizeof(static_trace_buffer) - s,
      msg, ap);
   va_end(ap);
}


/* _al_trace_prefix:
 *  Conditionally write the initial part of a trace message.  If we do, return true
 *  and continue to hold the trace_mutex lock.
 */
bool _al_trace_prefix(char const *channel, int level,
   char const *file, int line, char const *function)
{
   size_t i;
   char *name;
   _AL_VECTOR const *v;

   if (!trace_info.configured) {
      _al_configure_logging();
   }

   if (level < trace_info.level)
      return false;

   v = &trace_info.channels;
   if (_al_vector_is_empty(v))
      goto channel_included;

   for (i = 0; i < _al_vector_size(v); i++) {
      ALLEGRO_USTR **iter = _al_vector_ref(v, i);
      if (!strcmp(al_cstr(*iter), channel))
         goto channel_included;
   }

   return false;

channel_included:

   v = &trace_info.excluded;
   if (_al_vector_is_nonempty(v)) {
      for (i = 0; i < _al_vector_size(v); i++) {
         ALLEGRO_USTR **iter = _al_vector_ref(v, i);
         if (!strcmp(al_cstr(*iter), channel))
            return false;
      }
   }

   /* Avoid interleaved output from different threads. */
   _al_mutex_lock(&trace_info.trace_mutex);

   if (!_al_user_trace_handler)
      open_trace_file();

   do_trace("%-8s ", channel);
   if (level == 0) do_trace("D ");
   if (level == 1) do_trace("I ");
   if (level == 2) do_trace("W ");
   if (level == 3) do_trace("E ");

#ifdef ALLEGRO_ANDROID
   {
      char pid_buf[16];
      snprintf(pid_buf, sizeof(pid_buf), "%i: ", gettid());
      do_trace(pid_buf);
   }
#endif

#ifdef ALLEGRO_MSVC
   name = strrchr(file, '\\');
#else
   name = strrchr(file, '/');
#endif
   if (trace_info.flags & 1) {
      do_trace("%20s:%-4d ", name ? name + 1 : file, line);
   }
   if (trace_info.flags & 2) {
      do_trace("%-32s ", function);
   }
   if (trace_info.flags & 4) {
      double t = 0;
      if (al_is_system_installed())
         t = al_get_time();
      do_trace("[%10.5f] ", t);
   }

   /* Do not unlock trace_mutex here; that is done by _al_trace_suffix. */
   return true;
}


/* _al_trace_suffix:
 *  Output the final part of a trace message, and release the trace_mutex lock.
 */
void _al_trace_suffix(const char *msg, ...)
{
   int olderr = errno;
   va_list ap;
   int s = strlen(static_trace_buffer);
   va_start(ap, msg);
   vsnprintf(static_trace_buffer + s, sizeof(static_trace_buffer) - s,
      msg, ap);
   va_end(ap);

   if (_al_user_trace_handler) {
      _al_user_trace_handler(static_trace_buffer);
   }
   else {
#ifdef ALLEGRO_ANDROID
      (void)__android_log_print(ANDROID_LOG_INFO, "allegro", "%s",
         static_trace_buffer);
#endif
#if defined(ALLEGRO_IPHONE) || defined(__EMSCRIPTEN__)
      fprintf(stderr, "%s", static_trace_buffer);
      fflush(stderr);
#endif
#ifdef ALLEGRO_WINDOWS
      if (trace_info.win_use_output_debug_string) {
         TCHAR *windows_output = _twin_utf8_to_tchar(static_trace_buffer);
         OutputDebugString(windows_output);
         al_free(windows_output);
      }
#endif

      /* We're intentially still writing to a file if it's set even with the
       * additional logging options above. */
      if (trace_info.trace_file) {
         fprintf(trace_info.trace_file, "%s", static_trace_buffer);
         fflush(trace_info.trace_file);
      }
   }
   static_trace_buffer[0] = '\0';
   errno = olderr;

   _al_mutex_unlock(&trace_info.trace_mutex);
}


void _al_shutdown_logging(void)
{
   if (trace_info.configured) {
      _al_mutex_destroy(&trace_info.trace_mutex);

      delete_string_list(&trace_info.channels);
      delete_string_list(&trace_info.excluded);

      trace_info.configured = false;
   }

   if (trace_info.trace_file && trace_info.need_close) {
      fclose(trace_info.trace_file);
   }

   trace_info.trace_file = NULL;
   trace_info.trace_virgin = true;
}


/* Function: al_register_assert_handler
 */
void al_register_assert_handler(void (*handler)(char const *expr,
   char const *file, int line, char const *func))
{
   _al_user_assert_handler = handler;
}


/* Function: al_register_trace_handler
 */
void al_register_trace_handler(void (*handler)(char const *))
{
   _al_user_trace_handler = handler;
}


/* vim: set sts=3 sw=3 et: */
