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

/* Function: al_malloc
 */
#define al_malloc(n) (al_malloc_with_context((n), __LINE__, __FILE__, __func__))

/* Function: al_free
 */
#define al_free(p) (al_free_with_context((p), __LINE__, __FILE__, __func__))

/* Function: al_realloc
 */
#define al_realloc(p, n) (al_realloc_with_context((p), (n), __LINE__, __FILE__, __func__))

/* Function: al_calloc
 */
#define al_calloc(c, n) (al_calloc_with_context((c), (n), __LINE__, __FILE__, __func__))

AL_FUNC(void *, al_malloc_with_context, (size_t n,
   int line, const char *file, const char *func));
AL_FUNC(void, al_free_with_context, (void *ptr,
   int line, const char *file, const char *func));
AL_FUNC(void *, al_realloc_with_context, (void *ptr, size_t n,
   int line, const char *file, const char *func));
AL_FUNC(void *, al_calloc_with_context, (size_t count, size_t n,
   int line, const char *file, const char *func));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_MEMORY_H */
