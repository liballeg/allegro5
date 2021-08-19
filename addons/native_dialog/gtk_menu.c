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
 *      GTK native dialog implementation.
 *
 *      See LICENSE.txt for copyright information.
 */

#include <gtk/gtk.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog_cfg.h"
#include "gtk_dialog.h"
#include "gtk_xgtk.h"

ALLEGRO_DEBUG_CHANNEL("menu")

typedef struct ARGS ARGS;

struct ARGS
{
   /* Must be first. */
   ARGS_BASE base;

   GtkWidget *gtk_window;
   ALLEGRO_MENU *menu;
   ALLEGRO_MENU_ITEM *item;
   int i;
};


static void build_menu(GtkWidget *gmenu, ALLEGRO_MENU *amenu);

/* [user thread] */
static bool clear_menu_extras(ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra)
{
   (void) index;
   (void) extra;
   
   if (item) 
      item->extra1 = NULL;
   else
      menu->extra1 = NULL;
      
   return false;
}

/* [user thread] */
static bool make_menu_item_args(ARGS *args, ALLEGRO_MENU_ITEM *item, int i)
{
   if (_al_gtk_init_args(args, sizeof(*args))) {
      args->item = item;
      args->i = i;
      return true;
   }
   return false;
}

/* [gtk thread] */
static void menuitem_response(ALLEGRO_MENU_ITEM *menu_item)
{
   if (menu_item->parent)
      _al_emit_menu_event(menu_item->parent->display, menu_item->unique_id);
}

/* [gtk thread] */
static void checkbox_on_toggle(ALLEGRO_MENU_ITEM *item)
{
   /* make sure the menu item remains the same state */
   if (gtk_check_menu_item_get_active(item->extra1)) {
      item->flags |= ALLEGRO_MENU_ITEM_CHECKED;
   }
   else {
      item->flags &= ~ALLEGRO_MENU_ITEM_CHECKED;
   }
}

/* [gtk thread] */
static GtkWidget *build_menu_item(ALLEGRO_MENU_ITEM *aitem)
{
   GtkWidget *gitem;
   
   if (!aitem->caption) {
      gitem = gtk_separator_menu_item_new();
   }
   else {
      ALLEGRO_USTR *caption = al_ustr_dup(aitem->caption);
      
      /* convert & to _ using unprintable chars as placeholders */
      al_ustr_find_replace_cstr(caption, 0, "_", "\x01\x02");
      al_ustr_find_replace_cstr(caption, 0, "&", "_");
      al_ustr_find_replace_cstr(caption, 0, "\x01\x02", "__");
      
      if (aitem->flags & ALLEGRO_MENU_ITEM_CHECKBOX) {
         gitem = gtk_check_menu_item_new_with_mnemonic(al_cstr(caption));
         gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gitem), aitem->flags & ALLEGRO_MENU_ITEM_CHECKED);
         g_signal_connect_swapped (gitem, "toggled", G_CALLBACK(checkbox_on_toggle),
            (gpointer) aitem);
      }
      else {
         /* always create an image menu item, in case the user ever sets an icon */
         gitem = gtk_menu_item_new_with_mnemonic(al_cstr(caption));

      }
      
      al_ustr_free(caption);
      
      gtk_widget_set_sensitive(gitem, !(aitem->flags & ALLEGRO_MENU_ITEM_DISABLED));
      
      aitem->extra1 = gitem;
      
      if (aitem->popup) {
         GtkWidget *gsubmenu = gtk_menu_new();
         build_menu(gsubmenu, aitem->popup);
         aitem->popup->extra1 = gsubmenu;
         gtk_widget_show(gsubmenu);
         gtk_menu_item_set_submenu(GTK_MENU_ITEM(gitem), gsubmenu);
      }
      else if (aitem->id) {
         g_signal_connect_swapped (gitem, "activate",
            G_CALLBACK(menuitem_response), (gpointer) aitem);
      }
   }
      
   gtk_widget_show(gitem);
   
   return gitem;
}

