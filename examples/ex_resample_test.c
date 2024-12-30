/* Resamping test. Probably should integreate into test_driver somehow */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"

#include "common.c"

#define SAMPLES_PER_BUFFER 1024

#define N 2

int frequency[N];
double samplepos[N];
ALLEGRO_AUDIO_STREAM *stream[N];
ALLEGRO_DISPLAY *display;

float waveform[640], waveform_buffer[640];

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
      }

      if (!al_attach_audio_stream_to_mixer(stream[i], al_get_default_mixer())) {
         abort_example("Could not attach stream to mixer.\n");
      }
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
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
               double t = samplepos[si]++ / (double)frequency[si];
               buf[i] = sin(t * pitch * ALLEGRO_PI * 2) / N;
            }

            if (!al_set_audio_stream_fragment(stream[si], buf)) {
               log_printf("Error setting stream fragment.\n");
            }

            n++;
            log_printf("%d", si);
            if ((n % 60) == 0)
               log_printf("\n");
         }
      }

      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }

      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
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
         ALLEGRO_COLOR c = al_map_rgb(0, 0, 0);
         int i;
         al_clear_to_color(al_map_rgb_f(1, 1, 1));

         for (i = 0; i < 640; i++) {
            al_draw_pixel(i, 50 + waveform[i] * 50, c);
         }

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

static void update_waveform(void *buf, unsigned int samples, void *data)
{
   static int pos;
   float *fbuf = (float *)buf;
   int i;
   int n = samples;
   (void)data;

   /* Yes, we could do something more advanced, but an oscilloscope of the
    * first 640 samples of each buffer is enough for our purpose here.
    */
   if (n > 640) n = 640;

   for (i = 0; i < n; i++) {
      waveform_buffer[pos++] = fbuf[i * 2];
      if (pos == 640) {
         memcpy(waveform, waveform_buffer, 640 * sizeof(float));
         pos = 0;
         break;
      }
   }
}

int main(int argc, char **argv)
{
   int i;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();

   open_log();

   display = al_create_display(640, 100);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   if (!al_install_audio()) {
      abort_example("Could not init sound.\n");
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
