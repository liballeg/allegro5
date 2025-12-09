#ifndef __al_included_allegro5_aintern_joystick_h
#define __al_included_allegro5_aintern_joystick_h

#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_events.h"

#ifdef __cplusplus
   extern "C" {
#endif


typedef struct ALLEGRO_JOYSTICK_DRIVER
{
   int  joydrv_id;
   const char *joydrv_name;
   const char *joydrv_desc;
   const char *joydrv_ascii_name;
   AL_METHOD(bool, init_joystick, (void));
   AL_METHOD(void, exit_joystick, (void));
   AL_METHOD(bool, reconfigure_joysticks, (void));
   AL_METHOD(int, num_joysticks, (void));
   AL_METHOD(ALLEGRO_JOYSTICK *, get_joystick, (int joyn));
   AL_METHOD(void, release_joystick, (ALLEGRO_JOYSTICK *joy));
   AL_METHOD(void, get_joystick_state, (ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state));
   AL_METHOD(const char *, get_name, (ALLEGRO_JOYSTICK *joy));
   AL_METHOD(bool, get_active, (ALLEGRO_JOYSTICK *joy));
} ALLEGRO_JOYSTICK_DRIVER;


extern _AL_DRIVER_INFO _al_joystick_driver_list[];


/* macros for constructing the driver list */
#define _AL_BEGIN_JOYSTICK_DRIVER_LIST                         \
   _AL_DRIVER_INFO _al_joystick_driver_list[] =                \
   {

#define _AL_END_JOYSTICK_DRIVER_LIST                           \
      {  0,                NULL,                false }        \
   };


/* Output mapping of a joystick button/stick/hat.
 * The output button/stick/axes are the things surfaced to the user via the
 * Allegro API.
 */
typedef struct _AL_JOYSTICK_OUTPUT {
   int in_idx;             /* The occurence index of a particular input kind.
                            * E.g. the second button seen by the driver has
                            * in_idx == 1.
                            */
   int offset_override;

   int button;             /* The output is a button. */
   bool button_enabled;    /* Button output is enabled. */
   float button_threshold; /* When an axis is mapped to a button, when to
                            * trigger the button press. Typically this is
                            * halfway through the axis range. */

   int pos_stick;          /* The stick mapping for the positive deflection
                            * of the input axis. */
   int pos_axis;
   float pos_min;          /* What output value the minimum/maximum deflection
                            * maps to. */
   float pos_max;
   bool pos_enabled;       /* Positive deflection output is enabled. */

   int neg_stick;
   int neg_axis;
   float neg_min;
   float neg_max;
   bool neg_enabled;
} _AL_JOYSTICK_OUTPUT;



typedef struct _AL_JOYSTICK_MAPPING {
   ALLEGRO_JOYSTICK_GUID guid;
   char name[100];
   char platform[17];

   _AL_VECTOR button_map;
   _AL_VECTOR axis_map;
   _AL_VECTOR hat_map;
} _AL_JOYSTICK_MAPPING;


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
   char *name;
} _AL_JOYSTICK_BUTTON_INFO;


/* information about an entire joystick */
typedef struct _AL_JOYSTICK_INFO
{
   ALLEGRO_JOYSTICK_TYPE type;
   ALLEGRO_JOYSTICK_GUID guid;
   int num_sticks;
   int num_buttons;
   _AL_JOYSTICK_STICK_INFO stick[_AL_MAX_JOYSTICK_STICKS];
   _AL_JOYSTICK_BUTTON_INFO button[_AL_MAX_JOYSTICK_BUTTONS];
} _AL_JOYSTICK_INFO;


/* Joystick has a driver field for per-device drivers,
 * needed on some platforms. */
struct ALLEGRO_JOYSTICK
{
   _AL_JOYSTICK_INFO info;
   ALLEGRO_JOYSTICK_DRIVER * driver;
};

struct ALLEGRO_JOYSTICK_STATE;


void _al_generate_joystick_event(ALLEGRO_EVENT *event);
void _al_fill_gamepad_info(_AL_JOYSTICK_INFO *info);
void _al_destroy_joystick_info(_AL_JOYSTICK_INFO *info);
_AL_JOYSTICK_OUTPUT * _al_get_joystick_output(const _AL_VECTOR *map, int in_idx);
const _AL_JOYSTICK_MAPPING *_al_get_gamepad_mapping(const char *platform, ALLEGRO_JOYSTICK_GUID guid);
ALLEGRO_JOYSTICK_GUID _al_new_joystick_guid(uint16_t bus, uint16_t vendor, uint16_t product, uint16_t version,
      const char *vendor_name, const char *product_name, uint8_t driver_signature, uint8_t driver_data);
void _al_joystick_guid_to_str(ALLEGRO_JOYSTICK_GUID guid, char* str);
ALLEGRO_JOYSTICK_GUID _al_joystick_guid_from_str(const char *str);
_AL_JOYSTICK_OUTPUT _al_new_joystick_button_output(int button);
_AL_JOYSTICK_OUTPUT _al_new_joystick_stick_output(int stick, int axis);
void _al_joystick_generate_button_event(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *joystate, _AL_JOYSTICK_OUTPUT output, int value);
void _al_joystick_generate_axis_event(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *joystate, _AL_JOYSTICK_OUTPUT output, float pos);

#ifdef __cplusplus
   }
#endif

#endif

/* vi ts=8 sts=3 sw=3 et */
