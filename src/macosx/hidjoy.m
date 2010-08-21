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

#include "allegro5/allegro.h"
#include "allegro5/platform/aintosx.h"
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/hid/IOHIDUsageTables.h>
#import <CoreFoundation/CoreFoundation.h>


#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif                

ALLEGRO_DEBUG_CHANNEL("MacOSX")

#define _AL_MAX_JOYSTICKS 8

static bool init_joystick(void);
static void exit_joystick(void);
static int num_joysticks(void);
static ALLEGRO_JOYSTICK* get_joystick(int);
static void release_joystick(ALLEGRO_JOYSTICK*);
static void get_joystick_state(ALLEGRO_JOYSTICK*, ALLEGRO_JOYSTICK_STATE*);

/* ALJoystickHelper:
* The joystick events are delivered through the run loop. We need to
* attach the callback to the main thread's run loop 
* (otherwise the events are never delivered)
* The class methods are used with performOnMainThread:withObject:waitUntilDone:
* to ensure that we access the main thread
*/
@interface ALJoystickHelper : NSObject
{ }
+(void)startQueues;
+(void)stopQueues;
@end

/* OSX HID Joystick 
 * Maintains an array of links which connect a HID cookie to 
 * an element in the ALLEGRO_JOYSTICK_STATE structure.
 */
typedef struct {
   ALLEGRO_JOYSTICK parent;
   struct {
      IOHIDElementCookie cookie;
   } button_link[_AL_MAX_JOYSTICK_BUTTONS];
   struct {
      IOHIDElementCookie cookie;
      SInt32 intvalue;
      float offset;
      float multiplier;
      int stick, axis;
   } axis_link[_AL_MAX_JOYSTICK_AXES * _AL_MAX_JOYSTICK_STICKS];
   int num_axis_links;
   ALLEGRO_JOYSTICK_STATE state;
   IOHIDDeviceInterface122** interface;
   IOHIDQueueInterface** queue;
   CFRunLoopSourceRef source;
   CFTypeRef unique_id;
} ALLEGRO_JOYSTICK_OSX;

static ALLEGRO_JOYSTICK_OSX joysticks[_AL_MAX_JOYSTICKS];
static int joystick_count;
static ALLEGRO_JOYSTICK_OSX joysticks_real[_AL_MAX_JOYSTICKS];
static int joystick_count_real;
static bool merged;
static ALLEGRO_THREAD *cfg_thread;
static bool initialized = false;

/* create_device_iterator:
 * Create an iterator which will match all joysticks/
 * gamepads on the system. 
 */
static io_iterator_t create_device_iterator(UInt16 usage)
{
   NSMutableDictionary* matching;
   io_iterator_t iter;
   matching = (NSMutableDictionary*) IOServiceMatching(kIOHIDDeviceKey);
   // Add in our criteria:
   [matching setObject:[NSNumber numberWithShort: usage] forKey: (NSString*) CFSTR(kIOHIDPrimaryUsageKey)];
   [matching setObject:[NSNumber numberWithShort: kHIDPage_GenericDesktop] forKey: (NSString*) CFSTR(kIOHIDPrimaryUsagePageKey)];
   // Get the iterator
   IOReturn err = IOServiceGetMatchingServices(kIOMasterPortDefault, (CFDictionaryRef) matching, &iter);
   return (err == kIOReturnSuccess) ? iter : 0;
}

/* create_interface:
 * Create the interface to access this device, via
 * the intermediate plug-in interface
*/
static BOOL create_interface(io_object_t device, IOHIDDeviceInterface122*** interface)
{
   io_name_t class_name;
   IOReturn err = IOObjectGetClass(device,class_name);
   SInt32 score;
   IOCFPlugInInterface** plugin;
   err = IOCreatePlugInInterfaceForService(device,
                                 kIOHIDDeviceUserClientTypeID,
                                 kIOCFPlugInInterfaceID,
                                 &plugin,
                                 &score);
   (*plugin)->QueryInterface(plugin,
                       CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122),
                       (LPVOID) interface);
   
   (*plugin)->Release(plugin);
   return YES;
}

