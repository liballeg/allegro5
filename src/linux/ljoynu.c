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
#include "allegro5/platform/aintunix.h"

#ifdef ALLEGRO_HAVE_LINUX_INPUT_H

/* To be safe, include sys/types.h before linux/joystick.h to avoid conflicting
 * definitions of fd_set.
 */
#include <sys/types.h>
#include <linux/input.h>

#if defined(ALLEGRO_HAVE_SYS_INOTIFY_H)
   #define SUPPORT_HOTPLUG
   #include <sys/inotify.h>
#endif

#ifndef KEY_CNT
   #define KEY_CNT (KEY_MAX+1)
#endif

#ifndef ABS_CNT
   #define ABS_CNT (ABS_MAX+1)
#endif

ALLEGRO_DEBUG_CHANNEL("ljoy");

#include "allegro5/internal/aintern_ljoynu.h"



#define LONG_BITS    (sizeof(long) * 8)
#define NLONGS(x)    (((x) + LONG_BITS - 1) / LONG_BITS)
/* Tests if a bit in an array of longs is set. */
#define TEST_BIT(nr, addr) \
   ((1UL << ((nr) % LONG_BITS)) & (addr)[(nr) / LONG_BITS])


/* We only accept the axes ABS_X to ABS_HAT3Y as real joystick axes.
 * The axes ABS_PRESSURE, ABS_DISTANCE, ABS_TILT_X, ABS_TILT_Y,
 * ABS_TOOL_WIDTH are for use by a drawing tablet.
 * ABS_VOLUME is for volume sliders or key pairs on some keyboards.
 * Other axes up to ABS_MISC may exist, such as on the PS3, but they are
 * most likely not useful.
 */
#define LJOY_AXIS_RANGE_START (ABS_X)
#define LJOY_AXIS_RANGE_END   (ABS_HAT3Y + 1)


/* The range of potential joystick button codes in the Linux input API
 * is from BTN_MISC to BTN_GEAR_UP.
 */
#define LJOY_BTN_RANGE_START  (BTN_MISC)
#if !defined(ALLEGRO_ANDROID) && defined(BTN_TRIGGER_HAPPY)
#define LJOY_BTN_RANGE_END    (BTN_TRIGGER_HAPPY40 + 1)
#else
#define LJOY_BTN_RANGE_END    (BTN_GEAR_UP + 1)
#endif



/* forward declarations */
static bool ljoy_init_joystick(void);
static void ljoy_exit_joystick(void);
static bool ljoy_reconfigure_joysticks(void);
static int ljoy_num_joysticks(void);
static ALLEGRO_JOYSTICK *ljoy_get_joystick(int num);
static void ljoy_release_joystick(ALLEGRO_JOYSTICK *joy_);
static void ljoy_get_joystick_state(ALLEGRO_JOYSTICK *joy_, ALLEGRO_JOYSTICK_STATE *ret_state);
static const char *ljoy_get_name(ALLEGRO_JOYSTICK *joy_);
static bool ljoy_get_active(ALLEGRO_JOYSTICK *joy_);

static void ljoy_process_new_data(void *data);



/* the driver vtable */
ALLEGRO_JOYSTICK_DRIVER _al_joydrv_linux =
{
   _ALLEGRO_JOYDRV_LINUX,
   "",
   "",
   "Linux joystick(s)",
   ljoy_init_joystick,
   ljoy_exit_joystick,
   ljoy_reconfigure_joysticks,
   ljoy_num_joysticks,
   ljoy_get_joystick,
   ljoy_release_joystick,
   ljoy_get_joystick_state,
   ljoy_get_name,
   ljoy_get_active
};


