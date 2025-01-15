/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This program tests joystick events.
 */

#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

#define MAX_AXES     3
#define MAX_STICKS   16
#define MAX_BUTTONS  32


/* globals */
ALLEGRO_EVENT_QUEUE  *event_queue;
ALLEGRO_FONT         *font;
ALLEGRO_COLOR        black;
ALLEGRO_COLOR        grey;
ALLEGRO_COLOR        blue;
ALLEGRO_COLOR        white;

int num_sticks = 0;
int num_buttons = 0;
char *guid_str = NULL;
int num_axes[MAX_STICKS] = { 0 };


static char *guid_to_str(ALLEGRO_JOYSTICK_GUID guid)
{
   char *ret = al_malloc(33);
   char *str = ret;
   const char chars[] = "0123456789abcdef";
   for (int i = 0; i < (int)sizeof(guid.val); i++) {
      *str++ = chars[guid.val[i] >> 4];
      *str++ = chars[guid.val[i] & 0xf];
   }
   *str = '\0';
   return ret;
}


static void setup_joystick_values(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *state)
{
   int i;
   memset(state, 0, sizeof(ALLEGRO_JOYSTICK_STATE));
   if (joy == NULL) {
      num_sticks = 0;
      num_buttons = 0;
      return;
   }

   al_get_joystick_state(joy, state);
   num_sticks = al_get_joystick_num_sticks(joy);
   num_buttons = al_get_joystick_num_buttons(joy);
   for (i = 0; i < num_sticks; i++) {
      num_axes[i] = al_get_joystick_num_axes(joy, i);
   }
}


static void setup_joystick_all_values(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *state1, ALLEGRO_JOYSTICK_STATE *state2)
{
   if (joy) {
      al_free(guid_str);
      guid_str = guid_to_str(al_get_joystick_guid(al_get_joystick(0)));
      log_printf("Joystick GUID: %s\n", guid_str);
   }
   setup_joystick_values(joy, state1);
   setup_joystick_values(joy, state2);
}



static void draw_joystick_axes(ALLEGRO_JOYSTICK *joy, int cx, int cy, int stick, bool first, ALLEGRO_JOYSTICK_STATE *state)
{
   const int size = 30;
   const int csize = 5;
   const int osize = size + csize;
   int rsize = first ? 5 : 3;
   ALLEGRO_COLOR rcolor = first ? black : blue;
   int zx = cx + osize + csize * 2;
   int x = cx + state->stick[stick].axis[0] * size;
   int y = cy + state->stick[stick].axis[1] * size;
   int z = cy + state->stick[stick].axis[2] * size;
   int i;

   if (first) {
      al_draw_filled_rectangle(cx-osize, cy-osize, cx+osize, cy+osize, grey);
      al_draw_rectangle(cx-osize+0.5, cy-osize+0.5, cx+osize-0.5, cy+osize-0.5, black, 0);
   }
   al_draw_filled_rectangle(x-rsize, y-rsize, x+rsize, y+rsize, rcolor);

   if (num_axes[stick] >= 3) {
      if (first) {
         al_draw_filled_rectangle(zx-csize, cy-osize, zx+csize, cy+osize, grey);
         al_draw_rectangle(zx-csize+0.5f, cy-osize+0.5f, zx+csize-0.5f, cy+osize-0.5f, black, 0);
      }
      al_draw_filled_rectangle(zx-rsize, z-rsize, zx+rsize, z+rsize, rcolor);
   }

   if (joy && first) {
      al_draw_text(font, black, cx + csize + osize + 16, cy - size, ALLEGRO_ALIGN_LEFT,
         al_get_joystick_stick_name(joy, stick));
      for (i = 0; i < num_axes[stick]; i++) {
         al_draw_text(font, black, cx + csize + osize + 16, cy - size + 20 + i * 10,
            ALLEGRO_ALIGN_LEFT,
            al_get_joystick_axis_name(joy, stick, i));
      }
   }
}



static void draw_joystick_button(ALLEGRO_JOYSTICK *joy, int button, bool first, bool down)
{
   ALLEGRO_BITMAP *bmp = al_get_target_bitmap();
   int x = al_get_bitmap_width(bmp) / 2;
   int y = 60 + button * 30;
   int rsize = first ? 12 : 10;
   ALLEGRO_COLOR rcolor = first ? black : blue;

   if (first)
      al_draw_rectangle(x+0.5, y+0.5, x + 24.5, y + 24.5, black, 0);
   if (down)
      al_draw_filled_rectangle(x + 12 - rsize, y + 12 - rsize, x + 13 + rsize, y + 13 + rsize, rcolor);

   if (joy && first)
      al_draw_text(font, black, x + 33, y + 8, ALLEGRO_ALIGN_LEFT, al_get_joystick_button_name(joy, button));
}



