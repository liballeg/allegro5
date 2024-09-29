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
 *      X-Windows touch module.
 *
 *      By Ryan Gumbs.
 *
 *      See readme.txt for copyright information.
 */

#define A5O_NO_COMPATIBILITY

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xmouse.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/internal/aintern_xtouch.h"

#ifdef A5O_XWINDOWS_WITH_XINPUT2

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>



A5O_DEBUG_CHANNEL("touch")


/* the one and only touch object */
static A5O_TOUCH_INPUT_STATE touch_input_state;
static A5O_TOUCH_INPUT touch_input;
static A5O_MOUSE_STATE mouse_state;
static int touch_ids[A5O_TOUCH_INPUT_MAX_TOUCH_COUNT]; /* the XInput id for touches */
static int primary_touch_id = -1;
static bool installed = false;
static size_t initiali_time_stamp = ~0;
static int opcode;


static void reset_touch_input_state(void)
{
   int i;

   for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; i++) {
      touch_input_state.touches[i].id = -1;
   }
}


static void generate_touch_input_event(unsigned int type, double timestamp,
   int id, float x, float y, float dx, float dy, bool primary,
   A5O_DISPLAY *disp)
{
   A5O_EVENT event;

   bool want_touch_event = _al_event_source_needs_to_generate_event(&touch_input.es);
   bool want_mouse_emulation_event;

   if (touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_5_0_x) {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input.mouse_emulation_es) && al_is_mouse_installed();
   }
   else {
      want_mouse_emulation_event = _al_event_source_needs_to_generate_event(&touch_input.mouse_emulation_es) && primary && al_is_mouse_installed();
   }

   if (touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_NONE)
      want_mouse_emulation_event = false;
   else if (touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_INCLUSIVE)
      want_touch_event = al_is_mouse_installed() ? (want_touch_event && !primary) : want_touch_event;
   else if (touch_input.mouse_emulation_mode == A5O_MOUSE_EMULATION_EXCLUSIVE)
      want_touch_event = al_is_mouse_installed() ? false : want_touch_event;


   if (!want_touch_event && !want_mouse_emulation_event)
      return;

   if (want_touch_event) {
      event.touch.type      = type;
      event.touch.display   = (A5O_DISPLAY*)disp;
      event.touch.timestamp = timestamp;
      event.touch.id        = id;
      event.touch.x         = x;
      event.touch.y         = y;
      event.touch.dx        = dx;
      event.touch.dy        = dy;
      event.touch.primary   = primary;

      _al_event_source_lock(&touch_input.es);
      _al_event_source_emit_event(&touch_input.es, &event);
      _al_event_source_unlock(&touch_input.es);
   }

   if (touch_input.mouse_emulation_mode != A5O_MOUSE_EMULATION_NONE) {
      mouse_state.x = (int)x;
      mouse_state.y = (int)y;
      if (type == A5O_EVENT_TOUCH_BEGIN)
         mouse_state.buttons++;
      else if (type == A5O_EVENT_TOUCH_END)
         mouse_state.buttons--;

      mouse_state.pressure = mouse_state.buttons ? 1.0 : 0.0; /* TODO */

      _al_event_source_lock(&touch_input.mouse_emulation_es);
      if (want_mouse_emulation_event) {

         switch (type) {
            case A5O_EVENT_TOUCH_BEGIN: type = A5O_EVENT_MOUSE_BUTTON_DOWN; break;
            case A5O_EVENT_TOUCH_CANCEL:
            case A5O_EVENT_TOUCH_END:   type = A5O_EVENT_MOUSE_BUTTON_UP;   break;
            case A5O_EVENT_TOUCH_MOVE:  type = A5O_EVENT_MOUSE_AXES;        break;
         }

         event.mouse.type      = type;
         event.mouse.timestamp = timestamp;
         event.mouse.display   = (A5O_DISPLAY*)disp;
         event.mouse.x         = (int)x;
         event.mouse.y         = (int)y;
         event.mouse.dx        = (int)dx;
         event.mouse.dy        = (int)dy;
         event.mouse.dz        = 0;
         event.mouse.dw        = 0;
         if (touch_input.mouse_emulation_mode != A5O_MOUSE_EMULATION_5_0_x) {
            event.mouse.button = 1;
         }
         else {
            event.mouse.button = id;
         }
         event.mouse.pressure  = mouse_state.pressure;

         if (touch_input.mouse_emulation_mode != A5O_MOUSE_EMULATION_5_0_x) {
            al_set_mouse_xy(event.mouse.display, event.mouse.x, event.mouse.y);
         }

         _al_event_source_emit_event(&touch_input.mouse_emulation_es, &event);
      }
      _al_event_source_unlock(&touch_input.mouse_emulation_es);
   }
}


