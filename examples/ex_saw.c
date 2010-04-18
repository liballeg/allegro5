/* Recreate exstream.c from A4. */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro5.h"
#include "allegro5/allegro_audio.h"

#include "common.c"

#define SAMPLES_PER_BUFFER    1024


static void saw(ALLEGRO_AUDIO_STREAM *stream)
{
   ALLEGRO_EVENT_QUEUE *queue;
   int8_t *buf;
   int pitch = 0x10000;
   int val = 0;
   int i;
   int n = 200;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_audio_stream_event_source(stream));

   while (n > 0) {
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
         buf = al_get_audio_stream_fragment(stream);
         if (!buf) {
            /* This is a normal condition you must deal with. */
            continue;
         }

         for (i = 0; i < SAMPLES_PER_BUFFER; i++) {
            /* Crude saw wave at maximum aplitude. Please keep this compatible
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
            fprintf(stderr, "Error setting stream fragment.\n");
         }

         n--;
         if ((n % 10) == 0) {
            putchar('.');
            fflush(stdout);
         }
      }
   }

   al_drain_audio_stream(stream);

   putchar('\n');

   al_destroy_event_queue(queue);
}


int main(void)
{
   ALLEGRO_AUDIO_STREAM *stream;

   if (!al_init()) {
      fprintf(stderr, "Could not init Allegro.\n");
      return 1;
   }

   if (!al_install_audio()) {
      fprintf(stderr, "Could not init sound.\n");
      return 1;
   }
   al_reserve_samples(0);

   stream = al_create_audio_stream(8, SAMPLES_PER_BUFFER, 22050,
      ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);
   if (!stream) {
      fprintf(stderr, "Could not create stream.\n");
      return 1;
   }

   if (!al_attach_audio_stream_to_mixer(stream, al_get_default_mixer())) {
      fprintf(stderr, "Could not attach stream to mixer.\n");
      return 1;
   }

   saw(stream);

   al_destroy_audio_stream(stream);
   al_uninstall_audio();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