/* joystick_callback:
 * Called when an event occurs on the joystick event queue
 * target: the joystick
 * refcon: always null
 * sender: the queue
 */
static void joystick_callback(void *target, IOReturn result, void *refcon __attribute__((unused)), void *sender)
{
   if (!initialized) return;
   ALLEGRO_JOYSTICK_OSX* joy = (ALLEGRO_JOYSTICK_OSX*) target;
   IOHIDQueueInterface** queue = (IOHIDQueueInterface**) sender;
   AbsoluteTime past = {0,0};
   ALLEGRO_EVENT_SOURCE* src = al_get_joystick_event_source();
   _al_event_source_lock(src);
   while (result == kIOReturnSuccess) {
      IOHIDEventStruct event;
      result = (*queue)->getNextEvent(queue, &event, past, 0);
      if (result == kIOReturnSuccess) {
         int i;
         for (i = 0; i < joy->parent.info.num_buttons; ++i) {
            if (joy->button_link[i].cookie == event.elementCookie) {
               int newvalue = event.value;
               if (joy->state.button[i] != newvalue) {
                  joy->state.button[i] = newvalue;
                  // emit event
                  ALLEGRO_EVENT evt;
                  if (newvalue)
                     evt.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
                  else
                     evt.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
		  evt.joystick.id = (intptr_t)joy;
                  evt.joystick.button = i;
                  _al_event_source_emit_event(src, &evt);
               }
            }
         }
         for (i = 0; i < joy->num_axis_links; ++i) {
            if (joy->axis_link[i].cookie == event.elementCookie) {
               SInt32 newvalue = event.value;
               if (joy->axis_link[i].intvalue != newvalue) {
                  joy->axis_link[i].intvalue = newvalue;
                  joy->state.stick[0].axis[i] = (joy->axis_link[i].offset + newvalue) * joy->axis_link[i].multiplier;
                  // emit event
                  ALLEGRO_EVENT evt;
                  evt.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		  evt.joystick.id = (intptr_t)joy;
                  evt.joystick.axis = joy->axis_link[i].axis;
                  evt.joystick.pos = joy->state.stick[0].axis[i];
                  evt.joystick.stick = joy->axis_link[i].stick;
                  _al_event_source_emit_event(src, &evt);
               }
            }
         }
      }
   }
   _al_event_source_unlock(src);
}

/* add_device:
 * Create the joystick structure for this device
 * and add it to the 'joysticks' vector
 * TODO this only works for simple joysticks and
 * only allows access to the primary X & Y axes.
 * In reality there can be more axes than this and
 * more that one distinct controller handled by the same
 * interface. 
 * We should iterate through the application collections to
 * find the joysticks then through the physical collections
 * therein to identify the individual sticks.
 */
