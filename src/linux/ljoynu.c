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
 *      Linux joystick driver.
 *
 *      By George Foot and Peter Wang.
 *
 *      Updated for new joystick API by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define ALLEGRO_NO_KEY_DEFINES
#define ALLEGRO_NO_COMPATIBILITY

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include ALLEGRO_INTERNAL_HEADER

#ifdef ALLEGRO_HAVE_LINUX_JOYSTICK_H

/* To be safe, include sys/types.h before linux/joystick.h to avoid conflicting
 * definitions of fd_set.
 */
#include <sys/types.h>
#include <linux/joystick.h>

ALLEGRO_DEBUG_CHANNEL("ljoy");

#define TOTAL_JOYSTICK_AXES  (_AL_MAX_JOYSTICK_STICKS * _AL_MAX_JOYSTICK_AXES)



/* map a Linux joystick axis number to an Allegro (stick,axis) pair */
typedef struct {
   int stick;
   int axis;
} AXIS_MAPPING;


typedef struct ALLEGRO_JOYSTICK_LINUX
{
   ALLEGRO_JOYSTICK parent;
   int fd;
   AXIS_MAPPING axis_mapping[TOTAL_JOYSTICK_AXES];
   ALLEGRO_JOYSTICK_STATE joystate;
   char name[100];
} ALLEGRO_JOYSTICK_LINUX;



/* forward declarations */
static bool ljoy_init_joystick(void);
static void ljoy_exit_joystick(void);
static int ljoy_num_joysticks(void);
static ALLEGRO_JOYSTICK *ljoy_get_joystick(int num);
static void ljoy_release_joystick(ALLEGRO_JOYSTICK *joy_);
static void ljoy_get_joystick_state(ALLEGRO_JOYSTICK *joy_, ALLEGRO_JOYSTICK_STATE *ret_state);
static const char *ljoy_get_name(ALLEGRO_JOYSTICK *joy_);

static void ljoy_process_new_data(void *data);
static void ljoy_generate_axis_event(ALLEGRO_JOYSTICK_LINUX *joy, int stick, int axis, float pos);
static void ljoy_generate_button_event(ALLEGRO_JOYSTICK_LINUX *joy, int button, ALLEGRO_EVENT_TYPE event_type);



/* the driver vtable */
#define JOYDRV_LINUX    AL_ID('L','N','X','A')

ALLEGRO_JOYSTICK_DRIVER _al_joydrv_linux =
{
   _ALLEGRO_JOYDRV_LINUX,
   "",
   "",
   "Linux joystick(s)",
   ljoy_init_joystick,
   ljoy_exit_joystick,
   ljoy_num_joysticks,
   ljoy_get_joystick,
   ljoy_release_joystick,
   ljoy_get_joystick_state,
   ljoy_get_name
};


static int num_joysticks;
static int num_joysticks_real;
static _AL_VECTOR joysticks;        /* of ALLEGRO_JOYSTICK_LINUX pointers */
static _AL_VECTOR joysticks_real;   /* of ALLEGRO_JOYSTICK_LINUX pointers */
static bool config_merged;
static ALLEGRO_MUTEX *config_mutex;
static ALLEGRO_THREAD *config_thread;


/* check_js_api_version: [primary thread]
 *
 *  Return true if the joystick API used by the device is supported by
 *  this driver.
 */
static bool check_js_api_version(int fd)
{
   unsigned int raw_version;

   if (ioctl(fd, JSIOCGVERSION, &raw_version) < 0) {
      /* NOTE: IOCTL fails if the joystick API is version 0.x */
      TRACE("Your Linux joystick API is version 0.x which is unsupported.\n");
      return false;
   }

   /*
   struct { unsigned char build, minor, major; } version;

   version.major = (raw_version & 0xFF0000) >> 16;
   version.minor = (raw_version & 0xFF00) >> 8;
   version.build = (raw_version & 0xFF);
   */

   return true;
}



/* try_open_joy_device: [primary thread]
 *
 *  Try to open joystick device number NUM, returning the fd on success
 *  or -1 on failure.
 */
