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
 *      Linux console mouse driver for event interface (EVDEV).
 *
 *      By Annie Testes.
 *
 *      Handles all inputs that fill evdev with informations
 *      about movement and click.
 *
 *      Hacked for new mouse API by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_NO_COMPATIBILITY

#include "allegro5/allegro.h"

#ifdef ALLEGRO_HAVE_LINUX_INPUT_H

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/linalleg.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define PREFIX_I                "al-evdev INFO: "
#define PREFIX_W                "al-evdev WARNING: "
#define PREFIX_E                "al-evdev ERROR: "


typedef struct AL_MOUSE_EVDEV
{
   ALLEGRO_MOUSE parent;
   int fd;
   ALLEGRO_MOUSE_STATE state;
} AL_MOUSE_EVDEV;


/* the one and only mouse object */
static AL_MOUSE_EVDEV the_mouse;



/* forward declarations */
static void process_new_data(void *unused);
static void process_event(struct input_event *event);
static void handle_button_event(unsigned int button, bool is_down);
static void handle_axis_event(int dx, int dy, int dz);
static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button);



/*
 * Tools data
 */

/* Tool mode
 * This mode is the behavior the user sees, it is not the way the device
 * reports the position.
 * Tools that send relative informations can only be in relative mode,
 * tools that send absolute informations can be either in absolute mode or in
 * relative mode */
typedef enum {
   MODE_RELATIVE,
   MODE_ABSOLUTE
} MODE;



typedef struct TOOL {
   int tool_id; /* One of the BTN_TOOL_... from linux/input.h */
   MODE mode;   /* Relative or absolute */
} TOOL;



static TOOL tools[] = {
   { BTN_TOOL_MOUSE,    MODE_RELATIVE },
   { BTN_TOOL_PEN,      MODE_ABSOLUTE },
   { BTN_TOOL_RUBBER,   MODE_ABSOLUTE },
   { BTN_TOOL_BRUSH,    MODE_ABSOLUTE },
   { BTN_TOOL_PENCIL,   MODE_ABSOLUTE },
   { BTN_TOOL_AIRBRUSH, MODE_ABSOLUTE },
   { BTN_TOOL_FINGER,   MODE_ABSOLUTE },
   { BTN_TOOL_LENS,     MODE_ABSOLUTE },

   { -1, MODE_ABSOLUTE } /* No tool */
};



/* Default tool, in case the device does not send a tool id */
static TOOL *default_tool = tools + 0;

static TOOL *no_tool = tools + (sizeof(tools)/sizeof(tools[0]) - 1);

static TOOL *current_tool;



/* find_tool:
 *  Returns the tool with id 'tool_id' which is one of the BTN_TOOL_... defined
 *  in linux/input.h. Returns 'default_tool' if not found.
 */
static TOOL *find_tool(int tool_id)
{
   TOOL *t;
   for (t=tools; t!=no_tool; t++) {
      if (t->tool_id == tool_id)
         return t;
   }
   return default_tool;
}




/*
 * Pointer axis
 */
typedef struct AXIS {
   int in_min;      /* For absolute mode */
   int in_max;      /* For absolute mode */
   int out_min;     /* For absolute mode */
   int out_max;     /* For absolute mode */

   float speed;     /* For set_mouse_speed */
   int mickeys;     /* For get_mouse_mickeys */
   float scale;     /* Scale factor, because tablet mice generate more movement
                       than common mice */

   int in_abs;      /* Current absolute position, used for absolute input,
                       whether the output is absolute or relative */
   int out_abs;     /* Position on the screen */
} AXIS;


#define IN_RANGE(axis)  ( (axis).in_max-(axis).in_min+1 )
#define OUT_RANGE(axis)  ( (axis).out_max-(axis).out_min+1 )


/* in_to_screen:
 *  maps an input absolute position to a screen position
 */
static int in_to_screen(const AXIS *axis, int v)
{
   return (((v-axis->in_min) * OUT_RANGE(*axis)) / IN_RANGE(*axis)) + axis->out_min;
}



/* rel_event:
 *  returns the new screen position, given the input relative one.
 *  The tool mode is always relative
 */
static int rel_event(AXIS *axis, int v)
{
   /* When input only send relative events, the mode is always relative */
   int ret = axis->out_abs + v*axis->speed;
   axis->mickeys += v;
   axis->in_abs += v;
   return ret;
}



/* abs_event:
 *  returns the new screen position, given the input absolute one,
 *  and depending on the tool mode
 */
