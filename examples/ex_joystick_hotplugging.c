#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

int main(void)
{
   int num_joysticks;
   ALLEGRO_EVENT_QUEUE *queue;
   int i;
   int curr_joy_num;
   ALLEGRO_JOYSTICK *curr_joy;
   ALLEGRO_DISPLAY *display;

   al_init();
   al_install_joystick();
   al_install_keyboard();
   al_init_primitives_addon();

   display = al_create_display(640, 480);
   if (!display)
      return 1;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_joystick_event_source());

   num_joysticks = al_get_num_joysticks();
   curr_joy_num = 0;
   if (curr_joy_num < num_joysticks)
      curr_joy = al_get_joystick(curr_joy_num);
   else
      curr_joy = NULL;

   goto initial_draw;

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode ==
            ALLEGRO_KEY_ESCAPE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         int n = event.keyboard.unichar - '0';
         if (n >= 0 && n < num_joysticks) {
            curr_joy_num = n;
            curr_joy = al_get_joystick(n);
            printf("switching to joystick %d\n", n);
         }
      }
      else if (event.type == ALLEGRO_EVENT_JOYSTICK_CONFIGURATION) {
         num_joysticks = al_get_num_joysticks();
         if (curr_joy_num < num_joysticks)
            curr_joy = al_get_joystick(curr_joy_num);
         else
            curr_joy = NULL;
      }
      else if (event.type == ALLEGRO_EVENT_JOYSTICK_AXIS) {
         printf("axis event from %p\n", event.joystick.id);
      }
      else if (event.type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN) {
      	printf("button down event %d from %p\n", event.joystick.button, event.joystick.id);
      }

initial_draw:

      al_clear_to_color(al_map_rgb(0, 0, 0));

      if (curr_joy) {
         int x = 100;
         int y = 100;
         ALLEGRO_JOYSTICK_STATE joystate;
         al_get_joystick_state(curr_joy, &joystate);
         al_draw_filled_circle(
            x+joystate.stick[0].axis[0]*20,
            y+joystate.stick[0].axis[1]*20,
            20, al_map_rgb(255, 255, 255)
         );
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

   return 0;
}
