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
 *      Bugfixes and enhanced POV support by Johan Peitz.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


static int joy_init(void);
static void joy_exit(void);
static int joy_poll(void);


JOYSTICK_DRIVER joystick_win32 =
{
   JOY_TYPE_WIN32,
   empty_string,
   empty_string,
   "Win32 joystick",
   joy_init,
   joy_exit,
   joy_poll,
   NULL,
   NULL,
   NULL,
   NULL
};


/* bitflags for hat directions */
#define HAT_FORWARD    0x0001
#define HAT_RIGHT      0x0002
#define HAT_BACKWARD   0x0004
#define HAT_LEFT       0x0008

/* the directions of the POV are based on degrees (0-359) but the
 * Win32 joystick API (mmsystem.h) defines only 4 directions:	
 *
 *   #define JOY_POVFORWARD        0
 *   #define JOY_POVRIGHT       9000
 *   #define JOY_POVBACKWARD   18000
 *   #define JOY_POVLEFT       27000
 *
 * therefore we need to define constants for the diagonals as well:
 */
#define JOY_POVFORWARDRIGHT     4500
#define JOY_POVBACKWARDRIGHT   13500
#define JOY_POVBACKWARDLEFT    22500
#define JOY_POVFORWARDLEFT     31500


/* Win32 joystick state */
#define WIN32_MAX_AXES   6

struct WIN32_JOYSTICK {
   int dev_num;
   int caps;
   int axes_num;
   int min[WIN32_MAX_AXES];
   int max[WIN32_MAX_AXES];
   int axis[WIN32_MAX_AXES];
   int has_hat;
   int hat;
   int buttons_num;
   int button[MAX_JOYSTICK_BUTTONS];
};

struct WIN32_JOYSTICK win32_joystick[MAX_JOYSTICKS];

static int win32_joy_num = 0;
static int win32_joy_attached = 0;

static char name_x[] = "X";
static char name_y[] = "Y";
static char name_stick[] = "stick";
static char name_throttle[] = "throttle";
static char name_rudder[] = "rudder";
static char name_slider[] = "slider";
static char name_hat[] = "hat";
static char *name_b[MAX_JOYSTICK_BUTTONS] = {
   "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8",
   "B9", "B10", "B11", "B12", "B13", "B14", "B15", "B16"
};



/* poll_win32_joysticks:
 *  poll the Win32 joystick devices
 */