static int abs_event(AXIS *axis, MODE mode, int v)
{
   if (mode == MODE_ABSOLUTE) {
      axis->mickeys = 0; /* No mickeys in absolute mode */
      axis->in_abs = v;
      return in_to_screen(axis, v);
   }
   else { /* Input is absolute, but tool is relative */
      int value = (v-axis->in_abs)*axis->scale;
      axis->mickeys += value;
      axis->in_abs = v;
      return axis->out_abs + value*axis->speed;
   }
}



/* get_axis_value:
 *  returns the input absolute position
 */
static void get_axis_value(int fd, AXIS *axis, int type)
{
  int abs[5];
  int ret = ioctl(fd, EVIOCGABS(type), abs);
  if (ret>=0) {
    axis->in_abs = abs[0];
  }
}



/* has_event:
 *  returns true if the device generates the event
 */
static int has_event(int fd, unsigned short type, unsigned short code)
{
   const unsigned int len = sizeof(unsigned long)*8;
   const unsigned int max = MAX(EV_MAX, MAX(KEY_MAX, MAX(REL_MAX, MAX(ABS_MAX, MAX(LED_MAX, MAX(SND_MAX, FF_MAX))))));
   unsigned long bits[(max+len-1)/len];
   if (ioctl(fd, EVIOCGBIT(type, max), bits)) {
     return (bits[code/len] >> (code%len)) & 1;
   }
   return 0;
}



/* get_num_buttons:
 *  return the number of buttons
 */
static int get_num_buttons(int fd)
{
   if (has_event(fd, EV_KEY, BTN_MIDDLE))
      return 3;
   if (has_event(fd, EV_KEY, BTN_RIGHT))
      return 2;
   if (has_event(fd, EV_KEY, BTN_MOUSE))
      return 1;
   return 0; /* Not a mouse */
}



/* The three axis: horizontal, vertical and wheel */
static AXIS x_axis;
static AXIS y_axis;
static AXIS z_axis;



/*
 * Initialization functions
 */

/* init_axis:
 *  initialize an AXIS structure, from the device and the config file
 */
static void init_axis(int fd, AXIS *axis, const char *name, const char *section, int type)
{
   char tmp1[256]; /* config string */
   char tmp2[256]; /* format string */
   char tmp3[256]; /* Converted 'name' */
   int abs[5]; /* values given by the input */
   int config_speed;

   uszprintf(tmp1, sizeof(tmp1), uconvert_ascii("ev_min_%s", tmp2), uconvert_ascii(name, tmp3));
   axis->in_min = get_config_int(section, tmp1, 0);
   uszprintf(tmp1, sizeof(tmp1), uconvert_ascii("ev_max_%s", tmp2), uconvert_ascii(name, tmp3));
   axis->in_max = get_config_int(section, tmp1, 0);

   uszprintf(tmp1, sizeof(tmp1), uconvert_ascii("ev_abs_to_rel_%s", tmp2), uconvert_ascii(name, tmp3));
   config_speed = get_config_int(section, tmp1, 1);
   if (config_speed<=0)
      config_speed = 1;
   axis->scale = 1;

   /* Ask the input */
   if (ioctl(fd, EVIOCGABS(type), abs)>=0) {
      if (axis->in_min==0)
         axis->in_min=abs[1];
      if (axis->in_max==0)
         axis->in_max=abs[2];
      axis->in_abs = abs[0];
      axis->scale = 320.0*config_speed/IN_RANGE(*axis);
   }
   if (axis->in_min>axis->in_max) {
      axis->in_min = axis->in_max = 0;
      axis->scale = 1;
   }

   axis->out_min = INT_MIN;
   axis->out_max = INT_MAX;

   axis->speed = 1;
   axis->mickeys = 0;
}



/* init_tablet:
 *  initialize the tools and axis
 */
static void init_tablet(int fd)
{
   char tmp[256];
   char *mouse_str = uconvert_ascii("mouse", tmp);
   char tmp2[256];
   int default_abs = default_tool->mode==MODE_ABSOLUTE;

   default_abs = get_config_int(mouse_str, uconvert_ascii("ev_absolute", tmp2), default_abs);
   if (default_abs) {
      default_tool->mode = MODE_ABSOLUTE;
   }
   else {
      default_tool->mode = MODE_RELATIVE;
   }

   init_axis(fd, &x_axis, "x", mouse_str, ABS_X);
   init_axis(fd, &y_axis, "y", mouse_str, ABS_Y);
   init_axis(fd, &z_axis, "z", mouse_str, ABS_Z);
}




/*
 * Process input functions
 */

/* process_key: [fdwatch thread]
 *  process events of type key (button clicks and vicinity events are currently
 *  supported)
 */
