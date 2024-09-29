#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

static void print_joystick_info(A5O_JOYSTICK *joy)
{
   int i, n, a;

   if (!joy)
      return;

   log_printf("Joystick: '%s'\n", al_get_joystick_name(joy));

   log_printf("  Buttons:");
   n = al_get_joystick_num_buttons(joy);
   for (i = 0; i < n; i++) {
      log_printf(" '%s'", al_get_joystick_button_name(joy, i));
   }
   log_printf("\n");

   n = al_get_joystick_num_sticks(joy);
   for (i = 0; i < n; i++) {
      log_printf("  Stick %d: '%s'\n", i, al_get_joystick_stick_name(joy, i));

      for (a = 0; a < al_get_joystick_num_axes(joy, i); a++) {
         log_printf("    Axis %d: '%s'\n",
            a, al_get_joystick_axis_name(joy, i, a));
      }
   }
}

static void draw(A5O_JOYSTICK *curr_joy)
{
   int x = 100;
   int y = 100;
   A5O_JOYSTICK_STATE joystate;
   int i;

   al_clear_to_color(al_map_rgb(0, 0, 0));

   if (curr_joy) {
      al_get_joystick_state(curr_joy, &joystate);
      for (i = 0; i < al_get_joystick_num_sticks(curr_joy); i++) {
         al_draw_filled_circle(
               x+joystate.stick[i].axis[0]*20 + i * 80,
               y+joystate.stick[i].axis[1]*20,
               20, al_map_rgb(255, 255, 255)
            );
      }
      for (i = 0; i < al_get_joystick_num_buttons(curr_joy); i++) {
         if (joystate.button[i]) {
            al_draw_filled_circle(
                  i*20+10, 400, 9, al_map_rgb(255, 255, 255)
                  );
         }
      }
   }

   al_flip_display();
}

int main(int argc, char **argv)
{
   int num_joysticks;
   A5O_EVENT_QUEUE *queue;
   A5O_JOYSTICK *curr_joy;
   A5O_DISPLAY *display;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   if (!al_install_joystick()) {
      abort_example("Could not init joysticks.\n");
   }
   al_install_keyboard();
   al_init_primitives_addon();

   open_log();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_joystick_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   num_joysticks = al_get_num_joysticks();
   log_printf("Num joysticks: %d\n", num_joysticks);

   if (num_joysticks > 0) {
      curr_joy = al_get_joystick(0);
      print_joystick_info(curr_joy);
   }
   else {
      curr_joy = NULL;
   }

   draw(curr_joy);

   while (1) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_KEY_DOWN &&
            event.keyboard.keycode == A5O_KEY_ESCAPE) {
         break;
      }
      else if (event.type == A5O_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == A5O_EVENT_KEY_CHAR) {
         int n = event.keyboard.unichar - '0';
         if (n >= 0 && n < num_joysticks) {
            curr_joy = al_get_joystick(n);
            log_printf("switching to joystick %d\n", n);
            print_joystick_info(curr_joy);
         }
      }
      else if (event.type == A5O_EVENT_JOYSTICK_CONFIGURATION) {
         al_reconfigure_joysticks();
         num_joysticks = al_get_num_joysticks();
         log_printf("after reconfiguration num joysticks = %d\n",
            num_joysticks);
         if (curr_joy) {
            log_printf("current joystick is: %s\n",
               al_get_joystick_active(curr_joy) ? "active" : "inactive");
         }
         curr_joy = al_get_joystick(0);
      }
      else if (event.type == A5O_EVENT_JOYSTICK_AXIS) {
         log_printf("axis event from %p, stick %d, axis %d\n", event.joystick.id, event.joystick.stick, event.joystick.axis);
      }
      else if (event.type == A5O_EVENT_JOYSTICK_BUTTON_DOWN) {
         log_printf("button down event %d from %p\n",
            event.joystick.button, event.joystick.id);
      } 
      else if (event.type == A5O_EVENT_JOYSTICK_BUTTON_UP) {
         log_printf("button up event %d from %p\n",
            event.joystick.button, event.joystick.id);
      } 

      draw(curr_joy);
   }

   close_log(false);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