static void poll_win32_joysticks(void)
{
   int n_joy, n_axis, n_but;
   JOYINFOEX js;

   win32_joy_attached = 0;

   for (n_joy = 0; n_joy < win32_joy_num; n_joy++) {
      js.dwSize = sizeof(js);
      js.dwFlags = JOY_RETURNALL;

      if (joyGetPosEx(win32_joystick[n_joy].dev_num, &js) == JOYERR_NOERROR) {
	 win32_joy_attached++;

	 /* axes */
	 win32_joystick[n_joy].axis[0] = js.dwXpos;
	 win32_joystick[n_joy].axis[1] = js.dwYpos;
	 n_axis = 2;

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASZ) {
            win32_joystick[n_joy].axis[n_axis] = js.dwZpos;
            n_axis++;
         }

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASR) {
            win32_joystick[n_joy].axis[n_axis] = js.dwRpos;
            n_axis++;
         }

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASU) {
            win32_joystick[n_joy].axis[n_axis] = js.dwUpos;
            n_axis++;
         }

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASV) {
            win32_joystick[n_joy].axis[n_axis] = js.dwVpos;
            n_axis++;
         }

	 /* hat */
         if (win32_joystick[n_joy].has_hat) {
            switch (js.dwPOV) {

               case JOY_POVFORWARD:
                  win32_joystick[n_joy].hat = HAT_FORWARD;
                  break;

               case JOY_POVFORWARDRIGHT:
                  win32_joystick[n_joy].hat = HAT_FORWARD | HAT_RIGHT;
                  break;

               case JOY_POVRIGHT:
                  win32_joystick[n_joy].hat = HAT_RIGHT;
                  break;

               case JOY_POVBACKWARDRIGHT:
                  win32_joystick[n_joy].hat = HAT_BACKWARD | HAT_RIGHT;
                  break;

               case JOY_POVBACKWARD:
                  win32_joystick[n_joy].hat = HAT_BACKWARD;
                  break;

               case JOY_POVBACKWARDLEFT:
                  win32_joystick[n_joy].hat = HAT_BACKWARD | HAT_LEFT;
                  break;

               case JOY_POVLEFT:
                  win32_joystick[n_joy].hat = HAT_LEFT;
                  break;

               case JOY_POVFORWARDLEFT:
                  win32_joystick[n_joy].hat = HAT_FORWARD | HAT_LEFT;
                  break;

               case JOY_POVCENTERED:
               default:
                  win32_joystick[n_joy].hat = 0;
                  break;
            }
         }

	 /* buttons */
	 for (n_but = 0; n_but < win32_joystick[n_joy].buttons_num; n_but++)
	    win32_joystick[n_joy].button[n_but] = ((js.dwButtons & (1 << n_but)) != 0);
      }
      else {
         for(n_axis = 0; n_axis<win32_joystick[n_joy].axes_num; n_axis++) 
            win32_joystick[n_joy].axis[n_axis] = 0;

         if (win32_joystick[n_joy].has_hat)
            win32_joystick[n_joy].hat = 0;

	 for (n_but = 0; n_but < win32_joystick[n_joy].buttons_num; n_but++)
	    win32_joystick[n_joy].button[n_but] = FALSE;
      }
   }
}



/* joy_poll:
 *  updates the joystick status variables
 */
static int joy_poll(void)
{
   int win32_axis, num_sticks;
   int n_joy, n_stick, n_axis, n_but, p, range;

   /* update the Win32 joysticks status */
   poll_win32_joysticks();

   /* translate Win32 joystick status to Allegro joystick status */
   for (n_joy = 0; n_joy < num_joysticks; n_joy++) {

      /* sticks */
      n_stick = 0;
      win32_axis = 0;
      while (win32_axis < win32_joystick[n_joy].axes_num) {
	 /* skip hat at this point, it's handled later */
	 if (win32_joystick[n_joy].has_hat)
	    num_sticks = joy[n_joy].num_sticks - 1;
	 else
	    num_sticks = joy[n_joy].num_sticks;

	 for (n_stick = 0; n_stick < num_sticks; n_stick++) {
	    for (n_axis = 0; n_axis < joy[n_joy].stick[n_stick].num_axis; n_axis++) {
	       /* map Windows axis range to 0-256 Allegro range */
	       p = win32_joystick[n_joy].axis[win32_axis] - win32_joystick[n_joy].min[win32_axis];
	       range = win32_joystick[n_joy].max[win32_axis] - win32_joystick[n_joy].min[win32_axis];
	       if (range > 0)
		  p = p * 256 / range;
	       else
		  p = 0;

	       /* set pos of analog stick */
	       if (joy[n_joy].stick[n_stick].flags & JOYFLAG_ANALOGUE) {
		  if (joy[n_joy].stick[n_stick].flags & JOYFLAG_SIGNED)
		     joy[n_joy].stick[n_stick].axis[n_axis].pos = p - 128;
		  else
		     joy[n_joy].stick[n_stick].axis[n_axis].pos = p;
	       }

	       /* set pos of digital stick */
	       if (joy[n_joy].stick[n_stick].flags & JOYFLAG_DIGITAL) {
		  if (p < 64)
		     joy[n_joy].stick[n_stick].axis[n_axis].d1 = TRUE;
		  else
		     joy[n_joy].stick[n_stick].axis[n_axis].d1 = FALSE;

		  if (p > 192)
		     joy[n_joy].stick[n_stick].axis[n_axis].d2 = TRUE;
		  else
		     joy[n_joy].stick[n_stick].axis[n_axis].d2 = FALSE;
	       }

	       win32_axis++;
	    }
	 }
      }

      /* hat */
      if (win32_joystick[n_joy].has_hat) {
         /* emulate analog joystick */
         joy[n_joy].stick[n_stick].axis[0].pos = 0;	
         joy[n_joy].stick[n_stick].axis[1].pos = 0;

         if (win32_joystick[n_joy].hat & HAT_LEFT) {
            joy[n_joy].stick[n_stick].axis[0].d1 = TRUE;
            joy[n_joy].stick[n_stick].axis[0].pos = -128;
         }
         else
            joy[n_joy].stick[n_stick].axis[0].d1 = FALSE;

         if (win32_joystick[n_joy].hat & HAT_RIGHT) {
            joy[n_joy].stick[n_stick].axis[0].d2 = TRUE;
            joy[n_joy].stick[n_stick].axis[0].pos = +128;
         }
         else
            joy[n_joy].stick[n_stick].axis[0].d2 = FALSE;

         if (win32_joystick[n_joy].hat & HAT_FORWARD) {
            joy[n_joy].stick[n_stick].axis[1].d1 = TRUE;
            joy[n_joy].stick[n_stick].axis[1].pos = -128;
         }
         else
            joy[n_joy].stick[n_stick].axis[1].d1 = FALSE;

         if (win32_joystick[n_joy].hat & HAT_BACKWARD) {
            joy[n_joy].stick[n_stick].axis[1].d2 = TRUE;
            joy[n_joy].stick[n_stick].axis[1].pos = +128;
         }
         else
            joy[n_joy].stick[n_stick].axis[1].d2 = FALSE;
      }

      /* buttons */
      for (n_but = 0; n_but < win32_joystick[n_joy].buttons_num; n_but++)
	 joy[n_joy].button[n_but].b = win32_joystick[n_joy].button[n_but];
   }

   return 0;
}



