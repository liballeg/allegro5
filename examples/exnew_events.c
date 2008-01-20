/*
 *    Example program for the Allegro library, by Peter Wang.
 *    Updated with to conform to new API by Ryan Dickie
 *
 *    This is a very simple program showing how to use the new event
 *    interface introduced in Allegro 4.9.0.
 *
 *    Everything is still subject to change, though the event interface is
 *    unlikely to change very much now.  The new display API is less stable.
 */
#include <allegro5/allegro5.h>
#include <allegro5/font.h>

#define WIDTH 640
#define HEIGHT 480

/* globals */
ALLEGRO_EVENT_QUEUE *event_queue;
ALLEGRO_DISPLAY     *display;
ALLEGRO_TIMER       *timer_a;
ALLEGRO_TIMER       *timer_b;
ALLEGRO_TIMER       *timer_c;
FONT                *myfont;
float          joys_x;
float          joys_y;
ALLEGRO_COLOR black;
ALLEGRO_COLOR white;


/* Print some text.
   borrowed and modified from exnew_bitmap_target.c
*/
static void print(int x, int y, char const *format, ...)
{
   va_list list;
   va_start(list, format);
   char message[1024];
   uvszprintf(message, sizeof message, format, list);
   va_end(list);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &white);
   al_draw_rectangle(x, y, strlen(message)*25, 25, &black, ALLEGRO_FILLED);
   a5font_textout(myfont, message, x, y);
}

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
void log_key_down(int keycode, int unichar, int modifiers)
{
   char buf[512];

   uszprintf(buf, sizeof(buf),
      "Down: %3d '%c' [%08x]", keycode, unichar, modifiers);
   print(5,0,buf);
}



void log_key_repeat(int keycode, int unichar, int modifiers)
{
   char buf[512];

   uszprintf(buf, sizeof(buf),
      "Rept: %3d '%c' [%08x]", keycode, unichar, modifiers);
   print(5,0,buf);
}



void log_key_up(int keycode, int unichar, int modifiers)
{
   char buf[512];

   uszprintf(buf, sizeof(buf),
      "Up: %3d '%c' [%08x]", keycode, unichar, modifiers);
   print(5,0,buf);
}



void draw_timer_tick(ALLEGRO_TIMER *timer, long count)
{
   int y;

   if (timer == timer_a) {
      y = 50;
   } else if (timer == timer_b) {
      y = 70;
   } else if (timer == timer_c) {
      y = 90;
   } else {
      fatal_error("draw_timer_tick");
      y = -1;
   }

   char buf[512];
   uszprintf(buf, sizeof(buf),"Timer: %ld", count);
   print(400, y, buf);
}



void draw_mouse_button(int but, bool down)
{
   const int offset[3] = {0, 70, 35};
   int x;
   int y;
   
   x = 400 + offset[but-1];
   y = 130;

   al_draw_rectangle(x,y,x+25,y+40,(down ? &white : &black), ALLEGRO_FILLED);
   al_draw_rectangle(x,y,x+25,y+40, &white, ALLEGRO_FILLED);
}



void draw_mouse_pos(int x, int y, int z)
{
   char buf[512];
   uszprintf(buf, sizeof(buf),"(%d, %d, %d) ", x, y, z);
   print(400, 180, buf);
}



void draw_joystick_axes(void)
{
   int x;
   int y;

   x = 470 + joys_x * 50;
   y = 300 + joys_y * 50;

   al_draw_rectangle(470-60, 300-60, 470+60, 300+60, &black, ALLEGRO_FILLED);
   al_draw_rectangle(470-60, 300-60, 470+60, 300+60, &white, ALLEGRO_OUTLINED);
   al_draw_rectangle(x-5,y-5,x+5,y+5,&white, ALLEGRO_FILLED); /* this makes up for lack of al_draw_circle */
}



void draw_joystick_button(int button, bool down)
{
   int x;
   int y;

   x = 400 + (button % 5) * 30;
   y = 400 + (button / 5) * 30;

   ALLEGRO_COLOR fill = (down?white:black);
   
   al_draw_rectangle(x, y, x + 25, y + 25, &fill , ALLEGRO_FILLED);
   al_draw_rectangle(x, y, x + 25, y + 25, &white, ALLEGRO_OUTLINED);
}


