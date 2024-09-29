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
 *      Convert between memory and video bitmap types.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_shader.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_vector.h"

A5O_DEBUG_CHANNEL("bitmap")


/* Global list of MEMORY bitmaps with A5O_CONVERT_BITMAP flag. */
struct BITMAP_CONVERSION_LIST {
   A5O_MUTEX *mutex;
   _AL_VECTOR bitmaps;
};


static struct BITMAP_CONVERSION_LIST convert_bitmap_list;


static void cleanup_convert_bitmap_list(void)
{
   _al_vector_free(&convert_bitmap_list.bitmaps);
   al_destroy_mutex(convert_bitmap_list.mutex);
}


/* This is called in al_install_system. Exit functions are called in
 * al_uninstall_system.
 */
void _al_init_convert_bitmap_list(void)
{
   convert_bitmap_list.mutex = al_create_mutex_recursive();
   _al_vector_init(&convert_bitmap_list.bitmaps, sizeof(A5O_BITMAP *));
   _al_add_exit_func(cleanup_convert_bitmap_list,
      "cleanup_convert_bitmap_list");
}


void _al_register_convert_bitmap(A5O_BITMAP *bitmap)
{
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   if (!(bitmap_flags & A5O_MEMORY_BITMAP))
      return;
   if (bitmap_flags & A5O_CONVERT_BITMAP) {
      A5O_BITMAP **back;
      al_lock_mutex(convert_bitmap_list.mutex);
      back = _al_vector_alloc_back(&convert_bitmap_list.bitmaps);
      *back = bitmap;
      al_unlock_mutex(convert_bitmap_list.mutex);
   }
}


void _al_unregister_convert_bitmap(A5O_BITMAP *bitmap)
{
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   if (!(bitmap_flags & A5O_MEMORY_BITMAP))
      return;
   if (bitmap_flags & A5O_CONVERT_BITMAP) {
      al_lock_mutex(convert_bitmap_list.mutex);
      _al_vector_find_and_delete(&convert_bitmap_list.bitmaps, &bitmap);
      al_unlock_mutex(convert_bitmap_list.mutex);
   }
}


static void swap_bitmaps(A5O_BITMAP *bitmap, A5O_BITMAP *other)
{
   A5O_BITMAP temp;
   _AL_LIST_ITEM *bitmap_dtor_item = bitmap->dtor_item;
   _AL_LIST_ITEM *other_dtor_item = other->dtor_item;
   A5O_DISPLAY *bitmap_display, *other_display;

   _al_unregister_convert_bitmap(bitmap);
   _al_unregister_convert_bitmap(other);

   if (other->shader)
      _al_unregister_shader_bitmap(other->shader, other);
   if (bitmap->shader)
      _al_unregister_shader_bitmap(bitmap->shader, bitmap);

   temp = *bitmap;
   *bitmap = *other;
   *other = temp;

   /* Re-associate the destructors back, as they are tied to the object
    * pointers.
    */
   bitmap->dtor_item = bitmap_dtor_item;
   other->dtor_item = other_dtor_item;

   bitmap_display = _al_get_bitmap_display(bitmap);
   other_display = _al_get_bitmap_display(other);

   /* We are basically done already. Except we now have to update everything
    * possibly referencing any of the two bitmaps.
    */

   if (bitmap_display && !other_display) {
      /* This means before the swap, other was the display bitmap, and we
       * now should replace it with the swapped pointer.
       */
      A5O_BITMAP **back;
      int pos = _al_vector_find(&bitmap_display->bitmaps, &other);
      ASSERT(pos >= 0);
      back = _al_vector_ref(&bitmap_display->bitmaps, pos);
      *back = bitmap;
   }

   if (other_display && !bitmap_display) {
      A5O_BITMAP **back;
      int pos = _al_vector_find(&other_display->bitmaps, &bitmap);
      ASSERT(pos >= 0);
      back = _al_vector_ref(&other_display->bitmaps, pos);
      *back = other;
   }

   if (other->shader)
      _al_register_shader_bitmap(other->shader, other);
   if (bitmap->shader)
      _al_register_shader_bitmap(bitmap->shader, bitmap);

   _al_register_convert_bitmap(bitmap);
   _al_register_convert_bitmap(other);

   if (bitmap->vt && bitmap->vt->bitmap_pointer_changed)
      bitmap->vt->bitmap_pointer_changed(bitmap, other);

   if (other->vt && other->vt->bitmap_pointer_changed)
      other->vt->bitmap_pointer_changed(other, bitmap);
}


/* Function: al_convert_bitmap
 */
