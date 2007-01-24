/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This is a very simple program showing how to use the event
 *    interface.
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



void fatal_error(const char *msg)
{
   fprintf(stderr, "fatal_error: %s\n", msg);
   exit(EXIT_FAILURE);
}



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
   }

   buffer = al_get_buffer(al_main_display);
   textprintf_ex(buffer, font, 400, y, black(), white(), "Timer: %d", count);
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



void main_loop(void)
{
   AL_EVENT event;

   while (true) {

      al_wait_for_event(event_queue, &event, 0);

      switch (event.type) {

         case AL_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == AL_KEY_ESCAPE) {
               return;
            }
            log_key_down(event.keyboard.keycode,
                  event.keyboard.unichar,
                  event.keyboard.modifiers);
            break;

         case AL_EVENT_KEY_REPEAT:
            log_key_repeat(event.keyboard.keycode,
                  event.keyboard.unichar,
                  event.keyboard.modifiers);
            break;

         case AL_EVENT_KEY_UP:
            log_key_up(event.keyboard.keycode,
                  event.keyboard.modifiers);
            break;

         case AL_EVENT_MOUSE_AXES:
            draw_mouse_pos(event.mouse.x, event.mouse.y, event.mouse.z);
            break;

         case AL_EVENT_MOUSE_BUTTON_DOWN:
            draw_mouse_button(event.mouse.button, true);
            break;

         case AL_EVENT_MOUSE_BUTTON_UP:
            draw_mouse_button(event.mouse.button, false);
            break;

         case AL_EVENT_TIMER:
            draw_timer_tick(event.timer.source, event.timer.count);
            break;

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

         case AL_EVENT_JOYSTICK_BUTTON_DOWN:
            if (event.joystick.source == al_get_joystick(0)) {
               draw_joystick_button(event.joystick.button, true);
            }
            break;

         case AL_EVENT_JOYSTICK_BUTTON_UP:
            if (event.joystick.source == al_get_joystick(0)) {
               draw_joystick_button(event.joystick.button, false);
            }
            break;

         default:
            break;
      }
   }
}



int main(void)
{
   int num;
   int i;

   allegro_init();

   if (!al_create_display(GFX_AUTODETECT, AL_UPDATE_DOUBLE_BUFFER,
         AL_DEPTH_32, 640, 480))
   {
      fatal_error("al_create_display");
   }

   if (!al_install_keyboard()) {
      fatal_error("al_install_keyboard");
   }

   if (!al_install_mouse()) {
      fatal_error("al_install_mouse");
   }
   al_show_mouse_cursor();

   if (!al_install_joystick()) {
      fatal_error("al_install_joystick");
   }

   timer_a = al_install_timer(100);
   timer_b = al_install_timer(1000);
   timer_c = al_install_timer(5000);
   if (!timer_a || !timer_b || !timer_c) {
      fatal_error("al_install_timer");
   }
   
   event_queue = al_create_event_queue();
   if (!event_queue) {
      fatal_error("al_create_event_queue");
   }

   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)timer_a);
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)timer_b);
   al_register_event_source(event_queue, (AL_EVENT_SOURCE *)timer_c);

   num = al_num_joysticks();
   for (i = 0; i < num; i++) {
      al_register_event_source(event_queue,
         (AL_EVENT_SOURCE *)al_get_joystick(i));
   }

   draw_all();
   al_start_timer(timer_a);
   al_start_timer(timer_b);
   al_start_timer(timer_c);
   main_loop();

   return 0;
}

END_OF_MAIN()

/* vi: set ts=8 sts=3 sw=3 et: */
