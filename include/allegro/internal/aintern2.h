/* XXX: temporary hack to break circular dependencies
 * this stuff should be in aintern.h but they depend on _AL_MUTEX/_AL_VECTOR
 * but those are in aint*.h, which depend on aintern.h
 *
 * I'm thinking of reorganising the internal headers into two levels:
 * - level 1 provides simple, self-contained things
 *      eg. hiding platform-specific mutexes behind _AL_MUTEX
 *      eg. simple container classes like _AL_VECTOR
 * - level 2 can then rely on level 1 stuff
 */
#ifndef _al_included_aintern2_h
#define _al_included_aintern2_h

AL_BEGIN_EXTERN_C

/* event sources */
struct AL_EVENT_SOURCE
{
   unsigned long event_mask;
   _AL_MUTEX mutex;
   _AL_VECTOR queues;
   AL_EVENT *all_events;
   AL_EVENT *free_events;
};

AL_FUNC(void, _al_event_source_init, (AL_EVENT_SOURCE*, unsigned long event_mask));
AL_FUNC(void, _al_event_source_free, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_lock, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_unlock, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_on_registration_to_queue, (AL_EVENT_SOURCE*, AL_EVENT_QUEUE*));
AL_FUNC(void, _al_event_source_on_unregistration_from_queue, (AL_EVENT_SOURCE*, AL_EVENT_QUEUE*));
AL_FUNC(bool, _al_event_source_needs_to_generate_event, (AL_EVENT_SOURCE*, unsigned long event_type));
AL_FUNC(AL_EVENT*, _al_event_source_get_unused_event, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_emit_event, (AL_EVENT_SOURCE *, AL_EVENT*));

AL_FUNC(void, _al_release_event, (AL_EVENT*));

AL_FUNC(void, _al_event_queue_push_event, (AL_EVENT_QUEUE*, AL_EVENT*));



/* joystick */

typedef struct AL_JOYSTICK_DRIVER /* new joystick driver structure */
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(bool, init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(int, num_joysticks, (void));
   AL_METHOD(AL_JOYSTICK*, get_joystick, (int joyn));
   AL_METHOD(void, release_joystick, (AL_JOYSTICK*));
   AL_METHOD(void, get_state, (AL_JOYSTICK*, AL_JOYSTATE *ret_state));
} AL_JOYSTICK_DRIVER;


AL_ARRAY(_DRIVER_INFO, _al_joystick_driver_list);


/* macros for constructing the driver list */
#define _AL_BEGIN_JOYSTICK_DRIVER_LIST                         \
   _DRIVER_INFO _al_joystick_driver_list[] =                   \
   {

#define _AL_END_JOYSTICK_DRIVER_LIST                           \
      {  0,                NULL,                FALSE }        \
   };


/* information about a single joystick axis */
typedef struct _AL_JOYSTICK_AXIS_INFO
{
   AL_CONST char *name;
} _AL_JOYSTICK_AXIS_INFO;


/* information about one or more axis (a slider or directional control) */
typedef struct _AL_JOYSTICK_STICK_INFO
{
   int flags;
   int num_axes;
   _AL_JOYSTICK_AXIS_INFO axis[_AL_MAX_JOYSTICK_AXES];
   AL_CONST char *name;
} _AL_JOYSTICK_STICK_INFO;


/* information about a joystick button */
typedef struct _AL_JOYSTICK_BUTTON_INFO
{
   AL_CONST char *name;
} _AL_JOYSTICK_BUTTON_INFO;


/* information about an entire joystick */
typedef struct _AL_JOYSTICK_INFO
{
   int num_sticks;
   int num_buttons;
   _AL_JOYSTICK_STICK_INFO stick[_AL_MAX_JOYSTICK_STICKS];
   _AL_JOYSTICK_BUTTON_INFO button[_AL_MAX_JOYSTICK_BUTTONS];
} _AL_JOYSTICK_INFO;


struct AL_JOYSTICK
{
   AL_EVENT_SOURCE es;
   _AL_JOYSTICK_INFO info;
   int num;
};



/* keyboard */

typedef struct AL_KEYBOARD_DRIVER /* new keyboard driver structure */
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(bool, init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(AL_KEYBOARD*, get_keyboard, (void));
   AL_METHOD(bool, set_leds, (int leds));
   AL_METHOD(AL_CONST char *, keycode_to_name, (int keycode));
   AL_METHOD(void, get_state, (AL_KBDSTATE *ret_state));
} AL_KEYBOARD_DRIVER;


AL_ARRAY(_DRIVER_INFO, _al_keyboard_driver_list);

AL_ARRAY(AL_CONST char *, _al_keyboard_common_names);


struct AL_KEYBOARD
{
   AL_EVENT_SOURCE es;
};


/* Helpers for AL_KBDSTATE structures.  */

#define _AL_KBDSTATE_KEY_DOWN(STATE, KEYCODE)                                  \
   (((STATE).__key_down__internal__[(KEYCODE) / 32] & (1 << ((KEYCODE) % 32))) \
    ? true : false)

#define _AL_KBDSTATE_SET_KEY_DOWN(STATE, KEYCODE)                       \
   do {                                                                 \
      int kc = (KEYCODE);                                               \
      (STATE).__key_down__internal__[kc / 32] |= (1 << (kc % 32));      \
   } while (0)

#define _AL_KBDSTATE_CLEAR_KEY_DOWN(STATE, KEYCODE)                     \
   do {                                                                 \
      int kc = (KEYCODE);                                               \
      (STATE).__key_down__internal__[kc / 32] &= ~(1 << (kc % 32));     \
   } while (0)


AL_END_EXTERN_C

#endif

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
