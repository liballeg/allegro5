#ifndef __al_included_allegro5_native_dialog_gtk_xgtk_h
#define __al_included_allegro5_native_dialog_gtk_xgtk_h

#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTKGLEXT

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkx.h>

struct ALLEGRO_DISPLAY_XGLX_GTKGLEXT {
   GtkWidget *gtkwindow;
   GtkWidget *gtkdrawing_area;
   GdkGLContext *gtkcontext;
   GdkGLDrawable *gtkdrawable;
   int cfg_w, cfg_h;
   int toggle_w, toggle_h;
   bool ignore_configure_event;
   bool is_fullscreen;
};

bool _al_gtk_set_display_overridable_interface(bool on);
GtkWidget *_al_gtk_get_window(ALLEGRO_DISPLAY *display);

#endif

#endif

/* vim: set sts=3 sw=3 et: */
