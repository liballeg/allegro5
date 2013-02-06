#include <gtk/gtk.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog_cfg.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "gtk_dialog.h"

ALLEGRO_DEBUG_CHANNEL("gtk_dialog")


bool _al_init_native_dialog_addon(void)
{
   int argc = 0;
   char **argv = NULL;
   if (!gtk_init_check(&argc, &argv)) {
      ALLEGRO_ERROR("gtk_init_check failed\n");
      return false;
   }

   return true;
}


void _al_shutdown_native_dialog_addon(void)
{
}


static void really_make_transient(GtkWidget *window, ALLEGRO_DISPLAY_XGLX *glx)
{
   GdkDisplay *gdk = gdk_drawable_get_display(GDK_DRAWABLE(window->window));
   GdkWindow *parent = gdk_window_lookup_for_display(gdk, glx->window);
   if (!parent)
      parent = gdk_window_foreign_new_for_display(gdk, glx->window);
   gdk_window_set_transient_for(window->window, parent);
}


static void realized(GtkWidget *window, gpointer data)
{
   really_make_transient(window, (void *)data);
}


void _al_gtk_make_transient(ALLEGRO_DISPLAY *display, GtkWidget *window)
{
   /* Set the current display window (if any) as the parent of the dialog. */
   ALLEGRO_DISPLAY_XGLX *glx = (void *)display;
   if (glx) {
      if (!GTK_WIDGET_REALIZED(window))
         g_signal_connect(window, "realize", G_CALLBACK(realized), (void *)glx);
      else
         really_make_transient(window, glx);
   }
}

/* vim: set sts=3 sw=3 et: */
