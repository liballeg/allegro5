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
#include <sys/stat.h>

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
#define MAX_DEVICE_NAME      128


typedef enum {
   LJOY_MARK   = (1 << 0),
   LJOY_REMOVE = (1 << 1)
} CONFIG_BITS;


/* map a Linux joystick axis number to an Allegro (stick,axis) pair */
typedef struct {
   int stick;
   int axis;
} AXIS_MAPPING;


typedef struct ALLEGRO_JOYSTICK_LINUX
{
   ALLEGRO_JOYSTICK parent;
   int fd;
   char device_name[MAX_DEVICE_NAME];
   int config_bits;

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


static unsigned num_joysticks;   /* number of joysticks known to the user */
static _AL_VECTOR joysticks;     /* of ALLEGRO_JOYSTICK_LINUX pointers */
static volatile bool config_needs_merging;
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
      ALLEGRO_WARN("Your Linux joystick API is version 0.x which is unsupported.\n");
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



static ALLEGRO_JOYSTICK_LINUX *ljoy_by_device_name(const char *device_name)
{
   unsigned i;

   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *slot;

      if (joy && 0 == strcmp(device_name, joy->device_name))
         return joy;
   }

   return NULL;
}



static bool ljoy_detect_device_name(int num, char device_name[MAX_DEVICE_NAME])
{
   int try;
   struct stat stbuf;

   for (try = 0; try < 2; try++) {
      /* XXX allow user to override device path in config file */
      if (try == 0)
         snprintf(device_name, MAX_DEVICE_NAME, "/dev/input/js%d", num);
      else
         snprintf(device_name, MAX_DEVICE_NAME, "/dev/js%d", num);
      device_name[MAX_DEVICE_NAME-1] = 0;

      if (stat(device_name, &stbuf) == 0)
         return true;
   }

   return false;
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



static bool ljoy_schedule_removals(void)
{
   bool any_removals = false;
   unsigned i;

   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *slot;

      if (joy && !(joy->config_bits & LJOY_MARK)) {
         if (!(joy->config_bits & LJOY_REMOVE)) {
            ALLEGRO_DEBUG("Device %s to be removed\n", joy->device_name);
            joy->config_bits |= LJOY_REMOVE;
         }
         any_removals = true;
      }
   }

   return any_removals;
}



static void ljoy_generate_configure_event(void)
{
   ALLEGRO_EVENT event;
   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.joystick.timestamp = al_current_time();

   _al_generate_joystick_event(&event);
}



static void ljoy_scan(bool configure)
{
   int fd;
   ALLEGRO_JOYSTICK_LINUX *joy, **joypp;
   int num;
   char device_name[MAX_DEVICE_NAME];
   char name_path[MAX_DEVICE_NAME];
   FILE *name_file;
   unsigned i;
   bool any_removals;

   /* Any joystick which remains unmarked will be scheduled for removal. */
   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      joypp = _al_vector_ref(&joysticks, i);
      joy = *joypp;
      joy->config_bits &= ~LJOY_MARK;
   }

   /* This is an insanely big number, but there can be gaps */
   /* XXX this is going to make us look dumb to anyone watching strace */
   for (num = 0; num < 256; num++) {
      if (!ljoy_detect_device_name(num, device_name))
         continue;

      joy = ljoy_by_device_name(device_name);
      if (joy) {
         ALLEGRO_DEBUG("Device %s still exists\n", device_name);
         joy->config_bits |= LJOY_MARK;
         continue;
      }

      /* Try to open the device. */
      fd = open(device_name, O_RDONLY|O_NONBLOCK);
      if (fd == -1) {
         ALLEGRO_WARN("Failed to open device %s\n", device_name);
         continue;
      }
      if (!check_js_api_version(fd)) {
         close(fd);
         continue;
      }
      ALLEGRO_DEBUG("Device %s is new\n", device_name);

      /* Allocate a structure for the joystick. */
      joy = al_calloc(1, sizeof *joy);
      if (!joy) {
         close(fd);
         break;
      }

      joy->fd = fd;
      strcpy(joy->device_name, device_name);
      joy->config_bits = LJOY_MARK;

      sprintf(name_path, "/sys/class/input/js%d/device/name", num);
      name_file = fopen(name_path, "r");
      if (name_file) {
         if (fgets(joy->name, sizeof(joy->name), name_file) == NULL) {
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
   
      joypp = _al_vector_alloc_back(&joysticks);
      *joypp = joy;

      /* Register the joystick with the fdwatch subsystem.  */
      _al_unix_start_watching_fd(joy->fd, ljoy_process_new_data, joy);
   }

   /* Schedule unmarked joysticks for removal. */
   any_removals = ljoy_schedule_removals();

   /* Generate a configure event if any joysticks need to be added or removed.
    * Even if we generated one before that the user hasn't responded to,
    * we don't know if the user received it so always generate it.
    */
   if (any_removals || num_joysticks != _al_vector_size(&joysticks)) {
      ALLEGRO_DEBUG("Joystick configuration needs merging\n");
      config_needs_merging = true;
      if (configure) {
         ljoy_generate_configure_event();
      }
   }
}



static bool fill_empty_slot(ALLEGRO_JOYSTICK_LINUX *joy, unsigned max)
{
   unsigned i;

   ASSERT(!(joy->config_bits & LJOY_REMOVE));

   for (i = 0; i < max; i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      if (!*slot) {
         *slot = joy;
         return true;
      }
   }
   return false;
}



static void ljoy_merge(void)
{
   unsigned i;

   config_needs_merging = false;

   /* Remove old joysticks. */
   for (i = 0; i < num_joysticks; i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *slot;

      if (joy->config_bits & LJOY_REMOVE) {
         destroy_joy(joy);
         *slot = NULL;
      }
   }

   /* Move new joysticks into vacated slots. */
   for (i = num_joysticks; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *slot;

      if (fill_empty_slot(joy, num_joysticks))
         *slot = NULL;
      else
         break;
   }

   /* Delete remaining empty slots. */
   for (i = 0; i < _al_vector_size(&joysticks); ) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);

      if (!*slot)
         _al_vector_delete_at(&joysticks, i);
      else
         i++;
   }

   num_joysticks = _al_vector_size(&joysticks);
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
   num_joysticks = 0;

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

   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      destroy_joy(*slot);
   }
   _al_vector_free(&joysticks);
   num_joysticks = 0;

   al_destroy_mutex(config_mutex);
   config_mutex = NULL;
}



/* ljoy_num_joysticks: [primary thread]
 *
 *  Return the number of joysticks available on the system.
 */
static int ljoy_num_joysticks(void)
{
   if (config_needs_merging) {
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
   ALLEGRO_JOYSTICK_LINUX **slot;
   ASSERT(num >= 0 && num < (int)num_joysticks);

   slot = _al_vector_ref(&joysticks, num);
   return (ALLEGRO_JOYSTICK *) *slot;
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
