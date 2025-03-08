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
*      Modified for 4.9 mouse API by Peter Hull.
*
*      See readme.txt for copyright information.
*/


#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_driver.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"
#include "./osxgl.h"
#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif

ALLEGRO_DEBUG_CHANNEL("MacOSX");

typedef struct ALLEGRO_MOUSE AL_MOUSE;
typedef struct ALLEGRO_MOUSE_STATE AL_MOUSE_STATE;

static bool osx_init_mouse(void);
static unsigned int osx_get_mouse_num_buttons(void);
static unsigned int osx_get_mouse_num_axes(void);
static bool osx_set_mouse_axis(int axis, int value);
static ALLEGRO_MOUSE* osx_get_mouse(void);

/* Mouse info - includes extra info for OS X */
static struct {
   ALLEGRO_MOUSE parent;
   unsigned int button_count;
   unsigned int axis_count;
   int minx, miny, maxx, maxy;
   ALLEGRO_MOUSE_STATE state;
   float z_axis, w_axis;
   BOOL warped;
   int warped_x, warped_y;
   NSCursor* cursor;
} osx_mouse;

void _al_osx_clear_mouse_state(void)
{
   memset(&osx_mouse.state, 0, sizeof(ALLEGRO_MOUSE_STATE));
}

/* _al_osx_switch_keyboard_focus:
 *  Handle a focus switch event.
 */
void _al_osx_switch_mouse_focus(ALLEGRO_DISPLAY *dpy, bool switch_in)
{
   _al_event_source_lock(&osx_mouse.parent.es);

   if (switch_in)
      osx_mouse.state.display = dpy;
   else
      osx_mouse.state.display = NULL;

   _al_event_source_unlock(&osx_mouse.parent.es);
}

/* osx_get_mouse:
* Return the Allegro mouse structure
*/
static ALLEGRO_MOUSE* osx_get_mouse(void)
{
   return (ALLEGRO_MOUSE*) &osx_mouse.parent;
}

