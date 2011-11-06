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
 *      New API (Leopard) support and hotplugging by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/platform/aintosx.h"

#import <CoreFoundation/CoreFoundation.h>
#import <IOKit/hid/IOHIDKeys.h>

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050

#import <IOKit/hid/IOHIDBase.h>

/* State transitions:
 *    unused -> born
 *    born -> alive
 *    born -> dying
 *    active -> dying
 *    dying -> unused
 */
typedef enum {
   JOY_STATE_UNUSED,
   JOY_STATE_BORN,
   JOY_STATE_ALIVE,
   JOY_STATE_DYING
} CONFIG_STATE;

// These values can be found in the USB HID Usage Tables:
// http://www.usb.org/developers/hidpage
#define GENERIC_DESKTOP_USAGE_PAGE 0x01
#define JOYSTICK_USAGE_NUMBER      0x04
#define GAMEPAD_USAGE_NUMBER       0x05

typedef struct {
   ALLEGRO_JOYSTICK parent;
   int num_buttons;
   int num_x_axes;
   int num_y_axes;
   int num_z_axes;
   IOHIDElementRef buttons[_AL_MAX_JOYSTICK_BUTTONS];
   IOHIDElementRef axes[_AL_MAX_JOYSTICK_STICKS][_AL_MAX_JOYSTICK_AXES];
   long min[_AL_MAX_JOYSTICK_STICKS][_AL_MAX_JOYSTICK_AXES];
   long max[_AL_MAX_JOYSTICK_STICKS][_AL_MAX_JOYSTICK_AXES];
   CONFIG_STATE cfg_state;
   ALLEGRO_JOYSTICK_STATE state;
   IOHIDDeviceRef ident;
} ALLEGRO_JOYSTICK_OSX;

static IOHIDManagerRef hidManagerRef;
static _AL_VECTOR joysticks;
static CONFIG_STATE new_joystick_state = JOY_STATE_ALIVE;
static bool initialized = false;
static ALLEGRO_MUTEX *add_mutex;

ALLEGRO_DEBUG_CHANNEL("MacOSX")

// function to create matching dictionary (for devices)
static CFMutableDictionaryRef CreateDeviceMatchingDictionary(
   UInt32 inUsagePage,
   UInt32 inUsage
) {
   // create a dictionary to add usage page/usages to
   CFMutableDictionaryRef result = CFDictionaryCreateMutable(
      kCFAllocatorDefault,
      0,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks
   );

   if (result) {
      // Add key for device type to refine the matching dictionary.
      CFNumberRef pageCFNumberRef = CFNumberCreate(
         kCFAllocatorDefault,
         kCFNumberIntType,
         &inUsagePage
      );

      if (pageCFNumberRef) {
         CFStringRef usage_page = CFSTR(kIOHIDDeviceUsagePageKey);

         CFDictionarySetValue(
            result, 
            usage_page,
            pageCFNumberRef
         );

         CFRelease(pageCFNumberRef);

         // note: the usage is only valid if the usage page is also defined

         CFNumberRef usageCFNumberRef = CFNumberCreate(
            kCFAllocatorDefault,
            kCFNumberIntType,
            &inUsage
         );

         if (usageCFNumberRef) {
            CFStringRef usage_key = CFSTR(kIOHIDDeviceUsageKey);

            CFDictionarySetValue(
               result, 
               usage_key,
               usageCFNumberRef
            );

            CFRelease(usageCFNumberRef);
         }
      }
   }

   return result;
}

static ALLEGRO_JOYSTICK_OSX *find_joystick(IOHIDDeviceRef ident)
{
   int i;
   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_OSX *joy = *(ALLEGRO_JOYSTICK_OSX **)_al_vector_ref(&joysticks, i);
      if (ident == joy->ident) {
         return joy;
      }
   }

   return NULL;
}

static const char *get_element_name(IOHIDElementRef elem, const char *default_name)
{
   CFStringRef name = IOHIDElementGetName(elem);
   if (name) {
      return CFStringGetCStringPtr(name, kCFStringEncodingUTF8);
   }
   else
      return default_name;
}

static void joy_null(ALLEGRO_JOYSTICK_OSX *joy)
{
   int i, j;

   // NULL the parent
   for (i = 0; i < _AL_MAX_JOYSTICK_BUTTONS; i++) {
   	joy->parent.info.button[i].name = NULL;
   }
   for (i = 0; i < _AL_MAX_JOYSTICK_STICKS; i++) {
   	joy->parent.info.stick[i].name = NULL;
	for (j = 0; j < _AL_MAX_JOYSTICK_AXES; j++) {
		joy->parent.info.stick[i].axis[j].name = NULL;
	}
   }
}

