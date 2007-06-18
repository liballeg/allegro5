/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This is a very simple program showing how to use the new event
 *    interface introduced in Allegro 4.9.0.  The graphics are mainly done
 *    using the Allegro 4.2 interface, although a new display API is also
 *    used.
 *
 *    Everything is still subject to change, though the event interface is
 *    unlikely to change very much now.  The new display API is less stable.
 */


#include <stdio.h>

#define ALLEGRO_NO_COMPATIBILITY
#include <allegro.h>


/* globals */
AL_EVENT_QUEUE *event_queue;
AL_TIMER       *timer_a;
AL_TIMER       *timer_b;
AL_TIMER       *timer_c;
float          joy_x;
float          joy_y;



/* Display an error message and quit. */
void fatal_error(const char *msg)
{
   char buf[128];

   set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
   uszprintf(buf, sizeof(buf), "fatal_error: %s\n", msg);
   allegro_message(buf);
   exit(EXIT_FAILURE);
}



/*
 * The following functions deal with drawing some representation of the
 * keyboard, mouse, joystick and timer events on screen.
 * The interesting stuff doesn't really begin until main_loop(), below.
 */



int black(void)
{
   return makecol(0, 0, 0);
}



int white(void)
{
   return makecol(255, 255, 255);
}



void log_text(const char *msg)
{
   BITMAP *buffer;
   BITMAP *sub;
   int th;
   int y;

   buffer = al_get_buffer(al_main_display);
   sub = create_sub_bitmap(buffer, 50, 50, 300, 250);
   if (!sub) {
      fatal_error("create_sub_bitmap");
   }

   th = text_height(font);
   y = sub->h - th;
   blit(sub, sub, 0, th, 0, 0, sub->w, y);
   rectfill(sub, 0, y, sub->w, sub->h, white());
   textout_ex(sub, font, msg, 0, y, black(), white());

   destroy_bitmap(sub);
   al_flip_display(al_main_display);
}



void log_key_down(int keycode, int unichar, int modifiers)
{
   char buf[512];

   uszprintf(buf, sizeof(buf),
      "Down: %3d '%c' [%08x]", keycode, unichar, modifiers);
   log_text(buf);
}



void log_key_repeat(int keycode, int unichar, int modifiers)
{
   char buf[512];

   uszprintf(buf, sizeof(buf),
      "Rept: %3d '%c' [%08x]", keycode, unichar, modifiers);
   log_text(buf);
}



void log_key_up(int keycode, int modifiers)
{
   char buf[512];

   uszprintf(buf, sizeof(buf),
      "Up:   %3d     [%08x]", keycode, modifiers);
   log_text(buf);
}



void draw_timer_tick(AL_TIMER *timer, long count)
{
   BITMAP *buffer;
   int y;

   if (timer == timer_a) {
      y = 50;
   } else if (timer == timer_b) {
      y = 60;
   } else if (timer == timer_c) {
      y = 70;
   } else {
      fatal_error("draw_timer_tick");
      y = -1;
   }

   buffer = al_get_buffer(al_main_display);
   textprintf_ex(buffer, font, 400, y, black(), white(), "Timer: %ld", count);
   al_flip_display(al_main_display);
}



void draw_mouse_button(int but, bool down)
{
   const int offset[3] = {0, 70, 35};
   BITMAP *buffer;
   int x;
   int y;
   int fill_colour;

   buffer = al_get_buffer(al_main_display);
   x = 400 + offset[but-1];
   y = 130;
   fill_colour = down ? black() : white();

   rectfill(buffer, x, y, x+25, y+40, fill_colour);
   rect(buffer, x, y, x+25, y+40, black());
   al_flip_display(al_main_display);
}



void draw_mouse_pos(int x, int y, int z)
{
   BITMAP *buffer;

   buffer = al_get_buffer(al_main_display);
   textprintf_ex(buffer, font, 400, 180, black(), white(),
      "(%d, %d, %d) ", x, y, z);
   al_flip_display(al_main_display);
}



void draw_joystick_axes(void)
{
   BITMAP *buffer;
   int x;
   int y;

   x = 470 + joy_x * 50;
   y = 300 + joy_y * 50;

   buffer = al_get_buffer(al_main_display);
   rectfill(buffer, 470-60, 300-60, 470+60, 300+60, white());
   rect(buffer, 470-60, 300-60, 470+60, 300+60, black());
   circle(buffer, x, y, 10, black());
   al_flip_display(al_main_display);
}



void draw_joystick_button(int button, bool down)
{
   BITMAP *buffer;
   int x;
   int y;
   int fill_colour;

   x = 400 + (button % 5) * 30;
   y = 400 + (button / 5) * 30;
   fill_colour = down ? black() : white();

   buffer = al_get_buffer(al_main_display);
   rectfill(buffer, x, y, x + 25, y + 25, fill_colour);
   rect(buffer, x, y, x + 25, y + 25, black());
   al_flip_display(al_main_display);
}