/* [gtk thread] */
static void build_menu(GtkWidget *gmenu, ALLEGRO_MENU *amenu)
{
   size_t i;
   
   for (i = 0; i < _al_vector_size(&amenu->items); ++i) {
      ALLEGRO_MENU_ITEM *aitem = * (ALLEGRO_MENU_ITEM **) _al_vector_ref(&amenu->items, i);
      GtkWidget *gitem = build_menu_item(aitem);
      gtk_menu_shell_append(GTK_MENU_SHELL(gmenu), gitem);
   }
}

/* [user thread] */
bool _al_init_menu(ALLEGRO_MENU *menu)
{
   (void) menu;
   
   /* Do nothing here, because menu creation is defered until it is displayed. */
   
   return true;
}

/* [user thread] */
bool _al_init_popup_menu(ALLEGRO_MENU *menu)
{
   return _al_init_menu(menu);
}

/* [gtk thread] */
static gboolean do_destroy_menu(gpointer data)
{
   ARGS *args = _al_gtk_lock_args(data);
   
   gtk_widget_destroy(args->menu->extra1);
   args->menu->extra1 = NULL;
   
   return _al_gtk_release_args(args);
}

/* [user thread] */
bool _al_destroy_menu(ALLEGRO_MENU *menu)
{
   ARGS args;

   if (!menu->extra1)
      return true;
   
   if (!_al_gtk_init_args(&args, sizeof(args)))
      return false;

   args.menu = menu;
   _al_gtk_wait_for_args(do_destroy_menu, &args);

   _al_walk_over_menu(menu, clear_menu_extras, NULL);
   
   return true;
}

/* [gtk thread] */
static gboolean do_insert_menu_item_at(gpointer data)
{
   ARGS *args = _al_gtk_lock_args(data);
   if (!args->item->extra1) {
      args->item->extra1 = build_menu_item(args->item);
   }
   gtk_menu_shell_insert(GTK_MENU_SHELL(args->item->parent->extra1), args->item->extra1, args->i);
   
   return _al_gtk_release_args(data);
}

/* [user thread] */
bool _al_insert_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   if (item->parent->extra1) {
      ARGS args;

      if (!_al_gtk_init_args(&args, sizeof(args)))
         return false;
       
       args.item = item;
       args.i = i;
       
       _al_gtk_wait_for_args(do_insert_menu_item_at, &args);
   }
   
   return true;
}

/* [gtk thread] */
static gboolean do_destroy_menu_item_at(gpointer data)
{
   ARGS *args = _al_gtk_lock_args(data);
   
   gtk_widget_destroy(GTK_WIDGET(args->item->extra1));
   args->item->extra1 = NULL;
   
   return _al_gtk_release_args(args);
}

/* [user thread] */
bool _al_destroy_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   if (item->extra1) {
      ARGS args;

      if (!make_menu_item_args(&args, item, i))
         return false;
         
      _al_gtk_wait_for_args(do_destroy_menu_item_at, &args);
      
      if (item->popup) {
         /* if this has a submenu, then deleting this item will have automatically
            deleted all of its GTK widgets */
         _al_walk_over_menu(item->popup, clear_menu_extras, NULL);
      }
   }
   return true;
}

/* [gtk thread] */
static gboolean do_update_menu_item_at(gpointer data)
{
   ARGS *args = _al_gtk_lock_args(data);
   GtkWidget *gitem;
   
   gtk_widget_destroy(args->item->extra1);
   args->item->extra1 = NULL;
   
   gitem = build_menu_item(args->item);
   gtk_menu_shell_insert(GTK_MENU_SHELL(args->item->parent->extra1), gitem, args->i);
   
   return _al_gtk_release_args(args);
}

/* [user thread] */
bool _al_update_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   if (item->extra1) {
      ARGS args;

      if (!make_menu_item_args(&args, item, i))
         return false;

      _al_gtk_wait_for_args(do_update_menu_item_at, &args);
   }
   return true;
}