static void add_elements(CFArrayRef elements, ALLEGRO_JOYSTICK_OSX *joy)
{
   int i;
   char default_name[100];
           
   joy_null(joy);

   for (i = 0; i < CFArrayGetCount(elements); i++) {
      IOHIDElementRef elem = (IOHIDElementRef)CFArrayGetValueAtIndex(
         elements,
         i
      );
      int usage = IOHIDElementGetUsage(elem);
      if (IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Button) {
         if (usage >= 0 && usage < _AL_MAX_JOYSTICK_BUTTONS &&
            !joy->buttons[usage-1]) {
            joy->buttons[usage-1] = elem;
            sprintf(default_name, "Button %d", usage-1);
	    const char *name = get_element_name(elem, default_name);
	    char *str = al_malloc(strlen(name)+1);
	    strcpy(str, name);
            joy->parent.info.button[usage-1].name = str;
            joy->num_buttons++;
         }
      }
      else if (
         IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Misc) {
         long min = IOHIDElementGetLogicalMin(elem);
         long max = IOHIDElementGetLogicalMax(elem);
         if ((usage == kHIDUsage_GD_X ||
            usage == kHIDUsage_GD_Rx) &&
            joy->num_x_axes < _AL_MAX_JOYSTICK_STICKS) {
            joy->min[joy->num_x_axes][0] = min;
            joy->max[joy->num_x_axes][0] = max;
            sprintf(default_name, "Axis 0");
	    const char *name = get_element_name(elem, default_name);
	    char *str = al_malloc(strlen(name)+1);
	    strcpy(str, name);
            joy->parent.info.stick[joy->num_x_axes].axis[0].name = str;
            joy->axes[joy->num_x_axes++][0] = elem;
         }
         else if ((usage == kHIDUsage_GD_Y ||
            usage == kHIDUsage_GD_Ry) &&
            joy->num_x_axes < _AL_MAX_JOYSTICK_STICKS) {
            joy->min[joy->num_y_axes][1] = min;
            joy->max[joy->num_y_axes][1] = max;
            sprintf(default_name, "Axis 1");
	    const char *name = get_element_name(elem, default_name);
	    char *str = al_malloc(strlen(name)+1);
	    strcpy(str, name);
            joy->parent.info.stick[joy->num_y_axes].axis[1].name = str;
            joy->axes[joy->num_y_axes++][1] = elem;
         }
         else if ((usage == kHIDUsage_GD_Z ||
            usage == kHIDUsage_GD_Rz) &&
            joy->num_z_axes < _AL_MAX_JOYSTICK_STICKS) {
            joy->min[joy->num_z_axes][2] = min;
            joy->max[joy->num_z_axes][2] = max;
            sprintf(default_name, "Axis 2");
	    const char *name = get_element_name(elem, default_name);
	    char *str = al_malloc(strlen(name)+1);
	    strcpy(str, name);
            joy->parent.info.stick[joy->num_z_axes].axis[2].name = str;
            joy->axes[joy->num_z_axes++][2] = elem;
         }
      }
   }
}

static void osx_joy_generate_configure_event(void)
{
   if (!initialized) return;

   ALLEGRO_EVENT event;
   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.joystick.timestamp = al_current_time();

   _al_generate_joystick_event(&event);
}