static void process_key(const struct input_event *event)
{
   switch (event->code) {
      /* Buttons click
       * if event->value is 1 the button has just been pressed
       * if event->value is 0 the button has just been released */
      case BTN_LEFT: /* Mouse */
      case BTN_TOUCH: /* Stylus */
         handle_button_event(1, event->value);
         break;

      case BTN_RIGHT: /* Mouse */
      case BTN_STYLUS: /* Stylus */
         handle_button_event(2, event->value);
         break;

      case BTN_MIDDLE: /* Mouse */
      case BTN_STYLUS2: /* Stylus */
         handle_button_event(3, event->value);
         break;

      /* Vicinity events
       * if event->value is 1, the tool enters the tablet vicinity
       * if event->value is 0, the tool leaves the tablet vicinity */
      case BTN_TOOL_MOUSE:
      case BTN_TOOL_PEN:
      case BTN_TOOL_RUBBER:
      case BTN_TOOL_BRUSH:
      case BTN_TOOL_PENCIL:
      case BTN_TOOL_AIRBRUSH:
      case BTN_TOOL_FINGER:
      case BTN_TOOL_LENS:
         if (event->value) {
            current_tool = find_tool(event->code);
            get_axis_value(the_mouse.fd, &x_axis, ABS_X);
            get_axis_value(the_mouse.fd, &y_axis, ABS_Y);
            get_axis_value(the_mouse.fd, &z_axis, ABS_Z);
#ifdef ABS_WHEEL  /* absent in 2.2.x */
            get_axis_value(the_mouse.fd, &z_axis, ABS_WHEEL);
#endif
         }
         else {
            current_tool = no_tool;
         }
         break;
   }
}


/* [fdwatch thread] */
static void handle_button_event(unsigned int button, bool is_down)
{
   unsigned int event_type;

   if (is_down) {
      the_mouse.state.buttons |= (1 << (button-1));
      event_type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
   }
   else {
      the_mouse.state.buttons &=~ (1 << (button-1));
      event_type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
   }

   generate_mouse_event(
      event_type,
      the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
      0, 0, 0,
      button);
}
                          



/* process_rel: [fdwatch thread]
 *  process relative events (x, y and z movement are currently supported)
 */
static void process_rel(const struct input_event *event)
{
   /* The device can send a report when there's no tool */
   if (current_tool!=no_tool) {
      switch (event->code) {
         case REL_X:
            x_axis.out_abs = rel_event(&x_axis, event->value);
            handle_axis_event(x_axis.mickeys, 0, 0);
            x_axis.mickeys = 0;
            break;

         case REL_Y:
            y_axis.out_abs = rel_event(&y_axis, event->value);
            handle_axis_event(0, y_axis.mickeys, 0);
            y_axis.mickeys = 0;
            break;

#ifdef REL_WHEEL  /* absent in 2.2.x */
         case REL_WHEEL:
#endif
         case REL_Z:
            z_axis.out_abs = rel_event(&z_axis, event->value);
            handle_axis_event(0, 0, z_axis.mickeys);
            z_axis.mickeys = 0;
            break;
      }
   }
}



/* process_abs: [fdwatch thread]
 *  process absolute events (x, y and z movement are currently supported)
 *  TODO: missing handle_axis_event calls
 */
static void process_abs(const struct input_event *event)
{
   /* The device can send a report when there's no tool */
   if (current_tool!=no_tool) {
      switch (event->code) {
         case ABS_X:
            x_axis.out_abs = abs_event(&x_axis, current_tool->mode, event->value);
            break;

         case ABS_Y:
            y_axis.out_abs = abs_event(&y_axis, current_tool->mode, event->value);
            break;

#ifdef ABS_WHEEL  /* absent in 2.2.x */
         case ABS_WHEEL:
#endif
         case ABS_Z:
            z_axis.out_abs = abs_event(&z_axis, current_tool->mode, event->value);
            break;
      }
   }
}



/* [fdwatch thread] */
static void handle_axis_event(int dx, int dy, int dz)
{
   if (current_tool != no_tool) {
      x_axis.out_abs = CLAMP(x_axis.out_min, x_axis.out_abs, x_axis.out_max);
      y_axis.out_abs = CLAMP(y_axis.out_min, y_axis.out_abs, y_axis.out_max);
      /* There's no range for z */

      the_mouse.state.x = x_axis.out_abs;
      the_mouse.state.y = y_axis.out_abs;
      the_mouse.state.z = z_axis.out_abs;

      generate_mouse_event(
         ALLEGRO_EVENT_MOUSE_AXES,
         the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
         dx, dy, dz,
         0);
   }
}