static unsigned num_joysticks;   /* number of joysticks known to the user */
static _AL_VECTOR joysticks;     /* of ALLEGRO_JOYSTICK_LINUX pointers */
static volatile bool config_needs_merging;
static ALLEGRO_MUTEX *config_mutex;
#ifdef SUPPORT_HOTPLUG
static int inotify_fd = -1;
static ALLEGRO_THREAD *hotplug_thread;
static ALLEGRO_MUTEX *hotplug_mutex;
static ALLEGRO_COND *hotplug_cond;
static bool hotplug_ended = false;
#endif



/* Return true if a joystick-related button or key:
 *
 * BTN_MISC to BTN_9 for miscellaneous input,
 * BTN_JOYSTICK to BTN_DEAD for joysticks,
 * BTN_GAMEPAD to BTN_THUMBR for game pads,
 * BTN_WHEEL, BTN_GEAR_DOWN, BTN_GEAR_UP for steering wheels,
 * BTN_TRIGGER_HAPPY_n buttons just in case.
 */
static bool is_joystick_button(int i)
{
   ASSERT(i >= LJOY_BTN_RANGE_START && i < LJOY_BTN_RANGE_END);

   return (i >= BTN_MISC && i <= BTN_9)
      ||  (i >= BTN_JOYSTICK && i <= BTN_DEAD)
      ||  (i >= BTN_GAMEPAD && i <= BTN_THUMBR)
      ||  (i >= BTN_WHEEL && i <= BTN_GEAR_UP)
#if !defined(ALLEGRO_ANDROID) && defined(BTN_TRIGGER_HAPPY)
      ||  (i >= BTN_TRIGGER_HAPPY && i <= BTN_TRIGGER_HAPPY40)
#endif
   ;
}



static bool is_single_axis_throttle(int i)
{
   ASSERT(i >= LJOY_AXIS_RANGE_START && i < LJOY_AXIS_RANGE_END);

   /* Consider all these types of axis as throttle axis. */
   return i == ABS_THROTTLE
      ||  i == ABS_RUDDER
      ||  i == ABS_WHEEL
      ||  i == ABS_GAS
      ||  i == ABS_BRAKE
      ||  i == ABS_PRESSURE
      ||  i == ABS_DISTANCE
      ||  i == ABS_TOOL_WIDTH;
}



static bool is_hat_axis(int i)
{
   return (i >= ABS_HAT0X && i <= ABS_HAT3Y);
}



/* Check if the device has at least once joystick-related button. */
static bool have_joystick_button(int fd)
{
   unsigned long key_bits[NLONGS(KEY_CNT)] = {0};
   int i;

   if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0)
      return false;

   for (i = LJOY_BTN_RANGE_START; i < LJOY_BTN_RANGE_END; i++) {
      if (TEST_BIT(i, key_bits) && is_joystick_button(i))
         return true;
   }

   return false;
}



/* Check if the device has at least one joystick-related absolute axis.
 * Some devices, like (wireless) keyboards, may actually have absolute axes
 * such as a volume axis for the volume up/down buttons.
 * Also, some devices like a PS3 controller report many unusual axes, probably
 * used in nonstandard ways by the PS3 console. All such axes are ignored by
 * this function and by this driver.
 */
static bool have_joystick_axis(int fd)
{
    unsigned long abs_bits[NLONGS(ABS_CNT)] = {0};
    int i;

    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0)
       return false;

    for (i = LJOY_AXIS_RANGE_START; i < LJOY_AXIS_RANGE_END; i++) {
       if (TEST_BIT(i, abs_bits))
          return true;
    }

    return false;
}



static bool ljoy_detect_device_name(int num, ALLEGRO_USTR *device_name)
{
   char key[80];
   const char *value;
   struct stat stbuf;

   al_ustr_truncate(device_name, 0);

   snprintf(key, sizeof(key), "device%d", num);
   value = al_get_config_value(al_get_system_config(), "joystick", key);
   if (value)
      al_ustr_assign_cstr(device_name, value);

   return (stat(al_cstr(device_name), &stbuf) == 0);
}



