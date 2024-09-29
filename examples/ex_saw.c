/* Recreate exstream.c from A4. */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"

#include "common.c"

#define SAMPLES_PER_BUFFER    1024


static void saw(A5O_AUDIO_STREAM *stream)
{
   A5O_EVENT_QUEUE *queue;
   int8_t *buf;
   int pitch = 0x10000;
   int val = 0;
   int i;
   int n = 200;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_audio_stream_event_source(stream));
#ifdef A5O_POPUP_EXAMPLES
   if (textlog) {
      al_register_event_source(queue, al_get_native_text_log_event_source(textlog));
   }
#endif

   log_printf("Generating saw wave...\n");

   while (n > 0) {
      A5O_EVENT event;

      al_wait_for_event(queue, &event);

      if (event.type == A5O_EVENT_AUDIO_STREAM_FRAGMENT) {
         buf = al_get_audio_stream_fragment(stream);
         if (!buf) {
            /* This is a normal condition you must deal with. */
            continue;
         }

         for (i = 0; i < SAMPLES_PER_BUFFER; i++) {
            /* Crude saw wave at maximum amplitude. Please keep this compatible
             * to the A4 example so we know when something has broken for now.
             * 
             * It would be nice to have a better example with user interface
             * and some simple synth effects.
             */
            buf[i] = ((val >> 16) & 0xff);
            val += pitch;
            pitch++;
         }

         if (!al_set_audio_stream_fragment(stream, buf)) {
            log_printf("Error setting stream fragment.\n");
         }

         n--;
         if ((n % 10) == 0) {
            log_printf(".");
            fflush(stdout);
         }
      }

#ifdef A5O_POPUP_EXAMPLES
      if (event.type == A5O_EVENT_NATIVE_DIALOG_CLOSE) {
         break;
      }
#endif
   }

   al_drain_audio_stream(stream);

   log_printf("\n");

   al_destroy_event_queue(queue);
}


int main(int argc, char **argv)
{
   A5O_AUDIO_STREAM *stream;
   void *buf;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   if (!al_install_audio()) {
      abort_example("Could not init sound.\n");
   }
   al_reserve_samples(0);

   stream = al_create_audio_stream(8, SAMPLES_PER_BUFFER, 22050,
      A5O_AUDIO_DEPTH_UINT8, A5O_CHANNEL_CONF_1);
   while ((buf = al_get_audio_stream_fragment(stream))) {
      al_fill_silence(buf, SAMPLES_PER_BUFFER, A5O_AUDIO_DEPTH_UINT8,
         A5O_CHANNEL_CONF_1);
      al_set_audio_stream_fragment(stream, buf);
   }

   if (!stream) {
      abort_example("Could not create stream.\n");
   }

   if (!al_attach_audio_stream_to_mixer(stream, al_get_default_mixer())) {
      abort_example("Could not attach stream to mixer.\n");
   }

   open_log();

   saw(stream);

   close_log(false);

   al_destroy_audio_stream(stream);
   al_uninstall_audio();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
