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
 *      Internal header file for the MacOS X Allegro library port.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTOSX_H
#define AINTOSX_H

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/platform/aintunix.h"

#ifdef __OBJC__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>


#ifndef NSAppKitVersionNumber10_1
#define NSAppKitVersionNumber10_1       620
#endif
#ifndef NSAppKitVersionNumber10_2
#define NSAppKitVersionNumber10_2       663
#endif


#define OSX_GFX_NONE                    0
#define OSX_GFX_WINDOW                  1
#define OSX_GFX_FULL                    2

#define BMP_EXTRA(bmp)                  ((BMP_EXTRA_INFO *)((bmp)->extra))

#define HID_MAX_DEVICES                 MAX_JOYSTICKS
#define HID_MOUSE                       0
#define HID_JOYSTICK                    1
#define HID_GAMEPAD                     2

#define HID_MAX_DEVICE_ELEMENTS         ((MAX_JOYSTICK_AXIS * MAX_JOYSTICK_STICKS) + MAX_JOYSTICK_BUTTONS)
#define HID_ELEMENT_BUTTON              0
#define HID_ELEMENT_AXIS                1
#define HID_ELEMENT_AXIS_PRIMARY_X      2
#define HID_ELEMENT_AXIS_PRIMARY_Y      3
#define HID_ELEMENT_STANDALONE_AXIS     4
#define HID_ELEMENT_HAT                 5



	
typedef struct HID_ELEMENT
{
   int type;
   IOHIDElementCookie cookie;
   int max, min;
   int app;
   int col;
   int index;
   char *name;
} HID_ELEMENT;


typedef struct HID_DEVICE
{
   int type;
   char *manufacturer;
   char *product;
   int num_elements;
   int capacity;
   HID_ELEMENT *element;
   IOHIDDeviceInterface **interface;
   int cur_app;
} HID_DEVICE;

typedef struct 
{
   int count;
   int capacity;
   HID_DEVICE* devices;
} HID_DEVICE_COLLECTION;


int _al_osx_bootstrap_ok(void);

void _al_setup_direct_shifts(void);

void _al_osx_keyboard_handler(int pressed, NSEvent *event, ALLEGRO_DISPLAY*);
void _al_osx_keyboard_modifiers(unsigned int new_mods, ALLEGRO_DISPLAY*);
void _al_osx_keyboard_focused(int focused, int state);

void _al_osx_mouse_generate_event(NSEvent*, ALLEGRO_DISPLAY*);
void _al_osx_mouse_handler(NSEvent*);
int _al_osx_mouse_set_sprite(ALLEGRO_BITMAP *sprite, int x, int y);
int _al_osx_mouse_show(ALLEGRO_BITMAP *bmp, int x, int y);
void _al_osx_mouse_hide(void);
void _al_osx_mouse_move(int x, int y);

HID_DEVICE_COLLECTION *_al_osx_hid_scan(int type, HID_DEVICE_COLLECTION*);
void _al_osx_hid_free(HID_DEVICE_COLLECTION *);

// Record in the keyboard state that the main window has changed
void _al_osx_switch_keyboard_focus(ALLEGRO_DISPLAY *, bool switch_in);
// Record in the mouse state that the main window has changed
void _al_osx_switch_mouse_focus(ALLEGRO_DISPLAY *, bool switch_in);
// Notify the display that the mouse driver was installed/uninstalled.
void _al_osx_mouse_was_installed(BOOL);
// Notify the display that the keyboard driver was installed/uninstalled.
void _al_osx_keyboard_was_installed(BOOL);
// Notify that the quit menu was clicked
void _al_osx_post_quit(void);
// Get the underlying view
NSView* _al_osx_view_from_display(ALLEGRO_DISPLAY*);
// Create an image from an allegro bitmap
NSImage* NSImageFromAllegroBitmap(ALLEGRO_BITMAP* bmp);
// Do some one-time initialisation for the thread support
void _al_osx_threads_init(void);
// Drivers
AL_FUNC(ALLEGRO_KEYBOARD_DRIVER*, _al_osx_get_keyboard_driver, (void));
AL_FUNC(ALLEGRO_DISPLAY_INTERFACE*, _al_osx_get_display_driver, (void));
AL_FUNC(ALLEGRO_MOUSE_DRIVER*, _al_osx_get_mouse_driver, (void));
AL_FUNC(ALLEGRO_JOYSTICK_DRIVER*, _al_osx_get_joystick_driver, (void));
#endif

#endif

/* Local variables:       */
/* mode: objc             */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