void draw_all(void)
{
   BITMAP *buffer;
   AL_MSESTATE mst;

   buffer = al_get_buffer(al_main_display);
   clear_to_color(buffer, white());

   al_get_mouse_state(&mst);
   draw_mouse_pos(mst.x, mst.y, mst.z);
   draw_mouse_button(1, al_mouse_button_down(&mst, 1));
   draw_mouse_button(2, al_mouse_button_down(&mst, 2));
   draw_mouse_button(3, al_mouse_button_down(&mst, 3));

   draw_timer_tick(timer_a, al_timer_get_count(timer_a));
   draw_timer_tick(timer_b, al_timer_get_count(timer_b));
   draw_timer_tick(timer_c, al_timer_get_count(timer_c));
}



/* main_loop:
 *  The main loop of the program.  Here we wait for events to come in from
 *  any one of the event sources and react to each one accordingly.  While
 *  there are no events to react to the program sleeps and consumes very
 *  little CPU time.  See main() to see how the event sources and event queue
 *  are set up.
 */
void main_loop(void)
{
   AL_EVENT event;

   while (true) {

      /* Take the next event out of the event queue, and store it in `event'.
       * The third parameter is a time-out specification, allowing the function
       * to return even if no event arrives within the time period specified.
       * Zero represents infinite timeout.
       */
      al_wait_for_event(event_queue, &event, AL_WAIT_FOREVER);

      /* Check what type of event we got and act accordingly.  AL_EVENT is a
       * union type and interpretation of its contents is dependent on the
       * event type, which is given by the 'type' field.
       *
       * Each event also comes from an event source and has a timestamp.
       * These are accessible through the 'any.source' and 'any.timestamp'
       * fields respectively, e.g. 'event.any.timestamp'
       */
      switch (event.type) {

         /* AL_EVENT_KEY_DOWN - a keyboard key was pressed.
          * The three keyboard event fields we use here are:
          *
          * keycode -- an integer constant representing the key, e.g.
          *             AL_KEY_ESCAPE;
          *
          * unichar -- the Unicode character being typed, if any.  This can
          *             depend on the modifier keys and previous keys that were
          *             pressed, e.g. for accents.
          *
          * modifiers -- a bitmask containing the state of Shift/Ctrl/Alt, etc.
          *             keys.
          */
         case AL_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == AL_KEY_ESCAPE) {
               return;
            }
            log_key_down(event.keyboard.keycode,
                  event.keyboard.unichar,
                  event.keyboard.modifiers);
            break;

         /* AL_EVENT_KEY_REPEAT - a keyboard key was held down long enough to
          * 'repeat'.  This is a useful event if you are working on something
          * that requires typed input.  The repeat rate should be determined
          * by the operating environment the program is running in.
          */
         case AL_EVENT_KEY_REPEAT:
            log_key_repeat(event.keyboard.keycode,
                  event.keyboard.unichar,
                  event.keyboard.modifiers);
            break;

         /* AL_EVENT_KEY_UP - a keyboard key was released.
          * Note that the unichar field is unused for this event.
          */
         case AL_EVENT_KEY_UP:
            log_key_up(event.keyboard.keycode,
                  event.keyboard.modifiers);
            break;

         /* AL_EVENT_MOUSE_AXES - at least one mouse axis changed value.
          * The 'z' axis is for the scroll wheel.  We also have a fourth 'w'
          * axis for mice with two scroll wheels.
          */
         case AL_EVENT_MOUSE_AXES:
            draw_mouse_pos(event.mouse.x, event.mouse.y, event.mouse.z);
            break;

         /* AL_EVENT_MOUSE_BUTTON_UP - a mouse button was pressed. 
          * The axis fields are also valid for this event.
          */
         case AL_EVENT_MOUSE_BUTTON_DOWN:
            draw_mouse_button(event.mouse.button, true);
            break;

         /* AL_EVENT_MOUSE_BUTTON_UP - a mouse button was released.
          * The axis fields are also valid for this event.
          */
         case AL_EVENT_MOUSE_BUTTON_UP:
            draw_mouse_button(event.mouse.button, false);
            break;

         /* AL_EVENT_TIMER - a timer 'ticked'.
          * The `source' field in the event structure tells us which timer
          * went off, and the `count' field tells us the timer's counter
          * value at the time that the event was generated.  It's not
          * redundant, because although you can query the timer for its
          * counter value, that value might have changed by the time you got
          * around to processing this event.
          */
         case AL_EVENT_TIMER:
            draw_timer_tick(event.timer.source, event.timer.count);
            break;

         /* AL_EVENT_JOYSTICK_AXIS - a joystick axis value changed.
          * For simplicity, in this example we only work with the first
          * 'stick' on the first joystick on the system.
          */
         case AL_EVENT_JOYSTICK_AXIS:
            if (event.joystick.source == al_get_joystick(0) &&
               event.joystick.stick == 0)
            {
               switch (event.joystick.axis) {
                  case 0:
                     joy_x = event.joystick.pos;
                     break;
                  case 1:
                     joy_y = event.joystick.pos;
                     break;
               }
               draw_joystick_axes();
            }
            break;

         /* AL_EVENT_JOYSTICK_BUTTON_DOWN - a joystick button was pressed.
          */
         case AL_EVENT_JOYSTICK_BUTTON_DOWN:
            if (event.joystick.source == al_get_joystick(0)) {
               draw_joystick_button(event.joystick.button, true);
            }
            break;

         /* AL_EVENT_JOYSTICK_BUTTON_UP - a joystick button was released.
          */
         case AL_EVENT_JOYSTICK_BUTTON_UP:
            if (event.joystick.source == al_get_joystick(0)) {
               draw_joystick_button(event.joystick.button, false);
            }
            break;

         /* We received an event of some type we don't know about.
          * Just ignore it.
          */
         default:
            break;
      }
   }
}



