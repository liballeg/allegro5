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
 *      Linux console internal mouse driver for event interface
 *      (EVDEV).
 *
 *      By Annie Testes.
 *
 *      Handles all inputs that fill evdev with informations
 *      about movement and click.
 *
 *      See readme.txt for copyright information.
 */

/* TODO:
 * [ ] Remove axis range from the config file ?
 * [ ] Speed
 * [ ] Mickeys
 */


/* linux/input.h also defines KEY_A et al. */
#define ALLEGRO_NO_KEY_DEFINES
#include "allegro.h"

#ifdef HAVE_LINUX_INPUT_H

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/input.h>



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

   int speed;       /* For set_mouse_speed */
   int mickeys;     /* For get_mouse_mickeys */

   int in_abs;      /* Current absolute position */
   int out_abs;
} AXIS;


#define IN_RANGE(axis)  ( (axis).in_max-(axis).in_min+1 )
#define OUT_RANGE(axis)  ( (axis).out_max-(axis).out_min+1 )


/* in_to_screen:
 *  maps an input absolute position to a screen position
 */
static int in_to_screen(AL_CONST AXIS *axis, int v)
{
   return (((v-axis->in_min) * OUT_RANGE(*axis)) / IN_RANGE(*axis)) + axis->out_min;
}



/* screen_to_in:
 *  maps a screen position to an input one
 */
static int screen_to_in(AL_CONST AXIS *axis, int v)
{
   return (((v-axis->out_min) * IN_RANGE(*axis)) / OUT_RANGE(*axis)) + axis->in_min;
}



/* in_to_rel:
 *  scale an input position
 */
static int in_to_rel(AL_CONST AXIS *axis, int v)
{
   return v / axis->speed;
}



/* set_value:
 *  updates fields in an axis, depending on the mouse position on the screen
 */
static void set_value(AXIS *axis, int out_abs)
{
   if (current_tool->mode == MODE_ABSOLUTE) {
      axis->in_abs = screen_to_in(axis, out_abs);
   }
   else {
      axis->in_abs += (axis->out_abs-out_abs) * axis->speed;
   }
   axis->out_abs = out_abs;
   axis->mickeys = 0;
}



/* rel_event:
 *  returns the new screen position, given the input relative one.
 *  The tool mode is always relative
 */
