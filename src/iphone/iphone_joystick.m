#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_iphone.h"

A5O_DEBUG_CHANNEL("iphone")

typedef struct A5O_JOYSTICK_IPHONE {
   A5O_JOYSTICK parent;
   A5O_JOYSTICK_STATE joystate;
} A5O_JOYSTICK_IPHONE;

static A5O_JOYSTICK_IPHONE the_joystick;
static bool initialized;

static bool ijoy_init_joystick(void)
{
    A5O_JOYSTICK_IPHONE *ijoy;
    A5O_JOYSTICK *joy;
    
    ijoy = &the_joystick;
    
    memset(ijoy, 0, sizeof *ijoy);
    joy = (void *)ijoy;

    /* Fill in the joystick information fields. */
    joy->info.num_sticks = 1;
    joy->info.num_buttons = 0;
    joy->info.stick[0].name = "Accelerometer";
    joy->info.stick[0].num_axes = 3;
    joy->info.stick[0].axis[0].name = "X";
    joy->info.stick[0].axis[1].name = "Y";
    joy->info.stick[0].axis[2].name = "Z";
    joy->info.stick[0].flags = A5O_JOYFLAG_ANALOGUE;
    
    // TODO: What's a good frequency to use here?
    _al_iphone_accelerometer_control(60);
    initialized = true;

    return true;
}

static void ijoy_exit_joystick(void)
{
    _al_iphone_accelerometer_control(0);
    initialized = false;
}

static bool ijoy_reconfigure_joysticks(void)
{
    return false;
}

static int ijoy_num_joysticks(void)
{
    return 1;
}

static A5O_JOYSTICK *ijoy_get_joystick(int num)
{
    if (num != 0)
       return NULL;
    
    A5O_DEBUG("Joystick %d acquired.\n", num);

    return &the_joystick.parent;
}

static void ijoy_release_joystick(A5O_JOYSTICK *joy)
{
    (void)joy;
    A5O_DEBUG("Joystick released.\n");
    initialized = false;
}

static void ijoy_get_joystick_state(A5O_JOYSTICK *joy, A5O_JOYSTICK_STATE *ret_state)
{
    A5O_JOYSTICK_IPHONE *ijoy = (void *)joy;
    A5O_EVENT_SOURCE *es = al_get_joystick_event_source();
    
    _al_event_source_lock(es);
    *ret_state = ijoy->joystate;
    _al_event_source_unlock(es);
}

void _al_iphone_generate_joystick_event(float x, float y, float z)
{
    if (!initialized)
       return;
    A5O_EVENT_SOURCE *es = al_get_joystick_event_source();

    A5O_EVENT event;

    _al_event_source_lock(es);
    
    if (_al_event_source_needs_to_generate_event(es)) {
        float pos[] = {x, y, z};
        for (int i = 0; i < 3; i++) {
            event.joystick.type = A5O_EVENT_JOYSTICK_AXIS;
            event.joystick.timestamp = al_get_time();
            event.joystick.stick = 0;
            event.joystick.axis = i;
            event.joystick.pos = pos[i];
            event.joystick.button = 0;
            _al_event_source_emit_event(es, &event);
        }
    }
    _al_event_source_unlock(es);
}

static char const *ijoy_get_name(A5O_JOYSTICK *joy)
{
    (void)joy;
    return "Accelerometer";
}

static bool ijoy_get_active(A5O_JOYSTICK *joy)
{
    (void)joy;
    return true;
}

static A5O_JOYSTICK_DRIVER iphone_joystick_driver = {
   AL_ID('I', 'P', 'H', 'O'),
   "",
   "",
   "iphone joystick",
    ijoy_init_joystick,
    ijoy_exit_joystick,
    ijoy_reconfigure_joysticks,
    ijoy_num_joysticks,
    ijoy_get_joystick,
    ijoy_release_joystick,
    ijoy_get_joystick_state,
    ijoy_get_name,
    ijoy_get_active
};

A5O_JOYSTICK_DRIVER *_al_get_iphone_joystick_driver(void)
{
    return &iphone_joystick_driver;
}
