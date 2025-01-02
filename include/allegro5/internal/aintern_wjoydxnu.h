#ifndef __al_included_allegro_aintern_wjoydxnu_h
#define __al_included_allegro_aintern_wjoydxnu_h


/** Part of the Windows DirectInput joystick
 * types are shared here for use by the haptic susbystem. */

/* arbitrary limit */
#define MAX_JOYSTICKS        32

/* these limits are from DIJOYSTICK_STATE in dinput.h */
#define MAX_SLIDERS          2
#define MAX_POVS             4
#define MAX_BUTTONS          32

/* the number of joystick events that DirectInput is told to buffer */
#define DEVICE_BUFFER_SIZE   128

/* make sure all the constants add up */
/* the first two sticks are (x,y,z) and (rx,ry,rz) */
ALLEGRO_STATIC_ASSERT(wjoydxnu, _AL_MAX_JOYSTICK_STICKS >= (2 + MAX_SLIDERS + MAX_POVS));
ALLEGRO_STATIC_ASSERT(wjoydxnu, _AL_MAX_JOYSTICK_BUTTONS >= MAX_BUTTONS);


#define GUID_EQUAL(a, b)     (0 == memcmp(&(a), &(b), sizeof(GUID)))


typedef enum
{
   STATE_UNUSED,
   STATE_BORN,
   STATE_ALIVE,
   STATE_DYING
} CONFIG_STATE;

#define ACTIVE_STATE(st) \
   ((st) == STATE_ALIVE || (st) == STATE_DYING)


/* helper structure to record information through object_enum_callback */
#define NAME_LEN     128

typedef struct
{
   bool have_x;      TCHAR name_x[NAME_LEN];
   bool have_y;      TCHAR name_y[NAME_LEN];
   bool have_z;      TCHAR name_z[NAME_LEN];
   bool have_rx;     TCHAR name_rx[NAME_LEN];
   bool have_ry;     TCHAR name_ry[NAME_LEN];
   bool have_rz;     TCHAR name_rz[NAME_LEN];
   int num_sliders;  TCHAR name_slider[MAX_SLIDERS][NAME_LEN];
   int num_povs;     TCHAR name_pov[MAX_POVS][NAME_LEN];
   int num_buttons;  TCHAR name_button[MAX_BUTTONS][NAME_LEN];
} CAPS_AND_NAMES;


typedef struct ALLEGRO_JOYSTICK_DIRECTX
{
   ALLEGRO_JOYSTICK parent;          /* must be first */
   CONFIG_STATE config_state;
   bool marked;
   LPDIRECTINPUTDEVICE2 device;
   GUID guid;
   GUID product_guid;
   HANDLE waker_event;

   ALLEGRO_JOYSTICK_STATE joystate;
   _AL_JOYSTICK_OUTPUT x_mapping;
   _AL_JOYSTICK_OUTPUT y_mapping;
   _AL_JOYSTICK_OUTPUT z_mapping;
   _AL_JOYSTICK_OUTPUT rx_mapping;
   _AL_JOYSTICK_OUTPUT ry_mapping;
   _AL_JOYSTICK_OUTPUT rz_mapping;
   _AL_JOYSTICK_OUTPUT slider_mapping[MAX_SLIDERS];
   /* The X/Y directions are interleaved. */
   _AL_JOYSTICK_OUTPUT pov_mapping[2 * MAX_POVS];
   _AL_JOYSTICK_OUTPUT button_mapping[MAX_BUTTONS];
   char name[100];
} ALLEGRO_JOYSTICK_DIRECTX;



#ifdef __cplusplus
extern "C" {
#endif
  void _al_win_joystick_dinput_trigger_enumeration(void);
#ifdef __cplusplus
}
#endif


#endif

/* vim: set sts=3 sw=3 et: */