static int rel_event(AXIS *axis, int v)
{
   /* When input only send relative events, the mode is always relative */
   int rel = in_to_rel(axis, v);
   int ret = axis->out_abs + rel;
   axis->mickeys += rel;
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
   else {
      return rel_event(axis, v-axis->in_abs);
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



/* The three axis: horizontal, vertical and wheel */
static AXIS x_axis;
static AXIS y_axis;
static AXIS z_axis;


/*
 * Previous button status
 */
static int button_left = 0;
static int button_right = 0;
static int button_middle = 0;




/*
 * Initialization functions
 */

/* init_axis:
 *  initialize an AXIS structure, from the device and the config file
 */
static void init_axis(int fd, AXIS *axis, AL_CONST char *name, AL_CONST char *section, int type)
{
   char tmp1[256]; /* config string */
   char tmp2[256]; /* format string */
   char tmp3[256]; /* Converted 'name' */
   int abs[5]; /* values given by the input */

   uszprintf(tmp1, sizeof(tmp1), uconvert_ascii("ev_min_%s", tmp2), uconvert_ascii(name, tmp3));
   axis->in_min = get_config_int(section, tmp1, 0);
   uszprintf(tmp1, sizeof(tmp1), uconvert_ascii("ev_max_%s", tmp2), uconvert_ascii(name, tmp3));
   axis->in_max = get_config_int(section, tmp1, 0);
   /* Ask the input */
   if (ioctl(fd, EVIOCGABS(type), abs)>=0) {
      if (axis->in_min==0) axis->in_min=abs[1];
      if (axis->in_max==0) axis->in_max=abs[2];
      axis->in_abs = abs[0];
   }
   if (axis->in_min>axis->in_max)
      axis->in_min = axis->in_max = 0;

   axis->out_min = 0;
   axis->out_max = 0;

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
 * Driver structure
 */
static int processor (unsigned char *buf, int buf_size);
static INTERNAL_MOUSE_DRIVER intdrv = {
   -1, /* device, fill it later */
   processor,
   3  /* num_buttons */
};




/*
 * Process input functions
 */

/* process_key:
 *  process events of type key (button clicks and vicinity events are currently
 *  supported)
 */
static void process_key(AL_CONST struct input_event *event)
{
   switch (event->code) {
      /* Buttons click
       * if event->value is 1 the button has just been pressed
       * if event->value is 0 the button has just been released */
      case BTN_LEFT: /* Mouse */
      case BTN_TOUCH: /* Stylus */
         button_left = !!event->value;
         break;

      case BTN_RIGHT: /* Mouse */
      case BTN_STYLUS: /* Stylus */
         button_right = !!event->value;
         break;

      case BTN_MIDDLE: /* Mouse */
      case BTN_STYLUS2: /* Stylus */
         button_middle = !!event->value;
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
            get_axis_value(intdrv.device, &x_axis, ABS_X);
            get_axis_value(intdrv.device, &y_axis, ABS_Y);
            get_axis_value(intdrv.device, &z_axis, ABS_Z);
#ifdef ABS_WHEEL  /* absent in 2.2.x */
            get_axis_value(intdrv.device, &z_axis, ABS_WHEEL);
#endif
         }
         else {
            current_tool = no_tool;
         }
         break;
   }
}



/* process_rel:
 *  process relative events (x, y and z movement are currently supported)
 */
static void process_rel(AL_CONST struct input_event *event)
{
   /* The device can send a report when there's no tool */
   if (current_tool!=no_tool) {
      switch (event->code) {
         case REL_X:
            x_axis.out_abs = rel_event(&x_axis, event->value);
            break;

         case REL_Y:
            y_axis.out_abs = rel_event(&y_axis, event->value);
            break;

#ifdef REL_WHEEL  /* absent in 2.2.x */
         case REL_WHEEL:
#endif
         case REL_Z:
            z_axis.out_abs = rel_event(&z_axis, event->value);
            break;
      }
   }
}



/* process_abs:
 *  process absolute events (x, y and z movement are currently supported)
 */
static void process_abs(AL_CONST struct input_event *event)
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



/* processor:
 *  Processes the first packet in the buffer, if any, returning the number
 *  of bytes eaten.
 */
static int processor (unsigned char *buf, int buf_size)
{
   struct input_event *event;

   if (buf_size < (int)sizeof(struct input_event))
      return 0; /* not enough data */

   event = (struct input_event*)buf;
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

   if (current_tool!=no_tool) {
      x_axis.out_abs = MID(x_axis.out_min, x_axis.out_abs, x_axis.out_max);
      y_axis.out_abs = MID(y_axis.out_min, y_axis.out_abs, y_axis.out_max);
      /* There's no range for z */

      _mouse_x = x_axis.out_abs;
      _mouse_y = y_axis.out_abs;
      _mouse_z = z_axis.out_abs;
      _mouse_b = button_left + (button_right<<1) + (button_middle<<2);
      _handle_mouse_input();
   }
   /* Returns the size of the data used */
   return sizeof(struct input_event);
}



/* analyse_data:
 *  Analyses the given data, returning 0 if it is unparsable, nonzero
 *  if there's a reasonable chance that this driver can work with that
 *  data.
 *  Used in the setup code
 */
static int analyse_data (AL_CONST char *buffer, int size)
{
   struct input_event *event;
   struct input_event *last=(struct input_event*)(buffer+size);

   if (size < (int)sizeof(struct input_event))
      return 0; /* not enough data */

   for (event=(struct input_event*)buffer; event<last; event++) {
     if (event->type >= EV_MAX)
        return 0;

     switch (event->type) {
       case EV_KEY:
         if (event->code>=KEY_MAX)
           return 0;
         break;

       case EV_REL:
         if (event->code>=REL_MAX)
           return 0;
         break;

       case EV_ABS:
         if (event->code>=ABS_MAX)
           return 0;
         break;

#ifdef EV_MSC  /* absent in 2.2.x */
       case EV_MSC:
         if (event->code>=MSC_MAX)
           return 0;
         break;
#endif

       /* Mouse doesn't handle events of these types */
       case EV_LED:
       case EV_SND:
       case EV_REP:
#ifdef EV_FF  /* absent in 2.2.x */
       case EV_FF:
#endif
         return 0;
     }
   }
   return event==last;
}



/* mouse_init:
 *  Here we open the mouse device, initialise anything that needs it, 
 *  and chain to the framework init routine.
 */
static int mouse_init (void)
{
   char tmp1[128], tmp2[128];
   AL_CONST char *udevice;

   /* Set the current tool */
   current_tool = default_tool;

   /* Find the device filename */
   udevice = get_config_string (uconvert_ascii ("mouse", tmp1),
                                uconvert_ascii ("mouse_device", tmp2),
                                NULL);

   /* Open mouse device.  Devices are cool. */
   if (udevice) {
      intdrv.device = open (uconvert_toascii (udevice, tmp1), O_RDONLY | O_NONBLOCK);
      if (intdrv.device < 0) {
         uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open %s: %s"),
                    udevice, ustrerror (errno));
         return -1;
      }
   }
   else {
      /* If not specified in the config file, try some common device files
       * but not /dev/input/event because it may not be a mouse.
       */
      const char *device_name[] = { "/dev/input/mice",
                                    "/dev/input/mouse",
                                    "/dev/input/mouse0",
                                    NULL };
      int i;

      for (i=0; device_name[i]; i++) {
         intdrv.device = open (device_name[i], O_RDONLY | O_NONBLOCK);
         if (intdrv.device >= 0)
            goto Found;
      }

      uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open a mouse device: %s"),
                 ustrerror (errno));
      return -1;
   }

 Found:
   /* Init the tablet data */
   init_tablet(intdrv.device);

   return __al_linux_mouse_init (&intdrv);
}



