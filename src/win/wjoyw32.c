/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Win32 joystick driver.
 *
 *      By Stefan Schimanski.
 *
 *      Bugfixes by Jose Antonio Luque.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


/* driver functions */
static int joy_init(void);
static void joy_exit(void);
static int joy_poll(void);

static int poll(int *x, int *y, int *x2, int *y2, int poll_mask);


#define JOYSTICK_DRIVER_CONTENTS                      \
   joy_init,                                          \
   joy_exit,                                          \
   joy_poll,                                          \
   NULL, /* static int joy_save_data(void); */        \
   NULL, /* static int joy_load_data(void); */        \
   NULL, /* static char *joy_calibrate_name(int n); */\
   NULL, /* static int joy_calibrate(int n); */

JOYSTICK_DRIVER joystick_win32 =
{
   JOY_TYPE_WIN32,
   empty_string,
   empty_string,
   "Win32 joystick",
   JOYSTICK_DRIVER_CONTENTS
};



/* win32 joystick state */
#define MAXJOYSTICKS 4
#define MAXBUTTONS 16
#define MAXAXES 3

struct WIN32_JOYSTICK {
   int min[MAXAXES];
   int max[MAXAXES];
   int axes[MAXAXES];
   int buttons[MAXBUTTONS];
   int hat;
   int has_hat;
   int buttons_num;
   int axes_num;
   int caps;
   int dev_num;
};

struct WIN32_JOYSTICK _joysticks[MAXJOYSTICKS];

int win32_joy_num = 0;
int win32_joy_attached = 0;

/* old state variables for compatibility */
static int joy_old_x, joy_old_y;
static int joy2_old_x, joy2_old_y;



/* poll_win32_joysticks
 *  Poll the win32 joystick devices
 */
void poll_win32_joysticks()
{
   int n_joy, n_but, n_axis;
   MMRESULT result;
   JOYINFOEX js;

   win32_joy_attached = 0;

   for (n_joy = 0; n_joy < win32_joy_num; n_joy++) {
      js.dwSize = sizeof(js);
      js.dwFlags = JOY_RETURNALL;

      result = joyGetPosEx( (_joysticks[n_joy].dev_num), &js);
      if (result == JOYERR_NOERROR) {
	 win32_joy_attached++;

	 /* axes */
	 _joysticks[n_joy].axes[0] = js.dwXpos;
	 _joysticks[n_joy].axes[1] = js.dwYpos;

	 n_axis = 2;
	 if (_joysticks[n_joy].caps & JOYCAPS_HASZ) 
	    { _joysticks[n_joy].axes[n_axis] = js.dwZpos; n_axis++; }

	 if (_joysticks[n_joy].caps & JOYCAPS_HASR) 
	    { _joysticks[n_joy].axes[n_axis] = js.dwRpos; n_axis++; }

	 if (_joysticks[n_joy].caps & JOYCAPS_HASU) 
	    { _joysticks[n_joy].axes[n_axis] = js.dwUpos; n_axis++; }

	 if (_joysticks[n_joy].caps & JOYCAPS_HASV) 
	    { _joysticks[n_joy].axes[n_axis] = js.dwVpos; n_axis++; }

	 /* hat */
	 switch (js.dwPOV) {
	    case JOY_POVLEFT:
	       _joysticks[n_joy].hat = 1;
	       break;
	    case JOY_POVBACKWARD:
	       _joysticks[n_joy].hat = 2;
	       break;
	    case JOY_POVRIGHT:
	       _joysticks[n_joy].hat = 3;
	       break;
	    case JOY_POVFORWARD:
	       _joysticks[n_joy].hat = 4;
	       break;
	    default:
	       _joysticks[n_joy].hat = 0;
	 }

	 /* buttons */
	 for (n_but = 0; n_but < _joysticks[n_joy].buttons_num; n_but++)
	    _joysticks[n_joy].buttons[n_but] = ((js.dwButtons & (1 << n_but)) != 0);

	 for (; n_but < MAXBUTTONS; n_but++)
	    _joysticks[n_joy].buttons[n_but] = 0;
      }
      else {
	 _joysticks[n_joy].axes[0] = 0;
	 _joysticks[n_joy].axes[1] = 0;
	 _joysticks[n_joy].axes[2] = 0;
	 _joysticks[n_joy].hat = 0;

	 for (n_but = 0; n_but < MAXBUTTONS; n_but++)
	    _joysticks[n_joy].buttons[n_but] = 0;
      }
   }
}