static void add_device(io_object_t device)
{
   ALLEGRO_JOYSTICK_OSX* joy;
   NSArray* elements = nil;
   int num_buttons = 0;
   BOOL have_x = NO, have_y = NO;
   IOReturn err;
   joy = &joysticks_real[joystick_count_real++];
   memset(joy, 0, sizeof(*joy));
   joy->parent.info.num_sticks = 0;
   joy->parent.info.num_buttons = 0;
   IOHIDDeviceInterface122** interface;
   create_interface(device, &interface);
   // Open the device
   err = (*interface)->open(interface, 0);
   // Create an event queue
   IOHIDQueueInterface** queue = (*interface)->allocQueue(interface);
   err = (*queue)->create(queue, 0, 8);
   // Create a source
   err = (*queue)->createAsyncEventSource(queue, &joy->source);
   joy->queue = queue;
   joy->unique_id = IORegistryEntryCreateCFProperty(device, CFSTR(kIOHIDProductIDKey), kCFAllocatorDefault, 0);
   (*interface)->copyMatchingElements(interface, NULL, (CFArrayRef*) &elements);
   NSEnumerator* enumerator = [elements objectEnumerator];
   NSDictionary* element;
   while ((element = (NSDictionary*) [enumerator nextObject])) {
      short usage = [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementUsageKey)]) shortValue];
      short usage_page = [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementUsagePageKey)]) shortValue];

      if (usage_page == kHIDPage_Button && num_buttons < _AL_MAX_JOYSTICK_BUTTONS) {
         joy->button_link[num_buttons].cookie = (IOHIDElementCookie) [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementCookieKey)]) pointerValue];
         // Use the provided name or make one up.
         NSString* name = (NSString*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementNameKey)];
         if (name == nil) {
            name = [NSString stringWithFormat:@"Button %d", (num_buttons+1)];
         }
         ALLEGRO_INFO("Found button named \"%s\"\n", [name UTF8String]);
         // Say that we want events from this button
         err = (*queue)->addElement(queue, joy->button_link[num_buttons].cookie, 0);
         if (err != 0) {
            ALLEGRO_WARN("Button named \"%s\" NOT added to event queue\n", [name UTF8String]);
         } else {
            joy->parent.info.button[num_buttons].name = strdup([name UTF8String]);
            ++num_buttons;
         }
      }

      if (usage_page == kHIDPage_GenericDesktop) {
         if ((usage == kHIDUsage_GD_X) && (!have_x)) {
            NSNumber* minValue = (NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementMinKey)];
            NSNumber* maxValue = (NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementMaxKey)];
            joy->axis_link[0].axis = 0;
            joy->axis_link[0].stick = 0;
            joy->axis_link[0].offset = - ([minValue floatValue] + [maxValue floatValue]) / 2.0f;
            joy->axis_link[0].multiplier = 2.0f / ([maxValue floatValue] - [minValue floatValue]);
            joy->axis_link[0].cookie = (IOHIDElementCookie) [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementCookieKey)]) pointerValue];
            NSString* name = (NSString*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementNameKey)];
            if (name == nil) {
               name = @"X-axis";
            }
            ALLEGRO_INFO("Found X-axis named \"%s\"\n", [name UTF8String]);
            // Say that we want events from this axis
            err = (*queue)->addElement(queue, joy->axis_link[0].cookie, 0);
            if (err != 0) {
               ALLEGRO_WARN("X-axis named \"%s\" NOT added to event queue\n", [name UTF8String]);
            } else {
               have_x = YES;
               joy->parent.info.stick[0].axis[0].name = strdup([name UTF8String]);
            }
         }
         if ((usage == kHIDUsage_GD_Y) && (!have_y)) {
            NSNumber* minValue = (NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementMinKey)];
            NSNumber* maxValue = (NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementMaxKey)];
            joy->axis_link[1].axis = 1;
            joy->axis_link[1].stick = 0;
            joy->axis_link[1].offset = - ([minValue floatValue] + [maxValue floatValue]) / 2.0f;
            joy->axis_link[1].multiplier = 2.0f / ([maxValue floatValue] - [minValue floatValue]);
            joy->axis_link[1].cookie = (IOHIDElementCookie) [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementCookieKey)]) pointerValue];
            NSString* name = (NSString*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementNameKey)];
            if (name == nil) {
               name = @"Y-axis";
            }
            ALLEGRO_INFO("Found Y-axis named \"%s\"\n", [name UTF8String]);
            // Say that we want events from this axis
            err = (*queue)->addElement(queue, joy->axis_link[1].cookie, 0);
            if (err != 0) {
               ALLEGRO_WARN("Y-axis named \"%s\" NOT added to event queue\n", [name UTF8String]);
            } else {
               have_y = YES;
               joy->parent.info.stick[0].axis[1].name = strdup([name UTF8String]);
            }
         }
      }
   }
   joy->parent.info.num_buttons = num_buttons;
   if (have_x && have_y) {
      joy->parent.info.stick[0].name = strdup("Primary axis");
      joy->parent.info.num_sticks = 1;
      joy->parent.info.stick[0].num_axes = 2;
      joy->num_axis_links = 2;
   }
   joy->interface = interface;
   joystick_callback(joy,kIOReturnSuccess,NULL,queue);
   //[elements release];
}

