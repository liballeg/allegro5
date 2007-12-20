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
 *      Keyboard input routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Converted into an emulation layer by Peter Wang.
 *
 *      See readme.txt for copyright information.
 *
 *      This compatibility module uses a dedicated background thread
 *      to note when events are emitted by the keyboard. It then
 *      translates those events into the old API. To simulate auto-
 *      repeats it has a timer object. In the future the background
 *      thread will be shared with the mouse API compatibility
 *      module. To keep the code simple, the background thread is
 *      always used even if the user switches to "polling" mode.
 *
 *      We _could_ force the user to use poll_keyboard(), but that
 *      would break a lot of programs. Besides, polling sucks large
 *      nuts.
 *
 *      TODO: LED handling should be done at new-API level, not at
 *      compatibility module level.
 *
 *      TODO: proper locking.
 */


#include "allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"



/* dummy driver for this emulation */
static KEYBOARD_DRIVER keyboard_emu =
{
   AL_ID('K','E','M','U'),
   empty_string,
   empty_string,
   "Old keyboard API emulation"
};


KEYBOARD_DRIVER *keyboard_driver = NULL;     /* the active driver */

int _keyboard_installed = FALSE; 

static int keyboard_polled = FALSE;          /* are we in polling mode? */

volatile char key[KEY_MAX];                  /* key pressed flags */
volatile char _key[KEY_MAX];

volatile int key_shifts = 0;                 /* current shift state */
volatile int _key_shifts = 0;

int (*keyboard_callback)(int key) = NULL;    /* callback functions */
int (*keyboard_ucallback)(int key, int *scancode) = NULL;
void (*keyboard_lowlevel_callback)(int scancode) = NULL;

static int (*keypressed_hook)(void) = NULL;  /* hook functions */
static int (*readkey_hook)(void) = NULL;

static bool allow_repeats = true;


#define KEY_BUFFER_SIZE    64                /* character ring buffer */

typedef struct KEY_BUFFER
{
   volatile int lock;
   volatile int start;
   volatile int end;
   volatile int key[KEY_BUFFER_SIZE];
   volatile unsigned char scancode[KEY_BUFFER_SIZE];
} KEY_BUFFER;

static volatile KEY_BUFFER key_buffer;
static volatile KEY_BUFFER _key_buffer;

static _AL_THREAD cokeybd_thread;
static ALLEGRO_EVENT_QUEUE *cokeybd_event_queue;

/* At the moment the key_buffers_lock is still not done properly, it's
 * just needed to use key_buffers_cond.  And the flawed KEY_BUFFER.lock
 * variables are still used.
 */
static _AL_MUTEX key_buffers_lock = _AL_MUTEX_UNINITED;
static _AL_COND key_buffers_cond;



static void handle_key_press(int keycode, int scancode);
static void handle_key_release(int scancode);



/* add_key:
 *  Helper function to add a keypress to a buffer.
 */
static INLINE void add_key(volatile KEY_BUFFER *buffer, int key, int scancode)
{
   int c, d;

   if (buffer == &key_buffer) {
      if (keyboard_ucallback) {
	 key = keyboard_ucallback(key, &scancode);
	 if ((!key) && (!scancode))
	    return;
      }
      else if (keyboard_callback) {
	 c = ((key <= 0xFF) ? key : '^') | (scancode << 8);
	 d = keyboard_callback(c);

	 if (!d)
	    return;

	 if (d != c) {
	    key = (d & 0xFF);
	    scancode = (d >> 8);
	 }
      }
   }

   buffer->lock++;

   if (buffer->lock != 1) {
      buffer->lock--;
      return;
   }

   if (buffer->end < KEY_BUFFER_SIZE-1)
      c = buffer->end+1;
   else
      c = 0;

   if (c != buffer->start) {
      buffer->key[buffer->end] = key;
      buffer->scancode[buffer->end] = scancode;
      buffer->end = c;
   }

   buffer->lock--;
}



/* clear_keybuf:
 *  Clears the keyboard buffer.
 */
void clear_keybuf()
{
   if (keyboard_polled)
      poll_keyboard();

   key_buffer.lock++;
   _key_buffer.lock++;

   key_buffer.start = key_buffer.end = 0;
   _key_buffer.start = _key_buffer.end = 0;

   key_buffer.lock--;
   _key_buffer.lock--;

   if ((keypressed_hook) && (readkey_hook))
      while (keypressed_hook())
	 readkey_hook();
}



/* clear_key:
 *  Helper function to clear the key[] array.
 */
static void clear_key(void)
{
   int c;

   for (c=0; c<KEY_MAX; c++) {
      key[c] = 0;
      _key[c] = 0;
   }
}