int main(void)
{
   int num;
   int i;

   /* Initialise Allegro. We haven't changed the way this is done yet. */
   allegro_init();

   /* Open a window. This function is part of the new display API and is
    * still in flux.
    */
   if (!al_create_display(GFX_AUTODETECT_WINDOWED, AL_UPDATE_DOUBLE_BUFFER,
         AL_DEPTH_32, 640, 480))
   {
      fatal_error("al_create_display");
   }

   /* Install the keyboard handler.  In the new API, the install functions
    * return 'false' on error instead of 0.
    */
   if (!al_install_keyboard()) {
      fatal_error("al_install_keyboard");
   }

   /* Install the mouse handler. */
   if (!al_install_mouse()) {
      fatal_error("al_install_mouse");
   }

   /* Show the mouse cursor.  Currently this relies on the video card or
    * operating environment being able to overlay a mouse cursor on top of
    * the screen (or Allegro window).  If that is not possible then the user
    * would be responsible for drawing the mouse cursor himself.
    */
   al_show_mouse_cursor();

   /* Install the joystick routines. */
   al_install_joystick();

   /* Create three timer objects.  Unlike in the earlier API, these
    * automatically increment a counter every n milliseconds.  They no longer
    * run user-defined callbacks.
    */
   timer_a = al_install_timer(100);
   timer_b = al_install_timer(1000);
   timer_c = al_install_timer(5000);
   if (!timer_a || !timer_b || !timer_c) {
      fatal_error("al_install_timer");
   }

   /* Create an event queue. These collect events which are generated by
    * event sources.  In this example we demonstrate four types of event
    * sources: the keyboard, the mouse, joysticks and timers.
    */
   event_queue = al_create_event_queue();
   if (!event_queue) {
      fatal_error("al_create_event_queue");
   }

   /* To tell event sources where to deliver any events that they generate, we
    * 'register' event sources with event queues.  An event source can register
    * itself with multiple event queues at the same time.  An event generated
    * by such an event source would be delivered to each of those event queues.
    * Most of the time, you wouldn't need that feature.
    *
    * al_get_keyboard() and al_get_mouse() return objects which represent the
    * keyboard and mouse respectively.  For simplicity, the API assumes there
    * is only one of each on the system.  The timer objects were created
    * earlier with al_install_timer().
    *
    * The type-casts to AL_EVENT_SOURCE* are unfortunately required.
    */
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)timer_a);
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)timer_b);
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)timer_c);

   /* Register all the joysticks on the system with the event queue as well. */
   num = al_num_joysticks();
   for (i = 0; i < num; i++) {
      AL_EVENT_SOURCE *joysrc = (AL_EVENT_SOURCE *)al_get_joystick(i);
      al_register_event_source(event_queue, joysrc);
   }

   draw_all();

   /* Timers are not automatically started when they are created.  Start them
    * now before we enter the main loop.
    */
   al_start_timer(timer_a);
   al_start_timer(timer_b);
   al_start_timer(timer_c);

   /* Run the main loop. */
   main_loop();

   /* For now, Allegro still automatically registers allegro_exit() with
    * atexit() when you install it.  Further, all subsystems (keyboard, etc.)
    * are automatically shut down when Allegro is shut down.  All event queues
    * are also automatically destroyed.  So we don't actually need any
    * explicit shutdown code here.
    */

   return 0;
}

END_OF_MAIN()

/* vi: set ts=8 sts=3 sw=3 et: */