static ALLEGRO_JOYSTICK_LINUX *ljoy_by_device_name(
   const ALLEGRO_USTR *device_name)
{
   unsigned i;

   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *slot;

      if (joy && al_ustr_equal(device_name, joy->device_name))
         return joy;
   }

   return NULL;
}



static void ljoy_generate_configure_event(void)
{
   ALLEGRO_EVENT event;
   event.joystick.type = ALLEGRO_EVENT_JOYSTICK_CONFIGURATION;
   event.joystick.timestamp = al_get_time();

   _al_generate_joystick_event(&event);
}



static ALLEGRO_JOYSTICK_LINUX *ljoy_allocate_structure(void)
{
   ALLEGRO_JOYSTICK_LINUX **slot;
   ALLEGRO_JOYSTICK_LINUX *joy;
   unsigned i;

   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      slot = _al_vector_ref(&joysticks, i);
      joy = *slot;

      if (joy->config_state == LJOY_STATE_UNUSED)
         return joy;
   }

   joy = al_calloc(1, sizeof *joy);
   slot = _al_vector_alloc_back(&joysticks);
   *slot = joy;
   return joy;
}



static void inactivate_joy(ALLEGRO_JOYSTICK_LINUX *joy)
{
   if (joy->config_state == LJOY_STATE_UNUSED)
      return;
   joy->config_state = LJOY_STATE_UNUSED;

   _al_unix_stop_watching_fd(joy->fd);
   close(joy->fd);
   joy->fd = -1;

   _al_destroy_joystick_info(&joy->parent.info);
   memset(&joy->parent.info, 0, sizeof(joy->parent.info));
   memset(&joy->joystate, 0, sizeof(joy->joystate));

   al_ustr_free(joy->device_name);
   joy->device_name = NULL;
}



static void set_axis_mapping(AXIS_MAPPING *map, _AL_JOYSTICK_OUTPUT output,
   const struct input_absinfo *absinfo)
{
   map->output = output;
   map->min   = absinfo->minimum;
   map->max   = absinfo->maximum;
   map->value = absinfo->value;
   map->fuzz  = absinfo->fuzz;
   map->flat  = absinfo->flat;
}



static bool fill_joystick_axes(ALLEGRO_JOYSTICK_LINUX *joy, int fd)
{
   unsigned long abs_bits[NLONGS(ABS_CNT)] = {0};
   int stick;
   int axis;
   int name_sticks;
   int name_throttles;
   int res, i;

   /* Scan the axes to get their properties. */
   res = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
   if (res < 0)
      return false;

   stick = 0;
   axis = 0;
   name_sticks = 0;
   name_throttles = 0;

   for (i = LJOY_AXIS_RANGE_START; i < LJOY_AXIS_RANGE_END; i++) {
      struct input_absinfo absinfo;

      if (!TEST_BIT(i, abs_bits))
         continue;

      if (ioctl(fd, EVIOCGABS(i), &absinfo) < 0)
         continue;

      if (is_single_axis_throttle(i)) {
         /* One axis throttle. */
         name_throttles++;
         joy->parent.info.stick[stick].flags = ALLEGRO_JOYFLAG_ANALOGUE;
         joy->parent.info.stick[stick].num_axes = 1;
         joy->parent.info.stick[stick].axis[0].name = _al_strdup("X");
         joy->parent.info.stick[stick].name = al_malloc(32);
         snprintf((char *)joy->parent.info.stick[stick].name, 32,
            "Throttle %d", name_throttles);
         set_axis_mapping(&joy->axis_mapping[i], _al_new_joystick_stick_output(stick, 0), &absinfo);
         stick++;
      }
      else {
         _AL_JOYSTICK_OUTPUT output = _al_new_joystick_stick_output(stick, axis);
         /* Regular axis, two axis stick. */
         if (axis == 0) {
            /* First axis of new joystick. */
            name_sticks++;
            if (is_hat_axis(i)) {
               joy->parent.info.stick[stick].flags = ALLEGRO_JOYFLAG_DIGITAL;
            } else {
               joy->parent.info.stick[stick].flags = ALLEGRO_JOYFLAG_ANALOGUE;
            }
            joy->parent.info.stick[stick].num_axes = 2;
            joy->parent.info.stick[stick].axis[0].name = _al_strdup("X");
            joy->parent.info.stick[stick].axis[1].name = _al_strdup("Y");
            joy->parent.info.stick[stick].name = al_malloc(32);
            snprintf((char *)joy->parent.info.stick[stick].name, 32,
               "Stick %d", name_sticks);
            set_axis_mapping(&joy->axis_mapping[i], output, &absinfo);
            axis++;
         }
         else {
            /* Second axis. */
            ASSERT(axis == 1);
            set_axis_mapping(&joy->axis_mapping[i], output, &absinfo);
            stick++;
            axis = 0;
         }
      }
   }

   joy->parent.info.num_sticks = stick;

   return true;
}