/* _al_osx_mouse_generate_event:
* Convert an OS X mouse event to an Allegro event
* and push it into a queue.
* First check that the event is wanted.
*/
void _al_osx_mouse_generate_event(NSEvent* evt, ALLEGRO_DISPLAY* dpy)
{
   NSPoint pos;
   int type, b_change = 0, b = 0;
   float pressure = 0.0;
   float dx = 0, dy = 0, dz = 0, dw = 0;
   switch ([evt type])
   {
      case NSMouseMoved:
         type = ALLEGRO_EVENT_MOUSE_AXES;
         dx = [evt deltaX];
         dy = [evt deltaY];
         pressure = [evt pressure];
         break;
      case NSLeftMouseDragged:
      case NSRightMouseDragged:
      case NSOtherMouseDragged:
         type = ALLEGRO_EVENT_MOUSE_AXES;
         b = [evt buttonNumber]+1;
         dx = [evt deltaX];
         dy = [evt deltaY];
         pressure = [evt pressure];
         break;
      case NSLeftMouseDown:
      case NSRightMouseDown:
      case NSOtherMouseDown:
         type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
         b = [evt buttonNumber]+1;
         b_change = 1;
         osx_mouse.state.buttons |= (1 << (b-1));
         pressure = [evt pressure];
         break;
      case NSLeftMouseUp:
      case NSRightMouseUp:
      case NSOtherMouseUp:
         type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
         b = [evt buttonNumber]+1;
         b_change = 1;
         osx_mouse.state.buttons &= ~(1 << (b-1));
         pressure = [evt pressure];
         break;
      case NSScrollWheel:
         type = ALLEGRO_EVENT_MOUSE_AXES;
         dx = 0;
         dy = 0;
         osx_mouse.w_axis += al_get_mouse_wheel_precision() * [evt deltaX];
         osx_mouse.z_axis += al_get_mouse_wheel_precision() * [evt deltaY];
         dw = osx_mouse.w_axis - osx_mouse.state.w;
         dz = osx_mouse.z_axis - osx_mouse.state.z;
         break;
      case NSMouseEntered:
         type = ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY;
         b = [evt buttonNumber]+1;
         dx = [evt deltaX];
         dy = [evt deltaY];
         break;
      case NSMouseExited:
         type = ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY;
         b = [evt buttonNumber]+1;
         dx = [evt deltaX];
         dy = [evt deltaY];
         break;
      default:
         return;
   }
   pos = [evt locationInWindow];
   BOOL within = true;
   float scaling_factor = 1.0;
   if ([evt window])
   {
      NSRect frm = [[[evt window] contentView] frame];
      within = NSMouseInRect(pos, frm, NO);
      // Y-coordinates in OS X start from the bottom.
      pos.y = NSHeight(frm) - pos.y;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
      scaling_factor = [[evt window] backingScaleFactor];
#endif
   }
   else
   {
      pos.y = [[NSScreen mainScreen] frame].size.height - pos.y;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
      scaling_factor = [[NSScreen mainScreen] backingScaleFactor];
#endif
   }
   dx *= scaling_factor;
   dy *= scaling_factor;
   if (osx_mouse.warped && type == ALLEGRO_EVENT_MOUSE_AXES) {
       dx -= osx_mouse.warped_x;
       dy -= osx_mouse.warped_y;
       osx_mouse.warped = FALSE;
   }
   _al_event_source_lock(&osx_mouse.parent.es);
   if ((within || b_change || type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY)
      && _al_event_source_needs_to_generate_event(&osx_mouse.parent.es))
   {
      ALLEGRO_EVENT new_event;
      ALLEGRO_MOUSE_EVENT* mouse_event = &new_event.mouse;
      mouse_event->type = type;
      // Note: we use 'allegro time' rather than the time stamp
      // from the event
      mouse_event->timestamp = al_get_time();
      mouse_event->display = dpy;
      mouse_event->button = b;
      mouse_event->x = pos.x * scaling_factor;
      mouse_event->y = pos.y * scaling_factor;
      mouse_event->z = osx_mouse.z_axis;
      mouse_event->w = osx_mouse.w_axis;
      mouse_event->dx = dx;
      mouse_event->dy = dy;
      mouse_event->dz = dz;
      mouse_event->dw = dw;
      mouse_event->pressure = pressure;
      _al_event_source_emit_event(&osx_mouse.parent.es, &new_event);
   }
   // Record current state
   osx_mouse.state.x = pos.x * scaling_factor;
   osx_mouse.state.y = pos.y * scaling_factor;
   osx_mouse.state.w = osx_mouse.w_axis;
   osx_mouse.state.z = osx_mouse.z_axis;
   osx_mouse.state.pressure = pressure;
   _al_event_source_unlock(&osx_mouse.parent.es);
}

/* osx_init_mouse:
*  Initializes the driver.
*/
static bool osx_init_mouse(void)
{
   /* NOTE: This function is deprecated, however until we have a better fix
    * or it is removed, we can used it. The problem without calling this
    * function is that after synthesizing events (as with al_set_mouse_xy)
    * there is an unacceptably long delay in receiving new events (mouse
    * locks up for about 0.5 seconds.)
    */
   CGSetLocalEventsSuppressionInterval(0.0);

   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   HID_DEVICE_COLLECTION devices={0,0,NULL};
   int i = 0, j = 0;
   int axes = 0, buttons = 0;
   int max_axes = 0, max_buttons = 0;
   HID_DEVICE* device = nil;
   NSString* desc = nil;

   _al_osx_hid_scan(HID_MOUSE, &devices);
   ALLEGRO_INFO("Detected %d pointing devices\n", devices.count);
   for (i=0; i<devices.count; i++) {
      device=&devices.devices[i];
      buttons = 0;
      axes = 0;

      desc = [NSString stringWithFormat: @"%s %s",
           device->manufacturer ? device->manufacturer : "",
           device->product ? device->product : ""];
      ALLEGRO_INFO("Device %d is a \"%s\"\n", i, [desc UTF8String]);

      for (j = 0; j < device->num_elements; j++) {
         switch (device->element[j].type) {
            case HID_ELEMENT_BUTTON:
               buttons++;
               break;
            case HID_ELEMENT_AXIS:
            case HID_ELEMENT_AXIS_PRIMARY_X:
            case HID_ELEMENT_AXIS_PRIMARY_Y:
            case HID_ELEMENT_STANDALONE_AXIS:
               axes ++;
               break;
         }
      }
      ALLEGRO_INFO("Detected %d axes and %d buttons\n", axes, buttons);

      /* When mouse events reach the application, it is not clear which
       * device generated them, so effectively the largest number of
       * buttons and axes reported corresponds to the device that the
       * application "sees"
       */
      if (axes > max_axes) max_axes = axes;
      if (buttons > max_buttons) max_buttons = buttons;
   }
   _al_osx_hid_free(&devices);
   ALLEGRO_INFO("Device effectively has %d axes and %d buttons\n", axes, buttons);

   if (max_buttons <= 0) return false;
   _al_event_source_init(&osx_mouse.parent.es);
   osx_mouse.button_count = max_buttons;
   osx_mouse.axis_count = max_axes;
   osx_mouse.warped = FALSE;
   memset(&osx_mouse.state, 0, sizeof(ALLEGRO_MOUSE_STATE));
   _al_osx_mouse_was_installed(YES);
   [pool drain];
   return true;
}

