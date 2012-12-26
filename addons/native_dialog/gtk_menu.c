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
#include "gtk_xgtk.h"


/* The API is assumed to be synchronous, but the user calls will not be
 * on the GTK thread. The following structure is used to pass data from the 
 * user thread to the GTK thread, and then wait until the GTK has processed it.
 */
typedef struct ARGS ARGS;

struct ARGS
{
   ALLEGRO_MUTEX *mutex;
   ALLEGRO_COND *cond;
   bool done;
   bool response;
   
   GtkWidget *gtk_window;
   ALLEGRO_MENU *menu;
   ALLEGRO_MENU_ITEM *item;
   int i;
};


#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
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

/* [gtk thread] */
static void *lock_args(gpointer data)
{
   ARGS *args = (ARGS *) data;
   al_lock_mutex(args->mutex);
   return args;
}

/* [gtk thread] */
static gboolean release_args(gpointer data)
{
   ARGS *args = (ARGS *) data;
   
   args->done = true;
   al_signal_cond(args->cond);
   al_unlock_mutex(args->mutex);
   
   return FALSE;
}

/* [user thread] */
static void *create_args(void)
{
   ARGS *args = al_malloc(sizeof(*args));
   if (args) {
      args->mutex = al_create_mutex();
      args->cond = al_create_cond();
      args->done = false;
      args->response = true;
   }
   return args;
}

/* [user thread] */
static ARGS *make_menu_item_args(ALLEGRO_MENU_ITEM *item, int i)
{
   ARGS *args = create_args();
   if (args) {
      args->item = item;
      args->i = i;
   }
   return args;
}

/* [user thread] */
static bool wait_for_args(GSourceFunc func, void *data)
{
   ARGS *args = (ARGS *) data;
   bool response;
   
   al_lock_mutex(args->mutex);
   g_timeout_add(0, func, data);
   while (args->done == false) {
      al_wait_cond(args->cond, args->mutex);
   }
   al_unlock_mutex(args->mutex);
   
   response = args->response;
   
   al_destroy_mutex(args->mutex);
   al_destroy_cond(args->cond);
   
   al_free(args);
   
   return response;
}

/* [gtk thread] */
static void menuitem_response(ALLEGRO_MENU_ITEM *menu_item)
{
   if (menu_item->parent)
      _al_emit_menu_event(menu_item->parent->display, menu_item->id);
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
static void destroy_pixbuf(guchar *pixels, gpointer data)
{
   (void) data;
   
   al_free(pixels);
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
         gitem = gtk_image_menu_item_new_with_mnemonic(al_cstr(caption));
         
         if (aitem->icon) {
            const int w = al_get_bitmap_width(aitem->icon), h = al_get_bitmap_height(aitem->icon);
            const int stride = w * 4;
            int x, y, i;
            GdkPixbuf *pixbuf;
            uint8_t *data = al_malloc(stride * h);
            
            if (data) {
               for (y = 0, i = 0; y < h; ++y) {
                  for (x = 0; x < w; ++x, i += 4) {
                     al_unmap_rgba(al_get_pixel(aitem->icon, x, y),
                        &data[i],
                        &data[i + 1],
                        &data[i + 2],
                        &data[i + 3]
                     );
                  }
               }
               
               pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB, TRUE, 8,
                  w, h, stride, destroy_pixbuf, NULL);
               
               aitem->extra2 = gtk_image_new_from_pixbuf(pixbuf);
               
               gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gitem), aitem->extra2);
               
               /* Subtract the main reference. the image still holds a reference, so the
                * pixbuf won't be destroyed until the image itself is. */
               g_object_unref(pixbuf);
            }
         }
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
#endif

/* [user thread] */
bool _al_init_menu(ALLEGRO_MENU *menu)
{
#ifndef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   (void)menu;
   return false;
#else
   (void) menu;
   
   /* Do nothing here, because menu creation is defered until it is displayed. */
   
   return true;
#endif
}

/* [user thread] */
bool _al_init_popup_menu(ALLEGRO_MENU *menu)
{
#ifndef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   (void)menu;
   return false;
#else
   return _al_init_menu(menu);
#endif
}

#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
/* [gtk thread] */
static gboolean do_destroy_menu(gpointer data)
{
   ARGS *args = lock_args(data);
   
   gtk_widget_destroy(args->menu->extra1);
   args->menu->extra1 = NULL;
   
   return release_args(args);
}
#endif

/* [user thread] */
bool _al_destroy_menu(ALLEGRO_MENU *menu)
{
#ifndef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   (void)menu;
   return false;
#else
   ARGS *args;
   
   if (!menu->extra1)
      return true;
   
   args = create_args();
   args->menu = menu;
   
   wait_for_args(do_destroy_menu, args);
   _al_walk_over_menu(menu, clear_menu_extras, NULL);
   
   return true;
#endif
}

#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
/* [gtk thread] */
static gboolean do_insert_menu_item_at(gpointer data)
{
   ARGS *args = lock_args(data);
   if (!args->item->extra1) {
      args->item->extra1 = build_menu_item(args->item);
   }
   gtk_menu_shell_insert(GTK_MENU_SHELL(args->item->parent->extra1), args->item->extra1, args->i);
   
   return release_args(data);
}
#endif

