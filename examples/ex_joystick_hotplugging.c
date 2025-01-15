#include <math.h>
#include <stdio.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"


typedef struct {
   ALLEGRO_JOYSTICK *joy;
   char last_event[128];
} JOYSTICK_SLOT;

#define NUM_SLOTS (6)
JOYSTICK_SLOT slots[NUM_SLOTS];
ALLEGRO_FONT *font;



static JOYSTICK_SLOT *get_slot(ALLEGRO_JOYSTICK *joy)
{
   for (int i = 0; i < NUM_SLOTS; i++) {
      if (joy == slots[i].joy)
         return &slots[i];
   }
   return NULL;
}



static void fill_slots(void)
{
   int num_joysticks = al_get_num_joysticks();
   for (int i = 0; i < num_joysticks; i++) {
      ALLEGRO_JOYSTICK *joy = al_get_joystick(i);
      bool found = false;
      for (int j = 0; j < NUM_SLOTS; j++) {
         if (joy == slots[j].joy) {
            found = true;
            break;
         }
      }
      if (found)
         continue;
      for (int j = 0; j < NUM_SLOTS; j++) {
         JOYSTICK_SLOT *slot = &slots[j];
         if (slot->joy == NULL) {
            slot->joy = joy;
            break;
         }
      }
   }
}



static void draw_slots(void)
{
   ALLEGRO_COLOR inactive_color = al_map_rgb(0x80, 0x80, 0x80);

   for (int i = 0; i < NUM_SLOTS; i++) {
      JOYSTICK_SLOT *slot = &slots[i];
      bool active = slot->joy != NULL && al_get_joystick_active(slot->joy);
      ALLEGRO_COLOR active_color = al_color_lch(1., 1., 2 * ALLEGRO_PI * fmod(0.2 * al_get_time() + (float)i / NUM_SLOTS, 1.));
      ALLEGRO_COLOR color = active ? active_color : inactive_color;

      int x = 5;
      int y = 5 + i * 80;
      int w = 630;
      int h = 75;
      int dy = al_get_font_line_height(font) + 5;
      al_draw_rounded_rectangle(x, y, x + w, y + h, 16, 16, color, 3);
      x += 5;
      y += 5;

      y += dy;
      al_draw_textf(font, color, x, y, 0, "Slot: %d", i);

      if (!active) {
         y += dy;
         al_draw_textf(font, color, x, y, 0, "Inactive");
         continue;
      }
      y += dy;
      al_draw_textf(font, color, x, y, 0, "Name: %s", al_get_joystick_name(slot->joy));

      y += dy;
      al_draw_textf(font, color, x, y, 0, "Last Event: %s", slot->last_event);
   }
}



int main(int argc, char **argv)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;

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
   al_init_font_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   timer = al_create_timer(1. / 60);
   al_start_timer(timer);
   font = al_create_builtin_font();

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_joystick_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   fill_slots();

   bool redraw = true;
   while (1) {
      ALLEGRO_EVENT event;
      JOYSTICK_SLOT *slot = NULL;
      if (redraw && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb(0, 0, 0));
         draw_slots();
         al_flip_display();
         redraw = false;
      }
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_JOYSTICK_CONFIGURATION) {
         al_reconfigure_joysticks();
         fill_slots();
      }
      else if (event.type == ALLEGRO_EVENT_JOYSTICK_AXIS) {
         if ((slot = get_slot(event.joystick.id))) {
            snprintf(slot->last_event, sizeof(slot->last_event), "ALLEGRO_EVENT_JOYSTICK_AXIS, stick: %d, axis: %d, pos: %.3f",
                  event.joystick.stick, event.joystick.axis, event.joystick.pos);
         }
      }
      else if (event.type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN) {
         if ((slot = get_slot(event.joystick.id))) {
            snprintf(slot->last_event, sizeof(slot->last_event), "ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN, button: %d", event.joystick.button);
         }
      }
      else if (event.type == ALLEGRO_EVENT_JOYSTICK_BUTTON_UP) {
         if ((slot = get_slot(event.joystick.id))) {
            snprintf(slot->last_event, sizeof(slot->last_event), "ALLEGRO_EVENT_JOYSTICK_BUTTON_UP, button: %d", event.joystick.button);
         }
      }
   }

   al_uninstall_system();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
