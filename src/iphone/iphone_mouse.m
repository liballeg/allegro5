#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_display.h"

typedef struct ALLEGRO_MOUSE_IPHONE {
    ALLEGRO_MOUSE parent;
    ALLEGRO_MOUSE_STATE state;
} ALLEGRO_MOUSE_IPHONE;

static ALLEGRO_MOUSE_IPHONE the_mouse = {0};

static bool imouse_installed;

/*
 *  Helper to generate a mouse event.
 */
void _al_iphone_generate_mouse_event(unsigned int type, int x, int y,
   unsigned int button, ALLEGRO_DISPLAY *d)
{
   ALLEGRO_EVENT event;
   
   _al_event_source_lock(&the_mouse.parent.es);

   /* See iphone_display.c for details about the view scaling. */
   if (d->w != 320 || d->h != 480) {
      int ox = x;
      int oy = y;
      int cx = 0, cy = 0;
      double iscale;
      if (d->w >= d->h) {
         if (d->w * 320 > d->h * 480.0) {
            iscale = d->w / 480.0;
            cy = 0.5 * (d->h - 320 * iscale);
         }
         else {
            iscale = d->h / 320.0;
            cx = 0.5 * (d->w - 480 * iscale);
         }
         x = cx + oy * iscale;
         y = cy + (320 - ox) * iscale;
      }
      else {
         // TODO
      }
   }

   // FIXME: also update state (right now only events work)
   
   if (_al_event_source_needs_to_generate_event(&the_mouse.parent.es)) {
      event.mouse.type = type;
      event.mouse.timestamp = al_current_time();
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
      _al_event_source_emit_event(&the_mouse.parent.es, &event);
   }

   _al_event_source_unlock(&the_mouse.parent.es);

   if (type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
   	the_mouse.state.x = x;
	the_mouse.state.y = y;
	the_mouse.state.buttons |= (1 << button);
   }
   else if (type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
   	the_mouse.state.x = x;
	the_mouse.state.y = y;
	the_mouse.state.buttons &= ~(1 << button);
   }
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
static bool imouse_set_mouse_xy(int x, int y)
{
    return false;
}

static bool imouse_set_mouse_axis(int which, int z)
{
    return false;
}

static bool imouse_set_mouse_range(int x1, int y1, int x2, int y2)
{
    return false;
}

static void imouse_get_state(ALLEGRO_MOUSE_STATE *ret_state)
{
    ASSERT(imouse_installed);
    
    _al_event_source_lock(&the_mouse.parent.es);
    {
        *ret_state = the_mouse.state;
    }
    _al_event_source_unlock(&the_mouse.parent.es);
}

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
    imouse_set_mouse_range,
    imouse_get_state
};

ALLEGRO_MOUSE_DRIVER *_al_get_iphone_mouse_driver(void)
{
    return &iphone_mouse_driver;
}
