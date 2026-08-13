#ifndef __al_included_allegro5_aintern_wlfullscreen_h
#define __al_included_allegro5_aintern_wlfullscreen_h

/* fullscreen and multi monitor stuff */

int _al_wayland_get_num_video_adapters(ALLEGRO_SYSTEM_WAYLAND *s);
int _al_wayland_get_default_adapter(ALLEGRO_SYSTEM_WAYLAND *s);

#endif