#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_wl.h"
#include "allegro5/internal/aintern_wldisplay.h"
#include "allegro5/internal/aintern_wlfullscreen.h"
#include "allegro5/internal/aintern_wlsystem.h"

ALLEGRO_DEBUG_CHANNEL("wlfullscreen")

/* The wl_output listeners run on the event thread while it holds the system
 * lock, so they do not lock again; only the app-thread query functions do. */

static void output_geometry(void *data, struct wl_output *output,
    int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
    int32_t subpixel, const char *make, const char *model, int32_t transform)
{
    struct ALLEGRO_WL_OUTPUT *o = data;
    (void)output;
    (void)physical_width;
    (void)physical_height;
    (void)subpixel;
    (void)make;
    (void)model;
    (void)transform;

    o->x = x;
    o->y = y;
}


static void output_mode(void *data, struct wl_output *output,
    uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    struct ALLEGRO_WL_OUTPUT *o = data;
    (void)output;
    (void)flags;
    (void)refresh;

    o->mode_width = width;
    o->mode_height = height;
    o->has_mode = true;

    ALLEGRO_DEBUG("wlfullscreen: output mode %dx%d\n", width, height);
}


static void output_done(void *data, struct wl_output *output)
{
    (void)data;
    (void)output;
}


static void output_scale(void *data, struct wl_output *output, int32_t factor)
{
    struct ALLEGRO_WL_OUTPUT *o = data;
    (void)output;

    if (factor > 0)
        o->scale = factor;
}


static void output_name(void *data, struct wl_output *output, const char *name)
{
    struct ALLEGRO_WL_OUTPUT *o = data;
    (void)o;
    (void)output;

    ALLEGRO_DEBUG("wlfullscreen: output name: %s\n", name);
}


static void output_description(void *data, struct wl_output *output,
    const char *description)
{
    (void)data;
    (void)output;
    (void)description;
}


static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
    .name = output_name,
    .description = output_description,
};


void _al_wayland_add_output(ALLEGRO_SYSTEM_WAYLAND *s,
    struct wl_output *output, uint32_t registry_name)
{
    struct ALLEGRO_WL_OUTPUT *o = al_calloc(1, sizeof *o);

    o->output = output;
    o->registry_name = registry_name;
    o->scale = 1;

    wl_output_add_listener(output, &output_listener, o);

    /* Runs during the wl_initialize roundtrip (no lock needed) or on the
     * event thread (which holds the lock). */
    struct ALLEGRO_WL_OUTPUT **add;
    add = _al_vector_alloc_back(&s->outputs);
    *add = o;

    ALLEGRO_INFO("wlfullscreen: new output, now %zu total\n",
        _al_vector_size(&s->outputs));
}


void _al_wayland_remove_output(ALLEGRO_SYSTEM_WAYLAND *s,
    uint32_t registry_name)
{
    struct ALLEGRO_WL_OUTPUT *o;
    int i;

    for (i = 0; i < (int)_al_vector_size(&s->outputs); i++) {
        o = *((struct ALLEGRO_WL_OUTPUT **)_al_vector_ref(&s->outputs, i));
        if (o->registry_name == registry_name) {
            wl_output_destroy(o->output);
            al_free(o);
            _al_vector_delete_at(&s->outputs, i);
            ALLEGRO_INFO("wlfullscreen: output removed\n");
            return;
        }
    }
}


int _al_wayland_get_num_video_adapters(ALLEGRO_SYSTEM_WAYLAND *s)
{
    int num;

    _al_mutex_lock(&s->lock);
    num = _al_vector_size(&s->outputs);
    _al_mutex_unlock(&s->lock);

    return num;
}


int _al_wayland_get_default_adapter(ALLEGRO_SYSTEM_WAYLAND *s)
{
    (void)s;
    return 0;
}


bool _al_wayland_get_monitor_info(ALLEGRO_SYSTEM_WAYLAND *s,
    int adapter, ALLEGRO_MONITOR_INFO *info)
{
    bool ret = false;

    _al_mutex_lock(&s->lock);

    if (adapter < (int)_al_vector_size(&s->outputs)) {
        struct ALLEGRO_WL_OUTPUT *o;
        o = *((struct ALLEGRO_WL_OUTPUT **)_al_vector_ref(&s->outputs, adapter));
        if (o->has_mode) {
            /* Report logical (scale-corrected) coordinates, like the
             * application's drawing surface. */
            int scale = o->scale > 0 ? o->scale : 1;
            info->x1 = o->x / scale;
            info->y1 = o->y / scale;
            info->x2 = info->x1 + o->mode_width / scale;
            info->y2 = info->y1 + o->mode_height / scale;
            ret = true;
        }
    }

    _al_mutex_unlock(&s->lock);

    return ret;
}