/* open_mouse_device:
 *  Open the specified device file and check that it is a mouse device.
 *  Returns the file descriptor if successful or a negative number on error.
 */
static int open_mouse_device (const char *device_file)
{
   int fd;

   fd = open (device_file, O_RDONLY | O_NONBLOCK);
   if (fd >= 0) {
      TRACE(PREFIX_I "Opened device %s\n", device_file);
      /* The device is a mouse if it has a BTN_MOUSE */
      if (has_event(fd, EV_KEY, BTN_MOUSE)) {
	 TRACE(PREFIX_I "Device %s was a mouse.\n", device_file);
      }
      else {
	 TRACE(PREFIX_I "Device %s was not mouse, closing.\n", device_file);
	 close(fd);
	 fd = -1;
      }
   }

   return fd;
}



/* mouse_init:
 *  Here we open the mouse device, initialise anything that needs it, 
 *  and chain to the framework init routine.
 */
static bool mouse_init (void)
{
   char tmp1[128], tmp2[128];
   const char *udevice;

   /* Set the current tool */
   current_tool = default_tool;

   /* Find the device filename */
   udevice = get_config_string (uconvert_ascii ("mouse", tmp1),
                                uconvert_ascii ("mouse_device", tmp2),
                                NULL);

   /* Open mouse device.  Devices are cool. */
   if (udevice) {
      TRACE(PREFIX_I "Trying %s device\n", udevice);
      the_mouse.fd = open (uconvert_toascii (udevice, tmp1), O_RDONLY | O_NONBLOCK);
      if (the_mouse.fd < 0) {
         uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open %s: %s"),
                    udevice, ustrerror (errno));
         return false;
      }
   }
   else {
      /* If not specified in the config file, try several /dev/input/event<n>
       * devices. */
      const char *device_name[] = { "/dev/input/event0",
                                    "/dev/input/event1",
                                    "/dev/input/event2",
                                    "/dev/input/event3",
                                    NULL };
      int i;

      TRACE(PREFIX_I "Trying /dev/input/event[0-3] devices\n");

      for (i=0; device_name[i]; i++) {
         the_mouse.fd = open_mouse_device (device_name[i]);
         if (the_mouse.fd >= 0)
	    break;
      }

      if (!device_name[i]) {
	 uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open a mouse device: %s"),
		    ustrerror (errno));
	 return false;
      }
   }

   /* Init the tablet data */
   init_tablet(the_mouse.fd);

   /* Initialise the mouse object for use as an event source. */
   _al_event_source_init(&the_mouse.parent.es);

   /* Start watching for data on the fd. */
   _al_unix_start_watching_fd(the_mouse.fd, process_new_data, &the_mouse);

   return true;
}



/* mouse_exit:
 *  Chain to the framework, then uninitialise things.
 */
static void mouse_exit (void)
{
   _al_unix_stop_watching_fd(the_mouse.fd);

   _al_event_source_free(&the_mouse.parent.es);

   close(the_mouse.fd);

   /* This may help catch bugs in the user program, since the pointer
    * we return to the user is always the same.
    */
   memset(&the_mouse, 0, sizeof the_mouse);
   the_mouse.fd = -1;
}



/* mouse_get_mouse:
 *  Returns the address of a ALLEGRO_MOUSE structure representing the mouse.
 */
static ALLEGRO_MOUSE *mouse_get_mouse(void)
{
   return (ALLEGRO_MOUSE *)&the_mouse;
}



/* mouse_get_mouse_num_buttons:
 *  Return the number of buttons on the mouse.
 */
static unsigned int mouse_get_mouse_num_buttons(void)
{
   ASSERT(the_mouse.fd >= 0);

   return get_num_buttons(the_mouse.fd);
}



/* mouse_get_mouse_num_axes:
 *  Return the number of axes on the mouse.
 */
static unsigned int mouse_get_mouse_num_axes(void)
{
   ASSERT(the_mouse.fd >= 0);

   /* XXX get the right number later */
   return 3;
}



/* mouse_set_mouse_xy:
 *
 */
