/* Resamping test. Probably should integreate into test_driver somehow */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"

#include "common.c"

#define SAMPLES_PER_BUFFER    1024

#define N 2

int frequency[N];
double samplepos[N];
ALLEGRO_AUDIO_STREAM *stream[N];
ALLEGRO_DISPLAY *display;

ALLEGRO_BITMAP *waveform;

static void mainloop(void)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_TIMER *timer;
   float *buf;
   double pitch = 440;
   int i, si;
   int n = 0;
   bool redraw = false;
   
   for (i = 0; i < N; i++) {
      frequency[i] = 22050 * pow(2, i / (double)N);
      stream[i] = al_create_audio_stream(4, SAMPLES_PER_BUFFER, frequency[i],
         ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_1);
      if (!stream[i]) {
         abort_example("Could not create stream.\n");
         return;
      }

      if (!al_attach_audio_stream_to_mixer(stream[i], al_get_default_mixer())) {
         abort_example("Could not attach stream to mixer.\n");
         return;
      }
   }

   queue = al_create_event_queue();
   for (i = 0; i < N; i++) {
      al_register_event_source(queue,
         al_get_audio_stream_event_source(stream[i]));
   }
#ifdef ALLEGRO_POPUP_EXAMPLES
   if (textlog) {
      al_register_event_source(queue, al_get_native_text_log_event_source(textlog));
   }
#endif

   log_printf("Generating %d sine waves of different sampling quality\n", N);
   log_printf("If Allegro's resampling is correct there should be little variation\n", N);

   timer = al_create_timer(1.0 / 60);
   al_register_event_source(queue, al_get_timer_event_source(timer));
   
   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);
   while (n < 60 * frequency[0] / SAMPLES_PER_BUFFER * N) {
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
         for (si = 0; si < N; si++) {
            buf = al_get_audio_stream_fragment(stream[si]);
            if (!buf) {
               continue;
            }

            for (i = 0; i < SAMPLES_PER_BUFFER; i++) {
               /* Crude saw wave at maximum amplitude. Please keep this compatible
                * to the A4 example so we know when something has broken for now.
                * 
                * It would be nice to have a better example with user interface
                * and some simple synth effects.
                */
               double t = samplepos[si]++ / (double)frequency[si];
               buf[i] = sin(t * pitch * ALLEGRO_PI * 2) / N;
            }

            if (!al_set_audio_stream_fragment(stream[si], buf)) {
               log_printf("Error setting stream fragment.\n");
            }
            
            n++;
            log_printf("%d", si);
            if ((n % 80) == 0) log_printf("\n");
            
         }
      }
      
      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }
      
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }

#ifdef ALLEGRO_POPUP_EXAMPLES
      if (event.type == ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE) {
         break;
      }
#endif

      if (redraw &&al_is_event_queue_empty(queue)) {
         al_draw_bitmap(waveform, 0, 0, 0);
         al_flip_display();
         redraw = false;
      }
   }

   for (si = 0; si < N; si++) {
      al_drain_audio_stream(stream[si]);
   }

   log_printf("\n");

   al_destroy_event_queue(queue);
}

static int last_pos = 0;
static void update_waveform(void *buf, unsigned int samples, void *data)
{
   float *fbuf = (float *)buf;
   int i;
   (void)data;
   ALLEGRO_COLOR black = al_map_rgb(0, 0, 0);

   al_set_target_bitmap(waveform);
   for (i = 0; i < (int)samples; i++) {
      al_put_pixel((last_pos + i) % 640, 50 + fbuf[i] * 50, black);
   }
   last_pos += samples;
   al_set_target_backbuffer(al_get_current_display());
}

int main(void)
{
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
      return 1;
   }
   
   open_log();
   
   display = al_create_display(640, 100);
   
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   waveform = al_create_bitmap(640, 100);
   al_set_target_bitmap(waveform);
   al_clear_to_color(al_map_rgb_f(1, 1, 1));
   al_set_target_backbuffer(al_get_current_display());

   if (!al_install_audio()) {
      abort_example("Could not init sound.\n");
      return 1;
   }
   al_reserve_samples(N);
   
   al_set_mixer_postprocess_callback(al_get_default_mixer(), update_waveform, NULL);

   mainloop();

   close_log(false);

   for (i = 0; i < N; i++) {
      al_destroy_audio_stream(stream[i]);
   }
   al_uninstall_audio();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
