#ifndef __al_included_allegro5_native_dialog_gtk_xgtk_h
#define __al_included_allegro5_native_dialog_gtk_xgtk_h

#include <gtk/gtk.h>

struct A5O_DISPLAY_XGLX_GTK {
   GtkWidget *gtkwindow;
   GtkWidget *gtksocket;
};

bool _al_gtk_set_display_overridable_interface(bool on);
GtkWidget *_al_gtk_get_window(A5O_DISPLAY *display);

#endif

/* vim: set sts=3 sw=3 et: */