/* [user thread] */
bool _al_insert_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
#ifndef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   (void)item;
   (void)i;
   return false;
#else
   if (item->parent->extra1) {
      ARGS *args = create_args();
      if (!args)
         return false;
       
       args->item = item;
       args->i = i;
       
       wait_for_args(do_insert_menu_item_at, args);
   }
   
   return true;
#endif
}

#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
/* [gtk thread] */
static gboolean do_destroy_menu_item_at(gpointer data)
{
   ARGS *args = lock_args(data);
   
   gtk_widget_destroy(GTK_WIDGET(args->item->extra1));
   args->item->extra1 = NULL;
   
   return release_args(args);
}
#endif

/* [user thread] */
bool _al_destroy_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
#ifndef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   (void)item;
   (void)i;
   return false;
#else
   if (item->extra1) {
      ARGS *args = make_menu_item_args(item, i);
      if (!args)
         return false;
         
      wait_for_args(do_destroy_menu_item_at, args);
      
      if (item->popup) {
         /* if this has a submenu, then deleting this item will have automatically
            deleted all of its GTK widgets */
         _al_walk_over_menu(item->popup, clear_menu_extras, NULL);
      }
   }
   return true;
#endif
}

#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
/* [gtk thread] */
static gboolean do_update_menu_item_at(gpointer data)
{
   ARGS *args = lock_args(data);
   GtkWidget *gitem;
   
   gtk_widget_destroy(args->item->extra1);
   args->item->extra1 = NULL;
   
   gitem = build_menu_item(args->item);
   gtk_menu_shell_insert(GTK_MENU_SHELL(args->item->parent->extra1), gitem, args->i);
   
   return release_args(args);
}
#endif

/* [user thread] */
bool _al_update_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
#ifndef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   (void)item;
   (void)i;
   return false;
#else
   if (item->extra1) {
      ARGS *args = make_menu_item_args(item, i);
      if (!args)
         return false;
         
      g_timeout_add(0, do_update_menu_item_at, args);
   }
   return true;
#endif
}

/* [gtk thread] */
#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
static gboolean do_show_display_menu(gpointer data)
{
   ARGS *args = lock_args(data);
   
   if (!args->menu->extra1) {
      GtkWidget *menu_bar = gtk_menu_bar_new();
      
      build_menu(menu_bar, args->menu);

      GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(args->gtk_window));
      gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
      gtk_box_reorder_child(GTK_BOX(vbox), menu_bar, 0);
      gtk_widget_show(menu_bar);

      args->menu->extra1 = menu_bar;
   }
   
   gtk_widget_show(gtk_widget_get_parent(args->menu->extra1));
   
   return release_args(args);
}
#endif

/* [user thread] */
bool _al_show_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   GtkWidget *gtk_window;
   ARGS *args;

   if (!_al_gtk_ensure_thread()) {
      return false;
   }

   gtk_window = _al_gtk_get_window(display);
   if (!gtk_window) {
      return false;
   }

   args = create_args();
   if (!args) {
      return false;
   }
   
   args->gtk_window = gtk_window;
   args->menu = menu;
   
   return wait_for_args(do_show_display_menu, args);
#else
   (void) display;
   (void) menu;
   return false;
#endif
}

/* [gtk thread] */
#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
static gboolean do_hide_display_menu(gpointer data)
{
   ARGS *args = lock_args(data);
   
   gtk_widget_destroy(GTK_WIDGET(args->menu->extra1));
   args->menu->extra1 = NULL;
   
   return release_args(data);
}
#endif

/* [user thread] */
bool _al_hide_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   GtkWidget *gtk_window;
   ARGS *args;

   if (!(gtk_window = _al_gtk_get_window(display)))
      return false;
   
   if (!(args = create_args()))
      return false;
   
   args->gtk_window = gtk_window;
   args->menu = menu;      
   wait_for_args(do_hide_display_menu, args);   
   _al_walk_over_menu(menu, clear_menu_extras, NULL);
   
   return true;
#else
   (void) display;
   (void) menu;
   
   return false;
#endif
}

#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
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
   
   lock_args(args);
   
   if (!args->menu->extra1) {
      GtkWidget *menu = gtk_menu_new();
      
      build_menu(menu, args->menu);
      
      gtk_widget_show(menu);
      args->menu->extra1 = menu;
      
      g_signal_connect_swapped (menu, "hide",
         G_CALLBACK(popop_on_hide), (gpointer) args->menu);
   }
   gtk_menu_popup(args->menu->extra1, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
   
   release_args(args);
   
   return FALSE;
}
#endif

bool _al_show_popup_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
#ifndef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT
   (void)display;
   (void)menu;
   return false;
#else
   ARGS *args;
   (void)display;
   
   if (!_al_gtk_ensure_thread()) {
      return false;
   }

   args = create_args();
   if (!args) {
      return false;
   }
   
   args->gtk_window = NULL;
   args->menu = menu;
   
   return wait_for_args(do_show_popup_menu, args);
#endif
}

/* vim: set sts=3 sw=3 et: */
