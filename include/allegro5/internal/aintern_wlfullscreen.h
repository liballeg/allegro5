#ifndef __al_included_allegro5_aintern_wlfullscreen_h
#define __al_included_allegro5_aintern_wlfullscreen_h

#include "allegro5/internal/aintern_wl.h"
#include "allegro5/monitor.h"

/* One wl_output, i.e. one video adapter/monitor from Allegro's point of
 * view.  Values are filled in by the wl_output listener events. */
struct ALLEGRO_WL_OUTPUT {
    struct wl_output *output;   /* owned proxy */
    uint32_t registry_name;     /* used to match global removal */

    /* from geometry: position in the global compositor coordinate space */
    int x, y;
    /* from mode: size in physical pixels */
    int mode_width, mode_height;

    /* from scale; defaults to 1 */
    int scale;

    bool has_mode;
};

/* Register/deregister outputs as the registry announces them. */
void _al_wayland_add_output(ALLEGRO_SYSTEM_WAYLAND *s,
    struct wl_output *output, uint32_t registry_name);
void _al_wayland_remove_output(ALLEGRO_SYSTEM_WAYLAND *s,
    uint32_t registry_name);

/* fullscreen and multi monitor stuff */
int _al_wayland_get_num_video_adapters(ALLEGRO_SYSTEM_WAYLAND *s);
int _al_wayland_get_default_adapter(ALLEGRO_SYSTEM_WAYLAND *s);
bool _al_wayland_get_monitor_info(ALLEGRO_SYSTEM_WAYLAND *s,
    int adapter, ALLEGRO_MONITOR_INFO *info);

#endif