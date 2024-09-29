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
 *      Debug facilities.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef __al_included_allegro5_debug_h
#define __al_included_allegro5_debug_h

#include <assert.h>
#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

AL_FUNC(bool, _al_trace_prefix, (char const *channel, int level,
   char const *file, int line, char const *function));

AL_PRINTFUNC(void, _al_trace_suffix, (const char *msg, ...), 1, 2);

#if defined(DEBUGMODE) || defined(A5O_CFG_RELEASE_LOGGING)
   /* Must not be used with a trailing semicolon. */
   #ifdef A5O_GCC
      #define A5O_DEBUG_CHANNEL(x) \
         static char const *__al_debug_channel __attribute__((unused)) = x;
   #else
      #define A5O_DEBUG_CHANNEL(x) \
         static char const *__al_debug_channel = x;
   #endif
   #define A5O_TRACE_CHANNEL_LEVEL(channel, level)                        \
      !_al_trace_prefix(channel, level, __FILE__, __LINE__, __func__)         \
      ? (void)0 : _al_trace_suffix
#else
   #define A5O_TRACE_CHANNEL_LEVEL(channel, x)  1 ? (void) 0 : _al_trace_suffix
   #define A5O_DEBUG_CHANNEL(x)
#endif

#define A5O_TRACE_LEVEL(x)   A5O_TRACE_CHANNEL_LEVEL(__al_debug_channel, x)
#define A5O_DEBUG            A5O_TRACE_LEVEL(0)
#define A5O_INFO             A5O_TRACE_LEVEL(1)
#define A5O_WARN             A5O_TRACE_LEVEL(2)
#define A5O_ERROR            A5O_TRACE_LEVEL(3)

/* Run-time assertions. */
AL_FUNCPTR(void, _al_user_assert_handler, (char const *expr, char const *file,
   int line, char const *func));

AL_FUNC(void, al_register_assert_handler, (void (*handler)(char const *expr,
   char const *file, int line, char const *func)));

AL_FUNC(void, al_register_trace_handler, (void (*handler)(char const *)));

#ifdef __clang_analyzer__
   /* Clang doesn't understand _al_user_assert_handler, so we simplify the
    * definition for analysis purposes. */
   #define A5O_ASSERT(e) assert(e)
#else
   #ifdef NDEBUG
      #define A5O_ASSERT(e)	((void)(0 && (e)))
   #else
      #define A5O_ASSERT(e)                                                  \
         ((e) ? (void) 0                                                         \
         : (_al_user_assert_handler) ?                                           \
            _al_user_assert_handler(#e, __FILE__, __LINE__, __func__)            \
         : assert(e))
   #endif
#endif

/* Compile time assertions. */
#define A5O_ASSERT_CONCAT_(a, b)   a##b
#define A5O_ASSERT_CONCAT(a, b)    A5O_ASSERT_CONCAT_(a, b)
#define A5O_STATIC_ASSERT(module, e) \
   struct A5O_ASSERT_CONCAT(static_assert_##module##_line_, __LINE__) \
      { unsigned int bf : !!(e); }

/* We are lazy and use just ASSERT while Allegro itself is compiled. */
#ifdef A5O_LIB_BUILD
    #define ASSERT(x) A5O_ASSERT(x)
#endif

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
