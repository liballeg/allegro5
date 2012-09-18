#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_events.h"

#include <stdio.h>

ALLEGRO_DEBUG_CHANNEL("keyboard")

static ALLEGRO_KEYBOARD the_keyboard;
static ALLEGRO_KEYBOARD_STATE the_state;

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
   static bool created = false;
   static char names[ALLEGRO_KEY_MAX][5];

   ASSERT(keycode >= 0 && keycode < ALLEGRO_KEY_MAX);

   if (!created) {
      int i;
      created = true;
      for (i = 0; i < ALLEGRO_KEY_MAX; i++) {
         snprintf(names[i], 5, "%d", i);
      }
   }

   return names[keycode];
}

static void android_get_keyboard_state(ALLEGRO_KEYBOARD_STATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.es);
   {
      *ret_state = the_state;
   }
   _al_event_source_unlock(&the_keyboard.es);
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

void _al_android_keyboard_handle_event(ALLEGRO_DISPLAY *display, int scancode, ALLEGRO_EVENT_TYPE event_type) 
{
   ALLEGRO_EVENT event;

   ASSERT(display != NULL);
   ASSERT(scancode > 0);

   if (event_type == ALLEGRO_EVENT_KEY_UP) {
      _AL_KEYBOARD_STATE_CLEAR_KEY_DOWN(the_state, scancode);
   }
   else {
      _AL_KEYBOARD_STATE_SET_KEY_DOWN(the_state, scancode);
   }

   _al_event_source_lock(&the_keyboard.es);
   
   if (_al_event_source_needs_to_generate_event(&the_keyboard.es)) {
      
      event.keyboard.type = event_type;
      event.keyboard.timestamp = al_get_time();
      event.keyboard.display = display;
      event.keyboard.keycode = scancode;
      event.keyboard.unichar = 0;
      event.keyboard.modifiers = 0;
      event.keyboard.repeat = event_type == ALLEGRO_EVENT_KEY_CHAR;
      
      _al_event_source_emit_event(&the_keyboard.es, &event);
   }
   
   _al_event_source_unlock(&the_keyboard.es);
}