/* init_win32_joysticks:
 *  initialises the Win32 joystick devices
 */
static int init_win32_joysticks(void)
{
   JOYCAPS caps;
   int n_joyat, n_joy, n_axis;

   win32_joy_num = joyGetNumDevs();

   if (win32_joy_num > MAX_JOYSTICKS)
      _TRACE("The system supports more than %d joysticks\n", MAX_JOYSTICKS);

   /* retrieve joystick infos */
   n_joy = 0;
   for (n_joyat = 0; n_joyat < win32_joy_num; n_joyat++) {
      if (n_joy == MAX_JOYSTICKS)
         break;

      if (joyGetDevCaps(n_joyat, &caps, sizeof(caps)) == JOYERR_NOERROR) {
	 win32_joystick[n_joy].dev_num = n_joyat;
	 win32_joystick[n_joy].caps = caps.wCaps;
	 win32_joystick[n_joy].buttons_num = MIN(caps.wNumButtons, MAX_JOYSTICK_BUTTONS);
	 win32_joystick[n_joy].axes_num = MIN(caps.wNumAxes, WIN32_MAX_AXES);

	 /* fill in ranges of axes */
	 win32_joystick[n_joy].min[0] = caps.wXmin;
	 win32_joystick[n_joy].max[0] = caps.wXmax;
	 win32_joystick[n_joy].min[1] = caps.wYmin;
	 win32_joystick[n_joy].max[1] = caps.wYmax;
	 n_axis = 2;

	 if (caps.wCaps & JOYCAPS_HASZ)	{
	    win32_joystick[n_joy].min[n_axis] = caps.wZmin;
	    win32_joystick[n_joy].max[n_axis] = caps.wZmax;
	    n_axis++;
	 }

	 if (caps.wCaps & JOYCAPS_HASR)	{
	    win32_joystick[n_joy].min[n_axis] = caps.wRmin;
	    win32_joystick[n_joy].max[n_axis] = caps.wRmax;
	    n_axis++;
	 }

	 if (caps.wCaps & JOYCAPS_HASU)	{
	    win32_joystick[n_joy].min[n_axis] = caps.wUmin;
	    win32_joystick[n_joy].max[n_axis] = caps.wUmax;
	    n_axis++;
	 }

	 if (caps.wCaps & JOYCAPS_HASV)	{
	    win32_joystick[n_joy].min[n_axis] = caps.wVmin;
	    win32_joystick[n_joy].max[n_axis] = caps.wVmax;
	    n_axis++;
	 }

	 if (caps.wCaps & JOYCAPS_HASPOV)
	    win32_joystick[n_joy].has_hat = TRUE;
	 else
	    win32_joystick[n_joy].has_hat = FALSE;

         n_joy++;
      }
   }

   win32_joy_num = n_joy;

   /* get the number of attached joysticks */
   joy_poll();

   return (win32_joy_attached == 0);
}