/* joy_poll:
 *  Updates the joystick status variables.
 */
static int joy_poll()
{
   int n_joy, n_stick=0, win32_axis, n_axes, n_but, p, range, h, v, num_sticks;

   poll_win32_joysticks();

   joy_old_x = joy[0].stick[0].axis[0].pos;
   joy_old_y = joy[0].stick[0].axis[1].pos;

   joy2_old_x = joy[1].stick[0].axis[0].pos;
   joy2_old_y = joy[1].stick[0].axis[1].pos;

   for (n_joy = 0; n_joy < num_joysticks; n_joy++) {
      /* buttons */
      for (n_but = 0; n_but < _joysticks[n_joy].buttons_num; n_but++)
	 joy[n_joy].button[n_but].b = _joysticks[n_joy].buttons[n_but];

      /* map win32 axes to Allegro sticks */
      win32_axis = 0;
      while (win32_axis < _joysticks[n_joy].axes_num) {
	 /* skip hat at this point, it's handled later */
	 if (_joysticks[n_joy].has_hat)
	    num_sticks = joy[n_joy].num_sticks - 1;
	 else
	    num_sticks = joy[n_joy].num_sticks;

	 for (n_stick = 0; n_stick < num_sticks; n_stick++) {

	    for (n_axes = 0; n_axes < joy[n_joy].stick[n_stick].num_axis; n_axes++) {
	       /* map Windows axis range to 0-256 Allegro range */
	       p = _joysticks[n_joy].axes[win32_axis] - _joysticks[n_joy].min[win32_axis];
	       range = _joysticks[n_joy].max[win32_axis] - _joysticks[n_joy].min[win32_axis];
	       if (range > 0)
		  p = p * 256 / range;
	       else
		  p = 0;

	       /* set pops of analog stick */
	       if (joy[n_joy].stick[n_stick].flags | JOYFLAG_ANALOGUE) {
		  if (joy[n_joy].stick[n_stick].flags | JOYFLAG_SIGNED)
		     joy[n_joy].stick[n_stick].axis[n_axes].pos = p - 128;
		  else
		     joy[n_joy].stick[n_stick].axis[n_axes].pos = p;
	       }

	       /* set pos digital stick */
	       if (joy[n_joy].stick[n_stick].flags | JOYFLAG_DIGITAL) {
		  if (p < 64)
		     joy[n_joy].stick[n_stick].axis[n_axes].d1 = 1;
		  else
		     joy[n_joy].stick[n_stick].axis[n_axes].d1 = 0;

		  if (p > 192)
		     joy[n_joy].stick[n_stick].axis[n_axes].d2 = 1;
		  else
		     joy[n_joy].stick[n_stick].axis[n_axes].d2 = 0;
	       }

	       win32_axis++;
	    }
	 }
      }

      /* hat */
      if (_joysticks[n_joy].has_hat) {
	 joy[n_joy].stick[n_stick].axis[0].d1 = _joysticks[n_joy].hat == 1;
	 joy[n_joy].stick[n_stick].axis[0].d2 = _joysticks[n_joy].hat == 3;
	 joy[n_joy].stick[n_stick].axis[1].d1 = _joysticks[n_joy].hat == 4;
	 joy[n_joy].stick[n_stick].axis[1].d2 = _joysticks[n_joy].hat == 2;

	 /* emulate analog joystick */
	 h = 0;
	 v = 0;
	 switch (_joysticks[n_joy].hat) {
	    case 1:
	       h = -128;
	       break;
	    case 3:
	       h = +128;
	       break;
	    case 4:
	       v = +128;
	       break;
	    case 2:
	       h = -128;
	       break;
	 }

	 joy[n_joy].stick[n_stick].axis[0].pos = h;
	 joy[n_joy].stick[n_stick].axis[0].pos = v;

	 n_stick++;
      }
   }

   return 0;
}