/* [gtk thread] */
static gboolean do_show_display_menu(gpointer data)
{
   ARGS *args = _al_gtk_lock_args(data);
   
   if (!args->menu->extra1) {
      GtkWidget *menu_bar = gtk_menu_bar_new();
      
      build_menu(menu_bar, args->menu);

      GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(args->gtk_window));
      gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
      gtk_widget_show(menu_bar);

      args->menu->extra1 = menu_bar;
   }
   
   gtk_widget_show(gtk_widget_get_parent(args->menu->extra1));
   
   return _al_gtk_release_args(args);
}

/* [user thread] */
bool _al_show_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   GtkWidget *gtk_window;
   ARGS args;

   gtk_window = _al_gtk_get_window(display);
   if (!gtk_window) {
      return false;
   }

   if (!_al_gtk_init_args(&args, sizeof(args))) {
      return false;
   }
   
   args.gtk_window = gtk_window;
   args.menu = menu;
   
   return _al_gtk_wait_for_args(do_show_display_menu, &args);
}

/* [gtk thread] */
static gboolean do_hide_display_menu(gpointer data)
{
   ARGS *args = _al_gtk_lock_args(data);
   
   gtk_widget_destroy(GTK_WIDGET(args->menu->extra1));
   args->menu->extra1 = NULL;
   
   return _al_gtk_release_args(data);
}

/* [user thread] */
bool _al_hide_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   GtkWidget *gtk_window;
   ARGS args;

   if (!(gtk_window = _al_gtk_get_window(display)))
      return false;
   
   if (!_al_gtk_init_args(&args, sizeof(args)))
      return false;
   
   args.gtk_window = gtk_window;
   args.menu = menu;
   _al_gtk_wait_for_args(do_hide_display_menu, &args);
   _al_walk_over_menu(menu, clear_menu_extras, NULL);
   
   return true;
}

/* [gtk thread] */
static void popop_on_hide(ALLEGRO_MENU *menu)
{
   (void) menu;
   /* in case we want to notify on popup close */
}

/* [gtk thread] */
static gboolean do_show_popup_menu(gpointer data)
{
   ARGS *args = (ARGS *) data;
   
   _al_gtk_lock_args(args);
   
   GtkWidget *menu = NULL;
   if (!args->menu->extra1) {
      menu = gtk_menu_new();
      
      build_menu(menu, args->menu);
      
      gtk_widget_show(menu);
      args->menu->extra1 = menu;
      
      g_signal_connect_swapped (menu, "hide",
         G_CALLBACK(popop_on_hide), (gpointer) args->menu);
   }

   bool position_called = false;
   if (menu)
      /* gtk_menu_popup_at_widget only exists in gtk newer than 3.22 */
#if GTK_CHECK_VERSION(3, 22, 0)
      gtk_menu_popup_at_widget(args->menu->extra1, menu,  GDK_GRAVITY_SOUTH_WEST,
                              GDK_GRAVITY_NORTH_WEST,
                              NULL);
#else
      gtk_menu_popup(args->menu->extra1, menu, NULL, NULL, NULL, 1, 0);
#endif

   if (!position_called) {
      ALLEGRO_DEBUG("Position canary not called, most likely the menu didn't show "
      "up due to outstanding mouse events.\n");
   }
   args->base.response = position_called;
   
   _al_gtk_release_args(args);
   
   return FALSE;
}

bool _al_show_popup_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   ARGS args;
   (void)display;
   
   if (!_al_gtk_init_args(&args, sizeof(args))) {
      return false;
   }

   args.gtk_window = NULL;
   args.menu = menu;

   return _al_gtk_wait_for_args(do_show_popup_menu, &args);
}

int _al_get_menu_display_height(void)
{
   return 0;
}

/* vim: set sts=3 sw=3 et: */
