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
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"



/* Function: al_malloc_with_context
 */
void *al_malloc_with_context(size_t n,
   int line, const char *file, const char *func)
{
   (void)line;
   (void)file;
   (void)func;

   return malloc(n);
}



/* Function: al_free_with_context
 */
void al_free_with_context(void *ptr,
   int line, const char *file, const char *func)
{
   (void)line;
   (void)file;
   (void)func;

   free(ptr);
}



/* Function: al_realloc_with_context
 */
void *al_realloc_with_context(void *ptr, size_t n,
   int line, const char *file, const char *func)
{
   (void)line;
   (void)file;
   (void)func;

   return realloc(ptr, n);
}



/* Function: al_calloc_with_context
 */
void *al_calloc_with_context(size_t count, size_t n,
   int line, const char *file, const char *func)
{
   (void)line;
   (void)file;
   (void)func;

   return calloc(count, n);
}


/* vim: set ts=8 sts=3 sw=3 et: */
