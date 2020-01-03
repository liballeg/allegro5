#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_android.h"

typedef struct ALLEGRO_MOUSE_ANDROID {
    ALLEGRO_MOUSE parent;
    ALLEGRO_MOUSE_FLOAT_STATE state;
} ALLEGRO_MOUSE_ANDROID;

static ALLEGRO_MOUSE_ANDROID the_mouse;

static bool amouse_installed;

static void amouse_exit(void);
static bool amouse_init(void)
{
    if (amouse_installed)
        amouse_exit();
    memset(&the_mouse, 0, sizeof the_mouse);
    _al_event_source_init(&the_mouse.parent.es);
    amouse_installed = true;
    return true;
}

static void amouse_exit(void)
{
    if (!amouse_installed)
        return;
    amouse_installed = false;
    _al_event_source_free(&the_mouse.parent.es);
}

static ALLEGRO_MOUSE *amouse_get_mouse(void)
{
    ASSERT(amouse_installed);
    return (ALLEGRO_MOUSE *)&the_mouse;
}

/* We report multi-touch as different buttons. */
static unsigned int amouse_get_mouse_num_buttons(void)
{
    return 5;
}

static unsigned int amouse_get_mouse_num_axes(void)
{
    return 2;
}

/* Hard to accomplish on a touch screen. */
static bool amouse_set_mouse_xy(ALLEGRO_DISPLAY *display, float x, float y)
{
    (void)display;
    (void)x;
    (void)y;
    return false;
}

static bool amouse_set_mouse_axis(int which, float z)
{
    (void)which;
    (void)z;
    return false;
}

static ALLEGRO_MOUSE_DRIVER android_mouse_driver = {
    AL_ID('A', 'N', 'D', 'R'),
    "",
    "",
    "android mouse",
    amouse_init,
    amouse_exit,
    amouse_get_mouse,
    amouse_get_mouse_num_buttons,
    amouse_get_mouse_num_axes,
    amouse_set_mouse_xy,
    amouse_set_mouse_axis,
    _al_android_mouse_get_state
};

ALLEGRO_MOUSE_DRIVER *_al_get_android_mouse_driver(void)
{
    return &android_mouse_driver;
}