static bool fill_joystick_axes_with_map(ALLEGRO_JOYSTICK_LINUX *joy, int fd, const _AL_JOYSTICK_MAPPING *mapping)
{
   unsigned long abs_bits[NLONGS(ABS_CNT)] = {0};
   int i;
   int hat = 0;
   int axis = 0;
   _AL_JOYSTICK_OUTPUT *output;
   int num_hats_mapped = 0;
   int num_axes_mapped = 0;

   /* Scan the axes to get their properties. */
   if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0)
      return false;

   for (i = LJOY_AXIS_RANGE_START; i < LJOY_AXIS_RANGE_END; i++) {
      struct input_absinfo absinfo;

      if (!TEST_BIT(i, abs_bits))
         continue;

      if (ioctl(fd, EVIOCGABS(i), &absinfo) < 0)
         continue;

      if (is_hat_axis(i)) {
         output = _al_get_joystick_output(&mapping->hat_map, hat);
         if (output) {
            set_axis_mapping(&joy->axis_mapping[i], *output, &absinfo);
            num_hats_mapped++;
         }
         hat++;
      }
      else {
         output = _al_get_joystick_output(&mapping->axis_map, axis);
         if (output) {
            set_axis_mapping(&joy->axis_mapping[i], *output, &absinfo);
            num_axes_mapped++;
         }
         axis++;
      }
   }
   if (num_hats_mapped != (int)_al_vector_size(&mapping->hat_map)) {
      ALLEGRO_ERROR("Could not use mapping, some hats are not mapped.\n");
      return false;
   }
   if (num_axes_mapped != (int)_al_vector_size(&mapping->axis_map)) {
      ALLEGRO_ERROR("Could not use mapping, some axes are not mapped.\n");
      return false;
   }
   return true;
}


static bool fill_joystick_buttons(ALLEGRO_JOYSTICK_LINUX *joy, int fd)
{
   unsigned long key_bits[NLONGS(KEY_CNT)] = {0};
   int b;
   int i;

   if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0)
      return false;

   b = 0;

   for (i = LJOY_BTN_RANGE_START; i < LJOY_BTN_RANGE_END; i++) {
      if (TEST_BIT(i, key_bits) && is_joystick_button(i)) {
         joy->button_mapping[b].ev_code = i;
         joy->button_mapping[b].output = _al_new_joystick_button_output(b);
         ALLEGRO_DEBUG("Input event code %d maps to button %d\n", i, b);

         joy->parent.info.button[b].name = al_malloc(32);
         snprintf((char *)joy->parent.info.button[b].name, 32, "B%d", b+1);

         b++;
         if (b == _AL_MAX_JOYSTICK_BUTTONS)
            break;
      }
   }

   joy->parent.info.num_buttons = b;

   /* Clear the rest. */
   for (; b < _AL_MAX_JOYSTICK_BUTTONS; b++) {
      joy->button_mapping[b].ev_code = -1;
   }

   return true;
}