static void draw_all(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *state1, ALLEGRO_JOYSTICK_STATE *state2)
{
   int i;

   al_clear_to_color(al_map_rgb(0xff, 0xff, 0xc0));

   if (joy) {
      al_draw_textf(font, black, 10, 10, ALLEGRO_ALIGN_LEFT,
         "Name: %s", al_get_joystick_name(joy));
      const char* type;
      switch (al_get_joystick_type(joy)) {
         case ALLEGRO_JOYSTICK_TYPE_GAMEPAD:
            type = "Gamepad";
            break;
         default:
            type = "Unknown";
      }
      al_draw_textf(font, black, 10, 20, ALLEGRO_ALIGN_LEFT,
         "GUID: %s", guid_str);
      al_draw_textf(font, black, 10, 30, ALLEGRO_ALIGN_LEFT,
         "Type: %s", type);
   }

   for (i = 0; i < num_sticks; i++) {
      int cx = 60;
      int cy = 100 + i * 90;
      draw_joystick_axes(joy, cx, cy, i, true, state1);
      draw_joystick_axes(joy, cx, cy, i, false, state2);
   }

   for (i = 0; i < num_buttons; i++) {
      draw_joystick_button(joy, i, true, state1->button[i] > 16384);
      draw_joystick_button(joy, i, false, state2->button[i] > 16384);
   }

   al_flip_display();
}



static void main_loop(void)
{
   ALLEGRO_JOYSTICK_STATE state1, state2;
   ALLEGRO_JOYSTICK *joy = al_get_joystick(0);
   setup_joystick_all_values(joy, &state1, &state2);
   ALLEGRO_EVENT event;

   while (true) {
      if (al_is_event_queue_empty(event_queue)) {
         setup_joystick_values(joy, &state2);
         draw_all(joy, &state1, &state2);
      }

      al_wait_for_event(event_queue, &event);

      switch (event.type) {
         /* ALLEGRO_EVENT_JOYSTICK_AXIS - a joystick axis value changed.
          */
         case ALLEGRO_EVENT_JOYSTICK_AXIS:
            if (event.joystick.id != joy)
               break;
            log_printf("stick: %d axis: %d pos: %f\n",
                       event.joystick.stick, event.joystick.axis, event.joystick.pos);
            if (event.joystick.stick < MAX_STICKS && event.joystick.axis < MAX_AXES) {
               state1.stick[event.joystick.stick].axis[event.joystick.axis] = event.joystick.pos;
            }
            break;

         /* ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN - a joystick button was pressed.
          */
         case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
            if (event.joystick.id != joy)
               break;
            log_printf("button down: %d\n", event.joystick.button);
            state1.button[event.joystick.button] = 32768;
            break;

         /* ALLEGRO_EVENT_JOYSTICK_BUTTON_UP - a joystick button was released.
          */
         case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
            if (event.joystick.id != joy)
               break;
            log_printf("button up: %d\n", event.joystick.button);
            state1.button[event.joystick.button] = 0;
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               return;
            break;

         /* ALLEGRO_EVENT_DISPLAY_CLOSE - the window close button was pressed.
          */
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return;

         case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
            log_printf("configuration changed\n");
            al_reconfigure_joysticks();
            joy = al_get_joystick(0);
            setup_joystick_values(joy, &state1);
            setup_joystick_values(joy, &state2);
            break;

         /* We received an event of some type we don't know about.
          * Just ignore it.
          */
         default:
            break;
      }
   }
}



int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_primitives_addon();
   al_init_font_addon();

   open_log_monospace();

   display = al_create_display(1024, 768);
   if (!display) {
      abort_example("al_create_display failed\n");
   }

   al_install_keyboard();

   black = al_map_rgb(0, 0, 0);
   grey = al_map_rgb(0xe0, 0xe0, 0xe0);
   white = al_map_rgb(255, 255, 255);
   blue = al_map_rgb(0xa0, 0xa0, 255);
   font = al_create_builtin_font();

   if (argc > 1) {
      log_printf("Using mappings from %s\n", argv[1]);
      al_set_joystick_mappings(argv[1]);
   }
   else
      log_printf("No mappings file specified. Pass the filename as an argument to this example.\n");
   al_install_joystick();

   event_queue = al_create_event_queue();
   if (!event_queue) {
      abort_example("al_create_event_queue failed\n");
   }

   if (al_get_keyboard_event_source()) {
      al_register_event_source(event_queue, al_get_keyboard_event_source());
   }
   al_register_event_source(event_queue, al_get_display_event_source(display));
   al_register_event_source(event_queue, al_get_joystick_event_source());

   main_loop();

   al_destroy_font(font);

   al_uninstall_system();

   return 0;
}

/* vim: set ts=8 sts=3 sw=3 et: */
