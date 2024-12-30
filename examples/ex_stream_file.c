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

char *default_files[] = {NULL, "../demos/skater/data/menu/skate2.ogg"};

int main(int argc, char **argv)
{
   int i;
   ALLEGRO_VOICE*  voice;
   ALLEGRO_MIXER*  mixer;
   bool loop = false;
   int arg_start = 1;

   if (!al_init()) {
       abort_example("Could not init Allegro.\n");
   }

   open_log();

   if (argc < 2) {
      log_printf("This example can be run from the command line.\n");
      log_printf("Usage: %s [--loop] {audio_files}\n", argv[0]);
      argv = default_files;
      argc = 2;
   }

   if (strcmp(argv[1], "--loop") == 0) {
      loop = true;
      arg_start = 2;
   }

   al_init_acodec_addon();

   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }

   voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
                           ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      abort_example("Could not create ALLEGRO_VOICE.\n");
   }
   log_printf("Voice created.\n");

#ifndef BYPASS_MIXER
   mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                           ALLEGRO_CHANNEL_CONF_2);
   if (!mixer) {
      abort_example("Could not create ALLEGRO_MIXER.\n");
   }
   log_printf("Mixer created.\n");

   if (!al_attach_mixer_to_voice(mixer, voice)) {
      abort_example("al_attach_mixer_to_voice failed.\n");
   }
#endif

   for (i = arg_start; i < argc; ++i)
   {
      ALLEGRO_AUDIO_STREAM* stream;
      const char*     filename = argv[i];
      bool playing = true;
      ALLEGRO_EVENT event;
      ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

      stream = al_load_audio_stream(filename, 4, 2048);
      if (!stream) {
         /* If it is not packed, e.g. on Android or iOS. */
         if (!strcmp(filename, default_files[1])) {
            stream = al_load_audio_stream("data/welcome.wav", 4, 2048);
         }
      }
      if (!stream) {
         log_printf("Could not create an ALLEGRO_AUDIO_STREAM from '%s'!\n",
                 filename);
         continue;
      }
      log_printf("Stream created from '%s'.\n", filename);
      if (loop) {
         al_set_audio_stream_playmode(stream, loop ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE);
      }

      al_register_event_source(queue, al_get_audio_stream_event_source(stream));

#ifndef BYPASS_MIXER
      if (!al_attach_audio_stream_to_mixer(stream, mixer)) {
         log_printf("al_attach_audio_stream_to_mixer failed.\n");
         continue;
      }
#else
      if (!al_attach_audio_stream_to_voice(stream, voice)) {
         abort_example("al_attach_audio_stream_to_voice failed.\n");
      }
#endif

      log_printf("Playing %s ... Waiting for stream to finish ", filename);
      do {
         al_wait_for_event(queue, &event);
         if(event.type == ALLEGRO_EVENT_AUDIO_STREAM_FINISHED)
            playing = false;
      } while (playing);
      log_printf("\n");

      al_destroy_event_queue(queue);
      al_destroy_audio_stream(stream);
   }
   log_printf("Done\n");

#ifndef BYPASS_MIXER
   al_destroy_mixer(mixer);
#endif
   al_destroy_voice(voice);

   al_uninstall_audio();
   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
