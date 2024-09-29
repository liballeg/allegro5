#ifndef __al_included_allegro5_aintern_xclipboard_h
#define __al_included_allegro5_aintern_xclipboard_h


#include "allegro5/internal/aintern_display.h"

void _al_xwin_display_selection_notify(A5O_DISPLAY  *display, XSelectionEvent *xselection);
void _al_xwin_display_selection_request(A5O_DISPLAY *display, XSelectionRequestEvent *xselectionrequest);
void _al_xwin_add_clipboard_functions(A5O_DISPLAY_INTERFACE *vt);

#endif

/* vim: set sts=3 sw=3 et: */
