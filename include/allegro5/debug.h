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


#ifndef ALLEGRO_DEBUG_H
#define ALLEGRO_DEBUG_H

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

AL_FUNC(bool, _al_trace_prefix, (char const *channel, int level,
   char const *file, int line, char const *function));

AL_FUNC(void, al_assert, (AL_CONST char *file, int linenr));
AL_PRINTFUNC(void, al_trace, (AL_CONST char *msg, ...), 1, 2);

AL_FUNC(void, register_assert_handler, (AL_METHOD(int, handler, (AL_CONST char *msg))));
AL_FUNC(void, register_trace_handler, (AL_METHOD(int, handler, (AL_CONST char *msg))));


#ifdef DEBUGMODE
   /* Must not be used with a trailing semicolon. */
   #define ALLEGRO_DEBUG_CHANNEL(x) static char const *__al_debug_channel = x;
   #define ALLEGRO_TRACE_CHANNEL_LEVEL(channel, level)                        \
      !_al_trace_prefix(channel, level, __FILE__, __LINE__, __func__)         \
      ? (void)0 : al_trace
   #define ALLEGRO_ASSERT(condition) do {                                     \
                                    if (!(condition))                         \
                                       al_assert(__FILE__, __LINE__);         \
                                 } while (0)
   #define TRACE                 al_trace
#else
   #define ALLEGRO_ASSERT(condition)
   #define TRACE                                    1 ? (void) 0 : al_trace
   #define ALLEGRO_TRACE_CHANNEL_LEVEL(channel, x)  1 ? (void) 0 : al_trace
   #define ALLEGRO_DEBUG_CHANNEL(x)
#endif

#define ALLEGRO_TRACE_LEVEL(x)   ALLEGRO_TRACE_CHANNEL_LEVEL(__al_debug_channel, x)
#define ALLEGRO_DEBUG            ALLEGRO_TRACE_LEVEL(0)
#define ALLEGRO_INFO             ALLEGRO_TRACE_LEVEL(1)
#define ALLEGRO_WARN             ALLEGRO_TRACE_LEVEL(2)
#define ALLEGRO_ERROR            ALLEGRO_TRACE_LEVEL(3)

/* Compile time assertions. */
#define ALLEGRO_ASSERT_CONCAT_(a, b)   a##b
#define ALLEGRO_ASSERT_CONCAT(a, b)    ALLEGRO_ASSERT_CONCAT_(a, b)
#define ALLEGRO_STATIC_ASSERT(e) \
   enum { ALLEGRO_ASSERT_CONCAT(static_assert_line_, __LINE__) = 1/!!(e) }

/* We are lazy and use just ASSERT while Allegro itself is compiled. */
#ifdef ALLEGRO_LIB_BUILD
    #define ASSERT(x) ALLEGRO_ASSERT(x)
#endif

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_DEBUG_H */


