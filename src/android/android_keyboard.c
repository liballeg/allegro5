#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_events.h"

ALLEGRO_DEBUG_CHANNEL("keyboard")

static ALLEGRO_KEYBOARD the_keyboard;

static bool android_init_keyboard(void)
{
    memset(&the_keyboard, 0, sizeof the_keyboard);
    _al_event_source_init(&the_keyboard.es);
    return true;
}

static void android_exit_keyboard(void)
{
    _al_event_source_free(&the_keyboard.es);
}


static ALLEGRO_KEYBOARD *android_get_keyboard(void)
{
    return &the_keyboard;
}

static bool android_set_keyboard_leds(int leds)
{
    (void)leds;
    return false;
}

static char const *android_keycode_to_name(int keycode)
{
    (void)keycode;
    return "none";
}

static void android_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
    memset(ret_state, 0, sizeof *ret_state);
}

static ALLEGRO_KEYBOARD_DRIVER android_keyboard_driver = {
    AL_ID('A','N','D','R'),
    "",
    "",
    "android keyboard",
    android_init_keyboard,
    android_exit_keyboard,
    android_get_keyboard,
    android_set_keyboard_leds,
    android_keycode_to_name,
    android_get_keyboard_state
};

ALLEGRO_KEYBOARD_DRIVER *_al_get_android_keyboard_driver(void)
{
    return &android_keyboard_driver;
}

void _al_android_keyboard_handle_event(ALLEGRO_DISPLAY *display, int scancode, bool down)
{
   ASSERT(display != NULL);
   ASSERT(scancode > 0);
   
   //_android_check_mutex(&the_keyboard.es);
   
   _al_event_source_lock(&the_keyboard.es);
   
   if (_al_event_source_needs_to_generate_event(&the_keyboard.es)) {
      ALLEGRO_EVENT event;
      
      event.keyboard.type = down ? ALLEGRO_EVENT_KEY_DOWN : ALLEGRO_EVENT_KEY_UP;
      event.keyboard.timestamp = al_get_time();
      event.keyboard.display = display;
      event.keyboard.keycode = scancode;
      event.keyboard.unichar = 0;
      event.keyboard.modifiers = 0;
      event.keyboard.repeat = false;
      
      _al_event_source_emit_event(&the_keyboard.es, &event);
   }
   
   _al_event_source_unlock(&the_keyboard.es);
}

