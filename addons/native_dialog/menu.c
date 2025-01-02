#define ALLEGRO_INTERNAL_UNSTABLE

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_vector.h"

/* DISPLAY_MENU keeps track of which menu is associated with a display */
typedef struct DISPLAY_MENU DISPLAY_MENU;

struct DISPLAY_MENU
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_MENU *menu;
};

static _AL_VECTOR display_menus = _AL_VECTOR_INITIALIZER(DISPLAY_MENU);

/* The unique id. This is used to reverse lookup menus.
 * The primarily need for this arises from Windows, which cannot store
 * ALLEGRO_MENU_ID's wholesale.*/
static uint16_t unique_id;
static _AL_VECTOR menu_ids  = _AL_VECTOR_INITIALIZER(_AL_MENU_ID);

/* The default event source is used with any menu that does not have
   its own event source enabled. */
static ALLEGRO_EVENT_SOURCE default_menu_es;

/* Private functions */
static ALLEGRO_MENU *clone_menu(ALLEGRO_MENU *menu, bool popup);
static ALLEGRO_MENU_ITEM *create_menu_item(char const *title, uint16_t id, int flags, ALLEGRO_MENU *popup);
static void destroy_menu_item(ALLEGRO_MENU_ITEM *item);
static bool find_menu_item_r(ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra);
static ALLEGRO_MENU_ITEM *interpret_menu_id_param(ALLEGRO_MENU **menu, int *id);
static ALLEGRO_MENU_INFO *parse_menu_info(ALLEGRO_MENU *parent, ALLEGRO_MENU_INFO *info);
static bool set_menu_display_r(ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra);

/* True if the id is actually unique.
 */
static bool get_unique_id(uint16_t* id)
{
   if (unique_id + 1 == UINT16_MAX) {
      return false;
   }
   *id = unique_id++;
   return true;
}

/* The menu item owns the icon bitmap. It is converted to a memory bitmap
 * when set to make sure any system threads will be able to read the data.
 */
static void set_item_icon(ALLEGRO_MENU_ITEM *item, ALLEGRO_BITMAP *icon)
{
   item->icon = icon;

   if (icon && al_get_bitmap_flags(item->icon) & ALLEGRO_VIDEO_BITMAP) {
      int old_flags = al_get_new_bitmap_flags();
      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      item->icon = al_clone_bitmap(icon);
      al_destroy_bitmap(icon);
      al_set_new_bitmap_flags(old_flags);
   }
}

static ALLEGRO_MENU_ITEM *create_menu_item(char const *title, uint16_t id, int flags, ALLEGRO_MENU *popup)
{
   ALLEGRO_MENU_ITEM *item = al_calloc(1, sizeof(*item));
   if (!item) return NULL;
   if (!get_unique_id(&item->unique_id)) {
      return NULL;
   }

   if (flags & ALLEGRO_MENU_ITEM_CHECKED)
      flags |= ALLEGRO_MENU_ITEM_CHECKBOX;

   if (title)
      item->caption = al_ustr_new(title);
   item->id = id;
   item->flags = flags;
   item->popup = popup;

   return item;
}

/* Recursively walks over the entire menu structure, calling the user proc once per menu item,
 * and once per menu. The deepest menu is called first. If the proc returns true, then the
 * process terminates. It is not safe for the proc to modify the structure (add/remove items).
 */
bool _al_walk_over_menu(ALLEGRO_MENU *menu,
   bool (*proc)(ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra), void *extra)
{
   ALLEGRO_MENU_ITEM **slot;
   size_t i;
   ASSERT(menu);
   ASSERT(proc);

   for (i = 0; i < _al_vector_size(&menu->items); ++i) {
      slot = _al_vector_ref(&menu->items, i);

      if ((*slot)->popup && _al_walk_over_menu((*slot)->popup, proc, extra))
         return true;

      if (proc(menu, *slot, i, extra))
         return true;
   }

   return proc(menu, NULL, -1, extra);
}

