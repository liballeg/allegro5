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
 *      MacOS X HID Manager access routines.
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

static void hid_store_element_data(CFTypeRef element, int type, HID_DEVICE *device);
static void hid_scan_element(CFTypeRef element, HID_DEVICE *device);
static void hid_scan_collection(CFMutableDictionaryRef properties, HID_DEVICE *device);



static void hid_store_element_data(CFTypeRef element, int type, HID_DEVICE *device)
{
   HID_ELEMENT *hid_element;
   CFTypeRef type_ref;
   AL_CONST char *name;
   
   if (device->num_elements >= HID_MAX_DEVICE_ELEMENTS)
      return;
   hid_element = &device->element[device->num_elements++];
   hid_element->type = type;
   CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementCookieKey)), kCFNumberIntType, &hid_element->cookie);
   CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementMinKey)), kCFNumberIntType, &hid_element->min);
   CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementMaxKey)), kCFNumberIntType, &hid_element->max);
   type_ref = CFDictionaryGetValue(element, CFSTR(kIOHIDElementNameKey));
   if ((type_ref) && (name = CFStringGetCStringPtr(type_ref, CFStringGetSystemEncoding())))
      hid_element->name = strdup(name);
   else
      hid_element->name = NULL;
}



static void hid_scan_element(CFTypeRef element, HID_DEVICE *device)
{
   int type, usage_page, usage;
   
   CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementTypeKey)), kCFNumberIntType, &type);
   if (type == kIOHIDElementTypeCollection)
      hid_scan_collection((CFMutableDictionaryRef)element, device);
   else if ((type == kIOHIDElementTypeInput_Misc) ||
            (type == kIOHIDElementTypeInput_Button) ||
	    ((device->type != HID_MOUSE) && (type == kIOHIDElementTypeInput_Axis))) {
      CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsagePageKey)), kCFNumberIntType, &usage_page);
      CFNumberGetValue(CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsageKey)), kCFNumberIntType, &usage);
      if (usage_page == kHIDPage_GenericDesktop) {
	 switch (usage) {
	    
	    case kHIDUsage_GD_X:
	    case kHIDUsage_GD_Y:
	    case kHIDUsage_GD_Z:
	    case kHIDUsage_GD_Slider:
	    case kHIDUsage_GD_Dial:
	    case kHIDUsage_GD_Wheel:
	       hid_store_element_data(element, HID_ELEMENT_AXIS, device);
	       break;
	       
	    case kHIDUsage_GD_Hatswitch:
	       hid_store_element_data(element, HID_ELEMENT_DIGITAL_AXIS, device);
	       break;
	 }
      }
      else if (usage_page == kHIDPage_Button) {
	 hid_store_element_data(element, HID_ELEMENT_BUTTON, device);
      }
   }
}



static void hid_scan_collection(CFMutableDictionaryRef properties, HID_DEVICE *device)
{
   CFTypeRef array_ref, type_ref;
   int i;
   
   array_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementKey));
   if ((array_ref) && (CFGetTypeID(array_ref) == CFArrayGetTypeID())) {
      for (i = 0; i < CFArrayGetCount(array_ref); i++) {
         type_ref = CFArrayGetValueAtIndex(array_ref, i);
	 if (CFGetTypeID(type_ref) == CFDictionaryGetTypeID())
	    hid_scan_element(type_ref, device);
      }
   }
}