static void device_add_callback(
   void *context,
   IOReturn result,
   void *sender,
   IOHIDDeviceRef ref
) {
   int i;

   (void)context;
   (void)result;
   (void)sender;
   
   al_lock_mutex(add_mutex);

   ALLEGRO_JOYSTICK_OSX *joy = find_joystick(ref);
   if (joy == NULL) {
      joy = al_calloc(1, sizeof(ALLEGRO_JOYSTICK_OSX));
      joy->ident = ref;
      ALLEGRO_JOYSTICK_OSX **back = _al_vector_alloc_back(&joysticks);
      *back = joy;
   }
   joy->cfg_state = new_joystick_state;

   CFArrayRef elements = IOHIDDeviceCopyMatchingElements(
      ref,
      NULL,
      kIOHIDOptionsTypeNone
   );

   add_elements(elements, joy);

   CFRelease(elements);

   // Fill in ALLEGRO_JOYSTICK properties
   joy->parent.info.num_sticks = joy->num_x_axes;
   joy->parent.info.num_buttons = joy->num_buttons;
   for (i = 0; i < joy->num_x_axes; i++) {
      int axes = 1;
      if (joy->num_y_axes >= i)
         axes++;
      if (joy->num_z_axes >= i)
         axes++;
      joy->parent.info.stick[i].num_axes = axes;
      char *buf = al_malloc(20);
      sprintf(buf, "Stick %d", i);
      joy->parent.info.stick[i].name = buf;
   }

   al_unlock_mutex(add_mutex);

   osx_joy_generate_configure_event();

   ALLEGRO_INFO("Found joystick (%d buttons, %d %d %d axes)\n",
      joy->num_buttons, joy->num_x_axes, joy->num_y_axes, joy->num_z_axes);
}

static void device_remove_callback(
   void *context,
   IOReturn result,
   void *sender,
   IOHIDDeviceRef ref
) {
   (void)context;
   (void)result;
   (void)sender;

   int i;
   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_OSX *joy = *(ALLEGRO_JOYSTICK_OSX **)_al_vector_ref(&joysticks, i);
      if (joy->ident == ref) {
         joy->cfg_state = JOY_STATE_DYING;
         osx_joy_generate_configure_event();
         return;
      }
   }
}

static void osx_joy_generate_axis_event(ALLEGRO_JOYSTICK_OSX *joy, int stick, int axis, float pos)
{
   joy->state.stick[stick].axis[axis] = pos;

   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!_al_event_source_needs_to_generate_event(es))
      return;

   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
   event.joystick.timestamp = al_current_time();
   event.joystick.id = (ALLEGRO_JOYSTICK *)joy;
   event.joystick.stick = stick;
   event.joystick.axis = axis;
   event.joystick.pos = pos;
   event.joystick.button = 0;

   _al_event_source_emit_event(es, &event);
}

static void osx_joy_generate_button_event(ALLEGRO_JOYSTICK_OSX *joy, int button, ALLEGRO_EVENT_TYPE event_type)
{
   joy->state.button[button] = event_type == ALLEGRO_EVENT_JOYSTICK_BUTTON_UP ?
      0 : 1;;

   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!_al_event_source_needs_to_generate_event(es))
      return;

   event.joystick.type = event_type;
   event.joystick.timestamp = al_current_time();
   event.joystick.id = (ALLEGRO_JOYSTICK *)joy;
   event.joystick.stick = 0;
   event.joystick.axis = 0;
   event.joystick.pos = 0.0;
   event.joystick.button = button;

   _al_event_source_emit_event(es, &event);
}

static void value_callback(
   void *context,
   IOReturn result,
   void *sender,
   IOHIDValueRef value
) {
   if (!initialized) return;

   (void)context;
   (void)result;
   (void)sender;

   IOHIDElementRef elem = IOHIDValueGetElement(value);
   IOHIDDeviceRef ref = IOHIDElementGetDevice(elem);
   ALLEGRO_JOYSTICK_OSX *joy = find_joystick(ref);

   if (!joy) return;
   
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
   _al_event_source_lock(es);

   int i;
   for (i = 0; i < joy->num_buttons; i++) {
      if (joy->buttons[i] == elem) {
         ALLEGRO_EVENT_TYPE type;
         if (IOHIDValueGetIntegerValue(value) == 0)
            type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
         else
            type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
         osx_joy_generate_button_event(joy, i, type);
         goto done;
      }
   }
   int int_value = IOHIDValueGetIntegerValue(value);
   int stick = -1;
   int axis = -1;
   for (i = 0; i < joy->num_x_axes; i++) {
      if (joy->axes[i][0] == elem) {
         stick = i;
         axis = 0;
         goto gen_axis_event;
      }
   }
   for (i = 0; i < joy->num_y_axes; i++) {
      if (joy->axes[i][1] == elem) {
         stick = i;
         axis = 1;
         goto gen_axis_event;
      }
   }
   for (i = 0; i < joy->num_z_axes; i++) {
      if (joy->axes[i][2] == elem) {
         stick = i;
         axis = 2;
         goto gen_axis_event;
      }
   }

   // Unknown event
   goto done;

gen_axis_event:
   {
      float pos;
      long min = joy->min[stick][axis];
      long max = joy->max[stick][axis];
      if (min < 0) {
         if (int_value < 0)
            pos = -(float)int_value/min;
         else
            pos = (float)int_value/max;
      }
      else {
         pos = ((float)int_value/max*2) - 1;
      }

      osx_joy_generate_axis_event(joy, stick, axis, pos);
   }

done:
   _al_event_source_unlock(es);
}

