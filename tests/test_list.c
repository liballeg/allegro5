/*
 *    Tests for Allegro's list function.
 */

#include <assert.h>
#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_list.h"

static void test_basic_dynamic_usage(void)
{
   int xs[4] = {1, 2, 3, 4};
   _AL_LIST_ITEM* item;
   int i;
   _AL_LIST* list = _al_list_create();
   assert(_al_list_is_empty(list));
   assert(_al_list_front(list) == NULL);
   assert(_al_list_back(list) == NULL);
   assert(_al_list_size(list) == 0);

   _al_list_push_back(list, &xs[0]);
   assert(!_al_list_is_empty(list));
   assert(_al_list_size(list) == 1);
   item = _al_list_front(list);
   i = 0;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_at(list, i);
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      assert(_al_list_contains(list, &xs[i]));
      item = _al_list_next(list, item);
      i++;
   }
   item = _al_list_back(list);
   i = _al_list_size(list) - 1;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_previous(list, item);
      i--;
   }

   _al_list_push_back(list, &xs[1]);
   assert(_al_list_size(list) == 2);
   item = _al_list_front(list);
   i = 0;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_at(list, i);
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      assert(_al_list_contains(list, &xs[i]));
      item = _al_list_next(list, item);
      i++;
   }
   item = _al_list_back(list);
   i = _al_list_size(list) - 1;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_previous(list, item);
      i--;
   }

   _al_list_push_back(list, &xs[2]);
   assert(_al_list_size(list) == 3);
   item = _al_list_front(list);
   i = 0;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_at(list, i);
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      assert(_al_list_contains(list, &xs[i]));
      item = _al_list_next(list, item);
      i++;
   }
   item = _al_list_back(list);
   i = _al_list_size(list) - 1;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_previous(list, item);
      i--;
   }

   _al_list_push_back(list, &xs[3]);
   assert(_al_list_size(list) == 4);
   item = _al_list_front(list);
   i = 0;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_at(list, i);
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      assert(_al_list_contains(list, &xs[i]));
      item = _al_list_next(list, item);
      i++;
   }
   item = _al_list_back(list);
   i = _al_list_size(list) - 1;
   while (item) {
      assert(*(int*)_al_list_item_data(item) == xs[i]);
      item = _al_list_previous(list, item);
      i--;
   }
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;

   test_basic_dynamic_usage();

   return 0;
}