static int try_open_joy_device(int num)
{
   const char *device_name = NULL;
   char tmp[128];
   /* char tmp1[128], tmp2[128]; */
   int fd;

   /* XXX use configuration system when we get one */
   device_name = NULL;
#if 0
   /* Check for a user override on the device to use. */
   snprintf(tmp, sizeof(tmp), "joystick_device_%d", num);
   device_name = get_config_string("joystick", tmp, NULL);

   /* Special case for the first joystick. */
   if (!device_name && (num == 0))
      device_name = get_config_string("joystick",
                                      "joystick_device",
                                      NULL);
#endif

   if (device_name)
      fd = open(device_name, O_RDONLY|O_NONBLOCK);
   else {
      snprintf(tmp, sizeof(tmp), "/dev/input/js%d", num);
      tmp[sizeof(tmp)-1] = 0;

      fd = open(tmp, O_RDONLY|O_NONBLOCK);
      if (fd == -1) {
         snprintf(tmp, sizeof(tmp), "/dev/js%d", num);
         tmp[sizeof(tmp)-1] = 0;

         fd = open(tmp, O_RDONLY|O_NONBLOCK);
         if (fd == -1)
	    return -1;
      }
   }

   if (!check_js_api_version(fd)) {
      close(fd);
      return -1;
   }

   return fd;   
}



static void destroy_joy(ALLEGRO_JOYSTICK_LINUX *joy)
{
   int i;
   
   _al_unix_stop_watching_fd(joy->fd);

   close(joy->fd);
   for (i = 0; i < joy->parent.info.num_sticks; i++)
      al_free((void *)joy->parent.info.stick[i].name);
   for (i = 0; i < joy->parent.info.num_buttons; i++)
      al_free((void *)joy->parent.info.button[i].name);
   al_free(joy);
}



static void ljoy_generate_configure_event(void)
{
   ALLEGRO_EVENT event;
   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.joystick.timestamp = al_current_time();

   _al_generate_joystick_event(&event);
}



/*
static void copy_joy(ALLEGRO_JOYSTICK_LINUX *dst, ALLEGRO_JOYSTICK_LINUX *src)
{
   int i;
   dst->parent.info.num_sticks = src->parent.info.num_sticks;
   dst->parent.info.num_buttons = src->parent.info.num_buttons;
   for (i = 0; i < src->parent.info.num_sticks; i++) {
      dst->parent.info.stick[i].name = strdup(src->parent.info.stick[i].name);
   }
   for (i = 0; i < src->parent.info.num_buttons; i++) {
      dst->parent.info.button[i].name = strdup(src->parent.info.button[i].name);
   }
   dst->fd = src->fd;
   memcpy(dst->axis_mapping, src->axis_mapping, sizeof(src->axis_mapping));
   memcpy(&dst->joystate, &src->joystate, sizeof(src->joystate));
   memcpy(dst->name, src->name, sizeof(src->name));
}
*/