/* init_joystick:
 *  Initializes the HID joystick driver.
 */
static bool init_joystick(void)
{
   add_mutex = al_create_mutex();

   hidManagerRef = IOHIDManagerCreate(
      kCFAllocatorDefault,
      kIOHIDOptionsTypeNone
   );

   if (CFGetTypeID(hidManagerRef) != IOHIDManagerGetTypeID()) {
      ALLEGRO_ERROR("Unable to create HID Manager\n");
      return false;
   }

   // Set which devices we want to match
   CFMutableDictionaryRef criteria0 = CreateDeviceMatchingDictionary(
      GENERIC_DESKTOP_USAGE_PAGE,
      JOYSTICK_USAGE_NUMBER
   );
   CFMutableDictionaryRef criteria1 = CreateDeviceMatchingDictionary(
      GENERIC_DESKTOP_USAGE_PAGE,
      GAMEPAD_USAGE_NUMBER
   );
   CFMutableDictionaryRef criteria_list[] = {
      criteria0,
      criteria1
   };
   CFArrayRef criteria = CFArrayCreate(
      kCFAllocatorDefault,
      (const void **)criteria_list,
      2, NULL
   );

   IOHIDManagerSetDeviceMatchingMultiple(
      hidManagerRef,
      criteria
   );

   CFRelease(criteria0);
   CFRelease(criteria1);
   CFRelease(criteria);

   /* Register for plug/unplug notifications */
   IOHIDManagerRegisterDeviceMatchingCallback(
      hidManagerRef,
      device_add_callback,
      NULL
   );
   IOHIDManagerRegisterDeviceRemovalCallback(
      hidManagerRef,
      device_remove_callback,
      NULL
   );

   // Register for value changes
   IOHIDManagerRegisterInputValueCallback(
      hidManagerRef,
      value_callback,
      NULL
   );

   IOHIDManagerScheduleWithRunLoop(
      hidManagerRef,
      CFRunLoopGetMain(),
      kCFRunLoopDefaultMode
   );

   _al_vector_init(&joysticks, sizeof(ALLEGRO_JOYSTICK_OSX *));

   al_lock_mutex(add_mutex);

   IOReturn ret = IOHIDManagerOpen(
      hidManagerRef,
      kIOHIDOptionsTypeSeizeDevice
   );

   al_unlock_mutex(add_mutex);

   if (ret != kIOReturnSuccess) {
      return false;
   }

   // Wait for the devices to be enumerated
   int count;
   int size;
   do {
      al_rest(0.001);
      CFSetRef devices = IOHIDManagerCopyDevices(hidManagerRef);
      if (devices == nil) {
         break;
      }
      count = CFSetGetCount(devices);
      CFRelease(devices);
      al_lock_mutex(add_mutex);
      size = _al_vector_size(&joysticks);
      al_unlock_mutex(add_mutex);
   } while (size < count);

   new_joystick_state = JOY_STATE_BORN;

   initialized = true;

   return true;
}

/* exit_joystick:
 *  Shuts down the HID joystick driver.
 */
static void exit_joystick(void)
{
   al_destroy_mutex(add_mutex);

   IOHIDManagerUnscheduleFromRunLoop(
      hidManagerRef,
      CFRunLoopGetCurrent(),
      kCFRunLoopDefaultMode
   );
   // Unregister from value changes
   IOHIDManagerRegisterInputValueCallback(
      hidManagerRef,
      NULL,
      NULL
   );
   IOHIDManagerClose(
      hidManagerRef,
      kIOHIDOptionsTypeNone
   );
   CFRelease(hidManagerRef);

   _al_vector_free(&joysticks);
}

/* num_joysticks:
 *  Return number of active joysticks
 */
static int num_joysticks(void)
{
   int i;
   int count = 0;
   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_OSX *joy = *(ALLEGRO_JOYSTICK_OSX **)_al_vector_ref(&joysticks, i);
      if (joy->cfg_state == JOY_STATE_ALIVE) {
         count++;
      }
   }

   return count;
}

/* get_joystick:
 * Get a pointer to a joystick structure
 */
