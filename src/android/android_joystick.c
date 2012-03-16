#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_android.h"

ALLEGRO_DEBUG_CHANNEL("android")

typedef struct ALLEGRO_JOYSTICK_ANDROID {
   ALLEGRO_JOYSTICK parent;
   ALLEGRO_JOYSTICK_STATE joystate;
} ALLEGRO_JOYSTICK_ANDROID;

static ALLEGRO_JOYSTICK_ANDROID the_joystick;
static bool initialized;

static bool andjoy_init_joystick(void)
{
    ALLEGRO_JOYSTICK_ANDROID *andjoy;
    ALLEGRO_JOYSTICK *joy;
    
    andjoy = &the_joystick;
    
    memset(andjoy, 0, sizeof *andjoy);
    joy = (void *)andjoy;

    /* Fill in the joystick information fields. */
    joy->info.num_sticks = 1;
    joy->info.num_buttons = 0;
    joy->info.stick[0].name = "Accelerometer";
    joy->info.stick[0].num_axes = 3;
    joy->info.stick[0].axis[0].name = "X";
    joy->info.stick[0].axis[1].name = "Y";
    joy->info.stick[0].axis[2].name = "Z";
    joy->info.stick[0].flags = ALLEGRO_JOYFLAG_ANALOGUE;
    
    initialized = true;

    return true;
}

static void andjoy_exit_joystick(void)
{
    initialized = false;
}

static bool andjoy_reconfigure_joysticks(void)
{
    return false;
}

static int andjoy_num_joysticks(void)
{
    return 1;
}

static ALLEGRO_JOYSTICK *andjoy_get_joystick(int num)
{
    if (num != 0)
       return NULL;
    
    ALLEGRO_DEBUG("Joystick %d acquired.\n", num);

    return &the_joystick.parent;
}

static void andjoy_release_joystick(ALLEGRO_JOYSTICK *joy)
{
    (void)joy;
    ALLEGRO_DEBUG("Joystick released.\n");
    initialized = false;
}

static void andjoy_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
    ALLEGRO_JOYSTICK_ANDROID *andjoy = (void *)joy;
    ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
    
    _al_event_source_lock(es);
    *ret_state = andjoy->joystate;
    _al_event_source_unlock(es);
}

void _al_android_generate_joystick_event(float x, float y, float z)
{
    if (!initialized)
       return;

    /* Android reports accelerometer data in the approximate range
     * -9.81 -> 9.81, but can be higher or lower. Allegro joysticks
     * use -1 -> 1. Also, the axis' are all reversed on Android,
     * hence the negative division.
     */
    x /= -9.81f;
    if (x < -1) x = -1;
    if (x > 1) x = 1;
    y /= -9.81f;
    if (y < -1) y = -1;
    if (y > 1) y = 1;
    z /= -9.81f;
    if (z < -1) z = -1;
    if (z > 1) z = 1;

    ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

    ALLEGRO_EVENT event;

    _al_event_source_lock(es);

    if (_al_event_source_needs_to_generate_event(es)) {
        float pos[] = {x, y, z};
        int i;
        for (i = 0; i < 3; i++) {
            event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
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

static char const *andjoy_get_name(ALLEGRO_JOYSTICK *joy)
{
    (void)joy;
    return "Accelerometer";
}

static bool andjoy_get_active(ALLEGRO_JOYSTICK *joy)
{
    (void)joy;
    return true;
}

static ALLEGRO_JOYSTICK_DRIVER android_joystick_driver = {
   AL_ID('A', 'N', 'D', 'R'),
   "",
   "",
   "android joystick",
    andjoy_init_joystick,
    andjoy_exit_joystick,
    andjoy_reconfigure_joysticks,
    andjoy_num_joysticks,
    andjoy_get_joystick,
    andjoy_release_joystick,
    andjoy_get_joystick_state,
    andjoy_get_name,
    andjoy_get_active
};

ALLEGRO_JOYSTICK_DRIVER *_al_get_android_joystick_driver(void)
{
    return &android_joystick_driver;
}