static bool fill_joystick_buttons_with_map(ALLEGRO_JOYSTICK_LINUX *joy, int fd, const _AL_JOYSTICK_MAPPING *mapping)
{
   unsigned long key_bits[NLONGS(KEY_CNT)] = {0};
   int button;
   int i;
   int num_buttons_mapped = 0;

   if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0)
      return false;

   button = 0;

   for (i = LJOY_BTN_RANGE_START; i < LJOY_BTN_RANGE_END; i++) {
      if (TEST_BIT(i, key_bits)) {
         joy->button_mapping[button].ev_code = i;
         _AL_JOYSTICK_OUTPUT *output = _al_get_joystick_output(&mapping->button_map, button);
         if (output) {
            joy->button_mapping[button].output = *output;
            num_buttons_mapped++;
         }
         button++;
      }
   }

   /* Clear the rest. */
   for (; button < _AL_MAX_JOYSTICK_BUTTONS; button++) {
      joy->button_mapping[button].ev_code = -1;
   }

   if (num_buttons_mapped != (int)_al_vector_size(&mapping->button_map)) {
      ALLEGRO_ERROR("Could not use mapping, some buttons are not mapped.\n");
      return false;
   }

   return true;
}



static bool fill_gamepad(ALLEGRO_JOYSTICK_LINUX *joy, int fd) {
   struct input_id id;
   if (ioctl(fd, EVIOCGID, &id) < 0)
      return false;

   ALLEGRO_JOYSTICK_GUID guid = _al_new_joystick_guid(id.bustype, id.vendor, id.product, id.version, NULL, joy->name, 0, 0);
   joy->parent.info.guid = guid;
   const _AL_JOYSTICK_MAPPING *mapping = _al_get_gamepad_mapping("Linux", guid);
   if (!mapping)
      return false;

   if (!fill_joystick_buttons_with_map(joy, fd, mapping))
      return false;

   if (!fill_joystick_axes_with_map(joy, fd, mapping))
      return false;

   _al_fill_gamepad_info(&joy->parent.info);

   memcpy(joy->name, mapping->name, sizeof(mapping->name));
   return true;
}



static void ljoy_device(ALLEGRO_USTR *device_name) {
   ALLEGRO_JOYSTICK_LINUX *joy = ljoy_by_device_name(device_name);
   if (joy) {
      ALLEGRO_DEBUG("Device %s still exists\n", al_cstr(device_name));
      joy->marked = true;
      return;
   }

   /* Try to open the device. The device must be opened in O_RDWR mode to
    * allow writing of haptic effects! The haptic driver for linux
    * reuses the joystick driver's fd.
    */
   int fd = open(al_cstr(device_name), O_RDWR|O_NONBLOCK);
   if (fd == -1) {
      ALLEGRO_WARN("Failed to open device %s\n", al_cstr(device_name));
      return;
   }

   /* The device must have at least one joystick-related axis, and one
    * joystick-related button.  Some devices, such as mouse pads, have ABS_X
    * and ABS_Y axes like a joystick but not joystick-related buttons.  By
    * checking for both axes and buttons, such devices can be excluded.
    */
   if (!have_joystick_button(fd) || !have_joystick_axis(fd)) {
      ALLEGRO_DEBUG("Device %s not a joystick\n", al_cstr(device_name));
      close(fd);
      return;
   }

   ALLEGRO_DEBUG("Device %s is new\n", al_cstr(device_name));

   joy = ljoy_allocate_structure();
   joy->fd = fd;
   joy->device_name = al_ustr_dup(device_name);
   joy->config_state = LJOY_STATE_BORN;
   joy->marked = true;
   config_needs_merging = true;

   if (ioctl(fd, EVIOCGNAME(sizeof(joy->name)), joy->name) < 0)
      strcpy(joy->name, "Unknown");

   /* Map Linux input API axis and button numbers to ours, and fill in
    * information.
    */
   if (!fill_gamepad(joy, fd)) {
      if (!fill_joystick_axes(joy, fd) || !fill_joystick_buttons(joy, fd)) {
         ALLEGRO_ERROR("fill_joystick_info failed %s\n", al_cstr(device_name));
         inactivate_joy(joy);
         close(fd);
         return;
      }
   }

   /* Register the joystick with the fdwatch subsystem.  */
   _al_unix_start_watching_fd(joy->fd, ljoy_process_new_data, joy);
}


