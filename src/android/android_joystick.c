#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_android.h"

ALLEGRO_DEBUG_CHANNEL("android")

typedef struct ALLEGRO_JOYSTICK_ANDROID {
   ALLEGRO_JOYSTICK parent;
   ALLEGRO_JOYSTICK_STATE joystate;
   char name[64];
} ALLEGRO_JOYSTICK_ANDROID;

static _AL_VECTOR joysticks = _AL_VECTOR_INITIALIZER(ALLEGRO_JOYSTICK_ANDROID *);
static bool initialized;

static char *android_get_joystick_name(JNIEnv *env, jobject activity, int index, char *buffer, size_t buffer_size)
{
    jobject str_obj = _jni_callObjectMethodV(env, activity, "getJoystickName", "(I)Ljava/lang/String;", (jint)index);
    ALLEGRO_USTR *s = _jni_getString(env, str_obj);
    _al_sane_strncpy(buffer, al_cstr(s), buffer_size);
    al_ustr_free(s);

    return buffer;
}

static void android_init_joysticks(int num)
{
    int i, j;
    JNIEnv *env = _al_android_get_jnienv();
    jobject activity = _al_android_activity_object();

    for (i = 0; i < num; i++) {
       ALLEGRO_JOYSTICK_ANDROID *stick = al_calloc(1, sizeof(ALLEGRO_JOYSTICK_ANDROID));
       ALLEGRO_JOYSTICK_ANDROID **ptr;
       ALLEGRO_JOYSTICK *joy;

       joy = (void *)stick;

       /* Fill in the joystick information fields. */
       android_get_joystick_name(env, activity, i, stick->name, sizeof(stick->name));
       joy->info.num_sticks = 2;
       joy->info.num_buttons = 11;
       joy->info.stick[0].name = "Stick 1";
       joy->info.stick[0].num_axes = 2;
       joy->info.stick[0].axis[0].name = "X";
       joy->info.stick[0].axis[1].name = "Y";
       joy->info.stick[0].flags = ALLEGRO_JOYFLAG_ANALOGUE;
       joy->info.stick[1].name = "Stick 2";
       joy->info.stick[1].num_axes = 2;
       joy->info.stick[1].axis[0].name = "X";
       joy->info.stick[1].axis[1].name = "Y";
       joy->info.stick[1].flags = ALLEGRO_JOYFLAG_ANALOGUE;

       for (j = 0; j < joy->info.num_buttons; j++) {
           joy->info.button[j].name = "";
       }

       ptr = _al_vector_alloc_back(&joysticks);
       *ptr = stick;
    }
}

static bool andjoy_init_joystick(void)
{
    ALLEGRO_JOYSTICK_ANDROID *accel = al_calloc(1, sizeof(ALLEGRO_JOYSTICK_ANDROID));
    ALLEGRO_JOYSTICK_ANDROID **ptr;
    ALLEGRO_JOYSTICK *joy;
    int num;

    joy = (void *)accel;

    /* Fill in the joystick information fields. */
    _al_sane_strncpy(accel->name, "Accelerometer", sizeof(accel->name));
    joy->info.num_sticks = 1;
    joy->info.num_buttons = 0;
    joy->info.stick[0].name = "Accelerometer";
    joy->info.stick[0].num_axes = 3;
    joy->info.stick[0].axis[0].name = "X";
    joy->info.stick[0].axis[1].name = "Y";
    joy->info.stick[0].axis[2].name = "Z";
    joy->info.stick[0].flags = ALLEGRO_JOYFLAG_ANALOGUE;

    ptr = _al_vector_alloc_back(&joysticks);
    *ptr = accel;

    num = _jni_callIntMethodV(_al_android_get_jnienv(), _al_android_activity_object(), "getNumJoysticks", "()I");

    android_init_joysticks(num);

    initialized = true;

    _jni_callVoidMethod(_al_android_get_jnienv(), _al_android_activity_object(), "setJoystickActive");

    return true;
}

static void andjoy_exit_joystick(void)
{
    initialized = false;
}

static bool andjoy_reconfigure_joysticks(void)
{
    int i;
    int sz;
    int num;

    sz = _al_vector_size(&joysticks);

    for (i = 1; i < sz; i++) {
        al_free(*((ALLEGRO_JOYSTICK_ANDROID **)_al_vector_ref(&joysticks, 1)));
        _al_vector_delete_at(&joysticks, 1);
    }

    _jni_callVoidMethod(_al_android_get_jnienv(), _al_android_activity_object(), "reconfigureJoysticks");

    num = _jni_callIntMethodV(_al_android_get_jnienv(), _al_android_activity_object(), "getNumJoysticks", "()I");

    android_init_joysticks(num);

    return true;
}

static int andjoy_num_joysticks(void)
{
    return _al_vector_size(&joysticks);
}

static ALLEGRO_JOYSTICK *andjoy_get_joystick(int num)
{
    ALLEGRO_JOYSTICK_ANDROID *andjoy;
    ALLEGRO_JOYSTICK *joy;

    if (num >= andjoy_num_joysticks())
       return NULL;
    
    andjoy = *((ALLEGRO_JOYSTICK_ANDROID **)_al_vector_ref(&joysticks, num));
    joy = &andjoy->parent;

    return joy;
}