static void ljoy_scan(bool configure)
{
   int fd;
   ALLEGRO_JOYSTICK_LINUX *joy, **joypp;
   int num;
   FILE *name_file;
   char buf[100];
   int i, j;

   config_merged = false;
   ASSERT(num_joysticks_real == 0);

   /* This is an insanely big number, but there can be gaps */
   for (num = 0; num < 256; num++) {
      /* Try to open the device. */
      fd = try_open_joy_device(num);
      if (fd == -1) {
         continue;
      }
   
      /* Allocate a structure for the joystick. */
      joy = al_malloc(sizeof *joy);
      if (!joy) {
         close(fd);
         break;
      }
      memset(joy, 0, sizeof *joy);

      sprintf(buf, "/sys/class/input/js%d/device/name", num);
      name_file = fopen(buf, "r");
      if (name_file) {
         if (fgets(joy->name, sizeof(joy->name)-1, name_file) == NULL) {
            joy->name[0] = 0;
         }
         fclose(name_file);
      }
      else {
         joy->name[0] = 0;
      }

      /* Fill in the joystick information fields. */
      {
         char num_axes;
         char num_buttons;
         int throttle;
         int s, a, b;
   
         ioctl(fd, JSIOCGAXES, &num_axes);
         ioctl(fd, JSIOCGBUTTONS, &num_buttons);
   
         if (num_axes > TOTAL_JOYSTICK_AXES)
            num_axes = TOTAL_JOYSTICK_AXES;
   
         if (num_buttons > _AL_MAX_JOYSTICK_BUTTONS)
            num_buttons = _AL_MAX_JOYSTICK_BUTTONS;
   
         /* XXX use configuration system when we get one */
         throttle = -1;
   #if 0
         /* User is allowed to override our simple assumption of which
          * axis number (kernel) the throttle is located at. */
         snprintf(tmp, sizeof(tmp), "throttle_axis_%d", num);
         throttle = get_config_int("joystick", tmp, -1);
         if (throttle == -1) {
            throttle = get_config_int("joystick", 
                                      "throttle_axis", -1);
         }
   #endif
   
         /* Each pair of axes is assumed to make up a stick unless it 
          * is the sole remaining axis, or has been user specified, in 
          * which case it is a throttle. */
   
         for (s = 0, a = 0;
              s < _AL_MAX_JOYSTICK_STICKS && a < num_axes;
              s++)
         {
            if ((a == throttle) || (a == num_axes-1)) {
               /* One axis throttle. */
               joy->parent.info.stick[s].flags = ALLEGRO_JOYFLAG_ANALOGUE;
               joy->parent.info.stick[s].num_axes = 1;
               joy->parent.info.stick[s].axis[0].name = "Throttle";
               char *name = joy->parent.info.stick[s].axis[0].name;
               joy->parent.info.stick[s].name = al_malloc(strlen(name) + 1);
               strcpy(joy->parent.info.stick[s].name, name);
               joy->axis_mapping[a].stick = s;
               joy->axis_mapping[a].axis = 0;
               a++;
            }
            else {
               /* Two axis stick. */
               joy->parent.info.stick[s].flags = ALLEGRO_JOYFLAG_ANALOGUE;
               joy->parent.info.stick[s].num_axes = 2;
               joy->parent.info.stick[s].axis[0].name = "X";
               joy->parent.info.stick[s].axis[1].name = "Y";
               joy->parent.info.stick[s].name = al_malloc (32);
               snprintf((char *)joy->parent.info.stick[s].name, 32, "Stick %d", s+1);
               joy->axis_mapping[a].stick = s;
               joy->axis_mapping[a].axis = 0;
               a++;
               joy->axis_mapping[a].stick = s;
               joy->axis_mapping[a].axis = 1;
               a++;
            }
         }
   
         joy->parent.info.num_sticks = s;
   
         /* Do the buttons. */
   
         for (b = 0; b < num_buttons; b++) {
            joy->parent.info.button[b].name = al_malloc (32);
            snprintf((char *)joy->parent.info.button[b].name, 32, "B%d", b+1);
         }
   
         joy->parent.info.num_buttons = num_buttons;
      }
   
      joy->fd = fd;
   
      joypp = _al_vector_alloc_back(&joysticks_real);
      *joypp = joy;

      num_joysticks_real++;

      /* Register the joystick with the fdwatch subsystem.  */
         
      _al_unix_start_watching_fd(joy->fd, ljoy_process_new_data, joy);
   }

   if (configure) {
      if (num_joysticks != num_joysticks_real) {
         ljoy_generate_configure_event();
	 return;
      }
      for (i = 0; i < num_joysticks; i++) {
         bool found = false;
         for (j = 0; j < num_joysticks_real; j++) {
            ALLEGRO_JOYSTICK_LINUX *joy1;
            ALLEGRO_JOYSTICK_LINUX *joy2;
            joy1 = *((ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks, i));
            joy2 = *((ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks_real, j));
            if (!strcmp(joy1->name, joy2->name)) {
               found = true;
               break;
            }
         }
         if (!found) {
            ljoy_generate_configure_event();
            return;
         }
      }
   }
}



