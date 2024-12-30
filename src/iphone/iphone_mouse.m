#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_iphone.h"

typedef struct ALLEGRO_MOUSE_IPHONE {
    ALLEGRO_MOUSE parent;
    ALLEGRO_MOUSE_STATE state;
} ALLEGRO_MOUSE_IPHONE;

static ALLEGRO_MOUSE_IPHONE the_mouse;

static bool imouse_installed;

/*
 *  Helper to generate a mouse event.
 */
void _al_iphone_generate_mouse_event(unsigned int type, int x, int y,
   unsigned int button, ALLEGRO_DISPLAY *d)
{
   ALLEGRO_EVENT event;

   _al_event_source_lock(&the_mouse.parent.es);

   _al_iphone_translate_from_screen(d, &x, &y);

   the_mouse.state.x = x;
   the_mouse.state.y = y;

   if (type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
      the_mouse.state.buttons |= (1 << button);
   }
   else if (type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
      the_mouse.state.buttons &= ~(1 << button);
   }

   the_mouse.state.pressure = the_mouse.state.buttons ? 1.0 : 0.0; // TODO

   if (_al_event_source_needs_to_generate_event(&the_mouse.parent.es)) {
      event.mouse.type = type;
      event.mouse.timestamp = al_get_time();
      event.mouse.display = d;
      event.mouse.x = x;
      event.mouse.y = y;
      event.mouse.z = 0;
      event.mouse.w = 0;
      event.mouse.dx = 0; // TODO
      event.mouse.dy = 0; // TODO
      event.mouse.dz = 0; // TODO
      event.mouse.dw = 0; // TODO
      event.mouse.button = button;
      event.mouse.pressure = the_mouse.state.pressure;
      _al_event_source_emit_event(&the_mouse.parent.es, &event);
   }

   _al_event_source_unlock(&the_mouse.parent.es);
}

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
static bool imouse_set_mouse_xy(ALLEGRO_DISPLAY *display, int x, int y)
{
    (void)display;
    (void)x;
    (void)y;
    return false;
}

static bool imouse_set_mouse_axis(int which, int z)
{
    (void)which;
    (void)z;
    return false;
}

/*
static void imouse_get_state(ALLEGRO_MOUSE_STATE *ret_state)
{
    ASSERT(imouse_installed);

    _al_event_source_lock(&the_mouse.parent.es);
    {
        *ret_state = the_mouse.state;
    }
    _al_event_source_unlock(&the_mouse.parent.es);
}
*/
void imouse_get_state(ALLEGRO_MOUSE_STATE *ret_state);

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
