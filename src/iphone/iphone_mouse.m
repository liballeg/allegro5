#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_iphone.h"

typedef struct ALLEGRO_MOUSE_IPHONE {
    ALLEGRO_MOUSE parent;
    ALLEGRO_MOUSE_FLOAT_STATE state;
} ALLEGRO_MOUSE_IPHONE;

static ALLEGRO_MOUSE_IPHONE the_mouse;

static bool imouse_installed;

static void imouse_exit(void);
static bool imouse_init(void)
{
    if (imouse_installed)
        imouse_exit();
    memset(&the_mouse, 0, sizeof the_mouse);
    _al_event_source_init(&the_mouse.parent.es);
    imouse_installed = true;
    return true;
}

static void imouse_exit(void)
{
    if (!imouse_installed)
        return;
    imouse_installed = false;
    _al_event_source_free(&the_mouse.parent.es);
}

static ALLEGRO_MOUSE *imouse_get_mouse(void)
{
    ASSERT(imouse_installed);
    return (ALLEGRO_MOUSE *)&the_mouse;
}

/* We report multi-touch as different buttons. */
static unsigned int imouse_get_mouse_num_buttons(void)
{
    return 5;
}

static unsigned int imouse_get_mouse_num_axes(void)
{
    return 2;
}

/* Hard to accomplish on a touch screen. */
static bool imouse_set_mouse_xy(ALLEGRO_DISPLAY *display, float x, float y)
{
    (void)display;
    (void)x;
    (void)y;
    return false;
}

static bool imouse_set_mouse_axis(int which, float z)
{
    (void)which;
    (void)z;
    return false;
}

void imouse_get_state(ALLEGRO_MOUSE_FLOAT_STATE *ret_state);

static ALLEGRO_MOUSE_DRIVER iphone_mouse_driver = {
    AL_ID('I', 'P', 'H', 'O'),
    "",
    "",
    "iphone mouse",
    imouse_init,
    imouse_exit,
    imouse_get_mouse,
    imouse_get_mouse_num_buttons,
    imouse_get_mouse_num_axes,
    imouse_set_mouse_xy,
    imouse_set_mouse_axis,
    imouse_get_state
};

ALLEGRO_MOUSE_DRIVER *_al_get_iphone_mouse_driver(void)
{
    return &iphone_mouse_driver;
}