static int find_free_touch_state_index(void)
{
   int i;

   for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; i++)
      if (touch_input_state.touches[i].id < 0)
         return i;

   return -1;
}


static int find_touch_state_index_with_id(int id)
{
   int i;

   for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; i++)
      if (touch_ids[i] == id)
         return i;

   return -1;
}


static void touch_input_handle_begin(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp)
{
   int index= find_free_touch_state_index();
   if (index < 0)
       return;

   A5O_TOUCH_STATE* state = touch_input_state.touches + index;
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->id      = index;
   state->x       = x;
   state->y       = y;
   state->dx      = 0.0f;
   state->dy      = 0.0f;
   state->primary = primary;
   state->display = disp;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(A5O_EVENT_TOUCH_BEGIN, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary,
      disp);

   touch_ids[index]= id;
}


static void touch_input_handle_end(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp)
{
   int index= find_touch_state_index_with_id(id);
   if (index < 0)
       return;

   A5O_TOUCH_STATE* state = touch_input_state.touches + index;
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx = x - state->x;
   state->dy = y - state->y;
   state->x  = x;
   state->y  = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(A5O_EVENT_TOUCH_END, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary,
      disp);

   _al_event_source_lock(&touch_input.es);
   state->id = -1;
   touch_ids[index]= -1;
   _al_event_source_unlock(&touch_input.es);
}


static void touch_input_handle_move(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp)
{
   int index= find_touch_state_index_with_id(id);
   if (index < 0)
      return;

   A5O_TOUCH_STATE* state = touch_input_state.touches + index;
   (void)primary;

   if (NULL == state)
      return;

   if (x == state->x && y == state->y)
      return; /* coordinates haven't changed */

   _al_event_source_lock(&touch_input.es);
   state->dx = x - state->x;
   state->dy = y - state->y;
   state->x  = x;
   state->y  = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(A5O_EVENT_TOUCH_MOVE, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary,
      disp);
}


static void touch_input_handle_cancel(int id, double timestamp, float x, float y, bool primary, A5O_DISPLAY *disp)
{
   int index= find_touch_state_index_with_id(id);
   if (index < 0)
      return;

   A5O_TOUCH_STATE* state = touch_input_state.touches + index;
   (void)primary;

   if (NULL == state)
      return;

   _al_event_source_lock(&touch_input.es);
   state->dx = x - state->x;
   state->dy = y - state->y;
   state->x  = x;
   state->y  = y;
   _al_event_source_unlock(&touch_input.es);

   generate_touch_input_event(A5O_EVENT_TOUCH_CANCEL, timestamp,
      state->id, state->x, state->y, state->dx, state->dy, state->primary, disp);

   _al_event_source_lock(&touch_input.es);
   state->id = -1;
   _al_event_source_unlock(&touch_input.es);
}


static bool init_touch_input_api(void)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Display *dpy = system->x11display;

   /* check XInput extension */
   int ev;
   int err;
   if (!XQueryExtension(dpy, "XInputExtension", &opcode, &ev, &err)) {
      A5O_DEBUG("XInput extension not available. Touch input unavailable.\n");
      return false;
   }

   /* check the version of XInput */
   int major = 2;
   int minor = 2;
   int rc = XIQueryVersion(dpy, &major, &minor);
   if (rc != Success) {
      A5O_DEBUG("XInput version is too old (%d.%d): Needs 2.2. Touch input unavailable.\n", major, minor);
      return false;
   }

   /* select device */
   int cnt;
   int i, j;
   int touch_devid;
   XIDeviceInfo *di = XIQueryDevice(dpy, XIAllDevices, &cnt);
   for (i = 0; i < cnt; i++) {
      XIDeviceInfo *dev = &di[i];
      for (j = 0; j < dev->num_classes; j++) {
         XITouchClassInfo *class = (XITouchClassInfo*)(dev->classes[j]);
         if (class->type == XITouchClass) {
            touch_devid = dev->deviceid;
            A5O_DEBUG("Found touchscreen deviceid: %i\n", touch_devid);
            goto STOP_SEARCH_DEVICE;
         }
      }
   }