/* A callback proc for _al_walk_over_menu that sets each menu's display parameter
 * to the "extra" parameter.
 */
static bool set_menu_display_r(ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra)
{
   (void) index;

   if (!item) {
      menu->display = extra;
   }

   return false;
}

/* A callback proc for _al_walk_over_menu that searches a menu for a given id. If found it sets
 * the "parent" parameter to the menu that contains it, and the "id" parameter to the index.
 */
static bool find_menu_item_r(ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra)
{
   ALLEGRO_MENU_ITEM *info = (ALLEGRO_MENU_ITEM *) extra;

   if (item != NULL && info->id == item->id) {
      info->id = index;
      info->parent = menu;
      return true;
   }

   return false;
}

/* Like find_menu_item_r, but searches by unique_id.
 */
static bool find_menu_item_r_unique(ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra)
{
   ALLEGRO_MENU_ITEM *info = (ALLEGRO_MENU_ITEM *) extra;

   if (item != NULL && info->unique_id == item->unique_id) {
      info->id = index;
      info->parent = menu;
      return true;
   }

   return false;
}

/* Carefully destroy a menu item... If the item is part of a menu, it must be
 * removed from it.
 */
static void destroy_menu_item(ALLEGRO_MENU_ITEM *item)
{
   ASSERT(item);

   if (!item->parent) {
      /* This normally won't happen. */
      _al_destroy_menu_item_at(item, -1);
   }
   else {
      size_t i;
      for (i = 0; i < _al_vector_size(&item->parent->items); ++i) {
         if (*(ALLEGRO_MENU_ITEM **)_al_vector_ref(&item->parent->items, i) == item) {
            /* Notify the platform that the item is to be removed. */
            _al_destroy_menu_item_at(item, i);

            /* Remove the command from the look-up vector. */
            if (item->id != 0) {
               _AL_MENU_ID *menu_id;
               size_t j;

               for (j = 0; j < _al_vector_size(&menu_ids); ++j) {
                  menu_id = (_AL_MENU_ID *) _al_vector_ref(&menu_ids, j);
                  if (menu_id->menu == item->parent && menu_id->unique_id == item->unique_id) {
                     _al_vector_delete_at(&menu_ids, j);
                     break;
                  }
               }
            }

            /* Remove the menu from the parent's list. */
            _al_vector_delete_at(&item->parent->items, i);

            break;
         }
      }
   }

   if (item->caption)
      al_ustr_free(item->caption);

   if (item->popup) {
      /* Delete the sub-menu. Must set the parent/display to NULL ahead of time to
       * avoid recursing back here.
       */
      item->popup->parent = NULL;
      item->popup->display = NULL;
      al_destroy_menu(item->popup);
   }

   if (item->icon) {
      al_destroy_bitmap(item->icon);
   }

   al_free(item);
}

/* An ALLEGRO_MENU_INFO structure represents a heirarchy of menus. This function
 * recursively steps through it and builds the entire menu.
 */
static ALLEGRO_MENU_INFO *parse_menu_info(ALLEGRO_MENU *parent, ALLEGRO_MENU_INFO *info)
{
   ASSERT(parent);
   ASSERT(info);

   /* The end of the menu is marked by a NULL caption and an id of 0. */
   while (info->caption || info->id) {
      if (!info->caption) {
         /* A separator */
         al_append_menu_item(parent, NULL, 0, 0, NULL, NULL);
         ++info;
      }
      else if (strlen(info->caption) > 2 &&
         !strncmp("->", info->caption + strlen(info->caption) - 2, 2)) {
         /* An item with a sub-menu has a -> marker as part of its caption.
          * (e.g., "File->").
          */
         ALLEGRO_MENU *menu = al_create_menu();
         if (menu) {
            /* Strip the -> mark off the end. */
            ALLEGRO_USTR *s = al_ustr_new(info->caption);
            al_ustr_remove_range(s, al_ustr_size(s) - 2, al_ustr_size(s));
            al_append_menu_item(parent, al_cstr(s), info->id, 0, NULL, menu);
            info = parse_menu_info(menu, info + 1);
            al_ustr_free(s);
         }
      }
      else {
         /* Just ar regular item */
         al_append_menu_item(parent, info->caption, info->id, info->flags, info->icon, NULL);
         ++info;
      }
   }

   return info + 1;
}

