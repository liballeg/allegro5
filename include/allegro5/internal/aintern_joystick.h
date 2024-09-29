#ifndef __al_included_allegro5_aintern_joystick_h
#define __al_included_allegro5_aintern_joystick_h

#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"

#ifdef __cplusplus
   extern "C" {
#endif


typedef struct A5O_JOYSTICK_DRIVER
{
   int  joydrv_id;
   const char *joydrv_name;
   const char *joydrv_desc;
   const char *joydrv_ascii_name;
   AL_METHOD(bool, init_joystick, (void));
   AL_METHOD(void, exit_joystick, (void));
   AL_METHOD(bool, reconfigure_joysticks, (void));
   AL_METHOD(int, num_joysticks, (void));
   AL_METHOD(A5O_JOYSTICK *, get_joystick, (int joyn));
   AL_METHOD(void, release_joystick, (A5O_JOYSTICK *joy));
   AL_METHOD(void, get_joystick_state, (A5O_JOYSTICK *joy, A5O_JOYSTICK_STATE *ret_state));
   AL_METHOD(const char *, get_name, (A5O_JOYSTICK *joy));
   AL_METHOD(bool, get_active, (A5O_JOYSTICK *joy));
} A5O_JOYSTICK_DRIVER;


extern _AL_DRIVER_INFO _al_joystick_driver_list[];


/* macros for constructing the driver list */
#define _AL_BEGIN_JOYSTICK_DRIVER_LIST                         \
   _AL_DRIVER_INFO _al_joystick_driver_list[] =                \
   {

#define _AL_END_JOYSTICK_DRIVER_LIST                           \
      {  0,                NULL,                false }        \
   };


/* information about a single joystick axis */
typedef struct _AL_JOYSTICK_AXIS_INFO
{
   char *name;
} _AL_JOYSTICK_AXIS_INFO;


/* information about one or more axis (a slider or directional control) */
typedef struct _AL_JOYSTICK_STICK_INFO
{
   int flags; /* bit-field */
   int num_axes;
   _AL_JOYSTICK_AXIS_INFO axis[_AL_MAX_JOYSTICK_AXES];
   char *name;
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


/* Joystick has a driver field for per-device drivers,
 * needed on some platforms. */
struct A5O_JOYSTICK
{
   _AL_JOYSTICK_INFO info;
   A5O_JOYSTICK_DRIVER * driver;
};

void _al_generate_joystick_event(A5O_EVENT *event);

#ifdef __cplusplus
   }
#endif

#endif

/* vi ts=8 sts=3 sw=3 et */
