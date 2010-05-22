#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_iphone.h"

ALLEGRO_DEBUG_CHANNEL("iphone")

typedef struct ALLEGRO_JOYSTICK_IPHONE {
   ALLEGRO_JOYSTICK parent;
   ALLEGRO_JOYSTICK_STATE joystate;
} ALLEGRO_JOYSTICK_IPHONE;

static ALLEGRO_JOYSTICK_IPHONE the_joystick;
static bool initialized;

static bool ijoy_init_joystick(void)
{
    return true;
}

static void ijoy_exit_joystick(void)
{
}

static int ijoy_num_joysticks(void)
{
    return 1;
}

static ALLEGRO_JOYSTICK *ijoy_get_joystick(int num)
{
    if (num != 0)
       return NULL;
    
    ALLEGRO_DEBUG("Joystick %d acquired.\n", num);

    ALLEGRO_JOYSTICK_IPHONE *ijoy;
    ALLEGRO_JOYSTICK *joy;

    ijoy = &the_joystick;

    memset(ijoy, 0, sizeof *ijoy);
    joy = (void *)ijoy;
    
    /* Initialise the event source part of it. */
    _al_event_source_init(&joy->es);

    /* Fill in the joystick information fields. */
    joy->num = num;
    joy->info.num_sticks = 1;
    joy->info.num_buttons = 0;
    joy->info.stick[0].name = "Accelerometer";
    joy->info.stick[0].num_axes = 3;
    joy->info.stick[0].axis[0].name = "X";
    joy->info.stick[0].axis[1].name = "Y";
    joy->info.stick[0].axis[2].name = "Z";
    joy->info.stick[0].flags = ALLEGRO_JOYFLAG_ANALOGUE;
    
    // TODO: What's a good frequency to use here?
    _al_iphone_accelerometer_control(60);
    initialized = true;

    return joy;
}

static void ijoy_release_joystick(ALLEGRO_JOYSTICK *joy)
{
    ALLEGRO_DEBUG("Joystick %d released.\n", joy->num);
    _al_iphone_accelerometer_control(0);
    initialized = false;

    _al_event_source_free(&joy->es);
}

static void ijoy_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
    ALLEGRO_JOYSTICK_IPHONE *ijoy = (void *)joy;
    
    _al_event_source_lock(&joy->es);
    *ret_state = ijoy->joystate;
    _al_event_source_unlock(&joy->es);
}

void _al_iphone_generate_joystick_event(float x, float y, float z)
{
    if (!initialized)
       return;

    ALLEGRO_JOYSTICK_IPHONE *ijoy = &the_joystick;
    ALLEGRO_JOYSTICK *joy = (void *)ijoy;
    ALLEGRO_EVENT event;

    _al_event_source_lock(&joy->es);
    
    if (_al_event_source_needs_to_generate_event(&joy->es)) {
        float pos[] = {x, y, z};
        for (int i = 0; i < 3; i++) {
            event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
            event.joystick.timestamp = al_current_time();
            event.joystick.stick = 0;
            event.joystick.axis = i;
            event.joystick.pos = pos[i];
            event.joystick.button = 0;
            _al_event_source_emit_event(&joy->es, &event);
        }
    }
    _al_event_source_unlock(&joy->es);
}

static ALLEGRO_JOYSTICK_DRIVER iphone_joystick_driver = {
   AL_ID('I', 'P', 'H', 'O'),
   "",
   "",
   "iphone joystick",
    ijoy_init_joystick,
    ijoy_exit_joystick,
    ijoy_num_joysticks,
    ijoy_get_joystick,
    ijoy_release_joystick,
    ijoy_get_joystick_state
};

ALLEGRO_JOYSTICK_DRIVER *_al_get_iphone_joystick_driver(void)
{
    return &iphone_joystick_driver;
}