/* joy_init:
 *  initialises the driver
 */
static int joy_init(void)
{
   int n_stick, n_joy, n_axis, n_but;

   if (init_win32_joysticks() != 0)
      return -1;

   num_joysticks = win32_joy_attached;

   /* fill in the joystick structure */
   for (n_joy = 0; n_joy < num_joysticks; n_joy++) {
      joy[n_joy].flags = JOYFLAG_ANALOGUE | JOYFLAG_DIGITAL;

      /* how many sticks ? */
      n_stick = 0;

      if (win32_joystick[n_joy].axes_num > 0) {
	 n_axis = 0;

         /* main analogue stick */
	 if (win32_joystick[n_joy].axes_num > 1) {
	    joy[n_joy].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[n_joy].stick[n_stick].axis[0].name = name_x;
	    joy[n_joy].stick[n_stick].axis[1].name = name_y;
	    joy[n_joy].stick[n_stick].name = name_stick;

	    if (win32_joystick[n_joy].caps & JOYCAPS_HASZ) {
	       joy[n_joy].stick[n_stick].num_axis = 3;
	       joy[n_joy].stick[n_stick].axis[2].name = name_throttle;
	       n_axis += 3;
	    }
            else {
	       joy[n_joy].stick[n_stick].num_axis = 2;
	       n_axis += 2;
	    }

	    n_stick++;
	 }

         /* first 1-axis stick: rudder */
	 if (win32_joystick[n_joy].caps & JOYCAPS_HASR) {
	    joy[n_joy].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
	    joy[n_joy].stick[n_stick].num_axis = 1;
	    joy[n_joy].stick[n_stick].axis[0].name = "";
	    joy[n_joy].stick[n_stick].name = name_rudder;
	    n_stick++;
	    n_axis++;
	 }

         /* other 1-axis sticks */
	 while (n_axis < win32_joystick[n_joy].axes_num) {
	    joy[n_joy].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
	    joy[n_joy].stick[n_stick].num_axis = 1;
	    joy[n_joy].stick[n_stick].axis[0].name = "";
	    joy[n_joy].stick[n_stick].name = name_slider;
	    n_stick++;
	    n_axis++;
	 }

         /* hat */
	 if (win32_joystick[n_joy].has_hat) {
	    joy[n_joy].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
	    joy[n_joy].stick[n_stick].num_axis = 2;
	    joy[n_joy].stick[n_stick].axis[0].name = "left/right";
	    joy[n_joy].stick[n_stick].axis[1].name = "up/down";
	    joy[n_joy].stick[n_stick].name = name_hat;
	    n_stick++;
	 }
      }

      joy[n_joy].num_sticks = n_stick;

      /* how many buttons ? */
      joy[n_joy].num_buttons = win32_joystick[n_joy].buttons_num;

      /* fill in the button names */
      for (n_but = 0; n_but < joy[n_joy].num_buttons; n_but++)
	 joy[n_joy].button[n_but].name = name_b[n_but];
   }

   return 0;
}



/* joy_exit:
 *  shuts down the driver
 */
static void joy_exit(void)
{
   win32_joy_num = 0;
}