static void ljoy_scan(bool configure)
{
   int num;
   ALLEGRO_USTR *device_name;
   const ALLEGRO_FS_INTERFACE *fs_interface;
   unsigned i;
   int t;

   /* Clear mark bits. */
   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **joypp = _al_vector_ref(&joysticks, i);
      (*joypp)->marked = false;
   }

   /* Make sure we're using stdio. */
   fs_interface = al_get_fs_interface();
   al_set_standard_fs_interface();

   device_name = al_ustr_new("");

   /* First try to read devices from allegro.cfg. */
   for (num = 0; num < 32; num++) {
      if (!ljoy_detect_device_name(num, device_name))
         continue;
      ljoy_device(device_name);
   }

   /* Then scan /dev/input/by-path for *-event-joystick devices and if
    * no device is found there scan all files in /dev/input.
    * Note: That last step might be overkill, we probably don't need
    * to support non-evdev kernels any longer.
    */
   static char const *folders[] = {"/dev/input/by-path", "/dev/input"};
   for (t = 0; t < 2; t++) {
      bool found = false;
      ALLEGRO_FS_ENTRY *dir = al_create_fs_entry(folders[t]);
      if (al_open_directory(dir)) {
         static char const *suffix = "-event-joystick";
         while (true) {
            ALLEGRO_FS_ENTRY *dev = al_read_directory(dir);
            if (!dev) {
               break;
            }
            if (al_get_fs_entry_mode(dev) & ALLEGRO_FILEMODE_ISDIR) {
               al_destroy_fs_entry(dev);
               continue;
            }
            char const *path = al_get_fs_entry_name(dev);
            /* In the second pass in /dev/input we don't filter anymore.
               In the first pass, in /dev/input/by-path, strlen(path) > strlen(suffix). */
            if (t == 1 || strcmp(suffix, path + strlen(path) - strlen(suffix)) == 0) {
               found = true;
               al_ustr_assign_cstr(device_name, path);
               ljoy_device(device_name);
            }
            al_destroy_fs_entry(dev);
         }
         al_close_directory(dir);
      }
      al_destroy_fs_entry(dir);
      if (found) {
         /* Don't scan the second folder if we found something in the
          * first as it would be duplicates.
          */
         break;
      }
      ALLEGRO_WARN("Could not find joysticks in %s\n", folders[t]);
   }

   al_ustr_free(device_name);

   /* Restore the fs interface. */
   al_set_fs_interface(fs_interface);

   /* Schedule unmarked structures to be inactivated. */
   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **joypp = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *joypp;

      if (joy->config_state == LJOY_STATE_ALIVE && !joy->marked) {
         ALLEGRO_DEBUG("Device %s to be inactivated\n",
            al_cstr(joy->device_name));
         joy->config_state = LJOY_STATE_DYING;
         config_needs_merging = true;
      }
   }

   /* Generate a configure event if necessary.
    * Even if we generated one before that the user hasn't responded to,
    * we don't know if the user received it so always generate it.
    */
   if (config_needs_merging && configure) {
      ljoy_generate_configure_event();
   }
}



static void ljoy_merge(void)
{
   unsigned i;

   config_needs_merging = false;
   num_joysticks = 0;

   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *slot;

      switch (joy->config_state) {
         case LJOY_STATE_UNUSED:
            break;

         case LJOY_STATE_BORN:
         case LJOY_STATE_ALIVE:
            joy->config_state = LJOY_STATE_ALIVE;
            num_joysticks++;
            break;

         case LJOY_STATE_DYING:
            inactivate_joy(joy);
            break;
      }
   }

   ALLEGRO_DEBUG("Merge done, num_joysticks=%d\n", num_joysticks);
}



