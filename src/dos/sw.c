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
 *      Joystick driver for the Microsoft Sidewinder.
 *
 *      By Marius Fodor, using information provided by Robert Grubbs.
 *      Major changes by Shawn Hargreaves after powerjaw sent me a pad to
 *      test it with (many thanks for that, it was a very cool thing to do).
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintdos.h"

#ifndef ALLEGRO_DOS
   #error Something is wrong with the makefile
#endif



/* driver functions */
static int sw_init(void); 
static void sw_exit(void); 
static int sw_poll(void);


/* from sidewind.s */
int _get_sidewinder_status(int *input);

char _sidewinder_dump[200], _sidewinder_data[200];



JOYSTICK_DRIVER joystick_sw =
{
   JOY_TYPE_SIDEWINDER,
   empty_string,
   empty_string,
   "Sidewinder",
   sw_init,
   sw_exit,
   sw_poll,
   NULL, NULL,
   NULL, NULL
};



/* sw_init:
 *  Initialises the driver.
 */
static int sw_init()
{
   int sw_raw_input[4];
   int i;

   i = _get_sidewinder_status(sw_raw_input);
   if (i < 0)
      return -1;

   num_joysticks = i;

   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_DIGITAL;

      joy[i].num_sticks = 1;
      joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[i].stick[0].num_axis = 2;
      joy[i].stick[0].axis[0].name = get_config_text("X");
      joy[i].stick[0].axis[1].name = get_config_text("Y");
      joy[i].stick[0].name = get_config_text("Pad");

      joy[i].num_buttons = 10;

      joy[i].button[0].name = get_config_text("A");
      joy[i].button[1].name = get_config_text("B");
      joy[i].button[2].name = get_config_text("C");
      joy[i].button[3].name = get_config_text("X");
      joy[i].button[4].name = get_config_text("Y");
      joy[i].button[5].name = get_config_text("Z");
      joy[i].button[6].name = get_config_text("L");
      joy[i].button[7].name = get_config_text("R");
      joy[i].button[8].name = get_config_text("Start");
      joy[i].button[9].name = get_config_text("M");
   }

   return 0;
}



/* sw_exit:
 *  Shuts down the driver.
 */
static void sw_exit()
{
}



/* sw_poll:
 *  Updates the joystick status variables.
 */
static int sw_poll()
{
   int sw_raw_input[4];
   int i, b, mask;

   if (_get_sidewinder_status(sw_raw_input) != num_joysticks)
      return -1;

   for (i=0; i<num_joysticks; i++) {
      joy[i].stick[0].axis[0].d1 = ((sw_raw_input[i] & 0x10) != 0);
      joy[i].stick[0].axis[0].d2 = ((sw_raw_input[i] & 0x08) != 0);
      joy[i].stick[0].axis[1].d1 = ((sw_raw_input[i] & 0x02) != 0);
      joy[i].stick[0].axis[1].d2 = ((sw_raw_input[i] & 0x04) != 0);

      for (b=0; b<2; b++) {
	 if (joy[i].stick[0].axis[b].d1)
	    joy[i].stick[0].axis[b].pos = -128;
	 else if (joy[i].stick[0].axis[b].d2)
	    joy[i].stick[0].axis[b].pos = 128;
	 else
	    joy[i].stick[0].axis[b].pos = 0;
      }

      mask = 0x20; 

      for (b=0; b<10; b++) {
	 joy[i].button[b].b = ((sw_raw_input[i] & mask) != 0);
	 mask <<= 1; 
      }
   }

   return 0;
}