static void ljoy_merge(void)
{
   int i, j;
   int count = 0;
   int size;
   ALLEGRO_JOYSTICK_LINUX *joy1, *joy2, **joypp;

   config_merged = true;

   ALLEGRO_INFO("Merging num_joysticks=%d, num_joysticks_real=%d\n",
      num_joysticks, num_joysticks_real);

   // Keep only joysticks that are still around
   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      bool found = false;
      joy1 = *((ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks, i));
      for (j = 0; j < num_joysticks_real; j++) {
         joy2 = *((ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks_real, j));
         if (!strcmp(joy1->name, joy2->name)) {
            found = true;
            break;
         }
      }
      if (found) {
         (*(ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks, count)) = joy1;
         count++;
      }
      else {
         destroy_joy(joy1);
      }
   }

   size = _al_vector_size(&joysticks);
   for (i = count; i < size; i++) {
      _al_vector_delete_at(&joysticks, count);
   }

   for (i = 0; i < (int)_al_vector_size(&joysticks_real); i++) {
      bool found = false;
      joy2 = *((ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks_real, i));
      for (j = 0; j < count; j++) {
         joy1 = *((ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks, j));
         if (!strcmp(joy1->name, joy2->name)) {
            found = true;
            break;
         }
      }
      if (!found) {
         joypp = _al_vector_alloc_back(&joysticks);
         *joypp = joy2;
         count++;
      }
      else {
         destroy_joy(joy2);
      }
   }

   size = _al_vector_size(&joysticks_real);
   for (i = 0; i < size; i++) {
      _al_vector_delete_at(&joysticks_real, 0);
   }

   num_joysticks = count;
   num_joysticks_real = 0;

   ALLEGRO_DEBUG("Merging done\n");
}



/* config_thread_proc: [joystick config thread]
 *  Rescans for joystick devices periodically.
 */
static void *config_thread_proc(ALLEGRO_THREAD *thread, void *data)
{
   (void)data;

   /* XXX We probably can do this more efficiently using directory change
    * notifications (dnotify?).
    */

   while (!al_get_thread_should_stop(thread)) {
      al_rest(1);

      al_lock_mutex(config_mutex);

      while (!_al_vector_is_empty(&joysticks_real)) {
         ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref_back(&joysticks_real);
         ALLEGRO_JOYSTICK_LINUX *joy = *slot;
         destroy_joy(joy);
         _al_vector_delete_at(&joysticks_real, _al_vector_size(&joysticks_real)-1);
      }

      num_joysticks_real = 0;
      ljoy_scan(true);

      al_unlock_mutex(config_mutex);
   }

   return NULL;
}



/* ljoy_init_joystick: [primary thread]
 *  Initialise the joystick driver.
 */
static bool ljoy_init_joystick(void)
{
   _al_vector_init(&joysticks, sizeof(ALLEGRO_JOYSTICK_LINUX *));
   _al_vector_init(&joysticks_real, sizeof(ALLEGRO_JOYSTICK_LINUX *));

   // Scan for joysticks
   ljoy_scan(false);
   ljoy_merge();

   config_mutex = al_create_mutex();
   config_thread = al_create_thread(config_thread_proc, NULL);
   /* XXX handle failure */
   al_start_thread(config_thread);

   return true;
}



/* ljoy_exit_joystick: [primary thread]
 *  Shut down the joystick driver.
 */
static void ljoy_exit_joystick(void)
{
   int i;

   al_destroy_thread(config_thread);
   config_thread = NULL;

   ljoy_merge();
   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      destroy_joy(*slot);
   }
   _al_vector_free(&joysticks);
   _al_vector_free(&joysticks_real);
   num_joysticks = 0;
   num_joysticks_real = 0;

   al_destroy_mutex(config_mutex);
   config_mutex = NULL;
}



/* ljoy_num_joysticks: [primary thread]
 *
 *  Return the number of joysticks available on the system.
 */
static int ljoy_num_joysticks(void)
{
   if (!config_merged) {
      al_lock_mutex(config_mutex);
      ljoy_merge();
      al_unlock_mutex(config_mutex);
   }
   return num_joysticks;
}