/* keypressed:
 *  Returns TRUE if there are keypresses waiting in the keyboard buffer.
 */
int keypressed()
{
   if (keyboard_polled)
      poll_keyboard();

   if (key_buffer.start == key_buffer.end) {
      if (keypressed_hook)
	 return keypressed_hook();
      else
	 return FALSE;
   }
   else
      return TRUE;
}



/* readkey:
 *  Returns the next character code from the keyboard buffer. If the
 *  buffer is empty, it waits until a key is pressed. The low byte of
 *  the return value contains the ASCII code of the key, and the high
 *  byte the scan code. 
 */
int readkey()
{
   int key, scancode;

   key = ureadkey(&scancode);

   return ((key <= 0xFF) ? key : '^') | (scancode << 8);
}



/* ureadkey:
 *  Returns the next character code from the keyboard buffer. If the
 *  buffer is empty, it waits until a key is pressed. The return value
 *  contains the Unicode value of the key, and the scancode parameter
 *  is set to the scancode.
 */
int ureadkey(int *scancode)
{
   int c;

   if ((!keyboard_driver) && (!readkey_hook)) {
      if (scancode)
	 *scancode = 0;
      return 0;
   }

   if ((readkey_hook) && (key_buffer.start == key_buffer.end)) {
      c = readkey_hook();
      if (scancode)
	 *scancode = (c >> 8);
      return (c & 0xFF);
   }

   while (key_buffer.start == key_buffer.end) {
      _al_mutex_lock(&key_buffers_lock);
      _al_cond_wait(&key_buffers_cond, &key_buffers_lock);
      _al_mutex_unlock(&key_buffers_lock);

      if (keyboard_polled)
	 poll_keyboard();
   }

   c = key_buffer.key[key_buffer.start];

   if (scancode)
      *scancode = key_buffer.scancode[key_buffer.start];

   if (key_buffer.start < KEY_BUFFER_SIZE-1)
      key_buffer.start++;
   else
      key_buffer.start = 0;

   return c;
}



/* simulate_keypress:
 *  Pushes a key into the keyboard buffer, as if it has just been pressed.
 */
void simulate_keypress(int key)
{
   add_key(&key_buffer, key&0xFF, key>>8);
}



/* simulate_ukeypress:
 *  Pushes a key into the keyboard buffer, as if it has just been pressed.
 */
void simulate_ukeypress(int key, int scancode)
{
   add_key(&key_buffer, key, scancode);
}



/* set_leds:
 *  Overrides the state of the keyboard LED indicators.
 *  Set to -1 to return to default behavior.
 */
void set_leds(int leds)
{
   if (leds < 0) {
      key_led_flag = TRUE;
      leds = key_shifts;
   }
   else
      key_led_flag = FALSE;

   al_set_keyboard_leds(leds);
}



/* set_keyboard_rate:
 *  Old behaviour:
 *  "Sets the keyboard repeat rate. Times are given in milliseconds.
 *  Passing zero times will disable the key repeat."
 *
 *  New behaviour: It no longer changes the repeat rate. However, you
 *  can still disable key repeats by passing zero for both arguments.
 */
void set_keyboard_rate(int delay, int repeat)
{
   allow_repeats = (delay || repeat);
}



/* install_keyboard_hooks:
 *  You should only use this function if you *aren't* using the rest of the 
 *  keyboard handler. It can be called in the place of install_keyboard(), 
 *  and lets you provide callback routines to detect and read keypresses, 
 *  which will be used by the main keypressed() and readkey() functions. This 
 *  can be useful if you want to use Allegro's GUI code with a custom 
 *  keyboard handler, as it provides a way for the GUI to access keyboard 
 *  input from your own code.
 */
void install_keyboard_hooks(int (*keypressed)(void), int (*readkey)(void))
{
   key_buffer.lock = _key_buffer.lock = 0;

   clear_keybuf();
   clear_key();

   keypressed_hook = keypressed;
   readkey_hook = readkey;
}



/* This bit is copied straight out of lkeybdnu.c.  It maintains the
 * _key_shifts variable based on the key press and release events we
 * get from the new API.  Unfortunately we can't use the `modifiers'
 * field of the keyboard event because the new API gives a value of
 * zero for that for key releases.  Maybe something to fix in the new
 * API.
 */

static AL_CONST unsigned int modifier_table[ALLEGRO_KEY_MAX - ALLEGRO_KEY_MODIFIERS] =
{
   ALLEGRO_KEYMOD_SHIFT,      ALLEGRO_KEYMOD_SHIFT,   ALLEGRO_KEYMOD_CTRL,
   ALLEGRO_KEYMOD_CTRL,       ALLEGRO_KEYMOD_ALT,     ALLEGRO_KEYMOD_ALTGR,
   ALLEGRO_KEYMOD_LWIN,       ALLEGRO_KEYMOD_RWIN,    ALLEGRO_KEYMOD_MENU,
   ALLEGRO_KEYMOD_SCROLLLOCK, ALLEGRO_KEYMOD_NUMLOCK, ALLEGRO_KEYMOD_CAPSLOCK
};

