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
#include "allegro5/internal/aintern_memory.h"


#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif                

static bool init_joystick(void);
static void exit_joystick(void);
static int num_joysticks(void);
static ALLEGRO_JOYSTICK* get_joystick(int);
static void release_joystick(ALLEGRO_JOYSTICK*);
static void get_joystick_state(ALLEGRO_JOYSTICK*, ALLEGRO_JOYSTICK_STATE*);

/* OSX HID Joystick */
typedef struct {
	ALLEGRO_JOYSTICK parent;
} ALLEGRO_JOYSTICK_OSX;

static _AL_VECTOR joysticks;

/* create_device_iterator:
 * Create an iterator which will match all joysticks/
 * gamepads on the system. Return YES if OK.
 */
static BOOL create_device_iterator(io_iterator_t* iter)
{
	NSMutableDictionary* matching;
	matching = (NSMutableDictionary*) IOServiceMatching(kIOHIDDeviceKey);
	// Add in our criteria:
	[matching setObject:[NSNumber numberWithInt: kHIDUsage_GD_GamePad] forKey: [NSString stringWithCString: kIOHIDPrimaryUsageKey]];
	[matching setObject:[NSNumber numberWithInt: kHIDPage_GenericDesktop] forKey: [NSString stringWithCString: kIOHIDPrimaryUsagePageKey]];
	// Get the iterator
	IOReturn err = IOServiceGetMatchingServices(kIOMasterPortDefault, (CFDictionaryRef) matching, iter);
	return (err == kIOReturnSuccess) && (*iter != 0);
}

/* create_interface:
 * Create the interface to access this device, via
 * the intermediate plug-in interface
 */
BOOL create_interface(io_object_t device, IOHIDDeviceInterface122*** interface)
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
/* add_device:
 * Create the joystick structure for this device
 * and add it to the 'joysticks' vector
 */
static void add_device(io_object_t device)
{
	ALLEGRO_JOYSTICK_OSX* joy;
	NSArray* elements = nil;
	int num = _al_vector_size(&joysticks);
	joy = (ALLEGRO_JOYSTICK_OSX*) _al_vector_alloc_back(&joysticks);
	joy->parent.num = num;
	joy->parent.info.num_sticks = 0;
	joy->parent.info.num_buttons = 0;
	_al_event_source_init(&joy->parent.es);
	IOHIDDeviceInterface122** interface;
	create_interface(device, &interface);
	(*interface)->copyMatchingElements(interface, NULL, (CFArrayRef*) &elements);
	NSEnumerator* enumerator = [elements objectEnumerator];
	NSDictionary* element;
	while ((element = (NSDictionary*) [enumerator nextObject])) {
		long usage = [((NSNumber*) [element objectForKey: [NSString stringWithCString: kIOHIDElementUsageKey]]) longValue];
		long usage_page = [((NSNumber*) [element objectForKey: [NSString stringWithCString: kIOHIDElementUsagePageKey]]) longValue];
		if (usage_page == 0x09) ++joy->parent.info.num_buttons; 
	}
	[elements release];
}

ALLEGRO_JOYSTICK_DRIVER* osx_get_joystick_driver(void)
{
	static ALLEGRO_JOYSTICK_DRIVER* vt = NULL;
	if (vt == NULL) {
		vt = _AL_MALLOC(sizeof(*vt));
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

/* init_joystick:
*  Initializes the HID joystick driver.
*/
static bool init_joystick(void)
{
	_al_vector_init(&joysticks, sizeof(ALLEGRO_JOYSTICK_OSX));
	io_iterator_t iterator;
	io_object_t device;
	if (create_device_iterator(&iterator)) {
		while ((device = IOIteratorNext(iterator))) {
			add_device(device);
		}
		IOObjectRelease(iterator);
	}
	return TRUE;
}



/* exit_joystick:
*  Shuts down the HID joystick driver.
*/
static void exit_joystick(void)
{
	_al_vector_free(&joysticks);
}




/* num_joysticks:
*  Return number of active joysticks
*/
int num_joysticks(void) 
{
	return _al_vector_size(&joysticks);
}

/* get_joystick:
* Get a pointer to a joystick structure
*/
ALLEGRO_JOYSTICK* get_joystick(int index) 
{
	ALLEGRO_JOYSTICK* joy = NULL;
	if (index >= 0 && index < (int) _al_vector_size(&joysticks)) {
		joy = _al_vector_ref(&joysticks, index);
	}
	return joy;
}

/* release_joystick:
* Release a pointer that has been obtained
*/
void release_joystick(ALLEGRO_JOYSTICK* joy)
{
	// No-op
}

/* get_state:
* Get the current status of a joystick
*/
void get_joystick_state(ALLEGRO_JOYSTICK* joy, ALLEGRO_JOYSTICK_STATE* state)
{
	memset(state,0,sizeof(*state));
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
