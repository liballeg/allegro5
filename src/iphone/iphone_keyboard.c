#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_events.h"

static ALLEGRO_KEYBOARD the_keyboard;

static bool iphone_init_keyboard(void)
{
    memset(&the_keyboard, 0, sizeof the_keyboard);
    _al_event_source_init(&the_keyboard.es);
    return true;
}

static void iphone_exit_keyboard(void)
{
    _al_event_source_free(&the_keyboard.es);
}


static ALLEGRO_KEYBOARD *iphone_get_keyboard(void)
{
    return &the_keyboard;
}

static bool iphone_set_keyboard_leds(int leds)
{
    (void)leds;
    return false;
}

static char const *iphone_keycode_to_name(int keycode)
{
    (void)keycode;
    return "none";
}

static void iphone_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
    memset(ret_state, 0, sizeof *ret_state);
}

static ALLEGRO_KEYBOARD_DRIVER iphone_keyboard_driver = {
    AL_ID('I','P','H','O'),
    "",
    "",
    "iphone keyboard",
    iphone_init_keyboard,
    iphone_exit_keyboard,
    iphone_get_keyboard,
    iphone_set_keyboard_leds,
    iphone_keycode_to_name,
    iphone_get_keyboard_state
};

ALLEGRO_KEYBOARD_DRIVER *_al_get_iphone_keyboard_driver(void)
{
    return &iphone_keyboard_driver;
}
