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

/* Title: Memory management routines
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"



/* forward declarations */
static void *default_malloc(void *opaque, size_t size);
static void default_free(void *opaque, void *ptr);
static void *default_realloc(void *opaque, void *ptr, size_t size);
static void *default_debug_malloc(int line, const char *file, const char *func,
   void *opaque, size_t size);
static void default_debug_free(int line, const char *file, const char *func,
   void *opaque, void *ptr);
static void *default_debug_realloc(int line, const char *file, const char *func,
   void *opaque, void *ptr, size_t size);



/* global variables */
void *_al_memory_opaque = NULL;
void *(*_al_malloc)(void *opaque, size_t size) = default_malloc;
void *(*_al_malloc_atomic)(void *opaque, size_t size) = default_malloc;
void (*_al_free)(void *opaque, void *ptr) = default_free;
void *(*_al_realloc)(void *opaque, void *ptr, size_t size) = default_realloc;
void *(*_al_debug_malloc)(int line, const char *file, const char *func,
   void *opaque, size_t size) = default_debug_malloc;
void *(*_al_debug_malloc_atomic)(int line, const char *file, const char *func,
   void *opaque, size_t size) = default_debug_malloc;
void (*_al_debug_free)(int line, const char *file, const char *func,
   void *opaque, void *ptr) = default_debug_free;
void *(*_al_debug_realloc)(int line, const char *file, const char *func,
   void *opaque, void *ptr, size_t size) = default_debug_realloc;



/* _al_set_memory_management_functions:
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
   void *user_opaque)
{
   _al_malloc        = malloc ? malloc : default_malloc;
   _al_malloc_atomic = malloc_atomic ? malloc_atomic : default_malloc;
   _al_free          = free ? free : default_free;
   _al_realloc       = realloc ? realloc : default_realloc;
   _al_debug_malloc  = debug_malloc ? debug_malloc : default_debug_malloc;
   _al_debug_malloc_atomic = debug_malloc_atomic ?
                             debug_malloc_atomic : default_debug_malloc;
   _al_debug_free    = debug_free ? debug_free : default_debug_free;
   _al_debug_realloc = debug_realloc ? debug_realloc : default_debug_realloc;
   _al_memory_opaque = user_opaque;
}



/*
 * The default implementations of the memory management functions.
 */
static void *default_malloc(void *opaque, size_t size)
{
   (void)opaque;
   return malloc(size);
}

static void default_free(void *opaque, void *ptr)
{
   (void)opaque;
   free(ptr);
}

static void *default_realloc(void *opaque, void *ptr, size_t size)
{
   (void)opaque;
   return realloc(ptr, size);
}

static void *default_debug_malloc(int line, const char *file, const char *func,
   void *opaque, size_t size)
{
   ASSERT(_al_malloc);
   (void)line;
   (void)file;
   (void)func;

   return _al_malloc(opaque, size);
}

static void default_debug_free(int line, const char *file, const char *func,
   void *opaque, void *ptr)
{
   ASSERT(_al_free);
   (void)line;
   (void)file;
   (void)func;

   _al_free(opaque, ptr);
}

static void *default_debug_realloc(int line, const char *file, const char *func,
   void *opaque, void *ptr, size_t size)
{
   ASSERT(_al_realloc);
   (void)line;
   (void)file;
   (void)func;

   return _al_realloc(opaque, ptr, size);
}


/* Function: al_free
 */
void al_free(void *ptr)
{
   _AL_FREE(ptr);
}


/* vim: set ts=8 sts=3 sw=3 et: */