/* All public functions that take a menu and id parameter have two interpretations:
 *
 *   1) If id > 0, then it represents an id anywhere within the menu structure,
 *   including child menus. If there are non-unique IDs, the first one found is
 *   returned, but the exact order is undefined. (IDs are meant to be unique.)
 *
 *   2) If id <= 0, then its absolute value represents an ordered index for that
 *   exact menu.
 *
 * If the parameters are valid, it returns a pointer to the corresponding
 * MENU_ITEM, and the menu/id parameters are set to the item's parent and its
 * index. Otherwise (on invalid parameters), it returns NULL and the menu/id
 * parameters are left undefined.
 *
 * (Note that the private OS specific functions always take a direct index.)
 */
static ALLEGRO_MENU_ITEM *interpret_menu_id_param(ALLEGRO_MENU **menu, int *id)
{
   if (*id > 0) {
      if (!al_find_menu_item(*menu, *id, menu, id))
         return NULL;
   }
   else {
      *id = (0 - *id);

      if ((size_t) *id >= _al_vector_size(&((*menu)->items)))
         return NULL;
   }

   return *(ALLEGRO_MENU_ITEM **) _al_vector_ref(&((*menu)->items), (size_t) *id);
}

/* A helper function for al_clone_menu() and al_clone_menu_for_popup().
 * Note that only the root menu is created as a "popup" (if popup == TRUE).
 */
static ALLEGRO_MENU *clone_menu(ALLEGRO_MENU *menu, bool popup)
{
   ALLEGRO_MENU *clone = NULL;
   size_t i;

   if (menu) {
      clone = popup ? al_create_popup_menu() : al_create_menu();

      for (i = 0; i < _al_vector_size(&menu->items); ++i) {
         const ALLEGRO_MENU_ITEM *item = *(ALLEGRO_MENU_ITEM **)_al_vector_ref(&menu->items, i);
         ALLEGRO_BITMAP *icon = item->icon;

         if (icon)
            icon = al_clone_bitmap(icon);

         al_append_menu_item(clone, item->caption ? al_cstr(item->caption) : NULL,
            item->id, item->flags, icon, al_clone_menu(item->popup));
      }
   }

   return clone;
}

/* Function: al_create_menu
 */
ALLEGRO_MENU *al_create_menu(void)
{
   ALLEGRO_MENU *m = al_calloc(1, sizeof(*m));

   if (m) {
      _al_vector_init(&m->items, sizeof(ALLEGRO_MENU_ITEM*));

      /* Make sure the platform actually supports menus */
      if (!_al_init_menu(m)) {
         al_destroy_menu(m);
         m = NULL;
      }
   }

   return m;
}

/* Function: al_create_popup_menu
 */
ALLEGRO_MENU *al_create_popup_menu(void)
{
   ALLEGRO_MENU *m = al_calloc(1, sizeof(*m));

   if (m) {
      _al_vector_init(&m->items, sizeof(ALLEGRO_MENU_ITEM*));

      if (!_al_init_popup_menu(m)) {
         al_destroy_menu(m);
         m = NULL;
      }
      else {
         /* Popups are slightly different... They can be used multiple times
          * with al_popup_menu(), but never as a display menu.
          */
         m->is_popup_menu = true;
      }
   }

   return m;
}

/* Function: al_clone_menu
 */
ALLEGRO_MENU *al_clone_menu(ALLEGRO_MENU *menu)
{
   return clone_menu(menu, false);
}

/* Function: al_clone_menu_for_popup
 */
