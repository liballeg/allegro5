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
 *      Memory management routines.
 *
 *      See readme.txt for copyright information.
 */

#ifndef ALLEGRO_MEMORY_H
#define ALLEGRO_MEMORY_H

#ifdef __cplusplus
   extern "C" {
#endif

/* Not exported for now. I'm not sure how to handle the debug situation for
 * user code.  Also, passing a pointer to a vtable might be better.
 */
void _al_set_memory_management_functions(
   void *(*malloc)(void *opaque, size_t size),
   void *(*malloc_atomic)(void *opaque, size_t size),
   void (*free)(void *opaque, void *ptr),
   void *(*realloc)(void *opaque, void *ptr, size_t size),
   void *(*debug_malloc)(int line, const char *file, const char *func,
      void *opaque, size_t size),
   void *(*debug_malloc_atomic)(int line, const char *file, const char *func,
      void *opaque, size_t size),
   void (*debug_free)(int line, const char *file, const char *func,
      void *opaque, void *ptr),
   void *(*debug_realloc)(int line, const char *file, const char *func,
      void *opaque, void *ptr, size_t size),
   void *user_opaque);

void *al_malloc(size_t n);
void al_free(void *ptr);

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_MEMORY_H */
