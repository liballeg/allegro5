#ifndef __al_included_allegro5_aintern_wlclipboard_h
#define __al_included_allegro5_aintern_wlclipboard_h

#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_wlsystem.h"

void _al_wl_clipboard_seat_changed(ALLEGRO_SYSTEM_WAYLAND *system);
void _al_wl_clipboard_shutdown(ALLEGRO_SYSTEM_WAYLAND *system);
void _al_wl_clipboard_reap_transfers(ALLEGRO_SYSTEM_WAYLAND *system);
void _al_wl_clipboard_add_functions(ALLEGRO_DISPLAY_INTERFACE *vt);

#endif
