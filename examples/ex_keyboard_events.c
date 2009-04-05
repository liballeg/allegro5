/*
 *    Example program for the Allegro library, by Peter Wang.
 *    Updated by Ryan Dickie.
 *
 *    This program tests keyboard events.
 */

#include <stdio.h>
#include <allegro5/allegro5.h>
#include <allegro5/a5_iio.h>
#include <allegro5/a5_font.h>


#define WIDTH        640
#define HEIGHT       480
#define MAX_MSG_LEN  80
#define SIZE_LOG     40


/* globals */
ALLEGRO_EVENT_QUEUE *event_queue;
ALLEGRO_DISPLAY     *display;
ALLEGRO_FONT         *myfont;
ALLEGRO_COLOR        black;
ALLEGRO_COLOR        white;

/* circular array of log messages */
static char msg_log[SIZE_LOG][MAX_MSG_LEN];
static int msg_head = 0;
static int msg_tail = 0;



/* Add a message to the log. */
static void log_message(char const *message)
{
   ustrzcpy(msg_log[msg_head], MAX_MSG_LEN, message);
   msg_head = (msg_head + 1) % SIZE_LOG;
   if (msg_head == msg_tail) {
      msg_tail = (msg_tail + 1) % SIZE_LOG;
   }
}



/* Draw the message log to the screen. */
static int num_messages(void)
{
   if (msg_head < msg_tail) {
      return (SIZE_LOG - msg_tail) + msg_head;
   }
   else {
      return msg_head - msg_tail;
   }
}

static void draw_message_log(void)
{
   const int th = al_get_font_line_height(myfont);
   int y;
   int i;

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, black);

   /* Scroll down the log if necessary. */
   while (num_messages() >= (HEIGHT/th)) {
      msg_tail = (msg_tail + 1) % SIZE_LOG;
   }

   y = 0;
   i = msg_tail;
   while (1) {
      al_draw_text(myfont, 5, y, 0, msg_log[i], 0, 0);
      y += th;

      i = (i + 1) % SIZE_LOG;
      if (i == msg_head) {
         break;
      }
   }

   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, white);
}



void log_key_down(int keycode, int unichar, int modifiers)
{
   char buf[MAX_MSG_LEN];
   char unistr[10] = "";

   usetat(unistr, 0, unichar);
   uszprintf(buf, sizeof(buf),
      "Down: %3d <%1s> %08x [%08x]", keycode, unistr, unichar, modifiers);
   log_message(buf);
}



void log_key_repeat(int keycode, int unichar, int modifiers)
{
   char buf[MAX_MSG_LEN];
   char unistr[10] = "";

   usetat(unistr, 0, unichar);
   uszprintf(buf, sizeof(buf),
      "Rept: %3d <%1s> %08x [%08x]", keycode, unistr, unichar, modifiers);
   log_message(buf);
}



void log_key_up(int keycode, int unichar, int modifiers)
{
   char buf[MAX_MSG_LEN];
   char unistr[10] = "";

   usetat(unistr, 0, unichar);
   uszprintf(buf, sizeof(buf),
      "Up:   %3d <%1s> %08x [%08x]", keycode, unistr, unichar, modifiers);
   log_message(buf);
}



void log_general(char const *format, ...)
{
   char buf[MAX_MSG_LEN];
   va_list args;
   va_start(args, format);

   uvszprintf(buf, sizeof(buf), format, args);
   va_end(args);

   log_message(buf);
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
      if (al_event_queue_is_empty(event_queue)) {
         al_clear(white);
         draw_message_log();
         al_flip_display();
      }

      /* Take the next event out of the event queue, and store it in `event'. */
      al_wait_for_event(event_queue, &event);

      /* Check what type of event we got and act accordingly.  ALLEGRO_EVENT
       * is a union type and interpretation of its contents is dependent on
       * the event type, which is given by the 'type' field.
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

         /* ALLEGRO_EVENT_DISPLAY_CLOSE - the window close button was pressed.
          */
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return;

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
   ALLEGRO_BITMAP *a4font;
   int ranges[] = {
       0x0020, 0x007F,  /* ASCII */
       0x00A1, 0x00FF,  /* Latin 1 */
       0x0100, 0x017F,  /* Extended-A */
       0x20AC, 0x20AC}; /* Euro */

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_init_font_addon();

   display = al_create_display(WIDTH, HEIGHT);
   if (!display) {
      TRACE("al_create_display failed\n");
      return 1;
   }

   if (!al_install_keyboard()) {
      TRACE("al_install_keyboard failed\n");
      return 1;
   }

   /* Unlike data/fixed_font.tga this contains some non-ASCII characters. */
   printf("Loading a4_font.tga, might be a bit slow...\n");
   a4font = al_iio_load("data/a4_font.tga");
   if (!a4font) {
      TRACE("Failed to load a4_font.tga\n");
      return 1;
   }

   myfont = al_grab_font_from_bitmap(a4font, 4, ranges);
   black = al_map_rgb(0, 0, 0);
   white = al_map_rgb(255, 255, 255);

   event_queue = al_create_event_queue();
   if (!event_queue) {
      TRACE("al_create_event_queue failed\n");
      return 1;
   }

   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(event_queue, (ALLEGRO_EVENT_SOURCE *)display);

   main_loop();

   return 0;
}
END_OF_MAIN()

/* vim: set ts=8 sts=3 sw=3 et: */