ALLEGRO_JOYSTICK_DRIVER* _al_osx_get_joystick_driver(void)
{
   static ALLEGRO_JOYSTICK_DRIVER* vt = NULL;
   if (vt == NULL) {
      vt = al_malloc(sizeof(*vt));
      memset(vt, 0, sizeof(*vt));
      vt->joydrv_ascii_name = "OSX HID Driver";
      vt->init_joystick = init_joystick;
      vt->exit_joystick = exit_joystick;
      vt->num_joysticks = num_joysticks;
      vt->get_joystick = get_joystick;
      vt->release_joystick = release_joystick;
      vt->get_joystick_state = get_joystick_state;
   }
   return vt;
}
  
static void joyosx_generate_configure_event(void)
{
   ALLEGRO_EVENT event;
   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.joystick.timestamp = al_current_time();

   _al_generate_joystick_event(&event);
}

static void joyosx_scan(bool configure)
{
   io_iterator_t iterator;
   io_object_t device;
   int i, j;

   merged = false;
   joystick_count_real = 0;
   iterator = create_device_iterator(kHIDUsage_GD_GamePad);
   if (iterator != 0) {
      while ((device = IOIteratorNext(iterator))) {
         add_device(device);
      }
      IOObjectRelease(iterator);
   }
   iterator = create_device_iterator(kHIDUsage_GD_Joystick);
   if (iterator != 0) {
      while ((device = IOIteratorNext(iterator))) {
         add_device(device);
      }
      IOObjectRelease(iterator);
   }

   if (configure) {
      if (joystick_count != joystick_count_real) {
         joyosx_generate_configure_event();
	 return;
      }
      for (i = 0; i < joystick_count; i++) {
         bool found = false;
         for (j = 0; j < joystick_count_real; j++) {
            if (CFEqual(joysticks[i].unique_id, joysticks_real[j].unique_id)) {
               found = true;
               break;
            }
         }
         if (!found) {
            joyosx_generate_configure_event();
            return;
         }
      }
   }
}

static void destroy_joy(ALLEGRO_JOYSTICK_OSX *joy)
{
   CFRelease(joy->unique_id);
   CFRelease(joy->source);
   if (joy->queue) {
      (*joy->queue)->dispose(joy->queue);
      (*joy->queue)->Release(joy->queue);
   }
   if (joy->interface) {
      (*joy->interface)->close(joy->interface);
      (*joy->interface)->Release(joy->interface);
   }
   int a, b, s;
   /* Free everything we might have created 
   * (all fields set to NULL initially so this is OK.)
   */
   for (b = 0; b < _AL_MAX_JOYSTICK_BUTTONS; ++ b) {
      al_free((void*) joy->parent.info.button[b].name);
   }
   for (s = 0; s < _AL_MAX_JOYSTICK_STICKS; ++s) {
      al_free((void*) joy->parent.info.stick[s].name);
      for (a = 0; a < _AL_MAX_JOYSTICK_AXES; ++a) {
         al_free((void*) joy->parent.info.stick[s].axis[a].name);
      }
   }
}

static void joyosx_merge(void)
{
   int i, j;
   int count = 0;

   merged = true;

   ALLEGRO_INFO("Merging %d %d\n", joystick_count, joystick_count_real);

   // Delete all current
   for (i = 0; i < joystick_count; i++) {
      bool found = false;
      for (j = 0; j < joystick_count_real; j++) {
         if (CFEqual(joysticks[i].unique_id, joysticks_real[j].unique_id)) {
            found = true;
            break;
         }
      }
      if (found) {
         memcpy(&joysticks[count], &joysticks[i], sizeof(ALLEGRO_JOYSTICK_OSX));
         count++;
      }
      else {
         destroy_joy(&joysticks[i]);
      }
   }
   for (i = 0; i < joystick_count_real; i++) {
      bool found = false;
      for (j = 0; j < count; j++) {
         if (CFEqual(joysticks[j].unique_id, joysticks_real[i].unique_id)) {
            found = true;
            break;
         }
      }
      if (!found) {
         memcpy(&joysticks[count], &joysticks_real[i], sizeof(ALLEGRO_JOYSTICK_OSX));
         (*joysticks[count].queue)->setEventCallout(joysticks[count].queue, joystick_callback, &joysticks[count], NULL);
         count++;
      }
      else {
         destroy_joy(&joysticks_real[i]);
      }
   }

   joystick_count = count;
   joystick_count_real = 0;
}