/* osx_exit_mouse:
* Shut down the mouse driver
*/
static void osx_exit_mouse(void)
{
   _al_osx_mouse_was_installed(NO);
}

/* osx_get_mouse_num_buttons:
* Return the number of buttons on the mouse
*/
static unsigned int osx_get_mouse_num_buttons(void)
{
   return osx_mouse.button_count;
}

/* osx_get_mouse_num_buttons:
* Return the number of buttons on the mouse
*/
static unsigned int osx_get_mouse_num_axes(void)
{
   return osx_mouse.axis_count;
}


static void osx_get_mouse_state(ALLEGRO_MOUSE_STATE *ret_state)
{
   _al_event_source_lock(&osx_mouse.parent.es);
   memcpy(ret_state, &osx_mouse.state, sizeof(ALLEGRO_MOUSE_STATE));
   _al_event_source_unlock(&osx_mouse.parent.es);
}

static NSScreen *screen_from_display_id(CGDirectDisplayID display) {
   for (NSScreen *screen in [NSScreen screens]) {
      NSDictionary *info = [screen deviceDescription];
      NSNumber *display_id = [info objectForKey:@"NSScreenNumber"];
      if ([display_id unsignedIntValue] == display)
         return screen;
   }
   return nil;
}

/* osx_set_mouse_xy:
* Set the current mouse position
*/
static bool osx_set_mouse_xy(ALLEGRO_DISPLAY *dpy_, int x, int y)
{
   CGPoint pos;
   CGDirectDisplayID display = 0;
   ALLEGRO_DISPLAY_OSX_WIN *dpy = (ALLEGRO_DISPLAY_OSX_WIN *)dpy_;

   if ((dpy) && !(dpy->parent.flags & ALLEGRO_FULLSCREEN) &&
         !(dpy->parent.flags & ALLEGRO_FULLSCREEN_WINDOW) && (dpy->win)) {
      NSWindow *window = dpy->win;
      NSRect content = [window contentRectForFrameRect: [window frame]];
      NSRect frame = [[window screen] frame];
      CGRect rect = { { NSMinX(frame), NSMinY(frame) }, { NSWidth(frame), NSHeight(frame) } };
      CGDirectDisplayID displays[16];
      CGDisplayCount displayCount;

      if ((CGGetDisplaysWithRect(rect, 16, displays, &displayCount) == 0) && (displayCount >= 1))
         display = displays[0];

      CGPoint point_pos = CGPointMake(x, y);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
      point_pos = NSPointToCGPoint([[window contentView] convertPointFromBacking: NSPointFromCGPoint(point_pos)]);
#endif
      pos.x = content.origin.x + point_pos.x;
      pos.y = rect.size.height - content.origin.y - content.size.height + point_pos.y;
   }
   else {
      float scaling_factor = 1.0;
      NSScreen *screen = nil;
      if (dpy) {
         display = dpy->display_id;
         screen = screen_from_display_id(display);
      }
      if (!screen)
         screen = [NSScreen mainScreen];
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
      scaling_factor = [screen backingScaleFactor];
#endif
      pos.x = x / scaling_factor;
      pos.y = y / scaling_factor;
   }

   _al_event_source_lock(&osx_mouse.parent.es);
   if (_al_event_source_needs_to_generate_event(&osx_mouse.parent.es)) {
      ALLEGRO_EVENT new_event;
      ALLEGRO_MOUSE_EVENT* mouse_event = &new_event.mouse;
      mouse_event->type = ALLEGRO_EVENT_MOUSE_WARPED;
      // Note: we use 'allegro time' rather than the time stamp
      // from the event
      mouse_event->timestamp = al_get_time();
      mouse_event->display = (ALLEGRO_DISPLAY *)dpy;
      mouse_event->button = 0;
      mouse_event->x = x;
      mouse_event->y = y;
      mouse_event->z = osx_mouse.z_axis;
      mouse_event->dx = x - osx_mouse.state.x;
      mouse_event->dy = y - osx_mouse.state.y;
      mouse_event->dz = 0;
      mouse_event->pressure = osx_mouse.state.pressure;
      if (mouse_event->dx || mouse_event->dy) {
         osx_mouse.warped = TRUE;
         osx_mouse.warped_x = mouse_event->dx;
         osx_mouse.warped_y = mouse_event->dy;
         _al_event_source_emit_event(&osx_mouse.parent.es, &new_event);
      }
   }
   CGDisplayMoveCursorToPoint(display, pos);
   osx_mouse.state.x = x;
   osx_mouse.state.y = y;
   _al_event_source_unlock(&osx_mouse.parent.es);
   return true;
}

