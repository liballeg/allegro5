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
 *      Double linked list.
 *
 *      By Michał Cichoń.
 *
 *      See readme.txt for copyright information.
 *
 *
 *      This is a simple general purpose double linked list.
 *
 *      This module is NOT thread-safe.
 */

#include <stdlib.h>
#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_list.h"


ALLEGRO_DEBUG_CHANNEL("list")


/* Definition of list, holds root and size. */
struct _AL_LIST {
   /* Root of the list. It is an element, but
    * not visible one. Using it end and the
    * beginning can be easily identified. */
   _AL_LIST_ITEM* root;
   size_t         size;
   size_t         capacity;
   size_t         item_size;
   size_t         item_size_with_extra;
   _AL_LIST_ITEM* next_free;
   void*          user_data;
   _AL_LIST_DTOR  dtor;
};

/* List item, holds user data and destructor. */
struct _AL_LIST_ITEM {
   _AL_LIST*          list;
   _AL_LIST_ITEM*     next;
   _AL_LIST_ITEM*     prev;
   void*              data;
   _AL_LIST_ITEM_DTOR dtor;
};


/* List of the internal functions. */
static _AL_LIST* list_do_create(size_t capacity, size_t item_extra_size);
static bool      list_is_static(_AL_LIST* list);

static _AL_LIST_ITEM* list_get_free_item(_AL_LIST* list);
static _AL_LIST_ITEM* list_create_item(_AL_LIST* list);
static void           list_destroy_item(_AL_LIST* list, _AL_LIST_ITEM* item);


/*
 *  Create an instance of double linked list.
 *
 *  Parameters:
 *     capacity [in]
 *        Maximum number of elements list can hold. If it is zero, list is
 *        created as fully dynamic linked list with unlimited item count.
 *        For any other positive number static linked list is created,
 *        memory for all elements is allocated once and then used.
 *
 *     extra_item_size [in]
 *        Number of extra bytes which should be left after each list item.
 *        It is currently not used, so default value is zero.
 *
 *  Returns:
 *     Pointer to new instance of double linked list.
 *
 *  Remarks:
 *     There are two kind of double linked list supported: dynamic and static.
 *     For dynamic linked list each item is allocated while adding and freed
 *     while removing. This kind of list does not have capacity limit but
 *     suffer from memory allocation delay.
 *     Static linked list use one memory allocation and are hold as solid
 *     piece of memory. This kind of list have capacity, but adding and
 *     removing elements is very cheap operation.
 */
static _AL_LIST* list_do_create(size_t capacity, size_t extra_item_size)
{
   size_t i;
   size_t memory_size;
   uint8_t* memory_ptr;
   _AL_LIST* list = NULL;
   _AL_LIST_ITEM* item = NULL;
   _AL_LIST_ITEM* prev = NULL;


   /* Calculate amount of memory needed for the list.
    * Always at least one element is allocated together with list,
    * which is intended to be a root.
    */
   memory_size = sizeof(_AL_LIST) + (capacity + 1) * (sizeof(_AL_LIST_ITEM) + extra_item_size);

   memory_ptr = (uint8_t*)al_malloc(memory_size);
   if (NULL == memory_ptr) {
      ALLEGRO_ERROR("Out of memory.");
      return NULL;
   }

   list                       = (_AL_LIST*)memory_ptr;
   memory_ptr                += sizeof(_AL_LIST);
   list->size                 = 0;
   list->capacity             = capacity;
   list->item_size            = sizeof(_AL_LIST_ITEM);
   list->item_size_with_extra = sizeof(_AL_LIST_ITEM) + extra_item_size;
   list->next_free            = (_AL_LIST_ITEM*)memory_ptr;
   list->user_data            = NULL;
   list->dtor                 = NULL;

   /* Initialize free item list.
    */
   prev = NULL;
   item = list->next_free;
   for (i = 0; i <= list->capacity; ++i) {

      memory_ptr += list->item_size_with_extra;
      item->list  = list;
      item->next  = (_AL_LIST_ITEM*)memory_ptr;
      prev        = item;
      item        = item->next;
   }

   /* Set proper free list tail value. */
   prev->next = NULL;

   /* Initialize root. */
   list->root = list_get_free_item(list);
   list->root->dtor = NULL;
   list->root->next = list->root;
   list->root->prev = list->root;

   return list;
}