ALLEGRO_MENU *al_clone_menu_for_popup(ALLEGRO_MENU *menu)
{
   return clone_menu(menu, true);
}

/* Function: al_build_menu
 */
ALLEGRO_MENU *al_build_menu(ALLEGRO_MENU_INFO *info)
{
   ALLEGRO_MENU *root = al_create_menu();

   if (root)
      parse_menu_info(root, info);

   return root;
}

/* Function: al_append_menu_item
 */
int al_append_menu_item(ALLEGRO_MENU *parent, char const *title, uint16_t id,
   int flags, ALLEGRO_BITMAP *icon, ALLEGRO_MENU *submenu)
{
   ASSERT(parent);

   /* Same thing as inserting a menu item at position == -SIZE */
   return al_insert_menu_item(parent, 0 - (int) _al_vector_size(&parent->items),
      title, id, flags, icon, submenu);
}

/* Function: al_insert_menu_item
 */
int al_insert_menu_item(ALLEGRO_MENU *parent, int pos, char const *title,
   uint16_t id, int flags, ALLEGRO_BITMAP *icon, ALLEGRO_MENU *submenu)
{
   ALLEGRO_MENU_ITEM *item;
   ALLEGRO_MENU_ITEM **slot;
   _AL_MENU_ID *menu_id;
   size_t i;

   ASSERT(parent);

   /* If not found, then treat as an append. */
   if (!interpret_menu_id_param(&parent, &pos))
      pos = _al_vector_size(&parent->items);

   /* At this point pos == the _index_ of where to insert */

   /* The sub-menu must not already be in use. */
   if (submenu && (submenu->display || submenu->parent || submenu->is_popup_menu))
      return -1;

   item = create_menu_item(title, id, flags, submenu);
   if (!item)
      return -1;
   item->parent = parent;

   set_item_icon(item, icon);

   i = (size_t) pos;

   if (i >= _al_vector_size(&parent->items)) {
      /* Append */
      i = _al_vector_size(&parent->items);
      slot = _al_vector_alloc_back(&parent->items);
   }
   else {
      /* Insert */
      slot = _al_vector_alloc_mid(&parent->items, i);
   }

   if (!slot) {
      destroy_menu_item(item);
      return -1;
   }
   *slot = item;

   if (submenu) {
      submenu->parent = item;

      if (parent->display)
         _al_walk_over_menu(submenu, set_menu_display_r, parent->display);
   }

   _al_insert_menu_item_at(item, (int) i);

   if (id) {
      /* Append the menu's ID to the search vector */
      menu_id = (_AL_MENU_ID *) _al_vector_alloc_back(&menu_ids);
      menu_id->unique_id = item->unique_id;
      menu_id->id = id;
      menu_id->menu = parent;
   }

   return (int) i;
}

/* Function: al_remove_menu_item
 */
bool al_remove_menu_item(ALLEGRO_MENU *menu, int pos)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);
   if (!item)
      return false;

   destroy_menu_item(item);

   return true;
}

/* Function: al_find_menu
 */
ALLEGRO_MENU *al_find_menu(ALLEGRO_MENU *haystack, uint16_t id)
{
   int index;

   return !al_find_menu_item(haystack, id, &haystack, &index) ? NULL :
      (*(ALLEGRO_MENU_ITEM **)_al_vector_ref(&haystack->items, index))->popup;
}

/* Function: al_find_menu_item
 */
bool al_find_menu_item(ALLEGRO_MENU *haystack, uint16_t id, ALLEGRO_MENU **menu,
   int *index)
{
   ALLEGRO_MENU_ITEM item;

   ASSERT(haystack);

   /* Abuse the ALLEGRO_MENU_ITEM struct as a container for the _al_walk_over_menu callback.
    * If found, it will return true, and the "parent" field will be the menu that and
    * the "id" will be the index.
    */
   item.id = id;

   if (!_al_walk_over_menu(haystack, find_menu_item_r, &item))
      return false;

   if (menu)
      *menu = item.parent;

   if (index)
      *index = item.id;

   return true;
}

