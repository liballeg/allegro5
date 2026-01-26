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
   IOHIDElementRef elem;
   _AL_JOYSTICK_OUTPUT output;
} BUTTON_MAPPING;

typedef struct {
   IOHIDElementRef elem;
   long min;
   long max;
   _AL_JOYSTICK_OUTPUT x_output;
   _AL_JOYSTICK_OUTPUT y_output;
} HAT_MAPPING;

typedef struct {
   IOHIDElementRef elem;
   long min;
   long max;
   _AL_JOYSTICK_OUTPUT output;
} AXIS_MAPPING;

typedef struct {
   ALLEGRO_JOYSTICK parent;
   BUTTON_MAPPING buttons[_AL_MAX_JOYSTICK_BUTTONS];
   AXIS_MAPPING axes[_AL_MAX_JOYSTICK_STICKS * _AL_MAX_JOYSTICK_AXES];
   HAT_MAPPING hats[_AL_MAX_JOYSTICK_STICKS];
   int hat_state[_AL_MAX_JOYSTICK_AXES];
   CONFIG_STATE cfg_state;
   ALLEGRO_JOYSTICK_STATE state;
   IOHIDDeviceRef ident;
   char *name;
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

static ALLEGRO_JOYSTICK_OSX *find_or_insert_joystick(IOHIDDeviceRef ident)
{
   ALLEGRO_JOYSTICK_OSX *joy = find_joystick(ident);
   if (joy)
      return joy;
   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      joy = *(ALLEGRO_JOYSTICK_OSX **)_al_vector_ref(&joysticks, i);
      if (joy->cfg_state == JOY_STATE_UNUSED) {
         joy->ident = ident;
         return joy;
      }
   }

   joy = al_calloc(1, sizeof(ALLEGRO_JOYSTICK_OSX));
   joy->ident = ident;
   ALLEGRO_JOYSTICK_OSX **back = _al_vector_alloc_back(&joysticks);
   *back = joy;
   return joy;
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

static bool compat_5_2_9(void) {
   // Broken usages.
   return _al_get_joystick_compat_version() < AL_ID(5, 2, 10, 0);
}

static bool compat_5_2_10(void) {
   // Different axis normalization, hat events + mappings.
   return _al_get_joystick_compat_version() < AL_ID(5, 2, 11, 0);
}

static void add_elements(CFArrayRef elements, ALLEGRO_JOYSTICK_OSX *joy)
{
   int i;
   char default_name[100];
   int stick_class = -1;
   int axis = 0;
   int num_sticks = -1;
   int num_buttons = 0;
   int num_hats = 0;
   int num_axes = 0;
   bool old_style = compat_5_2_9();

   for (i = 0; i < CFArrayGetCount(elements); i++) {
      IOHIDElementRef elem = (IOHIDElementRef)CFArrayGetValueAtIndex(
         elements,
         i
      );

      int usage = IOHIDElementGetUsage(elem);
      int usage_page = IOHIDElementGetUsagePage(elem);
      if (IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Button) {
         int idx;
         if (old_style)
            idx = usage - 1;
         else {
            idx = num_buttons;
            // 0x09 is the Button Page.
            if (usage_page != 0x09)
               continue;
         }
         if (idx >= 0 && idx < _AL_MAX_JOYSTICK_BUTTONS && !joy->buttons[idx].elem) {
            joy->buttons[idx].elem = elem;
            joy->buttons[idx].output = _al_new_joystick_button_output(idx);
            sprintf(default_name, "Button %d", idx);
            joy->parent.info.button[idx].name = _al_strdup(get_element_name(elem, default_name));
            num_buttons++;
         }
      }
      else if (IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Misc) {
         long min = IOHIDElementGetLogicalMin(elem);
         long max = IOHIDElementGetLogicalMax(elem);
         int new_stick_class = -1;

         switch (usage) {
            case kHIDUsage_GD_X:
            case kHIDUsage_GD_Y:
            case kHIDUsage_GD_Z:
               new_stick_class = 1;
               break;

            case kHIDUsage_GD_Rx:
            case kHIDUsage_GD_Ry:
            case kHIDUsage_GD_Rz:
               new_stick_class = 2;
               break;

            case kHIDUsage_GD_Hatswitch:
               new_stick_class = 3;
               break;
         }

         if (new_stick_class < 0)
            continue;

         if (stick_class != new_stick_class) {
            if (num_sticks >= _AL_MAX_JOYSTICK_STICKS - 1)
               break;

            num_sticks++;

            stick_class = new_stick_class;
            axis = 0;

            char *buf = al_malloc(20);
            sprintf(buf, "Stick %d", num_sticks);
            joy->parent.info.stick[num_sticks].name = buf;
         }
         else
            axis++;

         if (stick_class == 3) {
            if (num_hats >= _AL_MAX_JOYSTICK_AXES)
               continue;

            HAT_MAPPING *map = &joy->hats[num_hats];
            map->min = min;
            map->max = max;
            map->elem = elem;
            map->x_output = _al_new_joystick_stick_output(num_sticks, 0);
            map->y_output = _al_new_joystick_stick_output(num_sticks, 1);

            for (axis = 0; axis < 2; axis++) {
               sprintf(default_name, "Axis %i", axis);
               joy->parent.info.stick[num_sticks].axis[axis].name = _al_strdup(get_element_name(elem, default_name));
            }
            joy->parent.info.stick[num_sticks].num_axes = 2;

            num_hats++;
         }
         else {
            if (num_axes >= _AL_MAX_JOYSTICK_STICKS * _AL_MAX_JOYSTICK_AXES)
               continue;

            AXIS_MAPPING *map = &joy->axes[num_axes];
            map->min = min;
            map->max = max;
            map->elem = elem;
            map->output = _al_new_joystick_stick_output(num_sticks, axis);

            sprintf(default_name, "Axis %i", axis);
            joy->parent.info.stick[num_sticks].axis[axis].name = _al_strdup(get_element_name(elem, default_name));
            joy->parent.info.stick[num_sticks].num_axes++;

            num_axes++;
         }
      }
   }
   joy->parent.info.num_sticks = num_sticks + 1;
   joy->parent.info.num_buttons = num_buttons;
}

static NSInteger element_compare_func(id val1, id val2)
{
    IOHIDElementRef elem1 = [(NSValue *)val1 pointerValue];
    IOHIDElementRef elem2 = [(NSValue *)val2 pointerValue];

    int usage1 = IOHIDElementGetUsage(elem1);
    int usage2 = IOHIDElementGetUsage(elem2);
    if (usage1 < usage2)
       return kCFCompareLessThan;
    if (usage1 > usage2)
       return kCFCompareGreaterThan;
    return kCFCompareEqualTo;
}

static bool add_elements_with_mapping(CFArrayRef elements, ALLEGRO_JOYSTICK_OSX *joy, const _AL_JOYSTICK_MAPPING *mapping)
{
   int num_buttons = 0;
   int num_hats = 0;
   int num_axes = 0;
   int num_buttons_mapped = 0;
   int num_hats_mapped = 0;
   int num_axes_mapped = 0;
   _AL_JOYSTICK_OUTPUT *output;

   NSMutableArray* buttons_elems = [[[NSMutableArray alloc] init] autorelease];
   NSMutableArray* axes_elems = [[[NSMutableArray alloc] init] autorelease];
   NSMutableArray* hats_elems = [[[NSMutableArray alloc] init] autorelease];

   for (int i = 0; i < CFArrayGetCount(elements); i++) {
      IOHIDElementRef elem = (IOHIDElementRef)CFArrayGetValueAtIndex(
         elements,
         i
      );

      int usage = IOHIDElementGetUsage(elem);
      int usage_page = IOHIDElementGetUsagePage(elem);
      if (IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Button) {
         // 0x09 is the Button Page.
         if (usage_page != 0x09)
            continue;
         [buttons_elems addObject: [NSValue valueWithPointer:elem]];
      }
      else if (IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Misc) {
         switch (usage) {
            case kHIDUsage_GD_X:
            case kHIDUsage_GD_Y:
            case kHIDUsage_GD_Z:
            case kHIDUsage_GD_Rx:
            case kHIDUsage_GD_Ry:
            case kHIDUsage_GD_Rz:
            case kHIDUsage_GD_Slider:
            case kHIDUsage_GD_Dial:
            case kHIDUsage_GD_Wheel:
            case kHIDUsage_Sim_Accelerator:
            case kHIDUsage_Sim_Brake: {
               [axes_elems addObject: [NSValue valueWithPointer:elem]];
               break;
            }
            case kHIDUsage_GD_Hatswitch: {
               [hats_elems addObject: [NSValue valueWithPointer:elem]];
               break;
            }
            default:
               continue;
         }
      }
   }

   // This sorting is inspired by what GLFW does.
   [buttons_elems sortWithOptions:NSSortStable
               usingComparator:^NSComparisonResult(id val1, id val2) {
      return element_compare_func(val1, val2);
   }];
   [axes_elems sortWithOptions:NSSortStable
               usingComparator:^NSComparisonResult(id val1, id val2) {
      return element_compare_func(val1, val2);
   }];
   [hats_elems sortWithOptions:NSSortStable
               usingComparator:^NSComparisonResult(id val1, id val2) {
      return element_compare_func(val1, val2);
   }];

   for (int i = 0; i < (int)[buttons_elems count]; i++) {
      IOHIDElementRef elem = [(NSValue *)buttons_elems[i] pointerValue];
      int idx = num_buttons;
      if (idx >= 0 && idx < _AL_MAX_JOYSTICK_BUTTONS && !joy->buttons[idx].elem) {
         joy->buttons[idx].elem = elem;
         output = _al_get_joystick_output(&mapping->button_map, idx);
         if (output) {
            joy->buttons[idx].output = *output;
            num_buttons_mapped++;
         }
         num_buttons++;
      }
   }

   for (int i = 0; i < (int)[axes_elems count]; i++) {
      IOHIDElementRef elem = [(NSValue *)axes_elems[i] pointerValue];
      int usage = IOHIDElementGetUsage(elem);
      long min = IOHIDElementGetLogicalMin(elem);
      long max = IOHIDElementGetLogicalMax(elem);
      if (num_axes >= _AL_MAX_JOYSTICK_STICKS * _AL_MAX_JOYSTICK_AXES)
         continue;

      AXIS_MAPPING *map = &joy->axes[num_axes];
      map->min = min;
      map->max = max;
      map->elem = elem;
      output = _al_get_joystick_output(&mapping->axis_map, num_axes);
      if (output) {
         map->output = *output;
         num_axes_mapped++;
      }
      num_axes++;
   }

   for (int i = 0; i < (int)[hats_elems count]; i++) {
      IOHIDElementRef elem = [(NSValue *)hats_elems[i] pointerValue];
      long min = IOHIDElementGetLogicalMin(elem);
      long max = IOHIDElementGetLogicalMax(elem);
      if (num_hats >= _AL_MAX_JOYSTICK_AXES)
         continue;

      HAT_MAPPING *map = &joy->hats[num_hats];
      map->min = min;
      map->max = max;
      map->elem = elem;
      output = _al_get_joystick_output(&mapping->hat_map, 2 * num_hats);
      if (output) {
         map->x_output = *output;
         num_hats_mapped++;
      }
      output = _al_get_joystick_output(&mapping->hat_map, 2 * num_hats + 1);
      if (output) {
         map->y_output = *output;
         num_hats_mapped++;
      }
      num_hats++;
   }

   if (num_axes_mapped != (int)_al_vector_size(&mapping->axis_map)) {
      ALLEGRO_ERROR("Could not use mapping, some axes are not mapped.\n");
      return false;
   }
   if (num_hats_mapped != (int)_al_vector_size(&mapping->hat_map)) {
      ALLEGRO_ERROR("Could not use mapping, some hats are not mapped.\n");
      return false;
   }
   if (num_buttons_mapped != (int)_al_vector_size(&mapping->button_map)) {
      ALLEGRO_ERROR("Could not use mapping, some buttons are not mapped.\n");
      return false;
   }
   return true;
}

static void set_joystick_name(ALLEGRO_JOYSTICK_OSX *joy)
{
   CFStringRef str;

   str = IOHIDDeviceGetProperty(joy->ident, CFSTR(kIOHIDProductKey));
   if (str) {
      CFIndex length = CFStringGetLength(str);
      CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
      if (joy->name) {
         al_free(joy->name);
      }
      joy->name = (char *)al_malloc(maxSize);
      if (joy->name) {
         if (CFStringGetCString(str, joy->name, maxSize, kCFStringEncodingUTF8)) {
            return;
         }
      }
   }
   joy->name = _al_strdup("Joystick");
}

static void osx_joy_generate_configure_event(void)
{
   if (!initialized) return;

   ALLEGRO_EVENT event;
   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.joystick.timestamp = al_current_time();

   _al_generate_joystick_event(&event);
}

static void add_joystick_device(IOHIDDeviceRef ref, bool emit_reconfigure_event)
{
   al_lock_mutex(add_mutex);

   ALLEGRO_JOYSTICK_OSX *joy = find_or_insert_joystick(ref);

   if (joy && (joy->cfg_state == JOY_STATE_BORN || joy->cfg_state == JOY_STATE_ALIVE)) {
     al_unlock_mutex(add_mutex);
     return;
   }

   joy->cfg_state = new_joystick_state;

   CFArrayRef elements = IOHIDDeviceCopyMatchingElements(
      ref,
      NULL,
      kIOHIDOptionsTypeNone
   );

   CFTypeRef type;

   type = IOHIDDeviceGetProperty(joy->ident, CFSTR(kIOHIDVendorIDKey));
   int32_t vendor = 0;
   if (type)
      CFNumberGetValue(type, kCFNumberSInt32Type, &vendor);

   type = IOHIDDeviceGetProperty(joy->ident, CFSTR(kIOHIDProductIDKey));
   int32_t product = 0;
   if (type)
      CFNumberGetValue(type, kCFNumberSInt32Type, &product);

   type = IOHIDDeviceGetProperty(joy->ident, CFSTR(kIOHIDVersionNumberKey));
   int32_t version = 0;
   if (type)
      CFNumberGetValue(type, kCFNumberSInt32Type, &version);

   CFStringRef str;

   str = IOHIDDeviceGetProperty(joy->ident, CFSTR(kIOHIDManufacturerKey));
   char manufacturer_str[256];
   if (!str || !CFStringGetCString(str, manufacturer_str, sizeof(manufacturer_str), kCFStringEncodingUTF8))
      manufacturer_str[0] = '\0';

   str = IOHIDDeviceGetProperty(joy->ident, CFSTR(kIOHIDProductKey));
   char product_str[256];
   if (!str || !CFStringGetCString(str, product_str, sizeof(product_str), kCFStringEncodingUTF8))
      product_str[0] = '\0';

   ALLEGRO_JOYSTICK_GUID guid = _al_new_joystick_guid(0x03, (uint16_t)vendor, (uint16_t)product, (uint16_t)version, manufacturer_str, product_str, 0, 0);

   const _AL_JOYSTICK_MAPPING *mapping = _al_get_gamepad_mapping("Mac OS X", guid);

   joy->parent.info.guid = guid;
   if (mapping && add_elements_with_mapping(elements, joy, mapping)) {
      _al_fill_gamepad_info(&joy->parent.info);
      joy->name = _al_strdup(mapping->name);
   }
   else {
      set_joystick_name(joy);
      add_elements(elements, joy);
   }

   CFRelease(elements);

   al_unlock_mutex(add_mutex);

   if (emit_reconfigure_event) osx_joy_generate_configure_event();

   ALLEGRO_INFO("Found joystick (%d buttons, %d sticks)\n",
      joy->parent.info.num_buttons, joy->parent.info.num_sticks);
}

static int enumerate_and_create_initial_joystick_devices(IOHIDManagerRef manager)
{
   int i;
   int num_joysticks_enumerated = 0;

   CFSetRef devices = IOHIDManagerCopyDevices(manager);
   if (devices == NULL)
   {
      // There are no devices to enumerate
   }
   else
   {
      CFIndex num_devices = CFSetGetCount(devices);
      IOHIDDeviceRef *device_arr = al_calloc(num_devices, sizeof(IOHIDDeviceRef));
      CFSetGetValues(devices, (const void **) device_arr);

      for (i = 0; i < num_devices; i++) {
         IOHIDDeviceRef dev = device_arr[i];
         add_joystick_device(dev, false);
         num_joysticks_enumerated++;
      }
      al_free(device_arr);
      CFRelease(devices);
   }

   return num_joysticks_enumerated;
}

static void device_add_callback(
   void *context,
   IOReturn result,
   void *sender,
   IOHIDDeviceRef ref
) {
   (void)context;
   (void)result;
   (void)sender;

   add_joystick_device(ref, true);
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

#define MAX_HAT_DIRECTIONS 8
struct HAT_MAPPING {
   int axisV;
   int axisH;
} hat_mapping[MAX_HAT_DIRECTIONS + 1] = {
   { -1,  0 }, // 0
   { -1,  1 }, // 1
   {  0,  1 }, // 2
   {  1,  1 }, // 3
   {  1,  0 }, // 4
   {  1, -1 }, // 5
   {  0, -1 }, // 6
   { -1, -1 }, // 7
   {  0,  0 }, // 8
};

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
   int int_value = (int)IOHIDValueGetIntegerValue(value);
   IOHIDDeviceRef ref = IOHIDElementGetDevice(elem);
   ALLEGRO_JOYSTICK_OSX *joy = find_joystick(ref);

   if (!joy) return;

   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();
   _al_event_source_lock(es);

   for (int i = 0; i < _AL_MAX_JOYSTICK_BUTTONS; i++) {
      BUTTON_MAPPING *map = &joy->buttons[i];
      if (!map->elem)
         break;
      if (map->elem != elem)
         continue;
      _al_joystick_generate_button_event(&joy->parent, &joy->state, map->output, int_value);
   }

   for (int i = 0; i < _AL_MAX_JOYSTICK_STICKS * _AL_MAX_JOYSTICK_AXES; i++) {
      AXIS_MAPPING *map = &joy->axes[i];
      if (!map->elem)
         break;
      if (map->elem != elem)
         continue;

      float pos;
      long min = map->min;
      long max = map->max;
      if (compat_5_2_10()) {
         if (min < 0) {
            if (int_value < 0)
               pos = -(float)int_value / min;
            else
               pos = (float)int_value / max;
         }
         else {
            pos = ((float)int_value / max * 2) - 1;
         }
      }
      else {
         long range = max - min;
         if (range > 0)
            pos = 2. * ((float)int_value - min) / range - 1.;
         else
            /* TODO: Add auto-calibration? */
            pos = (float)int_value;
      }
      _al_joystick_generate_axis_event(&joy->parent, &joy->state, map->output, pos);
   }

   for (int i = 0; i < _AL_MAX_JOYSTICK_AXES; i++) {
      HAT_MAPPING *map = &joy->hats[i];
      if (!map->elem)
         break;
      if (map->elem != elem)
         continue;

      long min = map->min;
      long max = map->max;
      if (compat_5_2_10()) {
         if (int_value >= min && int_value <= max) {
            long index = int_value - min;
            if (index < MAX_HAT_DIRECTIONS) {
               _al_joystick_generate_axis_event(&joy->parent, &joy->state, map->x_output, (float)hat_mapping[index].axisH);
               _al_joystick_generate_axis_event(&joy->parent, &joy->state, map->y_output, (float)hat_mapping[index].axisV);
            }
         } else {
            _al_joystick_generate_axis_event(&joy->parent, &joy->state, map->x_output, 0);
            _al_joystick_generate_axis_event(&joy->parent, &joy->state, map->y_output, 0);
         }
      }
      else {
         long state = (int)(int_value - min);
         if (state < 0 || state >= MAX_HAT_DIRECTIONS)
            state = MAX_HAT_DIRECTIONS;
         float old_dx = (float)hat_mapping[joy->hat_state[i]].axisH;
         float old_dy = (float)hat_mapping[joy->hat_state[i]].axisV;
         float dx = (float)hat_mapping[state].axisH;
         float dy = (float)hat_mapping[state].axisV;
         joy->hat_state[i] = (int)state;

         if (old_dx != dx)
            _al_joystick_generate_axis_event(&joy->parent, &joy->state, map->x_output, dx);
         if (old_dy != dy)
            _al_joystick_generate_axis_event(&joy->parent, &joy->state, map->y_output, dy);
      }
   }

   _al_event_source_unlock(es);
}

static void destroy_joystick(ALLEGRO_JOYSTICK_OSX *joy) {
   _al_destroy_joystick_info(&joy->parent.info);
   al_free(joy->name);
   memset(joy, 0, sizeof(ALLEGRO_JOYSTICK_OSX));
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

   int num_joysticks_created = enumerate_and_create_initial_joystick_devices(hidManagerRef);
   if (num_joysticks_created > 0) osx_joy_generate_configure_event();

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

   for (int i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_OSX *joy = *(ALLEGRO_JOYSTICK_OSX **)_al_vector_ref(&joysticks, i);
      destroy_joystick(joy);
   }
   _al_vector_free(&joysticks);

   initialized = false;
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
         destroy_joystick(joy);
         joy->cfg_state = JOY_STATE_UNUSED;
      }
      else if (joy->cfg_state == JOY_STATE_BORN)
         joy->cfg_state = JOY_STATE_ALIVE;
      else
         continue;
      ret = true;
   }

   return ret;
}

static const char *get_joystick_name(ALLEGRO_JOYSTICK *joy_)
{
   ALLEGRO_JOYSTICK_OSX *joy = (ALLEGRO_JOYSTICK_OSX *)joy_;
   return joy->name;
}

static bool get_joystick_active(ALLEGRO_JOYSTICK *joy_)
{
   ALLEGRO_JOYSTICK_OSX *joy = (ALLEGRO_JOYSTICK_OSX *)joy_;
   return joy->cfg_state == JOY_STATE_ALIVE || joy->cfg_state == JOY_STATE_DYING;
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

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