#define KB_MODIFIERS    (ALLEGRO_KEYMOD_SHIFT | ALLEGRO_KEYMOD_CTRL | ALLEGRO_KEYMOD_ALT | \
                         ALLEGRO_KEYMOD_ALTGR | ALLEGRO_KEYMOD_LWIN | ALLEGRO_KEYMOD_RWIN | \
                         ALLEGRO_KEYMOD_MENU)

#define KB_LED_FLAGS    (ALLEGRO_KEYMOD_SCROLLLOCK | ALLEGRO_KEYMOD_NUMLOCK | \
                         ALLEGRO_KEYMOD_CAPSLOCK)

static INLINE void change__key_shifts(int mycode, bool press)
{
   if (mycode >= ALLEGRO_KEY_MODIFIERS) {
      int flag = modifier_table[mycode - ALLEGRO_KEY_MODIFIERS];
      if (press) {
         if (flag & KB_MODIFIERS)
            _key_shifts |= flag;
         else if ((flag & KB_LED_FLAGS) && _al_key_led_flag)
            _key_shifts ^= flag;
      }
      else {
         /* XXX: If the user presses lctrl, then rctrl, then releases
          * lctrl, the ALLEGRO_KEYMOD_CTRL modifier should still be on.
          */
         if (flag & KB_MODIFIERS)
            _key_shifts &= ~flag;
      }
   }
}



/* update_shifts: [cokeybd thread]
 *  Helper function to update the key_shifts variable and LED state.
 */
static INLINE void update_shifts(void)
{
   #define LED_FLAGS  (KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG)

   if (key_shifts != _key_shifts) {
      if ((key_led_flag) && ((key_shifts ^ _key_shifts) & LED_FLAGS))
         al_set_keyboard_leds(_key_shifts);

      key_shifts = _key_shifts;
   }
}



/* handle_key_press: [cokeybd thread]
 *  Called from the background thread to tell us when a key has been pressed.
 */
static void handle_key_press(int keycode, int scancode)
{
   if (!keyboard_polled) {
      /* process immediately */
      if (scancode >= 0) {
	 key[scancode] = -1;

	 if (keyboard_lowlevel_callback)
	    keyboard_lowlevel_callback(scancode);
      }

      if (keycode >= 0)
	 add_key(&key_buffer, keycode, scancode);

      update_shifts();
   }
   else {
      /* deal with this during the next poll_keyboard() */
      if (scancode >= 0) {
	 _key[scancode] = -1;
      }

      if (keycode >= 0)
	 add_key(&_key_buffer, keycode, scancode);
   }
}



/* handle_key_release: [cokeybd thread]
 *  Called from the background thread to tell us when a key has been released.
 */
static void handle_key_release(int scancode)
{
   if (!keyboard_polled) {
      /* process immediately */
      key[scancode] = 0;

      if (keyboard_lowlevel_callback)
	 keyboard_lowlevel_callback(scancode | 0x80);

      update_shifts();
   }
   else {
      /* deal with this during the next poll_keyboard() */
      _key[scancode] = 0;
   }
}



/* cokeybd_thread_func: [cokeybd thread]
 *  The thread loop function.
 */
static void cokeybd_thread_func(_AL_THREAD *self, void *unused)
{
   ALLEGRO_EVENT event;

   while (!_al_thread_should_stop(self)) {
      /* wait for an event; wait up every so often to check if we
       * should quit
       */
      if (!al_wait_for_event(cokeybd_event_queue, &event, 250))
         continue;

      _al_mutex_lock(&key_buffers_lock);

      switch (event.type) {

         case ALLEGRO_EVENT_KEY_REPEAT:
            if (!allow_repeats)
               break;
            /* FALL THROUGH */

         case ALLEGRO_EVENT_KEY_DOWN: {
            change__key_shifts(event.keyboard.keycode, true);
            if (event.keyboard.keycode >= ALLEGRO_KEY_MODIFIERS) {
               handle_key_press(-1, event.keyboard.keycode);
            }
            else if (_key_shifts & ALLEGRO_KEYMOD_ALT) {
               /* Allegro wants Alt+key to return ASCII code zero */
               handle_key_press(0, event.keyboard.keycode);
            }
            else {
               handle_key_press(event.keyboard.unichar, event.keyboard.keycode);
            }
            _al_cond_signal(&key_buffers_cond);
            break;
         }

         case ALLEGRO_EVENT_KEY_UP:
            change__key_shifts(event.keyboard.keycode, false);
            handle_key_release(event.keyboard.keycode);
            _al_cond_signal(&key_buffers_cond);
            break;

         default:
            /* shouldn't happen */
            TRACE("%s got unknown event of type = %d\n", __func__, event.type);
            break;
      }

      _al_mutex_unlock(&key_buffers_lock);
   }
}