/* As al_find_menu_item, but searches by the unique id.
 */
bool _al_find_menu_item_unique(ALLEGRO_MENU *haystack, uint16_t unique_id, ALLEGRO_MENU **menu,
   int *index)
{
   ALLEGRO_MENU_ITEM item;

   ASSERT(haystack);

   item.unique_id = unique_id;

   if (!_al_walk_over_menu(haystack, find_menu_item_r_unique, &item))
      return false;

   if (menu)
      *menu = item.parent;

   if (index)
      *index = item.id;

   return true;
}

/* Function: al_get_menu_item_caption
 */
const char *al_get_menu_item_caption(ALLEGRO_MENU *menu, int pos)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);
   return item && item->caption ? al_cstr(item->caption) : NULL;
}


/* Function: al_set_menu_item_caption
 */
void al_set_menu_item_caption(ALLEGRO_MENU *menu, int pos, const char *caption)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);

   if (item && item->caption) {
      al_ustr_free(item->caption);
      item->caption = al_ustr_new(caption);
      _al_update_menu_item_at(item, pos);
   }
}

/* Function: al_get_menu_item_flags
 */
int al_get_menu_item_flags(ALLEGRO_MENU *menu, int pos)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);
   return item ? item->flags : -1;
}

/* Function: al_set_menu_item_flags
 */
void al_set_menu_item_flags(ALLEGRO_MENU *menu, int pos, int flags)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);

   if (item) {
      /* The CHECKBOX flag is read-only after the menu is created, and
       * the CHECKED flag can only be set if it is a CHECKBOX.
       */
      if (item->flags & ALLEGRO_MENU_ITEM_CHECKBOX)
         flags |= ALLEGRO_MENU_ITEM_CHECKBOX;
      else {
         flags &= ~ALLEGRO_MENU_ITEM_CHECKED;
         flags &= ~ALLEGRO_MENU_ITEM_CHECKBOX;
      }

      item->flags = flags;
      _al_update_menu_item_at(item, pos);
   }
}

/* Function: al_toggle_menu_item_flags
 */
int al_toggle_menu_item_flags(ALLEGRO_MENU *menu, int pos, int flags)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);

   if (!item)
      return -1;

   /* The CHECKBOX flag is read-only after the menu is created, and
    * the CHECKED flag can only be set if it is a CHECKBOX.
    */
   flags &= ~ALLEGRO_MENU_ITEM_CHECKBOX;
   if (!(item->flags & ALLEGRO_MENU_ITEM_CHECKBOX)) {
      flags &= ~ALLEGRO_MENU_ITEM_CHECKED;
   }

   item->flags ^= flags;
   _al_update_menu_item_at(item, pos);

   return item->flags & flags;
}

/* Function: al_get_menu_item_icon
 */
ALLEGRO_BITMAP *al_get_menu_item_icon(ALLEGRO_MENU *menu, int pos)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);
   return item ? item->icon : NULL;
}


/* Function: al_set_menu_item_icon
 */
void al_set_menu_item_icon(ALLEGRO_MENU *menu, int pos, ALLEGRO_BITMAP *icon)
{
   ALLEGRO_MENU_ITEM *item;

   ASSERT(menu);

   item = interpret_menu_id_param(&menu, &pos);

   if (item) {
      if (item->icon)
         al_destroy_bitmap(item->icon);

      set_item_icon(item, icon);
      _al_update_menu_item_at(item, pos);
   }
}

/* Function: al_destroy_menu
 */