/* init_win32_joysticks
 *  Initialise win32 joysticks driver
 */
int init_win32_joysticks()
{
   JOYCAPS caps;
   int result;
   int n_joy, n_axes;
   int n_joyat;

   win32_joy_num = joyGetNumDevs();

   if (win32_joy_num > MAXJOYSTICKS) {
      _TRACE("More than four joysticks found\n");
   }

   /* get joystick calibration */
   n_joy = 0;
   for (n_joyat = 0; n_joyat < win32_joy_num; n_joyat++) {
      if ( (n_joy+1) > MAXJOYSTICKS) break;
      result = joyGetDevCaps(n_joyat, &caps, sizeof(caps));
      if (result == JOYERR_NOERROR) {
	 _joysticks[n_joy].dev_num = n_joyat;
	 _joysticks[n_joy].caps = caps.wCaps;
	 _joysticks[n_joy].buttons_num = (caps.wNumButtons>MAXBUTTONS ? MAXBUTTONS : caps.wNumButtons);
	 _joysticks[n_joy].axes_num = caps.wNumAxes;
	 _TRACE("Joystick caps: 0x%x\n", caps.wCaps);

	 for (n_axes = 0; n_axes < MAXAXES; n_axes++) {
	    _joysticks[n_joy].min[n_axes] = 0;
	    _joysticks[n_joy].max[n_axes] = 0;
	 }

	 /* fill in ranges of axes */
	 _joysticks[n_joy].min[0] = caps.wXmin;
	 _joysticks[n_joy].max[0] = caps.wXmax;

	 _joysticks[n_joy].min[1] = caps.wYmin;
	 _joysticks[n_joy].max[1] = caps.wYmax;

	 n_axes = 2;
	 if (caps.wCaps & JOYCAPS_HASZ)
	 {
	    _joysticks[n_joy].min[n_axes] = caps.wZmin;
	    _joysticks[n_joy].max[n_axes] = caps.wZmax;
	    n_axes++;
	 }

	 if (caps.wCaps & JOYCAPS_HASR)
	 {
	    _joysticks[n_joy].min[n_axes] = caps.wRmin;
	    _joysticks[n_joy].max[n_axes] = caps.wRmax;
	    n_axes++;
	 }

	 if (caps.wCaps & JOYCAPS_HASU)
	 {
	    _joysticks[n_joy].min[n_axes] = caps.wUmin;
	    _joysticks[n_joy].max[n_axes] = caps.wUmax;
	    n_axes++;
	 }

	 if (caps.wCaps & JOYCAPS_HASV)
	 {
	    _joysticks[n_joy].min[n_axes] = caps.wVmin;
	    _joysticks[n_joy].max[n_axes] = caps.wVmax;
	    n_axes++;
	 }

	 if (caps.wCaps & JOYCAPS_HASPOV)
	    _joysticks[n_joy].has_hat = 1;
	 else
	    _joysticks[n_joy].has_hat = 0;

     n_joy++;
      }
   }
   win32_joy_num = n_joy;
   /* poll joysticks */
   joy_poll();

   return !(win32_joy_attached > 0);
}



/* joy_init:
 *  Initialises the driver.
 */