void draw_all(void)
{
   ALLEGRO_MSESTATE mst;
   
/*   al_clear(&black); */
   /* basically clear the screen minus the keyboard state printing */
   al_draw_rectangle(0,26,WIDTH,HEIGHT,&black, ALLEGRO_FILLED);
   

   al_get_mouse_state(&mst);
   draw_mouse_pos(mst.x, mst.y, mst.z);
   draw_mouse_button(1, al_mouse_button_down(&mst, 1));
   draw_mouse_button(2, al_mouse_button_down(&mst, 2));
   draw_mouse_button(3, al_mouse_button_down(&mst, 3));

   draw_timer_tick(timer_a, al_timer_get_count(timer_a));
   draw_timer_tick(timer_b, al_timer_get_count(timer_b));
   draw_timer_tick(timer_c, al_timer_get_count(timer_c));

   al_flip_display();
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
   ALLEGRO_EVENT event;

   while (true) {
      draw_all();
      /* Take the next event out of the event queue, and store it in `event'.
       * The third parameter is a time-out specification, allowing the function
       * to return even if no event arrives within the time period specified.
       * Zero represents infinite timeout.
       */
      al_wait_for_event(event_queue, &event, ALLEGRO_WAIT_FOREVER);

      /* Check what type of event we got and act accordingly.  ALLEGRO_EVENT is a
       * union type and interpretation of its contents is dependent on the
       * event type, which is given by the 'type' field.
       *
       * Each event also comes from an event source and has a timestamp.
       * These are accessible through the 'any.source' and 'any.timestamp'
       * fields respectively, e.g. 'event.any.timestamp'
       */
      switch (event.type) {

         /* ALLEGRO_EVENT_KEY_DOWN - a keyboard key was pressed.
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
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               return;
            }
            log_key_down(event.keyboard.keycode,
                  event.keyboard.unichar,
                  event.keyboard.modifiers);
            break;

         /* ALLEGRO_EVENT_KEY_REPEAT - a keyboard key was held down long enough to
          * 'repeat'.  This is a useful event if you are working on something
          * that requires typed input.  The repeat rate should be determined
          * by the operating environment the program is running in.
          */
         case ALLEGRO_EVENT_KEY_REPEAT:
            log_key_repeat(event.keyboard.keycode,
                  event.keyboard.unichar,
                  event.keyboard.modifiers);
            break;

         /* ALLEGRO_EVENT_KEY_UP - a keyboard key was released.
          * Note that the unichar field is unused for this event.
          */
         case ALLEGRO_EVENT_KEY_UP:
            log_key_up(event.keyboard.keycode,
                  event.keyboard.unichar,
                  event.keyboard.modifiers);
            break;

         /* ALLEGRO_EVENT_MOUSE_AXES - at least one mouse axis changed value.
          * The 'z' axis is for the scroll wheel.  We also have a fourth 'w'
          * axis for mice with two scroll wheels.
          */
         case ALLEGRO_EVENT_MOUSE_AXES:
            draw_mouse_pos(event.mouse.x, event.mouse.y, event.mouse.z);
            break;

         /* ALLEGRO_EVENT_MOUSE_BUTTON_UP - a mouse button was pressed. 
          * The axis fields are also valid for this event.
          */
         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            draw_mouse_button(event.mouse.button, true);
            break;

         /* ALLEGRO_EVENT_MOUSE_BUTTON_UP - a mouse button was released.
          * The axis fields are also valid for this event.
          */
         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            draw_mouse_button(event.mouse.button, false);
            break;

         /* ALLEGRO_EVENT_TIMER - a timer 'ticked'.
          * The `source' field in the event structure tells us which timer
          * went off, and the `count' field tells us the timer's counter
          * value at the time that the event was generated.  It's not
          * redundant, because although you can query the timer for its
          * counter value, that value might have changed by the time you got
          * around to processing this event.
          */
         case ALLEGRO_EVENT_TIMER:
            draw_timer_tick(event.timer.source, event.timer.count);
            break;

         /* ALLEGRO_EVENT_JOYSTICK_AXIS - a joystick axis value changed.
          * For simplicity, in this example we only work with the first
          * 'stick' on the first joystick on the system.
          */
         case ALLEGRO_EVENT_JOYSTICK_AXIS:
            if (event.joystick.source == al_get_joystick(0) &&
               event.joystick.stick == 0)
            {
               switch (event.joystick.axis) {
                  case 0:
                     joys_x = event.joystick.pos;
                     break;
                  case 1:
                     joys_y = event.joystick.pos;
                     break;
               }
               draw_joystick_axes();
            }
            break;

         /* ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN - a joystick button was pressed.
          */
         case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
            if (event.joystick.source == al_get_joystick(0)) {
               draw_joystick_button(event.joystick.button, true);
            }
            break;

         /* ALLEGRO_EVENT_JOYSTICK_BUTTON_UP - a joystick button was released.
          */
         case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
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

   /* Initialise Allegro. */
   al_init();

   /* Open a window. This function is part of the new display API and is
    * still in flux.
    */
   display = al_create_display(WIDTH, HEIGHT);
   if (!display)
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

   myfont = a5font_load_font("font.tga", 0);
   al_map_rgb(&black, 0, 0, 0);
   al_map_rgb(&white, 255,255,255);

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
    * The type-casts to ALLEGRO_EVENT_SOURCE* are unfortunately required.
    */
   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)timer_a);
   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)timer_b);
   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)timer_c);

   /* Register all the joysticks on the system with the event queue as well. */
   num = al_num_joysticks();
   for (i = 0; i < num; i++) {
      ALLEGRO_EVENT_SOURCE *joysrc = (ALLEGRO_EVENT_SOURCE *)al_get_joystick(i);
      al_register_event_source(event_queue, joysrc);
   }

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
} END_OF_MAIN()

/* vi: set ts=8 sts=3 sw=3 et: */