/* osx_set_mouse_axis:
* Set the axis value of the mouse
*/
static bool osx_set_mouse_axis(int axis, int value)
{
    bool result = false;
   _al_event_source_lock(&osx_mouse.parent.es);
   switch (axis)
   {
      case 0:
      case 1:
         // By design, this doesn't apply to (x, y)
         break;
      case 2:
         osx_mouse.z_axis = value;
         result = true;
         break;
      case 3:
         osx_mouse.w_axis = value;
         result = true;
         break;
   }
   /* XXX generate an event if the axis value changed */
   _al_event_source_unlock(&osx_mouse.parent.es);
   return result;
}
/* Mouse driver */
static ALLEGRO_MOUSE_DRIVER osx_mouse_driver =
{
   0, //int  msedrv_id;
   "OSXMouse", //const char *msedrv_name;
   "Driver for Mac OS X",// const char *msedrv_desc;
   "OSX Mouse", //const char *msedrv_ascii_name;
   osx_init_mouse, //AL_METHOD(bool, init_mouse, (void));
   osx_exit_mouse, //AL_METHOD(void, exit_mouse, (void));
   osx_get_mouse, //AL_METHOD(ALLEGRO_MOUSE*, get_mouse, (void));
   osx_get_mouse_num_buttons, //AL_METHOD(unsigned int, get_mouse_num_buttons, (void));
   osx_get_mouse_num_axes, //AL_METHOD(unsigned int, get_mouse_num_axes, (void));
   osx_set_mouse_xy, //AL_METHOD(bool, set_mouse_xy, (int x, int y));
   osx_set_mouse_axis, //AL_METHOD(bool, set_mouse_axis, (int which, int value));
   osx_get_mouse_state, //AL_METHOD(void, get_mouse_state, (ALLEGRO_MOUSE_STATE *ret_state));
};
ALLEGRO_MOUSE_DRIVER* _al_osx_get_mouse_driver(void)
{
   return &osx_mouse_driver;
}

/* list the available drivers */
_AL_DRIVER_INFO _al_mouse_driver_list[] =
{
   {  1, &osx_mouse_driver, 1 },
   {  0,  NULL,  0  }
};

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