/*
 *  Returns true if 'list' point to static double linked list.
 */
static bool list_is_static(_AL_LIST* list)
{
   return 0 != list->capacity;
}


/*
 *  Returns free item from internal list. Call to this function
 *  is valid only for static lists.
 */
static _AL_LIST_ITEM* list_get_free_item(_AL_LIST* list)
{
   _AL_LIST_ITEM* item;

   //thedmd: disabled, root is always static-like element and this method
   //        is called even for dynamic lists
   //ASSERT(list_is_static(list));

   item = list->next_free;
   if (NULL != item)
      list->next_free = item->next;

   return item;
}


/*
 *  Create an instance of new double linked list item.
 */
static _AL_LIST_ITEM* list_create_item(_AL_LIST* list)
{
   _AL_LIST_ITEM* item = NULL;

   if (list_is_static(list)) {

      /* Items from internal list already are partially initialized.
       * So we do not have to setup list pointer.
       */
      item = list_get_free_item(list);
   }
   else {

      item = (_AL_LIST_ITEM*)al_malloc(list->item_size_with_extra);

      item->list = list;
   }

   return item;
}


/*
 *  Destroys double linked list item. Item destructor is called
 *  when necessary.
 */
static void list_destroy_item(_AL_LIST* list, _AL_LIST_ITEM* item)
{
   ASSERT(list == item->list);

   if (NULL != item->dtor)
      item->dtor(item->data, list->user_data);

   if (list_is_static(list)) {
      item->next      = list->next_free;
      list->next_free = item;
   }
   else
      al_free(item);
}


/*
 *  Create new instance of dynamic double linked list.
 *
 *  See:
 *     list_do_create
 */
_AL_LIST* _al_list_create(void)
{
   return list_do_create(0, 0);
}


/*
 *  Create new instance of list item. Maximum number of list items is
 *  limited by capacity.
 *
 *  See:
 *     list_do_create
 */
_AL_LIST* _al_list_create_static(size_t capacity)
{
   if (capacity < 1) {

      ALLEGRO_ERROR("Cannot create static list without any capacity.");
      return NULL;
   }

   return list_do_create(capacity, 0);
}


/*
 *  Destroys instance of the list. All elements
 *  that list contain are also destroyed.
 */
void _al_list_destroy(_AL_LIST* list)
{
   if (NULL == list)
      return;

   if (list->dtor)
      list->dtor(list->user_data);

   _al_list_clear(list);

   al_free(list);
}


/*
 *  Sets a destructor for the list.
 */
void _al_list_set_dtor(_AL_LIST* list, _AL_LIST_DTOR dtor)
{
   list->dtor = dtor;
}


/*
 *  Returns destructor of the list.
 */
_AL_LIST_DTOR _al_list_get_dtor(_AL_LIST* list)
{
   return list->dtor;
}


/*
 *  Create and push new item at the beginning of the list.
 *
 *  Returns pointer to new item.
 */
_AL_LIST_ITEM* _al_list_push_front(_AL_LIST* list, void* data)
{
   return _al_list_insert_after(list, list->root, data);
}


/*
 *  Pretty the same as _al_list_push_front(), but also allow
 *  to provide custom destructor for the item.
 */
_AL_LIST_ITEM* _al_list_push_front_ex(_AL_LIST* list, void* data, _AL_LIST_ITEM_DTOR dtor)
{
   return _al_list_insert_after_ex(list, list->root, data, dtor);
}


/*
 *  Create and push new item at the end of the list.
 *
 *  Returns pointer to new item.
 */
_AL_LIST_ITEM* _al_list_push_back(_AL_LIST* list, void* data)
{
   return _al_list_insert_before(list, list->root, data);
}


/*
 *  Pretty the same as _al_list_push_back(), but also allow
 *  to provide custom destructor for the item.
 */
_AL_LIST_ITEM* _al_list_push_back_ex(_AL_LIST* list, void* data, _AL_LIST_ITEM_DTOR dtor)
{
   return _al_list_insert_before_ex(list, list->root, data, dtor);
}