static int joy_init()
{
   static char name_x[] = "X";
   static char name_y[] = "Y";
   static char name_z[] = "Z";
   static char name_stick[] = "stick";
   static char name_hat[] = "hat";
   static char name_throttle[] = "throttle";
   static char *name_b[MAXBUTTONS] = {"B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8",
                                      "B9", "B10", "B11", "B12", "B13", "B14", "B15", "B16"};
   int n_but;
   int n_stick, n_joy, n_axes;

   if (init_win32_joysticks() != 0)
      return -1;

   /* store info about the stick type */
   joy_old_x = joy_old_y = 0;
   joy2_old_x = joy2_old_y = 0;

   /* fill in the joystick structure */
   num_joysticks = win32_joy_attached;

   joy[0].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE;

   /* how many buttons? */
   for (n_joy = 0; n_joy < num_joysticks; n_joy++) {
      joy[n_joy].num_buttons = _joysticks[n_joy].buttons_num;
      joy[n_joy].flags = JOYFLAG_ANALOGUE | JOYFLAG_DIGITAL;

      /* main analogue stick */
      if (_joysticks[n_joy].axes_num > 0) {
	 n_stick = 0;
	 n_axes = 0;

	 if (_joysticks[n_joy].axes_num > 1) {
	    joy[n_joy].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[n_joy].stick[n_stick].axis[0].name = name_x;
	    joy[n_joy].stick[n_stick].axis[1].name = name_y;
	    joy[n_joy].stick[n_stick].name = name_stick;

	    if (_joysticks[n_joy].caps & JOYCAPS_HASZ)
	    {
	       joy[n_joy].stick[n_stick].num_axis = 3;
	       joy[n_joy].stick[n_stick].axis[2].name = name_z;
	       n_axes += 3;
	    } else
	    {
	       joy[n_joy].stick[n_stick].num_axis = 2;
	       n_axes += 2;
	    }

	    n_stick++;
	 }

	 for (; n_axes < _joysticks[n_joy].axes_num;) {
	    joy[n_joy].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
	    joy[n_joy].stick[n_stick].num_axis = 1;
	    joy[n_joy].stick[n_stick].axis[0].name = "";
	    joy[n_joy].stick[n_stick].name = name_throttle;
	    n_stick++;
	    n_axes++;
	 }

	 if (_joysticks[n_joy].has_hat) {
	    joy[n_joy].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
	    joy[n_joy].stick[n_stick].num_axis = 2;
	    joy[n_joy].stick[n_stick].axis[0].name = "left/right";
	    joy[n_joy].stick[n_stick].axis[1].name = "up/down";
	    joy[n_joy].stick[n_stick].name = name_hat;
	    n_stick++;
	 }

	 joy[n_joy].num_sticks = n_stick;
      }
      else {
	 joy[n_joy].num_sticks = 0;
      }

      /* fill in the button names */
      for (n_but = 0; n_but < joy[n_joy].num_buttons; n_but++)
	 joy[n_joy].button[n_but].name = name_b[n_but];

   }

   return 0;
}



/* joy_exit:
 *  Shuts down the driver.
 */
static void joy_exit()
{
   win32_joy_num = 0;
}



/* calibrate_joystick_tl:
 *  For backward compatibility with the old API.
 */
int calibrate_joystick_tl()
{
   return 0;
}



/* calibrate_joystick_br:
 *  For backward compatibility with the old API.
 */
int calibrate_joystick_br()
{
   return 0;
}



/* calibrate_joystick_throttle_min:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "minimum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_max().
 */
int calibrate_joystick_throttle_min()
{
   return 0;
}



/* calibrate_joystick_throttle_max:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "maximum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_min().
 */
int calibrate_joystick_throttle_max()
{
   return 0;
}



/* calibrate_joystick_hat:
 *  For access to the Wingman Extreme's hat (I think this will work on all 
 *  Thrustmaster compatible joysticks), call this after initialise_joystick(), 
 *  passing the JOY_HAT constant you wish to calibrate.
 */
int calibrate_joystick_hat(int direction)
{
   return 0;
}