void al_destroy_menu(ALLEGRO_MENU *menu)
{
   ALLEGRO_MENU_ITEM **slot;
   size_t i;
   ASSERT(menu);

   if (menu->parent) {
      /* If the menu is attached to a menu item, then this is equivelant to
         removing that menu item. */
      ALLEGRO_MENU *parent = menu->parent->parent;
      ASSERT(parent);

      for (i = 0; i < _al_vector_size(&parent->items); ++i) {
         slot = _al_vector_ref(&parent->items, i);
         if (*slot == menu->parent) {
            al_remove_menu_item(parent, 0 - (int) i);
            return;
         }
      }

      /* Should never get here. */
      ASSERT(false);
      return;
   }
   else if (menu->display && !menu->is_popup_menu) {
      /* This is an active, top-level menu. */
      al_remove_display_menu(menu->display);
   }

   /* Destroy each item associated with the menu. */
   while (_al_vector_size(&menu->items)) {
      slot = _al_vector_ref_back(&menu->items);
      destroy_menu_item(*slot);
   }

   _al_vector_free(&menu->items);

   al_disable_menu_event_source(menu);
   al_free(menu);
}

/* Function: al_get_default_menu_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_default_menu_event_source(void)
{
   return &default_menu_es;
}

/* Function: al_enable_menu_event_source
 */
ALLEGRO_EVENT_SOURCE *al_enable_menu_event_source(ALLEGRO_MENU *menu)
{
   ASSERT(menu);

   if (!menu->is_event_source) {
      al_init_user_event_source(&menu->es);
      menu->is_event_source = true;
   }

   return &menu->es;
}

/* Function: al_disable_menu_event_source
 */
void al_disable_menu_event_source(ALLEGRO_MENU *menu)
{
   ASSERT(menu);

   if (menu->is_event_source) {
      al_destroy_user_event_source(&menu->es);
      menu->is_event_source = false;
   }
}

/* Function: al_get_display_menu
 */
ALLEGRO_MENU *al_get_display_menu(ALLEGRO_DISPLAY *display)
{
   size_t i;
   ASSERT(display);

   /* Search through the display_menus vector to see if this display has
    * a menu associated with it. */
   for (i = 0; i < _al_vector_size(&display_menus); ++i) {
      DISPLAY_MENU *dm = (DISPLAY_MENU *) _al_vector_ref(&display_menus, i);
      if (dm->display == display)
         return dm->menu;
   }

   return NULL;
}

/* Function: al_set_display_menu
 */
bool al_set_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   DISPLAY_MENU *dm = NULL;
   size_t i;
   int menu_height = _al_get_menu_display_height();
   bool automatic_menu_display_resize = true;
   const char* automatic_menu_display_resize_value =
      al_get_config_value(al_get_system_config(), "compatibility", "automatic_menu_display_resize");
   if (automatic_menu_display_resize_value && strcmp(automatic_menu_display_resize_value, "false") == 0)
      automatic_menu_display_resize = false;

   ASSERT(display);

   /* Check if this display has a menu associated with it */
   for (i = 0; i < _al_vector_size(&display_menus); ++i) {
      dm = (DISPLAY_MENU *) _al_vector_ref(&display_menus, i);
      if (dm->display == display)
         break;
   }

   /* If no display was found, reset dm to NULL */
   if (i == _al_vector_size(&display_menus))
      dm = NULL;

   if (!menu) {
      /* Removing the menu */

      if (!dm)
         return false;

      _al_hide_display_menu(display, dm->menu);
      _al_walk_over_menu(dm->menu, set_menu_display_r, NULL);
      _al_vector_delete_at(&display_menus, i);

      if (automatic_menu_display_resize && menu_height > 0) {
         al_resize_display(display, al_get_display_width(display), al_get_display_height(display));
      }
   }
   else {
      /* Setting the menu. It must not currently be attached to any
       * display, and it cannot have a parent menu. */
      if (menu->display || menu->parent)
         return false;

      if (dm) {
         /* hide the existing menu */
         _al_hide_display_menu(display, dm->menu);
         _al_walk_over_menu(dm->menu, set_menu_display_r, NULL);
      }

      if (!_al_show_display_menu(display, menu)) {
         /* Unable to set the new menu, but already have hidden the
          * previous one, so delete the display_menus slot. */
         if (dm)
            _al_vector_delete_at(&display_menus, i);
         return false;
      }

      /* Set the entire menu tree as owned by the display */
      _al_walk_over_menu(menu, set_menu_display_r, display);

      if (!dm)
         dm = _al_vector_alloc_back(&display_menus);

      if (automatic_menu_display_resize && menu_height > 0) {
         /* Temporarily disable the constraints so we don't send a RESIZE_EVENT. */
         bool old_constraints = display->use_constraints;
         display->use_constraints = false;
         al_resize_display(display, al_get_display_width(display), al_get_display_height(display));
         display->use_constraints = old_constraints;
      }

      dm->display = display;
      dm->menu = menu;
   }

   return true;
}