void al_convert_bitmap(A5O_BITMAP *bitmap)
{
   A5O_BITMAP *clone;
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   int new_bitmap_flags = al_get_new_bitmap_flags();
   bool want_memory = (new_bitmap_flags & A5O_MEMORY_BITMAP) != 0;
   bool clone_memory;
   A5O_BITMAP *target_bitmap;
   
   bitmap_flags &= ~_A5O_INTERNAL_OPENGL;

   /* If a cloned bitmap would be identical, we can just do nothing. */
   if (al_get_bitmap_format(bitmap) == al_get_new_bitmap_format() &&
         bitmap_flags == new_bitmap_flags &&
         _al_get_bitmap_display(bitmap) == al_get_current_display()) {
      return;
   }

   if (bitmap->parent) {
      al_convert_bitmap(bitmap->parent);
      return;
   }
   else {
      clone = al_clone_bitmap(bitmap);
   }

   if (!clone) {
      return;
   }

   clone_memory = (al_get_bitmap_flags(clone) & A5O_MEMORY_BITMAP) != 0;

   if (clone_memory != want_memory) {
      /* We couldn't convert. */
      al_destroy_bitmap(clone);
      return;
   }

   swap_bitmaps(bitmap, clone);

   /* Preserve bitmap state. */
   bitmap->cl = clone->cl;
   bitmap->ct = clone->ct;
   bitmap->cr_excl = clone->cr_excl;
   bitmap->cb_excl = clone->cb_excl;
   bitmap->transform = clone->transform;
   bitmap->inverse_transform = clone->inverse_transform;
   bitmap->inverse_transform_dirty = clone->inverse_transform_dirty;

   /* Memory bitmaps do not support custom projection transforms,
    * so reset it to the orthographic transform. */
   if (new_bitmap_flags & A5O_MEMORY_BITMAP) {
      al_identity_transform(&bitmap->proj_transform);
      al_orthographic_transform(&bitmap->proj_transform, 0, 0, -1.0, bitmap->w, bitmap->h, 1.0);
   } else {
      bitmap->proj_transform = clone->proj_transform;
   }

   /* If we just converted this bitmap, and the backing bitmap is the same
    * as the target's backing bitmap, then the viewports and transformations
    * will be messed up. Detect this, and just re-call al_set_target_bitmap
    * on the current target. */
   target_bitmap = al_get_target_bitmap();
   if (target_bitmap) {
      A5O_BITMAP *target_parent =
         target_bitmap->parent ? target_bitmap->parent : target_bitmap;
      if (bitmap == target_parent || bitmap->parent == target_parent) {
         al_set_target_bitmap(target_bitmap);
      }
   }

   al_destroy_bitmap(clone);
}


/* Function: al_convert_memory_bitmaps
 */
void al_convert_memory_bitmaps(void)
{
   A5O_STATE backup;
   A5O_DISPLAY *display = al_get_current_display();
   _AL_VECTOR copy;
   size_t i;
   if (!display) return;
   
   al_store_state(&backup, A5O_STATE_NEW_BITMAP_PARAMETERS);

   al_lock_mutex(convert_bitmap_list.mutex);
   
   _al_vector_init(&copy, sizeof(A5O_BITMAP *));
   for (i = 0; i <  _al_vector_size(&convert_bitmap_list.bitmaps); i++) {
      A5O_BITMAP **bptr, **bptr2;
      bptr = _al_vector_ref(&convert_bitmap_list.bitmaps, i);
      bptr2 = _al_vector_alloc_back(&copy);
      *bptr2 = *bptr;
   }
   _al_vector_free(&convert_bitmap_list.bitmaps);
   _al_vector_init(&convert_bitmap_list.bitmaps, sizeof(A5O_BITMAP *));
   for (i = 0; i < _al_vector_size(&copy); i++) {
      A5O_BITMAP **bptr;
      int flags;
      bptr = _al_vector_ref(&copy, i);
      flags = al_get_bitmap_flags(*bptr);
      flags &= ~A5O_MEMORY_BITMAP;
      al_set_new_bitmap_flags(flags);
      al_set_new_bitmap_format(al_get_bitmap_format(*bptr));
      
      A5O_DEBUG("converting memory bitmap %p to display bitmap\n", *bptr);
      
      al_convert_bitmap(*bptr);
   }

   _al_vector_free(&copy);

   al_unlock_mutex(convert_bitmap_list.mutex);

   al_restore_state(&backup);
}


/* Converts a memory bitmap to a display bitmap preserving its contents.
 * The created bitmap belongs to the current display.
 * 
 * If this is called for a sub-bitmap, the parent also is converted.
 */
void _al_convert_to_display_bitmap(A5O_BITMAP *bitmap)
{
   A5O_STATE backup;
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   /* Do nothing if it is a display bitmap already. */
   if (!(bitmap_flags & A5O_MEMORY_BITMAP))
      return;

   A5O_DEBUG("converting memory bitmap %p to display bitmap\n", bitmap);

   al_store_state(&backup, A5O_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags(bitmap_flags & ~A5O_MEMORY_BITMAP);
   al_set_new_bitmap_format(al_get_bitmap_format(bitmap));
   al_convert_bitmap(bitmap);
   al_restore_state(&backup);
}


/* Converts a display bitmap to a memory bitmap preserving its contents.
 * If this is called for a sub-bitmap, the parent also is converted.
 */
void _al_convert_to_memory_bitmap(A5O_BITMAP *bitmap)
{
   A5O_STATE backup;
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   /* Do nothing if it is a memory bitmap already. */
   if (bitmap_flags & A5O_MEMORY_BITMAP)
      return;

   A5O_DEBUG("converting display bitmap %p to memory bitmap\n", bitmap);

   al_store_state(&backup, A5O_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags((bitmap_flags & ~A5O_VIDEO_BITMAP) | A5O_MEMORY_BITMAP);
   al_set_new_bitmap_format(al_get_bitmap_format(bitmap));
   al_convert_bitmap(bitmap);
   al_restore_state(&backup);
}


/* vim: set ts=8 sts=3 sw=3 et: */