#ifdef SUPPORT_HOTPLUG
/* ljoy_config_dev_changed: [fdwatch thread]
 *  Called when the /dev hierarchy changes.
 */
static void ljoy_config_dev_changed(void *data)
{
   char buf[128];
   (void)data;

   /* Empty the event buffer. We only care that some inotify event was sent but it
    * doesn't matter what it is since we are going to do a full scan anyway once
    * the timer_fd fires.
    */
   while (read(inotify_fd, buf, sizeof(buf)) > 0) {
   }

   al_signal_cond(hotplug_cond);
}

static void *hotplug_proc(ALLEGRO_THREAD *thread, void *data)
{
   (void)data;

   while (!al_get_thread_should_stop(thread)) {
      if (!hotplug_ended) {
         al_wait_cond(hotplug_cond, hotplug_mutex);
      }
      if (hotplug_ended) {
         break;
      }

      al_rest(1);

      al_lock_mutex(config_mutex);
      ljoy_scan(true);
      al_unlock_mutex(config_mutex);
   }

   hotplug_ended = false;

   return NULL;
}
#endif



/* ljoy_init_joystick: [primary thread]
 *  Initialise the joystick driver.
 */
static bool ljoy_init_joystick(void)
{
   _al_vector_init(&joysticks, sizeof(ALLEGRO_JOYSTICK_LINUX *));
   num_joysticks = 0;

   if (!(config_mutex = al_create_mutex())) {
      return false;
   }

   // Scan for joysticks
   ljoy_scan(false);
   ljoy_merge();

#ifdef SUPPORT_HOTPLUG
   if (!(hotplug_mutex = al_create_mutex())) {
      al_destroy_mutex(config_mutex);
      return false;
   }
   if (!(hotplug_cond = al_create_cond())) {
      al_destroy_mutex(config_mutex);
      al_destroy_mutex(hotplug_mutex);
      return false;
   }
   if (!(hotplug_thread = al_create_thread(hotplug_proc, NULL))) {
      al_destroy_mutex(config_mutex);
      al_destroy_mutex(hotplug_mutex);
      al_destroy_cond(hotplug_cond);
      return false;
   }

   al_start_thread(hotplug_thread);

   inotify_fd = inotify_init();
   if (inotify_fd != -1) {
      fcntl(inotify_fd, F_SETFL, O_NONBLOCK);
      /* Modern Linux probably only needs to monitor /dev/input. */
      inotify_add_watch(inotify_fd, "/dev/input", IN_CREATE|IN_DELETE);
      _al_unix_start_watching_fd(inotify_fd, ljoy_config_dev_changed, NULL);
      ALLEGRO_INFO("Hotplugging enabled\n");
   }
   else {
      ALLEGRO_WARN("Hotplugging not enabled\n");
      if (inotify_fd != -1) {
         close(inotify_fd);
         inotify_fd = -1;
      }
   }
#endif

   return true;
}



/* ljoy_exit_joystick: [primary thread]
 *  Shut down the joystick driver.
 */
static void ljoy_exit_joystick(void)
{
   int i;

#ifdef SUPPORT_HOTPLUG
   if (inotify_fd != -1) {
      _al_unix_stop_watching_fd(inotify_fd);
      close(inotify_fd);
      inotify_fd = -1;
   }
   hotplug_ended = true;
   al_signal_cond(hotplug_cond);
   al_join_thread(hotplug_thread, NULL);
#endif

   al_destroy_mutex(config_mutex);
   config_mutex = NULL;

   for (i = 0; i < (int)_al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      inactivate_joy(*slot);
      al_free(*slot);
   }
   _al_vector_free(&joysticks);
   num_joysticks = 0;
}