static void andjoy_release_joystick(ALLEGRO_JOYSTICK *joy)
{
    int i;
    int sz;
    (void)joy;

    sz = _al_vector_size(&joysticks);

    for (i = 0; i < sz; i++) {
        al_free(*((ALLEGRO_JOYSTICK_ANDROID **)_al_vector_ref(&joysticks, 0)));
        _al_vector_delete_at(&joysticks, 0);
    }

    _jni_callVoidMethod(_al_android_get_jnienv(), _al_android_activity_object(), "setJoystickInactive");

    ALLEGRO_DEBUG("Joystick released.\n");
    initialized = false;
}

static void andjoy_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
    ALLEGRO_JOYSTICK_ANDROID *andjoy = (void *)joy;
    ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
    int i;
    bool found = false;

    for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
        ALLEGRO_JOYSTICK_ANDROID **ptr = _al_vector_ref(&joysticks, i);
        ALLEGRO_JOYSTICK_ANDROID *thisjoy = *ptr;
        if (andjoy == thisjoy) {
            found = true;
            break;
        }
    }

    if (!found) {
        memset(ret_state, 0, sizeof(*ret_state));
        return;
    }

    _al_event_source_lock(es);
    *ret_state = andjoy->joystate;
    _al_event_source_unlock(es);
}

void _al_android_generate_accelerometer_event(float x, float y, float z)
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

    ALLEGRO_JOYSTICK_ANDROID *accel = *((ALLEGRO_JOYSTICK_ANDROID **)_al_vector_ref(&joysticks, 0));
    ALLEGRO_JOYSTICK *joy = &accel->parent;

    ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

    ALLEGRO_EVENT event;

    _al_event_source_lock(es);

    if (_al_event_source_needs_to_generate_event(es)) {
        float pos[] = {x, y, z};
        int i;
        for (i = 0; i < 3; i++) {
            event.joystick.id = joy;
            event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
            event.joystick.timestamp = al_get_time();
            event.joystick.stick = 0;
            event.joystick.axis = i;
            event.joystick.pos = pos[i];
            event.joystick.button = 0;

            accel->joystate.stick[0].axis[i] = pos[i];

            _al_event_source_emit_event(es, &event);
        }
    }

    _al_event_source_unlock(es);
}

void _al_android_generate_joystick_axis_event(int index, int stick, int axis, float value)
{
    if (!initialized || index >= andjoy_num_joysticks())
        return;

    ALLEGRO_JOYSTICK_ANDROID *joystick = *((ALLEGRO_JOYSTICK_ANDROID **)_al_vector_ref(&joysticks, index));
    ALLEGRO_JOYSTICK *joy = &joystick->parent;

    ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

    ALLEGRO_EVENT event;

    _al_event_source_lock(es);

    if (_al_event_source_needs_to_generate_event(es)) {
        event.joystick.id = joy;
        event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
        event.joystick.timestamp = al_get_time();
        event.joystick.stick = stick;
        event.joystick.axis = axis;
        event.joystick.pos = value;
        event.joystick.button = 0;

        joystick->joystate.stick[stick].axis[axis] = value;

        _al_event_source_emit_event(es, &event);
    }
    _al_event_source_unlock(es);
}

void _al_android_generate_joystick_button_event(int index, int button, bool down)
{
    int type;

    if (!initialized || index >= andjoy_num_joysticks())
        return;

    ALLEGRO_JOYSTICK_ANDROID *joystick = *((ALLEGRO_JOYSTICK_ANDROID **)_al_vector_ref(&joysticks, index));
    ALLEGRO_JOYSTICK *joy = &joystick->parent;

    ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

    ALLEGRO_EVENT event;

    _al_event_source_lock(es);

    if (down)
        type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
    else
        type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;

    if (_al_event_source_needs_to_generate_event(es)) {
        event.joystick.id = joy;
        event.joystick.type = type;
        event.joystick.timestamp = al_get_time();
        event.joystick.stick = 0;
        event.joystick.axis = 0;
        event.joystick.pos = 0;
        event.joystick.button = button;

        joystick->joystate.button[button] = type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN ? 1 : 0;

        _al_event_source_emit_event(es, &event);
    }
    _al_event_source_unlock(es);
}

static char const *andjoy_get_name(ALLEGRO_JOYSTICK *joy)
{
    int i;
    (void)joy;

    for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
        ALLEGRO_JOYSTICK_ANDROID *andjoy = *((ALLEGRO_JOYSTICK_ANDROID **)_al_vector_ref(&joysticks, i));
        if ((ALLEGRO_JOYSTICK *)andjoy == joy)
            return andjoy->name;
    }

    return "";
}

static bool andjoy_get_active(ALLEGRO_JOYSTICK *joy)
{
    (void)joy;
    return true;
}

ALLEGRO_JOYSTICK_DRIVER _al_android_joystick_driver = {
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