/*
 *  Remove first item in the list.
 */
void _al_list_pop_front(_AL_LIST* list)
{
   if (list->size > 0)
      _al_list_erase(list, list->root->next);
}


/*
 *  Remove last item in the list.
 */
void _al_list_pop_back(_AL_LIST* list)
{
   if (list->size > 0)
      _al_list_erase(list, list->root->prev);
}


/*
 *  Create and insert new item after one specified by 'where'.
 *
 *  Returns pointer to new item.
 */
_AL_LIST_ITEM* _al_list_insert_after(_AL_LIST* list, _AL_LIST_ITEM* where, void* data)
{
   return _al_list_insert_after_ex(list, where, data, NULL);
}


/*
 *  Pretty the same as _al_list_insert_after(), but also allow
 *  to provide custom destructor for the item.
 */
_AL_LIST_ITEM* _al_list_insert_after_ex(_AL_LIST* list, _AL_LIST_ITEM* where, void* data, _AL_LIST_ITEM_DTOR dtor)
{
   _AL_LIST_ITEM* item;

   ASSERT(list == where->list);

   item = list_create_item(list);
   if (NULL == item)
      return NULL;

   item->data = data;
   item->dtor = dtor;

   item->prev = where;
   item->next = where->next;

   where->next->prev = item;
   where->next       = item;

   list->size++;

   return item;
}


/*
 *  Create and insert new item before one specified by 'where'.
 *
 *  Returns pointer to new item.
 */
_AL_LIST_ITEM* _al_list_insert_before(_AL_LIST* list, _AL_LIST_ITEM* where, void* data)
{
   return _al_list_insert_before_ex(list, where, data, NULL);
}


/*
 *  Pretty the same as _al_list_insert_before(), but also allow
 *  to provide custom destructor for the item.
 */
_AL_LIST_ITEM* _al_list_insert_before_ex(_AL_LIST* list, _AL_LIST_ITEM* where, void* data, _AL_LIST_ITEM_DTOR dtor)
{
   _AL_LIST_ITEM* item;

   ASSERT(list == where->list);

   item = list_create_item(list);
   if (NULL == item)
      return NULL;

   item->data = data;
   item->dtor = dtor;

   item->next = where;
   item->prev = where->prev;

   where->prev->next = item;
   where->prev       = item;

   list->size++;

   return item;
}


/*
 *  Remove specified item from the list.
 */
void _al_list_erase(_AL_LIST* list, _AL_LIST_ITEM* item)
{
   if (NULL == item)
      return;

   ASSERT(list == item->list);

   item->prev->next = item->next;
   item->next->prev = item->prev;

   list->size--;

   list_destroy_item(list, item);
}


/*
 *  Remove all items from the list.
 */
void _al_list_clear(_AL_LIST* list)
{
   _AL_LIST_ITEM* item;
   _AL_LIST_ITEM* next;

   item = _al_list_front(list);

   while (NULL != item) {

      next = _al_list_next(list, item);

      _al_list_erase(list, item);

      item = next;
   }
}


/*
 *  Remove all occurrences of specified value in the list.
 */
void _al_list_remove(_AL_LIST* list, void* data)
{
   _AL_LIST_ITEM* item = NULL;
   _AL_LIST_ITEM* next = NULL;

   item = _al_list_find_first(list, data);

   while (NULL != item) {

      next = _al_list_find_after(list, item, data);

      _al_list_erase(list, item);

      item = next;
   }
}


/*
 *  Returns true if list is empty.
 */
bool _al_list_is_empty(_AL_LIST* list)
{
   return 0 == list->size;
}


/*
 *  Returns true if list contain specified value.
 */
bool _al_list_contains(_AL_LIST* list, void* data)
{
   return NULL != _al_list_find_first(list, data);
}


/*
 *  Returns first occurrence of specified value in the list.
 */
_AL_LIST_ITEM* _al_list_find_first(_AL_LIST* list, void* data)
{
   return _al_list_find_after(list, list->root, data);
}


/*
 *  Returns last occurrence of specified value in the list.
 */
_AL_LIST_ITEM* _al_list_find_last(_AL_LIST* list, void* data)
{
   return _al_list_find_before(list, list->root, data);
}


