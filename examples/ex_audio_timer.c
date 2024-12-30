/*
 *    Example program for the Allegro library.
 */

#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_font.h"

#include "common.c"


#define RESERVED_SAMPLES   16
#define PERIOD             5


static ALLEGRO_DISPLAY *display;
static ALLEGRO_FONT *font;
static ALLEGRO_SAMPLE *ping;
static ALLEGRO_TIMER *timer;
static ALLEGRO_EVENT_QUEUE *event_queue;


static ALLEGRO_SAMPLE *create_sample_s16(int freq, int len)
{
   char *buf = al_malloc(freq * len * sizeof(int16_t));

   return al_create_sample(buf, len, freq, ALLEGRO_AUDIO_DEPTH_INT16,
                           ALLEGRO_CHANNEL_CONF_1, true);
}


/* Adapted from SPEED. */
static ALLEGRO_SAMPLE *generate_ping(void)
{
   float osc1, osc2, vol, ramp;
   int16_t *p;
   int len;
   int i;

   /* ping consists of two sine waves */
   len = 8192;
   ping = create_sample_s16(22050, len);
   if (!ping)
      return NULL;

   p = (int16_t *)al_get_sample_data(ping);

   osc1 = 0;
   osc2 = 0;

   for (i=0; i<len; i++) {
      vol = (float)(len - i) / (float)len * 4000;

      ramp = (float)i / (float)len * 8;
      if (ramp < 1.0f)
         vol *= ramp;

      *p = (sin(osc1) + sin(osc2) - 1) * vol;

      osc1 += 0.1;
      osc2 += 0.15;

      p++;
   }

   return ping;
}


int main(int argc, char **argv)
{
   ALLEGRO_TRANSFORM trans;
   ALLEGRO_EVENT event;
   int bps = 4;
   bool redraw = false;
   unsigned int last_timer = 0;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display\n");
   }

   font = al_create_builtin_font();
   if (!font) {
      abort_example("Could not create font\n");
   }

   if (!al_install_audio()) {
      abort_example("Could not init sound\n");
   }

   if (!al_reserve_samples(RESERVED_SAMPLES)) {
      abort_example("Could not set up voice and mixer\n");
   }

   ping = generate_ping();
   if (!ping) {
      abort_example("Could not generate sample\n");
   }

   timer = al_create_timer(1.0 / bps);
   al_set_timer_count(timer, -1);

   event_queue = al_create_event_queue();
   al_register_event_source(event_queue, al_get_keyboard_event_source());
   al_register_event_source(event_queue, al_get_timer_event_source(timer));
   al_register_event_source(event_queue, al_get_display_event_source(display));

   al_identity_transform(&trans);
   al_scale_transform(&trans, 16.0, 16.0);
   al_use_transform(&trans);

   al_start_timer(timer);

   while (true) {
      al_wait_for_event(event_queue, &event);
      if (event.type == ALLEGRO_EVENT_TIMER) {
         const float speed = pow(21.0/20.0, (event.timer.count % PERIOD));
         if (!al_play_sample(ping, 1.0, 0.0, speed, ALLEGRO_PLAYMODE_ONCE, NULL)) {
            log_printf("Not enough reserved samples.\n");
         }
         redraw = true;
         last_timer = event.timer.count;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            break;
         }
         if (event.keyboard.unichar == '+' || event.keyboard.unichar == '=') {
            if (bps < 32) {
               bps++;
               al_set_timer_speed(timer, 1.0 / bps);
            }
         }
         else if (event.keyboard.unichar == '-') {
            if (bps > 1) {
               bps--;
               al_set_timer_speed(timer, 1.0 / bps);
            }
         }
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }

      if (redraw && al_is_event_queue_empty(event_queue)) {
         ALLEGRO_COLOR c;
         if (last_timer % PERIOD == 0)
            c = al_map_rgb_f(1, 1, 1);
         else
            c = al_map_rgb_f(0.5, 0.5, 1.0);

         al_clear_to_color(al_map_rgb(0, 0, 0));
         al_draw_textf(font, c, 640/32, 480/32 - 4, ALLEGRO_ALIGN_CENTRE,
            "%u", last_timer);
         al_flip_display();
      }
   }

   close_log(false);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
