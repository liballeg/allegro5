#ifndef _al_included_aintern_joystick_h
#define _al_included_aintern_joystick_h

#include "allegro5/internal/aintern_events.h"

AL_BEGIN_EXTERN_C


typedef struct ALLEGRO_JOYSTICK_DRIVER
{
   int  joydrv_id;
   const char *joydrv_name;
   const char *joydrv_desc;
   const char *joydrv_ascii_name;
   AL_METHOD(bool, init_joystick, (void));
   AL_METHOD(void, exit_joystick, (void));
   AL_METHOD(int, num_joysticks, (void));
   AL_METHOD(ALLEGRO_JOYSTICK*, get_joystick, (int joyn));
   AL_METHOD(void, release_joystick, (ALLEGRO_JOYSTICK*));
   AL_METHOD(void, get_joystick_state, (ALLEGRO_JOYSTICK*, ALLEGRO_JOYSTATE *ret_state));
} ALLEGRO_JOYSTICK_DRIVER;


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
   const char *name;
} _AL_JOYSTICK_AXIS_INFO;


/* information about one or more axis (a slider or directional control) */
typedef struct _AL_JOYSTICK_STICK_INFO
{
   ALLEGRO_JOYFLAGS flags;
   int num_axes;
   _AL_JOYSTICK_AXIS_INFO axis[_AL_MAX_JOYSTICK_AXES];
   const char *name;
} _AL_JOYSTICK_STICK_INFO;


/* information about a joystick button */
typedef struct _AL_JOYSTICK_BUTTON_INFO
{
   const char *name;
} _AL_JOYSTICK_BUTTON_INFO;


/* information about an entire joystick */
typedef struct _AL_JOYSTICK_INFO
{
   int num_sticks;
   int num_buttons;
   _AL_JOYSTICK_STICK_INFO stick[_AL_MAX_JOYSTICK_STICKS];
   _AL_JOYSTICK_BUTTON_INFO button[_AL_MAX_JOYSTICK_BUTTONS];
} _AL_JOYSTICK_INFO;


struct ALLEGRO_JOYSTICK
{
   ALLEGRO_EVENT_SOURCE es;
   _AL_JOYSTICK_INFO info;
   int num;
};


AL_END_EXTERN_C

#endif

/* vi ts=8 sts=3 sw=3 et */