/* ljoy_reconfigure_joysticks: [primary thread]
 */
static bool ljoy_reconfigure_joysticks(void)
{
   bool ret = false;

   al_lock_mutex(config_mutex);

   if (config_needs_merging) {
      ljoy_merge();
      ret = true;
   }

   al_unlock_mutex(config_mutex);

   return ret;
}



/* ljoy_num_joysticks: [primary thread]
 *
 *  Return the number of joysticks available on the system.
 */
static int ljoy_num_joysticks(void)
{
   return num_joysticks;
}



/* ljoy_get_joystick: [primary thread]
 *
 *  Returns the address of a ALLEGRO_JOYSTICK structure for the device
 *  number NUM.
 */
static ALLEGRO_JOYSTICK *ljoy_get_joystick(int num)
{
   ALLEGRO_JOYSTICK *ret = NULL;
   unsigned i;
   ASSERT(num >= 0);

   al_lock_mutex(config_mutex);

   for (i = 0; i < _al_vector_size(&joysticks); i++) {
      ALLEGRO_JOYSTICK_LINUX **slot = _al_vector_ref(&joysticks, i);
      ALLEGRO_JOYSTICK_LINUX *joy = *slot;

      if (ACTIVE_STATE(joy->config_state)) {
         if (num == 0) {
            ret = (ALLEGRO_JOYSTICK *)joy;
            break;
         }
         num--;
      }
   }

   al_unlock_mutex(config_mutex);

   return ret;
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



static bool ljoy_get_active(ALLEGRO_JOYSTICK *joy_)
{
   ALLEGRO_JOYSTICK_LINUX *joy = (ALLEGRO_JOYSTICK_LINUX *)joy_;

   return ACTIVE_STATE(joy->config_state);
}



static bool map_button_number(const ALLEGRO_JOYSTICK_LINUX *joy, int ev_code, _AL_JOYSTICK_OUTPUT *output)
{
   int b;

   for (b = 0; b < _AL_MAX_JOYSTICK_BUTTONS; b++) {
      const BUTTON_MAPPING *m = &joy->button_mapping[b];
      if (m->ev_code == ev_code) {
         *output = m->output;
         return true;
      }
   }

   return false;
}



static float norm_pos(const AXIS_MAPPING *map, float value)
{
   const float range = (float)map->max - (float)map->min;
   return ((value - (float)map->min) / range) * 2.0f - 1.0f;
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
      struct input_event input_events[32];
      int bytes, nr, i;

      while ((bytes = read(joy->fd, &input_events, sizeof input_events)) > 0) {
         nr = bytes / sizeof(struct input_event);

         for (i = 0; i < nr; i++) {
            int type = input_events[i].type;
            int code = input_events[i].code;
            int value = input_events[i].value;

            if (type == EV_KEY) {
               _AL_JOYSTICK_OUTPUT output;
               if (map_button_number(joy, code, &output)) {
                  if (output.button_enabled || output.pos_enabled || output.neg_enabled)
                     _al_joystick_generate_button_event((ALLEGRO_JOYSTICK *)joy, &joy->joystate, output, value);
               }
            }
            else if (type == EV_ABS) {
               int number = code;
               if (number < TOTAL_JOYSTICK_AXES) {
                  const AXIS_MAPPING *map = &joy->axis_mapping[number];
                  _AL_JOYSTICK_OUTPUT output = map->output;
                  float pos = norm_pos(map, value);
                  if (output.button_enabled || output.pos_enabled || output.neg_enabled)
                     _al_joystick_generate_axis_event((ALLEGRO_JOYSTICK *)joy, &joy->joystate, output, pos);
               }
            }
         }
      }
   }
   _al_event_source_unlock(es);
}



#endif /* ALLEGRO_HAVE_LINUX_INPUT_H */



/* vim: set sts=3 sw=3 et: */