STOP_SEARCH_DEVICE:
   XIFreeDeviceInfo(di);

   if (i >= cnt) {
      /* no device found */
      A5O_DEBUG("No touchscreen device found.\n");
      return false;
   }

   return true;
}


static bool xtouch_init(void)
{
   if (installed)
      return false;

   if (!init_touch_input_api())
      return false;

   A5O_DEBUG("XInput2 touch input initialized.\n");

   memset(&touch_input, 0, sizeof(touch_input));
   reset_touch_input_state();

   /* Initialise the touch object for use as an event source. */
   _al_event_source_init(&touch_input.es);

   _al_event_source_init(&touch_input.mouse_emulation_es);
   touch_input.mouse_emulation_mode = A5O_MOUSE_EMULATION_TRANSPARENT;

   initiali_time_stamp = al_get_time();

   installed = true;

   return true;
}


static void xtouch_exit(void)
{
}


static A5O_TOUCH_INPUT* get_touch_input(void)
{
   return &touch_input;
}


static void get_touch_input_state(A5O_TOUCH_INPUT_STATE *ret_state)
{
   _al_event_source_lock(&touch_input.es);
   *ret_state = touch_input_state;
   _al_event_source_unlock(&touch_input.es);
}


static void set_mouse_emulation_mode(int mode)
{
   if (touch_input.mouse_emulation_mode != mode) {
      int i;

      for (i = 0; i < A5O_TOUCH_INPUT_MAX_TOUCH_COUNT; i++) {
         A5O_TOUCH_STATE* touch = touch_input_state.touches + i;

         if (touch->id > 0) {
            touch_input_handle_cancel(touch_ids[i], initiali_time_stamp,
               touch->x, touch->y, touch->primary, touch->display);
         }
      }

      touch_input.mouse_emulation_mode = mode;
   }
}


/* the driver vtable */
#define TOUCHDRV_XWIN  AL_ID('X','W','I','N')

static A5O_TOUCH_INPUT_DRIVER touchdrv_xwin =
{
   TOUCHDRV_XWIN,
   xtouch_init,
   xtouch_exit,
   get_touch_input,
   get_touch_input_state,
   set_mouse_emulation_mode,
   NULL
};

#endif

_AL_DRIVER_INFO _al_touch_input_driver_list[] =
{
#ifdef A5O_XWINDOWS_WITH_XINPUT2
   {TOUCHDRV_XWIN, &touchdrv_xwin, true},
#endif
   {0, NULL, 0}
};

void _al_x_handle_touch_event(A5O_SYSTEM_XGLX *s, A5O_DISPLAY_XGLX *d, XEvent *e)
{
#ifdef A5O_XWINDOWS_WITH_XINPUT2
   Display *x11display = s->x11display;
   XGenericEventCookie *cookie = &e->xcookie;
   if (!installed)
      return;
   /* extended event */
   if (XGetEventData(x11display, cookie)) {
      /* check if this belongs to XInput */
      if (cookie->type == GenericEvent && cookie->extension == opcode) {
         XIDeviceEvent *devev;

         devev = cookie->data;

         /* ignore events from a different display */
         if (devev->display != x11display)
            return;

         switch (devev->evtype) {
            case XI_TouchBegin:
               /* the next new touch gets primary flag if it's not set */
               if (primary_touch_id < 0)
                  primary_touch_id= devev->detail;

               touch_input_handle_begin(devev->detail, al_get_time(),
                  devev->event_x, devev->event_y, primary_touch_id == devev->detail, &d->display);
               break;
            case XI_TouchUpdate:
               touch_input_handle_move(devev->detail, al_get_time(),
                  devev->event_x, devev->event_y, primary_touch_id == devev->detail, &d->display);
               break;
            case XI_TouchEnd:
               touch_input_handle_end(devev->detail, al_get_time(),
                  devev->event_x, devev->event_y, primary_touch_id == devev->detail, &d->display);

               if (primary_touch_id == devev->detail)
                  primary_touch_id = -1;
               break;
         }
      }
   }
#else
   (void)s;
   (void)d;
   (void)e;
#endif
}