/* mouse_exit:
 *  Chain to the framework, then uninitialise things.
 */
static void mouse_exit (void)
{
   __al_linux_mouse_exit();
   close (intdrv.device);
}



/* mouse_position:
 */
static void mouse_position(int x, int y)
{
   DISABLE();

   set_value(&x_axis, x);
   set_value(&y_axis, y);

   _mouse_x = x;
   _mouse_y = y;

   ENABLE();
}



/* mouse_set_range:
 */
static void mouse_set_range(int x1, int y1, int x2, int y2)
{
   x_axis.out_min = x1;
   y_axis.out_min = y1;
   x_axis.out_max = x2;
   y_axis.out_max = y2;
   /* There's no set range for z */

   DISABLE();

   _mouse_x = MID(x1, _mouse_x, x2);
   _mouse_y = MID(y1, _mouse_y, y2);

   ENABLE();
}


/* mouse_set_speed:
 */
static void mouse_set_speed(int speedx, int speedy)
{
   int scale = 4;

   if (gfx_driver)
      scale = (4*gfx_driver->w)/320;

   DISABLE();

   x_axis.speed = MAX(1, (scale * MAX(1, speedx))/2);
   y_axis.speed = MAX(1, (scale * MAX(1, speedy))/2);

   set_value(&x_axis, _mouse_x);
   set_value(&y_axis, _mouse_y);

   ENABLE();
}



/* mouse_get_mickeys:
 */
static void mouse_get_mickeys(int *mickeyx, int *mickeyy)
{
   int mx = x_axis.mickeys;
   int my = y_axis.mickeys;

   x_axis.mickeys -= mx;
   y_axis.mickeys -= my;

   *mickeyx = in_to_rel(&x_axis, mx);
   *mickeyy = in_to_rel(&y_axis, my);
}




MOUSE_DRIVER mousedrv_linux_evdev =
{
   MOUSEDRV_LINUX_EVDEV,
   empty_string,
   empty_string,
   "Linux EVDEV mouse (and tablet)",
   mouse_init,
   mouse_exit,
   NULL, /* poll() */
   NULL, /* timer_poll() */
   mouse_position,
   mouse_set_range,
   mouse_set_speed,
   mouse_get_mickeys,
   analyse_data,
	NULL /* enable_hardware_cursor */
};

#endif

