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


#include "allegro5/allegro.h"


/* globals */
static ALLEGRO_MEMORY_INTERFACE *mem = NULL;



/* Function: al_set_memory_interface
 */
void al_set_memory_interface(ALLEGRO_MEMORY_INTERFACE *memory_interface)
{
   mem = memory_interface;
}



/* Function: al_malloc_with_context
 */
void *al_malloc_with_context(size_t n,
   int line, const char *file, const char *func)
{
   if (mem)
      return mem->mi_malloc(n, line, file, func);
   else
      return malloc(n);
}



/* Function: al_free_with_context
 */
void al_free_with_context(void *ptr,
   int line, const char *file, const char *func)
{
   if (mem)
      mem->mi_free(ptr, line, file, func);
   else
      free(ptr);
}



/* Function: al_realloc_with_context
 */
void *al_realloc_with_context(void *ptr, size_t n,
   int line, const char *file, const char *func)
{
   if (mem)
      return mem->mi_realloc(ptr, n, line, file, func);
   else
      return realloc(ptr, n);
}



/* Function: al_calloc_with_context
 */
void *al_calloc_with_context(size_t count, size_t n,
   int line, const char *file, const char *func)
{
   if (mem)
      return mem->mi_calloc(count, n, line, file, func);
   else
      return calloc(count, n);
}


/* vim: set ts=8 sts=3 sw=3 et: */
