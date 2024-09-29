#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"

#include <stdio.h>

A5O_DEBUG_CHANNEL("keyboard")

static A5O_KEYBOARD the_keyboard;
static A5O_KEYBOARD_STATE the_state;

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


static A5O_KEYBOARD *android_get_keyboard(void)
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
   static char names[A5O_KEY_MAX][5];

   ASSERT(keycode >= 0 && keycode < A5O_KEY_MAX);

   if (!created) {
      int i;
      created = true;
      for (i = 0; i < A5O_KEY_MAX; i++) {
         snprintf(names[i], 5, "%d", i);
      }
   }

   return names[keycode];
}

static void android_get_keyboard_state(A5O_KEYBOARD_STATE *ret_state)
{
   _al_event_source_lock(&the_keyboard.es);
   {
      *ret_state = the_state;
   }
   _al_event_source_unlock(&the_keyboard.es);
}

static void android_clear_keyboard_state(void)
{
   _al_event_source_lock(&the_keyboard.es);
   {
      memset(&the_state, 0, sizeof(the_state));
   }
   _al_event_source_unlock(&the_keyboard.es);
}

static A5O_KEYBOARD_DRIVER android_keyboard_driver = {
    AL_ID('A','N','D','R'),
    "",
    "",
    "android keyboard",
    android_init_keyboard,
    android_exit_keyboard,
    android_get_keyboard,
    android_set_keyboard_leds,
    android_keycode_to_name,
    android_get_keyboard_state,
    android_clear_keyboard_state
};

A5O_KEYBOARD_DRIVER *_al_get_android_keyboard_driver(void)
{
    return &android_keyboard_driver;
}

static void android_keyboard_handle_event(A5O_DISPLAY *display,
   int scancode, int unichar, A5O_EVENT_TYPE event_type)
{
   A5O_EVENT event;

   ASSERT(display != NULL);
   ASSERT(scancode > 0);

   if (event_type == A5O_EVENT_KEY_UP) {
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
      event.keyboard.unichar = unichar;
      event.keyboard.modifiers = 0;
      event.keyboard.repeat = event_type == A5O_EVENT_KEY_CHAR;

      _al_event_source_emit_event(&the_keyboard.es, &event);
   }

   _al_event_source_unlock(&the_keyboard.es);
}

JNI_FUNC(void, KeyListener, nativeOnKeyDown, (JNIEnv *env, jobject obj,
   jint scancode, jint unichar))
{
   (void)env;
   (void)obj;

   A5O_SYSTEM *system = al_get_system_driver();
   ASSERT(system != NULL);

   A5O_DISPLAY **dptr = _al_vector_ref(&system->displays, 0);
   A5O_DISPLAY *display = *dptr;
   ASSERT(display != NULL);

   android_keyboard_handle_event(display, scancode, unichar,
      A5O_EVENT_KEY_DOWN);
}

JNI_FUNC(void, KeyListener, nativeOnKeyUp, (JNIEnv *env, jobject obj,
   jint scancode))
{
   (void)env;
   (void)obj;

   A5O_SYSTEM *system = al_get_system_driver();
   ASSERT(system != NULL);

   A5O_DISPLAY **dptr = _al_vector_ref(&system->displays, 0);
   A5O_DISPLAY *display = *dptr;
   ASSERT(display != NULL);

   android_keyboard_handle_event(display, scancode, 0, A5O_EVENT_KEY_UP);
}

JNI_FUNC(void, KeyListener, nativeOnKeyChar, (JNIEnv *env, jobject obj,
   jint scancode, jint unichar))
{
   (void)env;
   (void)obj;

   A5O_SYSTEM *system = al_get_system_driver();
   ASSERT(system != NULL);

   A5O_DISPLAY **dptr = _al_vector_ref(&system->displays, 0);
   A5O_DISPLAY *display = *dptr;
   ASSERT(display != NULL);

   android_keyboard_handle_event(display, scancode, unichar,
      A5O_EVENT_KEY_CHAR);
}

JNI_FUNC(void, KeyListener, nativeOnJoystickButton, (JNIEnv *env, jobject obj,
   jint index, jint button, jboolean down))
{
   (void)env;
   (void)obj;
   _al_android_generate_joystick_button_event(index+1, button, down);
}

/* vim: set sts=3 sw=3 et: */
