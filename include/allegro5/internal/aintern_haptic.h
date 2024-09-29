#ifndef __al_included_allegro5_aintern_haptic_h
#define __al_included_allegro5_aintern_haptic_h

#include "allegro5/haptic.h"

#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)

typedef struct A5O_HAPTIC_DRIVER
{
   int hapdrv_id;
   const char *hapdrv_name;
   const char *hapdrv_desc;
   const char *hapdrv_ascii_name;
   AL_METHOD(bool, init_haptic, (void));
   AL_METHOD(void, exit_haptic, (void));

   AL_METHOD(bool, is_mouse_haptic, (A5O_MOUSE *));
   AL_METHOD(bool, is_joystick_haptic, (A5O_JOYSTICK *));
   AL_METHOD(bool, is_keyboard_haptic, (A5O_KEYBOARD *));
   AL_METHOD(bool, is_display_haptic, (A5O_DISPLAY *));
   AL_METHOD(bool, is_touch_input_haptic, (A5O_TOUCH_INPUT *));

   AL_METHOD(A5O_HAPTIC *, get_from_mouse, (A5O_MOUSE *));
   AL_METHOD(A5O_HAPTIC *, get_from_joystick, (A5O_JOYSTICK *));
   AL_METHOD(A5O_HAPTIC *, get_from_keyboard, (A5O_KEYBOARD *));
   AL_METHOD(A5O_HAPTIC *, get_from_display, (A5O_DISPLAY *));
   AL_METHOD(A5O_HAPTIC *, get_from_touch_input, (A5O_TOUCH_INPUT *));

   AL_METHOD(bool, get_active, (A5O_HAPTIC *));
   AL_METHOD(int, get_capabilities, (A5O_HAPTIC *));
   AL_METHOD(double, get_gain, (A5O_HAPTIC *));
   AL_METHOD(bool, set_gain, (A5O_HAPTIC *, double));
   AL_METHOD(int, get_max_effects, (A5O_HAPTIC *));

   AL_METHOD(bool, is_effect_ok, (A5O_HAPTIC *, A5O_HAPTIC_EFFECT *));
   AL_METHOD(bool, upload_effect, (A5O_HAPTIC *, A5O_HAPTIC_EFFECT *,
                                   A5O_HAPTIC_EFFECT_ID *));
   AL_METHOD(bool, play_effect, (A5O_HAPTIC_EFFECT_ID *, int));
   AL_METHOD(bool, stop_effect, (A5O_HAPTIC_EFFECT_ID *));
   AL_METHOD(bool, is_effect_playing, (A5O_HAPTIC_EFFECT_ID *));
   AL_METHOD(bool, release_effect, (A5O_HAPTIC_EFFECT_ID *));
   AL_METHOD(bool, release, (A5O_HAPTIC *));
   AL_METHOD(double, get_autocenter, (A5O_HAPTIC *));
   AL_METHOD(bool, set_autocenter, (A5O_HAPTIC *, double));
} A5O_HAPTIC_DRIVER;


enum A5O_HAPTIC_PARENT
{
   _AL_HAPTIC_FROM_JOYSTICK = 1,
   _AL_HAPTIC_FROM_MOUSE,
   _AL_HAPTIC_FROM_KEYBOARD,
   _AL_HAPTIC_FROM_DISPLAY,
   _AL_HAPTIC_FROM_TOUCH_INPUT
};

/* haptic has a driver field for per-device drivers on some platforms. */
struct A5O_HAPTIC
{
   enum A5O_HAPTIC_PARENT from;
   void *device;
   double gain;
   double autocenter;
   A5O_HAPTIC_DRIVER *driver;
};

/* Haptic driver list. */
extern const _AL_DRIVER_INFO _al_haptic_driver_list[];

/* Macros for constructing the driver list */
#define _AL_BEGIN_HAPTIC_DRIVER_LIST                                 \
   const _AL_DRIVER_INFO _al_haptic_driver_list[] =                  \
   {
#define _AL_END_HAPTIC_DRIVER_LIST                                   \
   { 0, NULL, false }                                                \
   };

#else

/* Forward declare it for A5O_SYSTEM_INTERFACE. */
typedef struct A5O_HAPTIC_DRIVER A5O_HAPTIC_DRIVER;

#endif

#ifdef __cplusplus
}
#endif

#endif

/* vim: set sts=3 sw=3 et: */