static bool mouse_set_mouse_xy(ALLEGRO_DISPLAY *display, int x, int y)
{
   (void)display;

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dx, dy;

      x_axis.out_abs = _ALLEGRO_CLAMP(x_axis.out_min, x, x_axis.out_max);
      y_axis.out_abs = _ALLEGRO_CLAMP(y_axis.out_min, y, y_axis.out_max);
      x_axis.mickeys = 0;
      y_axis.mickeys = 0;

      dx = x_axis.out_abs - the_mouse.state.x;
      dy = y_axis.out_abs - the_mouse.state.y;

      if ((dx != 0) && (dy != 0)) {
         the_mouse.state.x = x_axis.out_abs;
         the_mouse.state.y = y_axis.out_abs;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            dx, dy, 0,
            0);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* mouse_set_mouse_axis:
 *
 *   Number of mickeys to cross the screen horizontally: speed * 320.
 */
static bool mouse_set_mouse_axis(int which, int z)
{
   if (which != 2) {
      return false;
   }

   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dz;

      z_axis.out_abs = z;

      dz = z_axis.out_abs - the_mouse.state.z;

      if (dz != 0) {
         the_mouse.state.z = z_axis.out_abs;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            0, 0, dz,
            0);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* mouse_set_mouse_range:
 *
 */
static bool mouse_set_mouse_range(int x1, int y1, int x2, int y2)
{
   _al_event_source_lock(&the_mouse.parent.es);
   {
      int dx, dy;

      x_axis.out_min = x1;
      y_axis.out_min = y1;
      x_axis.out_max = x2;
      y_axis.out_max = y2;

      x_axis.out_abs = CLAMP(x_axis.out_min, x_axis.out_abs, x_axis.out_max);
      y_axis.out_abs = CLAMP(y_axis.out_min, y_axis.out_abs, y_axis.out_max);

      dx = x_axis.out_abs - the_mouse.state.x;
      dy = y_axis.out_abs - the_mouse.state.y;

      if ((dx != 0) && (dy != 0)) {
         the_mouse.state.x = x_axis.out_abs;
         the_mouse.state.y = y_axis.out_abs;

         generate_mouse_event(
            ALLEGRO_EVENT_MOUSE_AXES,
            the_mouse.state.x, the_mouse.state.y, the_mouse.state.z,
            dx, dy, 0,
            0);
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);

   return true;
}



/* mouse_get_state:
 *  Copy the current mouse state into RET_STATE, with any necessary locking.
 */
static void mouse_get_state(ALLEGRO_MOUSE_STATE *ret_state)
{
   _al_event_source_lock(&the_mouse.parent.es);
   {
      *ret_state = the_mouse.state;
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* process_new_data: [fdwatch thread]
 *  Process new data arriving in the keyboard's fd.
 */
static void process_new_data(void *data)
{
   ASSERT((AL_MOUSE_EVDEV *)data == &the_mouse);
   
   _al_event_source_lock(&the_mouse.parent.es);
   {
      struct input_event events[32];
      int bytes, nr, i;

      while ((bytes = read(the_mouse.fd, &events, sizeof events)) > 0) {
         nr = bytes / sizeof(events[0]);
         for (i = 0; i < nr; i++) {
            process_event(&events[i]);
         }
      }
   }
   _al_event_source_unlock(&the_mouse.parent.es);
}



/* [fdwatch thread] */
static void process_event(struct input_event *event)
{
   switch (event->type) {
      case EV_KEY:
         process_key(event);
         break;
      case EV_REL:
         process_rel(event);
         break;
      case EV_ABS:
         process_abs(event);
         break;
   }
}



/* [fdwatch thread] */
static void generate_mouse_event(unsigned int type,
                                 int x, int y, int z,
                                 int dx, int dy, int dz,
                                 unsigned int button)
{
   ALLEGRO_EVENT event;

   if (!_al_event_source_needs_to_generate_event(&the_mouse.parent.es))
      return;

   event.mouse.type = type;
   event.mouse.timestamp = al_get_time();
   event.mouse.display = NULL;
   event.mouse.x = x;
   event.mouse.y = y;
   event.mouse.z = z;
   event.mouse.dx = dx;
   event.mouse.dy = dy;
   event.mouse.dz = dz;
   event.mouse.button = button;
   event.mouse.pressure = 0.0; /* TODO */
   _al_event_source_emit_event(&the_mouse.parent.es, &event);
}



/* the driver vtable */
ALLEGRO_MOUSE_DRIVER _al_mousedrv_linux_evdev =
{
   AL_MOUSEDRV_LINUX_EVDEV,
   "",
   "",
   "Linux EVDEV mouse (and tablet)",
   mouse_init,
   mouse_exit,
   mouse_get_mouse,
   mouse_get_mouse_num_buttons,
   mouse_get_mouse_num_axes,
   mouse_set_mouse_xy,
   mouse_set_mouse_axis,
   mouse_set_mouse_range,
   mouse_get_state
};



#endif /* ALLEGRO_HAVE_LINUX_INPUT_H */

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