static void *config_thread_proc(ALLEGRO_THREAD *thread, void *data)
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

   int i;

   (void)data;

   while (!al_get_thread_should_stop(thread)) {
      al_rest(1);
      for (i = 0; i < joystick_count_real; ++i) {
         destroy_joy(&joysticks_real[i]);
      }
      joystick_count_real = 0;
      joyosx_scan(true);
   }

   [pool release];

   return NULL;
}

/* init_joystick:
 *  Initializes the HID joystick driver.
 */
static bool init_joystick(void)
{
   joyosx_scan(false);
   joyosx_merge();

   [ALJoystickHelper performSelectorOnMainThread: @selector(startQueues)
      withObject: nil
      waitUntilDone: YES];

   cfg_thread = al_create_thread(config_thread_proc, NULL);
   al_start_thread(cfg_thread);

   initialized = true;

   return true;
}

/* exit_joystick:
 *  Shuts down the HID joystick driver.
 */
static void exit_joystick(void)
{
   int i;

   al_join_thread(cfg_thread, NULL);

   [ALJoystickHelper performSelectorOnMainThread: @selector(stopQueues)
                              withObject: nil
                           waitUntilDone: YES];

   for (i=0; i< joystick_count; ++i) {
      destroy_joy(&joysticks[i]);
   }
   for (i=0; i< joystick_count_real; ++i) {
      destroy_joy(&joysticks_real[i]);
   }

   joystick_count = 0;
   joystick_count_real = 0;

   initialized = false;
}

/* num_joysticks:
 *  Return number of active joysticks
 */
static int num_joysticks(void)
{
   if (merged) {
      return joystick_count;
   }
   [ALJoystickHelper performSelectorOnMainThread: @selector(stopQueues)
                              withObject: nil
                           waitUntilDone: YES];
   joyosx_merge();
   [ALJoystickHelper performSelectorOnMainThread: @selector(startQueues)
                              withObject: nil
                           waitUntilDone: YES];
   return joystick_count;
}

/* get_joystick:
 * Get a pointer to a joystick structure
 */
static ALLEGRO_JOYSTICK* get_joystick(int index)
{
   ALLEGRO_JOYSTICK* joy = NULL;
   if (index >= 0 && index < (int) joystick_count) {
      joy = (ALLEGRO_JOYSTICK *)&joysticks[index];
   }
   return joy;
}

/* release_joystick:
 * Release a pointer that has been obtained
 */
static void release_joystick(ALLEGRO_JOYSTICK* joy __attribute__((unused)) )
{
   // No-op
}

/* get_joystick_state:
 * Get the current status of a joystick
 */
static void get_joystick_state(ALLEGRO_JOYSTICK* ajoy, ALLEGRO_JOYSTICK_STATE* state)
{
   ALLEGRO_JOYSTICK_OSX* joy = (ALLEGRO_JOYSTICK_OSX*)ajoy;
   memcpy(state, &joy->state, sizeof(*state));
}

@implementation ALJoystickHelper
+(void)startQueues 
{
   int i;
   CFRunLoopRef current = CFRunLoopGetCurrent();
   for (i=0; i<joystick_count; ++i) {
      ALLEGRO_JOYSTICK_OSX* joy = &joysticks[i];
      CFRunLoopAddSource(current,joy->source,kCFRunLoopDefaultMode);
      (*joy->queue)->start(joy->queue);
   }
}
+(void) stopQueues 
{
   int i;
   CFRunLoopRef current = CFRunLoopGetCurrent();
   for (i=0; i<joystick_count; ++i) {
      ALLEGRO_JOYSTICK_OSX* joy = &joysticks[i];
      (*joy->queue)->stop(joy->queue);
      CFRunLoopRemoveSource(current,joy->source,kCFRunLoopDefaultMode);
   }   
}
@end


/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
