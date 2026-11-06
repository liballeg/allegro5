#ifndef __al_included_allegro5_aintern_wlegl_h
#define __al_included_allegro5_aintern_wlegl_h

#include "allegro5/internal/aintern_wl.h"

void _al_wlegl_config_select_visual(ALLEGRO_DISPLAY_WAYLAND *d);
bool _al_wlegl_config_create_context(ALLEGRO_DISPLAY_WAYLAND *d);

#endif