/* ljoy_get_joystick: [primary thread]
 *
 *  Returns the address of a ALLEGRO_JOYSTICK structure for the device
 *  number NUM.  The top-level joystick functions will not call this
 *  function if joystick number NUM was already gotten.  If the
 *  device cannot be opened, NULL is returned.
 */
static ALLEGRO_JOYSTICK *ljoy_get_joystick(int num)
{
   ASSERT(num >= 0 && num < num_joysticks);

   return (ALLEGRO_JOYSTICK *)*((ALLEGRO_JOYSTICK_LINUX **)_al_vector_ref(&joysticks, num));
}



/* ljoy_release_joystick: [primary thread]
 *
 *  Close the device for a joystick then free the joystick structure.
 */
static void ljoy_release_joystick(ALLEGRO_JOYSTICK *joy_)
{
   (void)joy_;
}



/* ljoy_get_joystick_state: [primary thread]
 *
 *  Copy the internal joystick state to a user-provided structure.
 */
static void ljoy_get_joystick_state(ALLEGRO_JOYSTICK *joy_, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ALLEGRO_JOYSTICK_LINUX *joy = (ALLEGRO_JOYSTICK_LINUX *) joy_;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   _al_event_source_lock(es);
   {
      *ret_state = joy->joystate;
   }
   _al_event_source_unlock(es);
}



static const char *ljoy_get_name(ALLEGRO_JOYSTICK *joy_)
{
   ALLEGRO_JOYSTICK_LINUX *joy = (ALLEGRO_JOYSTICK_LINUX *)joy_;
   return joy->name;
}



/* ljoy_process_new_data: [fdwatch thread]
 *
 *  Process new data arriving in the joystick's fd.
 */
static void ljoy_process_new_data(void *data)
{
   ALLEGRO_JOYSTICK_LINUX *joy = data;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!es) {
      // Joystick driver not fully initialized
      return;
   }
   
   _al_event_source_lock(es);
   {
      struct js_event js_events[32];
      int bytes, nr, i;

      while ((bytes = read(joy->fd, &js_events, sizeof js_events)) > 0) {

         nr = bytes / sizeof(struct js_event);

         for (i = 0; i < nr; i++) {

            int type   = js_events[i].type;
            int number = js_events[i].number;
            int value  = js_events[i].value;

            if (type & JS_EVENT_BUTTON) {
               if (number < _AL_MAX_JOYSTICK_BUTTONS) {
                  if (value)
                     joy->joystate.button[number] = 32767;
                  else
                     joy->joystate.button[number] = 0;

                  ljoy_generate_button_event(joy, number,
                                             (value
                                              ? ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN
                                              : ALLEGRO_EVENT_JOYSTICK_BUTTON_UP));
               }
            }
            else if (type & JS_EVENT_AXIS) {
               if (number < TOTAL_JOYSTICK_AXES) {
                  int stick = joy->axis_mapping[number].stick;
                  int axis  = joy->axis_mapping[number].axis;
                  float pos = value / 32767.0;

                  joy->joystate.stick[stick].axis[axis] = pos;

                  ljoy_generate_axis_event(joy, stick, axis, pos);
               }
            }
         }
      }
   }
   _al_event_source_unlock(es);
}



/* ljoy_generate_axis_event: [fdwatch thread]
 *
 *  Helper to generate an event after an axis is moved.
 *  The joystick must be locked BEFORE entering this function.
 */
static void ljoy_generate_axis_event(ALLEGRO_JOYSTICK_LINUX *joy, int stick, int axis, float pos)
{
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



/* ljoy_generate_button_event: [fdwatch thread]
 *
 *  Helper to generate an event after a button is pressed or released.
 *  The joystick must be locked BEFORE entering this function.
 */
static void ljoy_generate_button_event(ALLEGRO_JOYSTICK_LINUX *joy, int button, ALLEGRO_EVENT_TYPE event_type)
{
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

#endif /* ALLEGRO_HAVE_LINUX_JOYSTICK_H */



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
