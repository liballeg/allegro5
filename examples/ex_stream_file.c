/*
 * An example program that plays a file from the disk using Allegro5
 * streaming API. The file is being read in small chunks and played on the
 * sound device instead of being loaded at once.
 *
 * usage: ./ex_stream_file file.[wav,ogg...] ...
 *
 * by Milan Mimica
 */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include "common.c"

/* Attaches the stream directly to a voice. Streamed file's and voice's sample
 * rate, channels and depth must match.
 */
//#define BYPASS_MIXER


int main(int argc, char **argv)
{
   int i;
   ALLEGRO_VOICE*  voice;
   ALLEGRO_MIXER*  mixer;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s {audio_files}\n", argv[0]);
      return 1;
   }

   if (!al_init()) {
       fprintf(stderr, "Could not init Allegro.\n");
       return 1;
   }

   al_init_acodec_addon();

   if (!al_install_audio()) {
       fprintf(stderr, "Could not init sound!\n");
       return 1;
   }

   voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
                           ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      fprintf(stderr, "Could not create ALLEGRO_VOICE.\n");
      return 1;
   }
   fprintf(stderr, "Voice created.\n");

#ifndef BYPASS_MIXER
   mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                           ALLEGRO_CHANNEL_CONF_2);
   if (!mixer) {
      fprintf(stderr, "Could not create ALLEGRO_MIXER.\n");
      return 1;
   }
   fprintf(stderr, "Mixer created.\n");

   if (!al_attach_mixer_to_voice(mixer, voice)) {
      fprintf(stderr, "al_attach_mixer_to_voice failed.\n");
      return 1;
   }
#endif

   for (i = 1; i < argc; ++i)
   {
      ALLEGRO_AUDIO_STREAM* stream;
      const char*     filename = argv[i];
      bool playing = true;
      ALLEGRO_EVENT event;
      ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

      stream = al_load_audio_stream(filename, 4, 2048);
      if (!stream) {
         fprintf(stderr, "Could not create an ALLEGRO_AUDIO_STREAM from '%s'!\n",
                 filename);
         continue;
      }
      fprintf(stderr, "Stream created from '%s'.\n", filename);
      
      al_register_event_source(queue, al_get_audio_stream_event_source(stream));

#ifndef BYPASS_MIXER
      if (!al_attach_audio_stream_to_mixer(stream, mixer)) {
         fprintf(stderr, "al_attach_audio_stream_to_mixer failed.\n");
         continue;
      }
#else
      if (!al_attach_audio_stream_to_voice(stream, voice)) {
         fprintf(stderr, "al_attach_audio_stream_to_voice failed.\n");
         return 1;
      }
#endif

      fprintf(stderr, "Playing %s ... Waiting for stream to finish ", filename);
      do {
         al_wait_for_event(queue, &event);
         if(event.type == ALLEGRO_EVENT_AUDIO_STREAM_FINISHED)
            playing = false;
      } while (playing);
      fprintf(stderr, "\n");

      al_destroy_event_queue(queue);
      al_destroy_audio_stream(stream);
   }

#ifndef BYPASS_MIXER
   al_destroy_mixer(mixer);
#endif
   al_destroy_voice(voice);

   al_uninstall_audio();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