/* poll_keyboard: [primary thread]
 *  Polls the current keyboard state, and updates the user-visible
 *  information accordingly. On some drivers this is actually required
 *  to get the input, while on others it is only present to keep
 *  compatibility with systems that do need it. So that people can test
 *  their polling code even on platforms that don't strictly require it,
 *  after this function has been called once, the entire system will
 *  switch into polling mode and will no longer operate asynchronously
 *  even if the driver actually does support that.
 */
int poll_keyboard()
{
   int i;

   if (!keyboard_driver)
      return -1;

   if (!keyboard_polled) {
      /* switch into polling emulation mode */
      for (i=0; i<KEY_MAX; i++)
	 _key[i] = key[i];

      keyboard_polled = TRUE;
   }
   else {
      /* update the real keyboard variables with stored input */
      for (i=0; i<KEY_MAX; i++) {
	 if (key[i] != _key[i]) {
	    key[i] = _key[i];

	    if (keyboard_lowlevel_callback)
	       keyboard_lowlevel_callback((key[i]) ? i : (i | 0x80));
	 }
      }

      while (_key_buffer.start != _key_buffer.end) {
	 add_key(&key_buffer, _key_buffer.key[_key_buffer.start], 
			      _key_buffer.scancode[_key_buffer.start]);

	 if (_key_buffer.start < KEY_BUFFER_SIZE-1)
	    _key_buffer.start++;
	 else
	    _key_buffer.start = 0;
      }

      update_shifts();
   }

   return 0;
}



/* keyboard_needs_poll: [primary thread]
 *  Checks whether the current driver uses polling.
 */
int keyboard_needs_poll()
{
   return keyboard_polled;
}



/* scancode_to_ascii: [primary thread]
 *  Converts the given scancode to an ASCII character for that key --
 *  that is the unshifted uncapslocked result of pressing the key, or
 *  0 if the key isn't a character-generating key or the lookup can't
 *  be done.
 */
int scancode_to_ascii(int scancode)
{
   /* TODO */

   /*if (keyboard_driver->scancode_to_ascii)
     return keyboard_driver->scancode_to_ascii(scancode);*/

   return 0;
}



/* scancode_to_name:
 *  Converts the given scancode to a description of the key.
 */
AL_CONST char *scancode_to_name(int scancode)
{
   return al_keycode_to_name(scancode);
}



/* install_keyboard: [primary thread]
 *  Installs Allegro's keyboard handler. You must call this before using 
 *  any of the keyboard input routines. Returns -1 on failure.
 */
int install_keyboard()
{
   if (keyboard_driver)
      return 0;

   key_buffer.lock = _key_buffer.lock = 0;

   clear_keybuf();
   clear_key();

   if (!al_install_keyboard())
      return -1;

   ASSERT(!cokeybd_event_queue);
   cokeybd_event_queue = al_create_event_queue();
   if (!cokeybd_event_queue) {
      al_uninstall_keyboard();
      return -1;
   }

   al_register_event_source(cokeybd_event_queue,
                            (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   _al_mutex_init(&key_buffers_lock);
   _al_cond_init(&key_buffers_cond);

   keyboard_driver = &keyboard_emu;

   keyboard_polled = FALSE;

   set_leds(-1);

   _add_exit_func(remove_keyboard, "remove_keyboard");
   _keyboard_installed = TRUE;

   _al_thread_create(&cokeybd_thread, cokeybd_thread_func, NULL);

   return 0;
}



/* remove_keyboard: [primary thread]
 *  Removes the keyboard handler. You don't normally need to call this, 
 *  because allegro_exit() will do it for you.
 */
void remove_keyboard(void)
{
   if (!keyboard_driver)
      return;

   set_leds(-1);

   _al_thread_join(&cokeybd_thread);

   _al_mutex_destroy(&key_buffers_lock);
   _al_cond_destroy(&key_buffers_cond);

   al_uninstall_keyboard();

   al_destroy_event_queue(cokeybd_event_queue);
   cokeybd_event_queue = NULL;

   keyboard_driver = NULL;

   allow_repeats = true;

   _keyboard_installed = FALSE;

   keyboard_polled = FALSE;

   clear_keybuf();
   clear_key();

   key_shifts = _key_shifts = 0;

   _remove_exit_func(remove_keyboard);
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
