/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This program tests joystick events.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

#define MAX_AXES     3
#define MAX_STICKS   16
#define MAX_BUTTONS  32


/* globals */
A5O_EVENT_QUEUE  *event_queue;
A5O_FONT         *font;
A5O_COLOR        black;
A5O_COLOR        grey;
A5O_COLOR        white;

int num_sticks = 0;
int num_buttons = 0;
int num_axes[MAX_STICKS] = { 0 };
float joys[MAX_STICKS][MAX_AXES] = {{ 0 }};
bool joys_buttons[MAX_BUTTONS] = { 0 };


static void setup_joystick_values(A5O_JOYSTICK *joy)
{
   A5O_JOYSTICK_STATE jst;
   int i, j;

   if (joy == NULL) {
      num_sticks = 0;
      num_buttons = 0;
      return;
   }

   al_get_joystick_state(joy, &jst);

   num_sticks = al_get_joystick_num_sticks(joy);
   if (num_sticks > MAX_STICKS)
      num_sticks = MAX_STICKS;
   for (i = 0; i < num_sticks; i++) {
      num_axes[i] = al_get_joystick_num_axes(joy, i);
      for (j = 0; j < num_axes[i]; ++j)
         joys[i][j] = jst.stick[i].axis[j];
   }

   num_buttons = al_get_joystick_num_buttons(joy);
   if (num_buttons > MAX_BUTTONS) {
      num_buttons = MAX_BUTTONS;
   }
   for (i = 0; i < num_buttons; i++) {
      joys_buttons[i] = (jst.button[i] >= 16384);
   }
}



static void draw_joystick_axes(A5O_JOYSTICK *joy, int cx, int cy, int stick)
{
   const int size = 30;
   const int csize = 5;
   const int osize = size + csize;
   int zx = cx + osize + csize * 2;
   int x = cx + joys[stick][0] * size;
   int y = cy + joys[stick][1] * size;
   int z = cy + joys[stick][2] * size;
   int i;

   al_draw_filled_rectangle(cx-osize, cy-osize, cx+osize, cy+osize, grey);
   al_draw_rectangle(cx-osize+0.5, cy-osize+0.5, cx+osize-0.5, cy+osize-0.5, black, 0);
   al_draw_filled_rectangle(x-5, y-5, x+5, y+5, black);

   if (num_axes[stick] >= 3) {
      al_draw_filled_rectangle(zx-csize, cy-osize, zx+csize, cy+osize, grey);
      al_draw_rectangle(zx-csize+0.5f, cy-osize+0.5f, zx+csize-0.5f, cy+osize-0.5f, black, 0);
      al_draw_filled_rectangle(zx-5, z-5, zx+5, z+5, black);
   }

   if (joy) {
      al_draw_text(font, black, cx, cy + osize + 1, A5O_ALIGN_CENTRE,
         al_get_joystick_stick_name(joy, stick));
      for (i = 0; i < num_axes[stick]; i++) {
         al_draw_text(font, black, cx, cy + osize + (1 + i) * 10,
            A5O_ALIGN_CENTRE,
            al_get_joystick_axis_name(joy, stick, i));
      }
   }
}



static void draw_joystick_button(A5O_JOYSTICK *joy, int button, bool down)
{
   A5O_BITMAP *bmp = al_get_target_bitmap();
   int x = al_get_bitmap_width(bmp)/2-120 + (button % 8) * 30;
   int y = al_get_bitmap_height(bmp)-120 + (button / 8) * 30;
   A5O_COLOR fg;

   al_draw_filled_rectangle(x, y, x + 25, y + 25, grey);
   al_draw_rectangle(x+0.5, y+0.5, x + 24.5, y + 24.5, black, 0);
   if (down) {
      al_draw_filled_rectangle(x + 2, y + 2, x + 23, y + 23, black);
      fg = white;
   }
   else {
      fg = black;
   }

   if (joy) {
      const char *name = al_get_joystick_button_name(joy, button);
      if (strlen(name) < 4) {
         al_draw_text(font, fg, x + 13, y + 8, A5O_ALIGN_CENTRE, name);
      }
   }
}



static void draw_all(A5O_JOYSTICK *joy)
{
   A5O_BITMAP *bmp = al_get_target_bitmap();
   int width = al_get_bitmap_width(bmp);
   int height = al_get_bitmap_height(bmp);
   int i;

   al_clear_to_color(al_map_rgb(0xff, 0xff, 0xc0));

   if (joy) {
      al_draw_textf(font, black, width / 2, 10, A5O_ALIGN_CENTRE,
         "Joystick: %s", al_get_joystick_name(joy));
   }

   for (i = 0; i < num_sticks; i++) {
      int u = i%4;
      int v = i/4;
      int cx = (u + 0.5) * width/4;
      int cy = (v + 0.5) * height/6;
      draw_joystick_axes(joy, cx, cy, i);
   }

   for (i = 0; i < num_buttons; i++) {
      draw_joystick_button(joy, i, joys_buttons[i]);
   }

   al_flip_display();
}



static void main_loop(void)
{
   A5O_EVENT event;

   while (true) {
      if (al_is_event_queue_empty(event_queue))
         draw_all(al_get_joystick(0));

      al_wait_for_event(event_queue, &event);

      switch (event.type) {
         /* A5O_EVENT_JOYSTICK_AXIS - a joystick axis value changed.
          */
         case A5O_EVENT_JOYSTICK_AXIS:
            if (event.joystick.stick < MAX_STICKS && event.joystick.axis < MAX_AXES) {
               joys[event.joystick.stick][event.joystick.axis] = event.joystick.pos;
            }
            break;

         /* A5O_EVENT_JOYSTICK_BUTTON_DOWN - a joystick button was pressed.
          */
         case A5O_EVENT_JOYSTICK_BUTTON_DOWN:
            joys_buttons[event.joystick.button] = true;
            break;

         /* A5O_EVENT_JOYSTICK_BUTTON_UP - a joystick button was released.
          */
         case A5O_EVENT_JOYSTICK_BUTTON_UP:
            joys_buttons[event.joystick.button] = false;
            break;

         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE)
               return;
            break;

         /* A5O_EVENT_DISPLAY_CLOSE - the window close button was pressed.
          */
         case A5O_EVENT_DISPLAY_CLOSE:
            return;

         case A5O_EVENT_JOYSTICK_CONFIGURATION:
            al_reconfigure_joysticks();
            setup_joystick_values(al_get_joystick(0));
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
   A5O_DISPLAY *display;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_primitives_addon();
   al_init_font_addon();

   display = al_create_display(1024, 768);
   if (!display) {
      abort_example("al_create_display failed\n");
   }

   al_install_keyboard();

   black = al_map_rgb(0, 0, 0);
   grey = al_map_rgb(0xe0, 0xe0, 0xe0);
   white = al_map_rgb(255, 255, 255);
   font = al_create_builtin_font();

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

   setup_joystick_values(al_get_joystick(0));

   main_loop();

   al_destroy_font(font);

   return 0;
}

/* vim: set ts=8 sts=3 sw=3 et: */
