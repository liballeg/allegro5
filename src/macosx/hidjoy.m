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
 *      HID Joystick driver routines for MacOS X.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif                


static int hid_joy_init(void);
static void hid_joy_exit(void);
static int hid_joy_poll(void);


JOYSTICK_DRIVER joystick_hid =
{
   JOYSTICK_HID,         // int  id; 
   empty_string,         // AL_CONST char *name; 
   empty_string,         // AL_CONST char *desc; 
   "HID Joystick",       // AL_CONST char *ascii_name;
   hid_joy_init,         // AL_METHOD(int, init, (void));
   hid_joy_exit,         // AL_METHOD(void, exit, (void));
   hid_joy_poll,         // AL_METHOD(int, poll, (void));
   NULL,                 // AL_METHOD(int, save_data, (void));
   NULL,                 // AL_METHOD(int, load_data, (void));
   NULL,                 // AL_METHOD(AL_CONST char *, calibrate_name, (int n));
   NULL                  // AL_METHOD(int, calibrate, (int n));
};


static HID_DEVICE *hid_device = NULL;



/* hid_joy_init:
 *  Initializes the HID joystick driver.
 */
static int hid_joy_init(void)
{
   static char *name_stick = "stick";
   static char *name_hat = "hat";
   static char *name_slider = "slider";
   static char *name_x = "x";
   static char *name_y = "y";
   static char *name_b[] =
      { "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "B10", "B11", "B12", "B13", "B14", "B15", "B16" };
   HID_ELEMENT *element;
   int i, j, stick;
   
   hid_device = osx_hid_scan(HID_JOYSTICK, &num_joysticks);
   for (i = 0; i < num_joysticks; i++) {
      memset(&joy[i], 0, sizeof(joy[i]));
      stick = 1;
      for (j = 0; j < hid_device[i].num_elements; j++) {
         element = &hid_device[i].element[j];
	 element->stick = element->index = -1;
	 switch (element->type) {
	    
	    case HID_ELEMENT_BUTTON:
	       if (joy[i].num_buttons >= MAX_JOYSTICK_BUTTONS)
	          break;
	       element->index = joy[i].num_buttons;
	       if (element->name)
                  joy[i].button[element->index].name = element->name;
	       else
	          joy[i].button[element->index].name = name_b[element->index];
	       joy[i].num_buttons++;
	       break;
	       
	    case HID_ELEMENT_AXIS:
	       if ((stick < MAX_JOYSTICK_STICKS) && (joy[i].stick[stick].num_axis >= 2))
	          stick++;
	       if (stick >= MAX_JOYSTICK_STICKS)
	          break;
	       if (joy[i].stick[stick].num_axis == 0)
	          joy[i].num_sticks++;
	       element->stick = stick;
	       element->index = joy[i].stick[stick].num_axis;
	       joy[i].stick[stick].name = name_stick;
	       joy[i].stick[stick].num_axis++;
	       joy[i].stick[stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	       if (element->name)
	          joy[i].stick[stick].axis[element->index].name = element->name;
	       else
	          joy[i].stick[stick].axis[element->index].name = (joy[i].stick[stick].num_axis == 1) ? name_x : name_y;
	       break;
	    
	    case HID_ELEMENT_AXIS_PRIMARY_X:
	    case HID_ELEMENT_AXIS_PRIMARY_Y:
	       if (joy[i].stick[0].num_axis == 0)
	          joy[i].num_sticks++;
	       element->stick = 0;
	       element->index = (element->type == HID_ELEMENT_AXIS_PRIMARY_X) ? 0 : 1;
	       joy[i].stick[0].name = name_stick;
	       joy[i].stick[0].num_axis++;
	       joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	       if (element->name)
	          joy[i].stick[0].axis[element->index].name = element->name;
	       else
	          joy[i].stick[0].axis[element->index].name = (element->index == 0) ? name_x : name_y;
	       break;
	    
	    case HID_ELEMENT_STANDALONE_AXIS:
	       if ((stick < MAX_JOYSTICK_STICKS) && (joy[i].stick[stick].num_axis > 0))
	          stick++;
	       if (stick >= MAX_JOYSTICK_STICKS)
	          break;
	       joy[i].num_sticks++;
	       element->stick = stick;
	       element->index = 0;
	       joy[i].stick[stick].name = name_slider;
	       joy[i].stick[stick].num_axis = 1;
	       joy[i].stick[stick].flags = JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
	       if (element->name)
	          joy[i].stick[stick].axis[0].name = element->name;
	       else
	          joy[i].stick[stick].axis[0].name = name_slider;
	       stick++;
	       break;
	    
	    case HID_ELEMENT_HAT:
	       if ((stick < MAX_JOYSTICK_STICKS) && (joy[i].stick[stick].num_axis > 0))
	          stick++;
	       if (stick >= MAX_JOYSTICK_STICKS)
	          break;
	       joy[i].num_sticks++;
	       element->stick = stick;
	       if (element->name)
	          joy[i].stick[stick].name = element->name;
	       else
	          joy[i].stick[stick].name = name_hat;
	       joy[i].stick[stick].num_axis = 2;
	       joy[i].stick[stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
               joy[i].stick[stick].axis[0].name = name_x;
               joy[i].stick[stick].axis[1].name = name_y;
	       stick++;
	       break;
	 }
      }
   }
   return hid_joy_poll();
}



/* hid_joy_exit:
 *  Shuts down the HID joystick driver.
 */
static void hid_joy_exit(void)
{
   if (hid_device)
      osx_hid_free(hid_device, num_joysticks);
}



/* hid_joy_poll:
 *  Polls the active joystick devices and updates internal states.
 */
static int hid_joy_poll(void)
{
   HID_DEVICE *device;
   HID_ELEMENT *element;
   IOHIDEventStruct hid_event;
   int i, j, pos;
   
   if ((!hid_device) || (num_joysticks <= 0))
      return -1;
   
   for (i = 0; i < num_joysticks; i++) {
      device = &hid_device[i];
      for (j = 0; j < device->num_elements; j++) {
         element = &device->element[j];
         if ((*(device->interface))->getElementValue(device->interface, element->cookie, &hid_event))
	    continue;
	 switch (element->type) {
	    
	    case HID_ELEMENT_BUTTON:
	       joy[i].button[element->index].b = hid_event.value;
	       break;
	    
	    case HID_ELEMENT_AXIS:
	    case HID_ELEMENT_AXIS_PRIMARY_X:
	    case HID_ELEMENT_AXIS_PRIMARY_Y:
	       pos = (((hid_event.value - element->min) * 256) / (element->max - element->min + 1)) - 128;
	       joy[i].stick[element->stick].axis[element->index].pos = pos;
	       joy[i].stick[element->stick].axis[element->index].d1 = FALSE;
	       joy[i].stick[element->stick].axis[element->index].d2 = FALSE;
	       if (pos < 0)
	          joy[i].stick[element->stick].axis[element->index].d1 = TRUE;
	       else if (pos > 0)
	          joy[i].stick[element->stick].axis[element->index].d2 = TRUE;
	       break;
	    
	    case HID_ELEMENT_STANDALONE_AXIS:
	       pos = (((hid_event.value - element->min) * 255) / (element->max - element->min + 1));
	       joy[i].stick[element->stick].axis[element->index].pos = pos;
	       break;
	       
	    case HID_ELEMENT_HAT:
	       switch (hid_event.value) {

                  case 0:  /* up */
                     joy[i].stick[element->stick].axis[0].pos = 0;
                     joy[i].stick[element->stick].axis[0].d1 = 0;
                     joy[i].stick[element->stick].axis[0].d2 = 0;
                     joy[i].stick[element->stick].axis[1].pos = 128;
                     joy[i].stick[element->stick].axis[1].d1 = 0;
                     joy[i].stick[element->stick].axis[1].d2 = 1;
		     break;
		     
                  case 1:  /* up and right */
                     joy[i].stick[element->stick].axis[0].pos = 128;
                     joy[i].stick[element->stick].axis[0].d1 = 0;
                     joy[i].stick[element->stick].axis[0].d2 = 1;
                     joy[i].stick[element->stick].axis[1].pos = 128;
                     joy[i].stick[element->stick].axis[1].d1 = 0;
                     joy[i].stick[element->stick].axis[1].d2 = 1;
		     break;
		     
                  case 2:  /* right */
                     joy[i].stick[element->stick].axis[0].pos = 128;
                     joy[i].stick[element->stick].axis[0].d1 = 0;
                     joy[i].stick[element->stick].axis[0].d2 = 1;
                     joy[i].stick[element->stick].axis[1].pos = 0;
                     joy[i].stick[element->stick].axis[1].d1 = 0;
                     joy[i].stick[element->stick].axis[1].d2 = 0;
		     break;
		     
                  case 3:  /* down and right */
                     joy[i].stick[element->stick].axis[0].pos = 128;
                     joy[i].stick[element->stick].axis[0].d1 = 0;
                     joy[i].stick[element->stick].axis[0].d2 = 1;
                     joy[i].stick[element->stick].axis[1].pos = -128;
                     joy[i].stick[element->stick].axis[1].d1 = 1;
                     joy[i].stick[element->stick].axis[1].d2 = 0;
		     break;
		     
                  case 4:  /* down */
                     joy[i].stick[element->stick].axis[0].pos = 0;
                     joy[i].stick[element->stick].axis[0].d1 = 0;
                     joy[i].stick[element->stick].axis[0].d2 = 0;
                     joy[i].stick[element->stick].axis[1].pos = -128;
                     joy[i].stick[element->stick].axis[1].d1 = 1;
                     joy[i].stick[element->stick].axis[1].d2 = 0;
		     break;
		     
                  case 5:  /* down and left */
                     joy[i].stick[element->stick].axis[0].pos = -128;
                     joy[i].stick[element->stick].axis[0].d1 = 1;
                     joy[i].stick[element->stick].axis[0].d2 = 0;
                     joy[i].stick[element->stick].axis[1].pos = -128;
                     joy[i].stick[element->stick].axis[1].d1 = 1;
                     joy[i].stick[element->stick].axis[1].d2 = 0;
		     break;
		     
                  case 6:  /* left */
                     joy[i].stick[element->stick].axis[0].pos = -128;
                     joy[i].stick[element->stick].axis[0].d1 = 1;
                     joy[i].stick[element->stick].axis[0].d2 = 0;
                     joy[i].stick[element->stick].axis[1].pos = 0;
                     joy[i].stick[element->stick].axis[1].d1 = 0;
                     joy[i].stick[element->stick].axis[1].d2 = 0;
		     break;
		     
                  case 7:  /* up and left */
                     joy[i].stick[element->stick].axis[0].pos = -128;
                     joy[i].stick[element->stick].axis[0].d1 = 1;
                     joy[i].stick[element->stick].axis[0].d2 = 0;
                     joy[i].stick[element->stick].axis[1].pos = 128;
                     joy[i].stick[element->stick].axis[1].d1 = 0;
                     joy[i].stick[element->stick].axis[1].d2 = 1;
		     break;
		     
                  case 15:  /* centered */
                     joy[i].stick[element->stick].axis[0].pos = 0;
                     joy[i].stick[element->stick].axis[0].d1 = 0;
                     joy[i].stick[element->stick].axis[0].d2 = 0;
                     joy[i].stick[element->stick].axis[1].pos = 0;
                     joy[i].stick[element->stick].axis[1].d1 = 0;
                     joy[i].stick[element->stick].axis[1].d2 = 0;
                     break;
	       }
	       break;
	 }
      }
   }
   return 0;
}
