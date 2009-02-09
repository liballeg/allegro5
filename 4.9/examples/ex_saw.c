/* Recreate exstream.c from A4. */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"


#define SAMPLES_PER_BUFFER    1024


void saw(ALLEGRO_STREAM *stream)
{
   ALLEGRO_EVENT_QUEUE *queue;
   void *buf_void;
   int8_t *buf;
   int pitch = 0x10000;
   int val = 0;
   int i;
   int n = 200;
   float gain;

   queue = al_create_event_queue();
   /* FIXME: this cast is pretty scarry */
   al_register_event_source(queue, *((ALLEGRO_EVENT_SOURCE **) stream));

   while (n > 0) {
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT) {
         al_get_stream_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER, &buf_void);
         buf = buf_void;

         for (i = 0; i < SAMPLES_PER_BUFFER; i++) {
            buf[i] = ((val >> 16) & 0xff) >> 4;    /* not so loud please */
            val += pitch;
            pitch++;
            if (pitch > 0x40000)
               pitch = 0x10000;
         }

         if (al_set_stream_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER, buf) != 0) {
            fprintf(stderr, "Error setting stream buffer.\n");
         }

         n--;
         if ((n % 10) == 0) {
            putchar('.');
            fflush(stdout);

            if (al_get_stream_float(stream, ALLEGRO_AUDIOPROP_GAIN, &gain)
                  == 0) {
               al_set_stream_float(stream, ALLEGRO_AUDIOPROP_GAIN, gain * 0.9);
            }
            else {
               fprintf(stderr, "Error getting gain.\n");
            }
         }
      }
   }

   al_drain_stream(stream);

   putchar('\n');

   al_destroy_event_queue(queue);
}


int main(void)
{
   ALLEGRO_VOICE *voice;
   ALLEGRO_MIXER *mixer;
   ALLEGRO_STREAM *stream;

   if (!al_init()) {
      fprintf(stderr, "Could not init Allegro.\n");
      return 1;
   }

   if (al_install_audio(ALLEGRO_AUDIO_DRIVER_AUTODETECT) != 0) {
      fprintf(stderr, "Could not init sound.\n");
      return 1;
   }

   voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
      ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      fprintf(stderr, "Could not create voice.\n");
      return 1;
   }

   mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   if (!mixer) {
      fprintf(stderr, "Could not create mixer.\n");
      return 1;
   }

   if (al_attach_mixer_to_voice(voice, mixer) != 0) {
      fprintf(stderr, "Could not attach mixer to voice.\n");
      return 1;
   }

   stream = al_create_stream(8, SAMPLES_PER_BUFFER, 22050,
      ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);
   if (!stream) {
      fprintf(stderr, "Could not create stream.\n");
      return 1;
   }

   if (al_attach_stream_to_mixer(mixer, stream) != 0) {
      fprintf(stderr, "Could not attach stream to mixer.\n");
      return 1;
   }

   saw(stream);

   al_destroy_stream(stream);
   al_destroy_mixer(mixer);
   al_destroy_voice(voice);
   al_uninstall_audio();

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
