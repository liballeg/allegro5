/* Recreate exstream.c from A4. */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"


#define SAMPLES_PER_BUFFER    1024


void saw(ALLEGRO_STREAM *stream)
{
   ALLEGRO_EVENT_QUEUE *queue;
   int8_t *buf;
   int pitch = 0x10000;
   int val = 0;
   int i;
   int n = 200;

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) stream);

   while (n > 0) {
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT) {
         buf = (int8_t *) event.stream.empty_fragment;
         for (i = 0; i < SAMPLES_PER_BUFFER; i++) {
            buf[i] = ((val >> 16) & 0xff) >> 4;    /* not so loud please */
            val += pitch;
            pitch++;
            if (pitch > 0x40000)
               pitch = 0x10000;
         }

         if (al_stream_set_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER, buf) != 0) {
            fprintf(stderr, "Error setting stream buffer.\n");
         }

         n--;
         if ((n % 10) == 0) {
            putchar('.');
            fflush(stdout);
         }
      }
   }

   al_stream_drain(stream);

   putchar('\n');

   al_destroy_event_queue(queue);
}


int main(void)
{
   ALLEGRO_VOICE *voice;
   ALLEGRO_MIXER *mixer;
   ALLEGRO_STREAM *stream;

   al_init();

   if (al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT) != 0) {
      fprintf(stderr, "Could not init sound.\n");
      return 1;
   }

   voice = al_voice_create(44100, ALLEGRO_AUDIO_DEPTH_INT16,
      ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      fprintf(stderr, "Could not create voice.\n");
      return 1;
   }

   mixer = al_mixer_create(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   if (!mixer) {
      fprintf(stderr, "Could not create mixer.\n");
      return 1;
   }

   if (al_voice_attach_mixer(voice, mixer) != 0) {
      fprintf(stderr, "Could not attach mixer to voice.\n");
      return 1;
   }

   stream = al_stream_create(8, SAMPLES_PER_BUFFER, 22050,
      ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);
   if (!stream) {
      fprintf(stderr, "Could not create stream.\n");
      return 1;
   }

   if (al_mixer_attach_stream(mixer, stream) != 0) {
      fprintf(stderr, "Could not attach stream to mixer.\n");
      return 1;
   }

   saw(stream);

   al_stream_destroy(stream);
   al_mixer_destroy(mixer);
   al_voice_destroy(voice);
   al_audio_deinit();

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