HID_DEVICE *osx_hid_scan(int type, int *num_devices)
{
   HID_DEVICE *device, *this_device;
   mach_port_t master_port = NULL;
   io_iterator_t hid_object_iterator = NULL;
   io_object_t hid_device = NULL;
   io_registry_entry_t parent1, parent2;
   CFMutableDictionaryRef class_dictionary = NULL;
   int usage, usage_page;
   CFTypeRef type_ref;
   CFNumberRef usage_ref = NULL, usage_page_ref = NULL;
   CFMutableDictionaryRef properties = NULL, usb_properties = NULL;
   IOReturn result;
   AL_CONST char *string;
   
   usage_page = kHIDPage_GenericDesktop;
   switch (type) {
   
      case HID_MOUSE:
	 usage = kHIDUsage_GD_Mouse;
	 break;
	 
      case HID_JOYSTICK:
         usage = kHIDUsage_GD_Joystick;
	 break;
	 
      case HID_GAMEPAD:
         usage = kHIDUsage_GD_GamePad;
	 break;
   }
   
   *num_devices = 0;
   device = (HID_DEVICE *)malloc(HID_MAX_DEVICES * sizeof(HID_DEVICE));
   if (device == NULL)
      return NULL;

   result = IOMasterPort(bootstrap_port, &master_port);
   if (result == kIOReturnSuccess) {
      class_dictionary = IOServiceMatching(kIOHIDDeviceKey);
      if (class_dictionary) {
         /* Add key for device type to refine the matching dictionary. */
         usage_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
         usage_page_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page);
         CFDictionarySetValue(class_dictionary, CFSTR(kIOHIDPrimaryUsageKey), usage_ref);
         CFDictionarySetValue(class_dictionary, CFSTR(kIOHIDPrimaryUsagePageKey), usage_page_ref);
      }
      result = IOServiceGetMatchingServices(master_port, class_dictionary, &hid_object_iterator);
      if ((result == kIOReturnSuccess) && (hid_object_iterator)) {
         /* Ok, we have a list of attached HID devices; scan them. */
	 while ((hid_device = IOIteratorNext(hid_object_iterator))) {
            /* Mac OS X currently is not mirroring all USB properties to HID page so need to look at USB device page also
             * get dictionary for usb properties: step up two levels and get CF dictionary for USB properties.
	     */
            if ((IORegistryEntryCreateCFProperties(hid_device, &properties, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS) &&
                (IORegistryEntryGetParentEntry(hid_device, kIOServicePlane, &parent1) == KERN_SUCCESS) &&
                (IORegistryEntryGetParentEntry (parent1, kIOServicePlane, &parent2) == KERN_SUCCESS) &&
                (IORegistryEntryCreateCFProperties (parent2, &usb_properties, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS) &&
		(properties) && (usb_properties) && (*num_devices < HID_MAX_DEVICES)) {
	       this_device = &device[*num_devices];
	       (*num_devices)++;
	       this_device->type = type;
	       type_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDManufacturerKey));
	       if (!type_ref)
	          type_ref = CFDictionaryGetValue(usb_properties, CFSTR("USB Vendor Name"));
	       if ((type_ref) && (string = CFStringGetCStringPtr(type_ref, CFStringGetSystemEncoding())))
	          this_device->manufacturer = strdup(string);
	       else
	          this_device->manufacturer = NULL;
	       type_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDProductKey));
	       if (!type_ref)
	          type_ref = CFDictionaryGetValue(usb_properties, CFSTR("USB Product Name"));
	       if ((type_ref) && (string = CFStringGetCStringPtr(type_ref, CFStringGetSystemEncoding())))
	          this_device->product = strdup(string);
	       else
	          this_device->product = NULL;
	       
	       type_ref = CFDictionaryGetValue(usb_properties, CFSTR("USB Address"));
	       if ((type == HID_MOUSE) && (!type_ref)) {
	          /* Not an USB mouse. Must be integrated trackpad: we do report it as a single button mouse */
		  this_device->num_elements = 1;
		  this_device->element[0].type = HID_ELEMENT_BUTTON;
	       }
	       else {
	          /* Scan for device elements */
	          this_device->num_elements = 0;
	          hid_scan_collection(properties, this_device);
	       }
	       
	       CFRelease(properties);
	       CFRelease(usb_properties);
	    }
	 }
         IOObjectRelease(hid_object_iterator);
      }
      CFRelease(usage_ref);
      CFRelease(usage_page_ref);
      mach_port_deallocate(mach_task_self(), master_port);
   }
   
   return device;
}



void osx_hid_free(HID_DEVICE *devices, int num_devices)
{
   HID_DEVICE *device;
   HID_ELEMENT *element;
   int i, j;
   
   for (i = 0; i < num_devices; i++) {
      device = &devices[i];
      if (device->manufacturer)
         free(device->manufacturer);
      if (device->product)
         free(device->product);
      for (j = 0; j < device->num_elements; j++) {
         element = &device->element[j];
         if (element->name)
	    free(element->name);
      }
   }
   free(devices);
}

