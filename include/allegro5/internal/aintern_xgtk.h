#ifndef __al_included_allegro5_aintern_xgtk_h
#define __al_included_allegro5_aintern_xgtk_h

#ifdef ALLEGRO_CFG_USE_GTK
bool _al_gtk_ensure_thread(void);
#endif

#ifdef ALLEGRO_CFG_USE_GTKGLEXT

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

ALLEGRO_DISPLAY *_al_gtk_create_display(int w, int h);
void _al_gtk_destroy_display_hook(ALLEGRO_DISPLAY *d);

bool _al_gtk_make_current(ALLEGRO_DISPLAY *d);
void _al_gtk_unmake_current(ALLEGRO_DISPLAY *d);
void _al_gtk_flip_display(ALLEGRO_DISPLAY *d);
bool _al_gtk_acknowledge_resize(ALLEGRO_DISPLAY *d);
bool _al_gtk_resize_display(ALLEGRO_DISPLAY *d, int w, int h);
void _al_gtk_set_window_title(ALLEGRO_DISPLAY *d, const char *title);
void _al_gtk_set_fullscreen_window(ALLEGRO_DISPLAY *display, bool onoff);
void _al_gtk_set_window_position(ALLEGRO_DISPLAY *display, int x, int y);
bool _al_gtk_set_window_constraints(ALLEGRO_DISPLAY *display,
   int min_w, int min_h, int max_w, int max_h);
void _al_gtk_set_size_hints(ALLEGRO_DISPLAY *display, int x_off, int y_off);
void _al_gtk_reset_size_hints(ALLEGRO_DISPLAY *d);

GtkWidget *_al_gtk_get_window(ALLEGRO_DISPLAY *display);

#endif

#endif

/* vim: set sts=3 sw=3 et: */
