#ifndef _al_included_aintern_joystick_h
#define _al_included_aintern_joystick_h

#include "allegro/internal/aintern_events.h"

AL_BEGIN_EXTERN_C


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


AL_END_EXTERN_C

#endif

/* vi ts=8 sts=3 sw=3 et */