/*
 *  Return occurrence of specified value in the list after 'where' item.
 */
_AL_LIST_ITEM* _al_list_find_after(_AL_LIST* list, _AL_LIST_ITEM* where, void* data)
{
   _AL_LIST_ITEM* item;

   ASSERT(list == where->list);

   for (item = where->next; item != list->root; item = item->next)
      if (item->data == data)
         return item;

   return NULL;
}


/*
 *  Return occurrence of specified value in the list before 'where' item.
 */
_AL_LIST_ITEM* _al_list_find_before(_AL_LIST* list, _AL_LIST_ITEM* where, void* data)
{
   _AL_LIST_ITEM* item;

   ASSERT(list == where->list);

   for (item = where->prev; item != list->root; item = item->prev)
      if (item->data == data)
         return item;

   return NULL;
}


/*
 *  Returns current size of the list.
 */
size_t _al_list_size(_AL_LIST* list)
{
   return list->size;
}


/*
 *  Returns item located at specified index.
 */
_AL_LIST_ITEM* _al_list_at(_AL_LIST* list, size_t index)
{
   if (index >= list->size)
      return NULL;

   if (index < list->size / 2) {

      _AL_LIST_ITEM* item = list->root->next;

      while (index--)
         item = item->next;

      return item;
   }
   else {

      _AL_LIST_ITEM* item = list->root->prev;

      index = list->size - index - 1;

      while (index--)
         item = item->prev;

      return item;
   }
}


/*
 *  Returns first item in the list.
 */
_AL_LIST_ITEM* _al_list_front(_AL_LIST* list)
{
   if (list->size > 0)
      return list->root->next;
   else
      return NULL;
}


/*
 *  Returns last item in the list.
 */
_AL_LIST_ITEM* _al_list_back(_AL_LIST* list)
{
   if (list->size > 0)
      return list->root->prev;
   else
      return NULL;
}


/*
 *  Returns next element in the list.
 */
_AL_LIST_ITEM* _al_list_next(_AL_LIST* list, _AL_LIST_ITEM* item)
{
   ASSERT(list == item->list);
   (void)list;

   if (item->next != item->list->root)
      return item->next;
   else
      return NULL;
}


/*
 *  Returns previous element in the list.
 */
_AL_LIST_ITEM* _al_list_previous(_AL_LIST* list, _AL_LIST_ITEM* item)
{
   ASSERT(list == item->list);
   (void)list;

   if (item->prev != item->list->root)
      return item->prev;
   else
      return NULL;
}


/*
 *  Returns next element in the list. If end of the list is reached,
 *  first element is returned instead of NULL.
 */
_AL_LIST_ITEM* _al_list_next_circular(_AL_LIST* list, _AL_LIST_ITEM* item)
{
   ASSERT(list == item->list);

   if (item->next != item->list->root)
      return item->next;
   else
      return list->root->next;
}


/*
 *  Returns previous element in the list. If beginning of the list is reached,
 *  last element is returned instead of NULL.
 */
_AL_LIST_ITEM* _al_list_previous_circular(_AL_LIST* list, _AL_LIST_ITEM* item)
{
   ASSERT(list == item->list);

   if (item->prev != item->list->root)
      return item->prev;
   else
      return list->root->prev;
}


/*
 *  Returns value associated with specified item.
 */
void* _al_list_item_data(_AL_LIST_ITEM* item)
{
   return item->data;
}


/*
 *  Sets item destructor.
 */
void _al_list_item_set_dtor(_AL_LIST_ITEM* item, _AL_LIST_ITEM_DTOR dtor)
{
   item->dtor = dtor;
}


/*
 *  Returns item destructor.
 */
_AL_LIST_ITEM_DTOR _al_list_item_get_dtor(_AL_LIST_ITEM* item)
{
   return item->dtor;
}


/*
 *  Sets user data for list. This pointer is passed to list destructor.
 */
void _al_list_set_user_data(_AL_LIST* list, void* user_data)
{
   list->user_data = user_data;
}


/*
 *  Returns user data for list.
 */
void* _al_list_get_user_data(_AL_LIST* list)
{
   return list->user_data;
}