/* Function: al_popup_menu
 */
bool al_popup_menu(ALLEGRO_MENU *popup, ALLEGRO_DISPLAY *display)
{
   bool ret;
   ASSERT(popup);

   if (!popup->is_popup_menu || popup->parent)
      return false;

   if (!display)
      display = al_get_current_display();

   /* Set the entire menu tree as owned by the display */
   _al_walk_over_menu(popup, set_menu_display_r, display);

   ret = _al_show_popup_menu(display, popup);

   if (!ret) {
      _al_walk_over_menu(popup, set_menu_display_r, NULL);
   }
   return ret;
}

/* Function: al_remove_display_menu
 */
ALLEGRO_MENU *al_remove_display_menu(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_MENU *menu;

   ASSERT(display);

   menu = al_get_display_menu(display);

   if (menu)
      al_set_display_menu(display, NULL);

   return menu;
}

/* Tries to find the menu that has a child with the given id. If display
 * is not NULL, then it must also match. The first match is returned.
 */
_AL_MENU_ID *_al_find_parent_menu_by_id(ALLEGRO_DISPLAY *display, uint16_t unique_id)
{
   _AL_MENU_ID *menu_id;
   size_t i;

   for (i = 0; i < _al_vector_size(&menu_ids); ++i) {
      menu_id = (_AL_MENU_ID *) _al_vector_ref(&menu_ids, i);
      if (menu_id->unique_id == unique_id) {
         if (!display || menu_id->menu->display == display) {
            return menu_id;
         }
      }
   }

   return NULL;
}

/* Each platform implementation must call this when a menu has been clicked.
 * The display parameter should be sent if at all possible! If it isn't sent,
 * and the user is using non-unique ids, it won't know which display actually
 * triggered the menu click.
 */
bool _al_emit_menu_event(ALLEGRO_DISPLAY *display, uint16_t unique_id)
{
   ALLEGRO_EVENT event;
   _AL_MENU_ID *menu_id = NULL;
   ALLEGRO_EVENT_SOURCE *source = al_get_default_menu_event_source();

   /* try to find the menu that triggered the event */
   menu_id = _al_find_parent_menu_by_id(display, unique_id);

   if (!menu_id)
      return false;

   if (menu_id->id == 0)
      return false;

   if (menu_id) {
      /* A menu was found associated with the id. See if it has an
       * event source associated with it, and adjust "source" accordingly. */
      ALLEGRO_MENU *m = menu_id->menu;
      while (true) {
         if (m->is_event_source) {
            source = &m->es;
            break;
         }

         if (!m->parent)
            break;

         /* m->parent is of type MENU_ITEM,
          *   which always has a parent of type MENU */
         ASSERT(m->parent->parent);
         m = m->parent->parent;
      }
   }

   event.user.type = ALLEGRO_EVENT_MENU_CLICK;
   event.user.data1 = menu_id->id;
   event.user.data2 = (intptr_t) display;
   event.user.data3 = (intptr_t) menu_id->menu;

   al_emit_user_event(source, &event, NULL);

   return true;
}


/* vim: set sts=3 sw=3 et: */
