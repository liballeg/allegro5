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
 *      MacOS X mouse driver.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif


static int osx_mouse_init(void);
static void osx_mouse_exit(void);
static void osx_mouse_position(int, int);
static void osx_mouse_set_range(int, int, int, int);
static void osx_mouse_get_mickeys(int *, int *);


MOUSE_DRIVER mouse_macosx =
{
   MOUSE_MACOSX,
   empty_string,
   empty_string,
   "MacOS X mouse",
   osx_mouse_init,
   osx_mouse_exit,
   NULL,       // AL_METHOD(void, poll, (void));
   NULL,       // AL_METHOD(void, timer_poll, (void));
   osx_mouse_position,
   osx_mouse_set_range,
   NULL,       // AL_METHOD(void, set_speed, (int xspeed, int yspeed));
   osx_mouse_get_mickeys,
   NULL        // AL_METHOD(int,  analyse_data, (AL_CONST char *buffer, int size));
};


/* global variable */
int osx_mouse_warped = FALSE;
NSTrackingRectTag osx_mouse_tracking_rect = -1;


static int mouse_minx = 0;
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;
static int mymickey_y = 0;



/* osx_mouse_handler:
 *  Mouse "interrupt" handler for mickey-mode driver.
 */
void osx_mouse_handler(int x, int y, int z, int buttons)
{
   if (!_mouse_on)
      return;

   _mouse_b = buttons;
   
   mymickey_x += x;
   mymickey_y += y;

   _mouse_x += x;
   _mouse_y += y;
   _mouse_z += z;
   
   if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx)
       || (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {
      _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
      _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);
   }

   _handle_mouse_input();
}



/* lookup_button_elements:
 *  Recursive function to scan for mouse button elements in the given HID
 *  elements collection.
 */
static void lookup_button_elements(const void *elements_array, int *num_buttons)
{
   int i, usage_page, usage_id, type;
   const void *element;
   
   if ((!elements_array) || ((CFGetTypeID(elements_array) != CFArrayGetTypeID())))
      return;
   for (i = 0; i < CFArrayGetCount(elements_array); i++) {
      element = CFArrayGetValueAtIndex(elements_array, i);
      if (CFGetTypeID(element) != CFDictionaryGetTypeID())
         continue;
      CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsagePageKey)), kCFNumberSInt32Type, &usage_page);
      CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsageKey)), kCFNumberSInt32Type, &usage_id);
      CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementTypeKey)), kCFNumberSInt32Type, &type);
      /* Check if this is a button element */
      if ((type) && (usage_page == 0x9) && (usage_id > 0))
	 (*num_buttons)++;
      /* Element is a collection of other elements. Descend into it */
      if (type == kIOHIDElementTypeCollection)
         lookup_button_elements(CFDictionaryGetValue(element, CFSTR(kIOHIDElementKey)), num_buttons);
   }
}



/* osx_mouse_init:
 *  Initializes the mickey-mode driver.
 */
static int osx_mouse_init(void)
{
   int num_buttons, max_num_buttons = 0;
   mach_port_t master_port = NULL;
   io_iterator_t hid_object_iterator = NULL;
   io_object_t hid_device = NULL;
   CFMutableDictionaryRef class_dictionary = NULL;
   CFMutableDictionaryRef properties = NULL;
   CFArrayRef elements_array = NULL;
   const void *element = NULL;
   IOReturn result;
   int i, usage_id, usage_page;

   pthread_mutex_lock(&osx_event_mutex);
   
   result = IOMasterPort(bootstrap_port, &master_port);
   if (result == kIOReturnSuccess) {
      class_dictionary = IOServiceMatching(kIOHIDDeviceKey);
      result = IOServiceGetMatchingServices(master_port, class_dictionary, &hid_object_iterator);
      if ((result == kIOReturnSuccess) && (hid_object_iterator)) {
         /* Ok, we have a list of attached HID devices. Scan them for mice */
	 while ((hid_device = IOIteratorNext(hid_object_iterator))) {
            result = IORegistryEntryCreateCFProperties(hid_device, &properties, kCFAllocatorDefault, kNilOptions);
	    if ((result == KERN_SUCCESS) && (properties)) {
	       CFNumberGetValue(CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsageKey)), kCFNumberSInt32Type, &usage_id);
	       CFNumberGetValue(CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsagePageKey)), kCFNumberSInt32Type, &usage_page);
	       /* Look for mouse devices (0x2) on generic desktop page (0x1) */
	       if ((usage_page == 0x1) && (usage_id == 0x2)) {
	          /* Found a mouse! */
		  num_buttons = 0;
		  elements_array = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementKey));
		  lookup_button_elements(elements_array, &num_buttons);
		  max_num_buttons = MAX(max_num_buttons, num_buttons);
	       }
	       CFRelease(properties);
	    }
	 }
         IOObjectRelease(hid_object_iterator);
      }
      mach_port_deallocate(mach_task_self(), master_port);
   }
   
   if (max_num_buttons == 0) {
      /* No mouse found */
      pthread_mutex_unlock(&osx_event_mutex);
      return -1;
   }
   
   /* On my iBook the HID Manager recognizes the integrated trackpad as a two
    * buttons mouse... Yes it is weird as it actually has only one button.
    * So we activate emulation if less than 3 buttons are found, just in case.
    */
   osx_emulate_mouse_buttons = (max_num_buttons < 3) ? TRUE : FALSE;
   
   pthread_mutex_unlock(&osx_event_mutex);
   
   return max_num_buttons;
}



/* osx_mouse_exit:
 *  Shuts down the mickey-mode driver.
 */
static void osx_mouse_exit(void)
{
}



/* osx_mouse_position:
 *  Sets the position of the mickey-mode mouse.
 */
static void osx_mouse_position(int x, int y)
{
   CGPoint point;
   NSRect frame;
   
   pthread_mutex_lock(&osx_event_mutex);
   
   _mouse_x = point.x = x;
   _mouse_y = point.y = y;
   
   if (osx_gfx_mode == OSX_GFX_WINDOW) {
      frame = [osx_window frame];
      point.x += frame.origin.x;
      point.y += frame.origin.y - frame.size.height;
   }
   
   CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, point);
   
   mymickey_x = mymickey_y = 0;

   osx_mouse_warped = TRUE;
   
   pthread_mutex_unlock(&osx_event_mutex);
}



/* osx_mouse_set_range:
 *  Sets the range of the mickey-mode mouse.
 */
static void osx_mouse_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   pthread_mutex_lock(&osx_event_mutex);

   _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

   pthread_mutex_unlock(&osx_event_mutex);
   
   osx_mouse_position(_mouse_x, _mouse_y);
}



/* osx_mouse_get_mickeys:
 *  Reads the mickey-mode count.
 */
static void osx_mouse_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}