static ALLEGRO_JOYSTICK* get_joystick(int index)
{
   ASSERT(index >= 0 && index < (int)_al_vector_size(&joysticks));

   int i;
   int count = 0;
   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_OSX *joy = *(ALLEGRO_JOYSTICK_OSX **)_al_vector_ref(&joysticks, i);
      if (joy->cfg_state == JOY_STATE_ALIVE || 
         joy->cfg_state == JOY_STATE_DYING) {
            if (count == index) {
               return (ALLEGRO_JOYSTICK *)joy;
            }
            count++;
      }
   }

   return NULL;
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
static void get_joystick_state(ALLEGRO_JOYSTICK *joy_, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ALLEGRO_JOYSTICK_OSX *joy = (ALLEGRO_JOYSTICK_OSX *) joy_;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   _al_event_source_lock(es);
   {
      *ret_state = joy->state;
   }
   _al_event_source_unlock(es);
}

static bool reconfigure_joysticks(void)
{
   int i;
   bool ret = false;
   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_OSX *joy = *(ALLEGRO_JOYSTICK_OSX **)_al_vector_ref(&joysticks, i);
      if (joy->cfg_state == JOY_STATE_DYING) {
         joy->cfg_state = JOY_STATE_UNUSED;
         for (i = 0; i < _AL_MAX_JOYSTICK_BUTTONS; i++) {
            al_free((char *)joy->parent.info.button[i].name);
         }
         for (i = 0; i < _AL_MAX_JOYSTICK_STICKS; i++) {
            int j;
            al_free(joy->parent.info.stick[i].name);
            for (j = 0; j < _AL_MAX_JOYSTICK_AXES; j++) {
               al_free(joy->parent.info.stick[i].axis[j].name);
            }
         }
	 joy_null(joy);
         joy->num_buttons =
         joy->num_x_axes =
         joy->num_y_axes =
         joy->num_z_axes = 0;
         memset(joy->buttons, 0, _AL_MAX_JOYSTICK_BUTTONS*sizeof(IOHIDElementRef));
         memset(&joy->state, 0, sizeof(ALLEGRO_JOYSTICK_STATE));
      }
      else if (joy->cfg_state == JOY_STATE_BORN)
         joy->cfg_state = JOY_STATE_ALIVE;
      else
         continue;
      ret = true;
   }

   return ret;
}

// FIXME!
static const char *get_joystick_name(ALLEGRO_JOYSTICK *joy_)
{  
   (void)joy_;
   return "Joystick";
}

static bool get_joystick_active(ALLEGRO_JOYSTICK *joy_)
{
   ALLEGRO_JOYSTICK_OSX *joy = (ALLEGRO_JOYSTICK_OSX *)joy_;
   return joy->cfg_state == JOY_STATE_ALIVE || joy->cfg_state == JOY_STATE_DYING;
}

ALLEGRO_JOYSTICK_DRIVER* _al_osx_get_joystick_driver_10_5(void)
{
   static ALLEGRO_JOYSTICK_DRIVER* vt = NULL;
   if (vt == NULL) {
      vt = al_malloc(sizeof(*vt));
      memset(vt, 0, sizeof(*vt));
      vt->joydrv_ascii_name = "OSX HID Driver";
      vt->init_joystick = init_joystick;
      vt->exit_joystick = exit_joystick;
      vt->reconfigure_joysticks = reconfigure_joysticks;
      vt->num_joysticks = num_joysticks;
      vt->get_joystick = get_joystick;
      vt->release_joystick = release_joystick;
      vt->get_joystick_state = get_joystick_state;
      vt->get_name = get_joystick_name;
      vt->get_active = get_joystick_active;
   }
   return vt;
}

#endif // Leopard+

ALLEGRO_JOYSTICK_DRIVER* _al_osx_get_joystick_driver_10_4(void);
ALLEGRO_JOYSTICK_DRIVER* _al_osx_get_joystick_driver_10_5(void);

ALLEGRO_JOYSTICK_DRIVER* _al_osx_get_joystick_driver(void)
{
   SInt32 major, minor;

   Gestalt(gestaltSystemVersionMajor, &major);
   Gestalt(gestaltSystemVersionMinor, &minor);

   if (major >= 10 && minor >= 5) {
   	return _al_osx_get_joystick_driver_10_5();
   }
   else {
   	return _al_osx_get_joystick_driver_10_4();
   }
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
