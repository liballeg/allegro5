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
 *	Vectors, aka growing arrays.
 *
 *	By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"


#define ITEM_START(vec, idx)    (vec->_items + ((idx) * vec->_itemsize))



void _al_vector_init(_AL_VECTOR *vec, size_t itemsize)
{
   ASSERT(vec);
   ASSERT(itemsize > 0);

   vec->_itemsize = itemsize;
   vec->_items = NULL;
   vec->_size = 0;
   vec->_unused = 0;
}



void *_al_vector_ref(const _AL_VECTOR *vec, unsigned int idx)
{
   ASSERT(vec);
   ASSERT(idx < vec->_size);

   return ITEM_START(vec, idx);
}



void* _al_vector_ref_front(const _AL_VECTOR *vec)
{
   return _al_vector_ref(vec, 0);
}



void* _al_vector_ref_back(const _AL_VECTOR *vec)
{
   ASSERT(vec);

   return _al_vector_ref(vec, vec->_size-1);
}



void* _al_vector_alloc_back(_AL_VECTOR *vec)
{
   ASSERT(vec);
   ASSERT(vec->_itemsize > 0);
   {
      if (vec->_items == NULL) {
         ASSERT(vec->_size == 0);
         ASSERT(vec->_unused == 0);

         vec->_items = malloc(vec->_itemsize);
         ASSERT(vec->_items);
         if (!vec->_items)
            return NULL;

         vec->_unused = 1;
      }
      else if (vec->_unused == 0) {
         char *new_items = realloc(vec->_items, 2 * vec->_size * vec->_itemsize);
         ASSERT(new_items);
         if (!new_items)
            return NULL;

         vec->_items = new_items;
         vec->_unused = vec->_size;
      }
   
      vec->_size++;
      vec->_unused--;

      return ITEM_START(vec, vec->_size-1);
   }
}



int _al_vector_find(const _AL_VECTOR *vec, const void *ptr_item)
{
   ASSERT(vec);
   ASSERT(ptr_item);

   if (vec->_itemsize == sizeof(void *)) {
      /* fast path for pointers */
      void **items = (void **)vec->_items;
      unsigned int i;

      for (i = 0; i < vec->_size; i++)
         if (items[i] == *(void **)ptr_item)
            return i;
   }
   else {
      /* slow path */
      unsigned int i;
      for (i = 0; i < vec->_size; i++)
         if (memcmp(ITEM_START(vec, i), ptr_item, vec->_itemsize) == 0)
            return i;
   }

   return -1;
}



bool _al_vector_contains(const _AL_VECTOR *vec, const void *ptr_item)
{
   return _al_vector_find(vec, ptr_item) >= 0;
}



void _al_vector_delete_at(_AL_VECTOR *vec, unsigned int idx)
{
   ASSERT(vec);
   ASSERT(idx < vec->_size);
   {
      int to_move = vec->_size - idx - 1;
      if (to_move > 0)
         memmove(ITEM_START(vec, idx),
                 ITEM_START(vec, idx+1),
                 to_move * vec->_itemsize);
      vec->_size--;
      vec->_unused++;
      memset(ITEM_START(vec, vec->_size), 0, vec->_itemsize);
   }
}



bool _al_vector_find_and_delete(_AL_VECTOR *vec, const void *ptr_item)
{
   int idx = _al_vector_find(vec, ptr_item);
   if (idx >= 0) {
      _al_vector_delete_at(vec, idx);
      return true;
   }
   else
      return false;
}



void _al_vector_free(_AL_VECTOR *vec)
{
   ASSERT(vec);

   if (vec->_items != NULL) {
      free(vec->_items);
      vec->_items = NULL;
   }
   vec->_size = 0;
   vec->_unused = 0;
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
