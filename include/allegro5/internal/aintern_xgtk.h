#ifndef __al_included_allegro5_aintern_xgtk_h
#define __al_included_allegro5_aintern_xgtk_h

#ifdef ALLEGRO_CFG_USE_GTK
bool _al_gtk_ensure_thread(void);
#endif

#ifdef ALLEGRO_CFG_USE_GTKGLEXT

ALLEGRO_DISPLAY *_al_gtk_create_display(int w, int h);

void _al_gtk_set_fullscreen_window(ALLEGRO_DISPLAY *display, bool onoff);
bool _al_gtk_acknowledge_resize(ALLEGRO_DISPLAY *d);

GtkWidget *_al_gtk_get_window(ALLEGRO_DISPLAY *display);

#endif

#endif

/* vim: set sts=3 sw=3 et: */
