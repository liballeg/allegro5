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
 *      Vectors, aka growing arrays.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 *
 *
 *      This is a simple growing array to hold objects of various sizes,
 *      growing by powers of two as needed.  At the moment the vector never
 *      shrinks, except when it is freed.  Usually the vector would hold
 *      pointers to objects, not the objects themselves, as the vector is
 *      allowed to move the objects around.
 *
 *      This module is NOT thread-safe.
 */

/* Internal Title: Vectors
 */


#include <stdlib.h>
#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_vector.h"


/* return the given item's starting address in the vector */
#define ITEM_START(vec, idx)    (vec->_items + ((idx) * vec->_itemsize))



/* Internal function: _al_vector_init
 *
 *  Initialise a vector.  ITEMSIZE is the number of bytes to allocate for
 *  each item in the vector.
 *
 *  Alternatively, you can statically initialise a vector using
 *  _AL_VECTOR vec = _AL_VECTOR_INITIALIZER(itemtype);
 */
void _al_vector_init(_AL_VECTOR *vec, size_t itemsize)
{
   ASSERT(vec);
   ASSERT(itemsize > 0);

   vec->_itemsize = itemsize;
   vec->_items = NULL;
   vec->_size = 0;
   vec->_unused = 0;
}



/*
 * Simple inline functions:
 *
 * size_t _al_vector_size(const _AL_VECTOR *vec);
 * bool _al_vector_is_empty(const _AL_VECTOR*);
 */



/* Internal function: _al_vector_ref
 *
 *  Return a pointer to the SLOT in the vector given by INDEX.  The returned
 *  address should only be used while the vector is not modified; after that
 *  it is invalid.
 *
 *  Tip: If you are storing pointers in the vector, you need to dereference
 *  the returned value!
 */
void *_al_vector_ref(const _AL_VECTOR *vec, unsigned int idx)
{
   ASSERT(vec);
   ASSERT(idx < vec->_size);

   return ITEM_START(vec, idx);
}



/* Internal function: _al_vector_ref_front
 *  Convenience function.
 */
void* _al_vector_ref_front(const _AL_VECTOR *vec)
{
   return _al_vector_ref(vec, 0);
}



/* Internal function: _al_vector_ref_back
 *  Convenience function.
 */
void* _al_vector_ref_back(const _AL_VECTOR *vec)
{
   ASSERT(vec);

   return _al_vector_ref(vec, vec->_size-1);
}



/* Internal function: _al_vector_append_array
 *  Append `num` elements from `arr` array to _AL_VECTOR `vec`
 */
bool _al_vector_append_array(_AL_VECTOR *vec, unsigned int num, const void *arr)
{
   ASSERT(vec);
   ASSERT(arr);
   ASSERT(num);

   if (vec->_items == NULL) {
      ASSERT(vec->_size == 0);
      ASSERT(vec->_unused == 0);

      vec->_items = al_malloc(vec->_itemsize * num);
      ASSERT(vec->_items);
      if (!vec->_items)
         return false;

      vec->_unused = num;
   }
   else if (vec->_unused < num) {
      char *new_items;
      new_items = al_realloc(vec->_items, (vec->_size + num) * vec->_itemsize);
      ASSERT(new_items);
      if (!new_items)
         return false;

      vec->_items = new_items;
      vec->_unused = num;
   }

   memcpy(vec->_items + (vec->_size * vec->_itemsize),
      arr, vec->_itemsize * num);

   vec->_size += num;
   vec->_unused -= num;

   return true;
}



/* Internal function: _al_vector_alloc_back
 *
 *  Allocate a block of memory at the back of the vector of the vector's item
 *  size (see _AL_VECTOR_INITIALIZER and _al_vector_init).  Returns a pointer
 *  to the start of this block.  This address should only be used while the
 *  vector is not modified; after that it is invalid.  You may fill the block
 *  with whatever you want.
 *
 *  Example:
 *   _AL_VECTOR vec = _AL_VECTOR_INITIALIZER(struct boo);
 *   struct boo *thing = _al_vector_alloc_back(&vec);
 *   thing->aaa = 100;
 *   thing->bbb = "a string";
 */
void* _al_vector_alloc_back(_AL_VECTOR *vec)
{
   ASSERT(vec);
   ASSERT(vec->_itemsize > 0);
   {
      if (vec->_items == NULL) {
         ASSERT(vec->_size == 0);
         ASSERT(vec->_unused == 0);

         vec->_items = al_malloc(vec->_itemsize);
         ASSERT(vec->_items);
         if (!vec->_items)
            return NULL;

         vec->_unused = 1;
      }
      else if (vec->_unused == 0) {
         char *new_items = al_realloc(vec->_items, 2 * vec->_size * vec->_itemsize);
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



/* Internal function: _al_vector_alloc_mid
 *
 *  Allocate a block of memory in the middle of the vector of the vector's
 *  item
 *  size (see _AL_VECTOR_INITIALIZER and _al_vector_init).  Returns a pointer
 *  to the start of this block.  This address should only be used while the
 *  vector is not modified; after that it is invalid.  You may fill the block
 *  with whatever you want.
 */
void* _al_vector_alloc_mid(_AL_VECTOR *vec, unsigned int index)
{
   ASSERT(vec);
   ASSERT(vec->_itemsize > 0);
   {
      if (vec->_items == NULL) {
         ASSERT(index == 0);
         return _al_vector_alloc_back(vec);
      }

      if (vec->_unused == 0) {
         char *new_items = al_realloc(vec->_items, 2 * vec->_size * vec->_itemsize);
         ASSERT(new_items);
         if (!new_items)
            return NULL;

         vec->_items = new_items;
         vec->_unused = vec->_size;
      }

      memmove(ITEM_START(vec, index + 1), ITEM_START(vec, index),
      vec->_itemsize * (vec->_size - index));

      vec->_size++;
      vec->_unused--;

      return ITEM_START(vec, index);
   }
}



/* Internal function: _al_vector_find
 *
 *  Find the slot in the vector where the contents of the slot
 *  match whatever PTR_ITEM points to, bit-for-bit.  If no such
 *  slot is found, a negative number is returned (currently -1).
 */
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



/* Internal function: _al_vector_contains
 *  A simple wrapper over _al_vector_find.
 */
bool _al_vector_contains(const _AL_VECTOR *vec, const void *ptr_item)
{
   return _al_vector_find(vec, ptr_item) >= 0;
}



/* Internal function: _al_vector_delete_at
 *
 *  Delete the slot given by index.  Deleting from the start or middle of the
 *  vector requires moving the rest of the vector towards the front, so it is
 *  better to delete from the tail of the vector.
 *
 *  Example:
 *   while (!_al_vector_is_empty(&v))
 *      _al_vector_delete_at(&v, _al_vector_size(&v)-1);
 */
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



/* Internal function: _al_vector_find_and_delete
 *
 *  Similar to _al_vector_delete_at(_al_vector_find(vec, ptr_item)) but is
 *  lenient if the item is not found.  Returns true if the item was found and
 *  deleted.
 */
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



/* Internal function: _al_vector_free
 *
 *  Free the space used by the vector.  You really must do this at some
 *  stage.  It is not enough to delete all the items in the vector (which you
 *  should usually do also).
 */
void _al_vector_free(_AL_VECTOR *vec)
{
   ASSERT(vec);

   if (vec->_items != NULL) {
      al_free(vec->_items);
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
