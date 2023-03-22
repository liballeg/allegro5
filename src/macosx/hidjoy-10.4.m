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

#include "allegro5/allegro5.h"
#include "allegro5/platform/aintosx.h"
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/hid/IOHIDUsageTables.h>


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

/* OSX HID Joystick
 * Maintains an array of links which connect a HID cookie to 
 * an element in the ALLEGRO_JOYSTICK_STATE structure.
 */
typedef struct {
   ALLEGRO_JOYSTICK parent;
   struct {
      IOHIDElementCookie cookie;
      int* ppressed;
   } button_link[_AL_MAX_JOYSTICK_BUTTONS];
   struct {
      IOHIDElementCookie cookie;
      SInt32 intvalue;
      float* pvalue;
      float offset;
      float multiplier;
      int stick, axis;
   } axis_link[_AL_MAX_JOYSTICK_AXES * _AL_MAX_JOYSTICK_STICKS];
   int num_axis_links;
   ALLEGRO_JOYSTICK_STATE state;
   IOHIDDeviceInterface122** interface;
   IOHIDQueueInterface** queue;
   CFRunLoopSourceRef source;
} ALLEGRO_JOYSTICK_OSX;

static ALLEGRO_JOYSTICK_OSX joysticks[_AL_MAX_JOYSTICKS];
static unsigned int joystick_count;

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
   ALLEGRO_JOYSTICK_OSX* joy = (ALLEGRO_JOYSTICK_OSX*) target;
   IOHIDQueueInterface** queue = (IOHIDQueueInterface**) sender;
   AbsoluteTime past = {0,0};
   ALLEGRO_EVENT_SOURCE *src = al_get_joystick_event_source();
   if (src == NULL) {
      return;
   }
   _al_event_source_lock(src);
   while (result == kIOReturnSuccess) {
      IOHIDEventStruct event;
      result = (*queue)->getNextEvent(queue, &event, past, 0);
      if (result == kIOReturnSuccess) {
         int i;
         for (i=0; i<joy->parent.info.num_buttons; ++i) {
            if (joy->button_link[i].cookie == event.elementCookie) {
               int newvalue = event.value;
               if (*joy->button_link[i].ppressed != newvalue) {
                  *joy->button_link[i].ppressed = newvalue;
                  // emit event
                  ALLEGRO_EVENT evt;
                  if (newvalue)
                     evt.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
                  else
                     evt.type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
                  evt.joystick.button = i;
                  _al_event_source_emit_event(src, &evt);
               }
            }
         }
         for (i=0; i<joy->num_axis_links; ++i) {
            if (joy->axis_link[i].cookie == event.elementCookie) {
               SInt32 newvalue = event.value;
               if (joy->axis_link[i].intvalue != newvalue) {
                  joy->axis_link[i].intvalue = newvalue;
                  *joy->axis_link[i].pvalue = (joy->axis_link[i].offset + newvalue) * joy->axis_link[i].multiplier;
                  // emit event
                  ALLEGRO_EVENT evt;
                  evt.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
                  evt.joystick.axis = joy->axis_link[i].axis;
                  evt.joystick.pos = *joy->axis_link[i].pvalue;
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
   joy = &joysticks[joystick_count++];
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
   err = (*queue)->setEventCallout(queue, joystick_callback, joy, NULL);
   joy->queue = queue;
   (*interface)->copyMatchingElements(interface, NULL, (CFArrayRef*) &elements);
   NSEnumerator* enumerator = [elements objectEnumerator];
   NSDictionary* element;
   while ((element = (NSDictionary*) [enumerator nextObject])) {
      short usage = [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementUsageKey)]) shortValue];
      short usage_page = [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementUsagePageKey)]) shortValue];

      if (usage_page == kHIDPage_Button && num_buttons < _AL_MAX_JOYSTICK_BUTTONS) {
         joy->button_link[num_buttons].cookie = (IOHIDElementCookie) [((NSNumber*) [element objectForKey: (NSString*) CFSTR(kIOHIDElementCookieKey)]) pointerValue];
         joy->button_link[num_buttons].ppressed = &joy->state.button[num_buttons];
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
            joy->axis_link[0].pvalue = &joy->state.stick[0].axis[0];
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
            joy->axis_link[1].pvalue = &joy->state.stick[0].axis[1];
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
   [elements release];
}

// FIXME!
static const char *get_joystick_name(ALLEGRO_JOYSTICK *joy_)
{  
   (void)joy_;
   return "Joystick";
}

static bool get_joystick_active(ALLEGRO_JOYSTICK *joy_)
{
   (void)joy_;
   return true;
}

static int get_joystick_device_id(ALLEGRO_JOYSTICK *joy_)
{
   (void)joy_;
   // TODO: Add implementation here
   ALLEGRO_INFO("get_joystick_device_id: not implemented");
   return 0;
}


static bool reconfigure_joysticks(void)
{
   return false;
}

ALLEGRO_JOYSTICK_DRIVER* _al_osx_get_joystick_driver_10_4(void)
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
      vt->reconfigure_joysticks = reconfigure_joysticks;
      vt->get_name = get_joystick_name;
      vt->get_active = get_joystick_active;
      vt->get_device_id = get_joystick_device_id;
   }
   return vt;
}

/* init_joystick:
 *  Initializes the HID joystick driver.
 */
static bool init_joystick(void)
{
   joystick_count = 0;
   io_iterator_t iterator;
   io_object_t device;
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
   /* Ensure the source is added and started on the main thread */
   dispatch_sync(dispatch_get_main_queue(), ^{
      unsigned int i;
      CFRunLoopRef current = CFRunLoopGetCurrent();
      for (i=0; i<joystick_count; ++i) {
         ALLEGRO_JOYSTICK_OSX* joy = &joysticks[i];
         CFRunLoopAddSource(current,joy->source,kCFRunLoopDefaultMode);
         (*joy->queue)->start(joy->queue);
      }
   });
   return true;
}



/* exit_joystick:
 *  Shuts down the HID joystick driver.
 */
static void exit_joystick(void)
{
   /* Ensure the source is stopped and removed on the main thread */
   dispatch_sync(dispatch_get_main_queue(), ^{
      unsigned int i;
      CFRunLoopRef current = CFRunLoopGetCurrent();
      for (i=0; i<joystick_count; ++i) {
         ALLEGRO_JOYSTICK_OSX* joy = &joysticks[i];
         (*joy->queue)->stop(joy->queue);
         CFRunLoopRemoveSource(current,joy->source,kCFRunLoopDefaultMode);
      }
   });
   unsigned int i;
   for (i=0; i< joystick_count; ++i) {
      ALLEGRO_JOYSTICK_OSX* joy = &joysticks[i];
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
}

/* num_joysticks:
 *  Return number of active joysticks
 */
static int num_joysticks(void)
{
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
   ALLEGRO_JOYSTICK_OSX* joy = (ALLEGRO_JOYSTICK_OSX*) ajoy;
   memcpy(state, &joy->state,sizeof(*state));
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
