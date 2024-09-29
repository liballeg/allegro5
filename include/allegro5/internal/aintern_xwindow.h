#ifndef __al_included_allegro5_aintern_xwindow_h
#define __al_included_allegro5_aintern_xwindow_h

void _al_xwin_set_size_hints(A5O_DISPLAY *d, int x_off, int y_off);
void _al_xwin_reset_size_hints(A5O_DISPLAY *d);
void _al_xwin_set_fullscreen_window(A5O_DISPLAY *display, int value);
void _al_xwin_set_above(A5O_DISPLAY *display, int value);
void _al_xwin_set_frame(A5O_DISPLAY *display, bool frame_on);
void _al_xwin_set_icons(A5O_DISPLAY *d,
   int num_icons, A5O_BITMAP *bitmaps[]);
void _al_xwin_maximize(A5O_DISPLAY *d, bool maximized);
void _al_xwin_check_maximized(A5O_DISPLAY *display);
void _al_xwin_get_borders(A5O_DISPLAY *display);

#endif

/* vim: set sts=3 sw=3 et: */
