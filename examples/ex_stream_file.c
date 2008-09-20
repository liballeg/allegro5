/*
 * An example program that plays a file from the disk using Allegro5
 * streaming API. The file is being read in small chunks and played on the
 * sound device instead of being loaded at once.
 *
 * usage: ./ex_stream_file file.[wav,ogg...] ...
 *
 * by Milan Mimica
 */

#include "allegro5/allegro5.h"
#include "allegro5/acodec.h"
#include <stdio.h>

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

   if (al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT)) {
       fprintf(stderr, "Could not init sound!\n");
       return 1;
   }

   voice = al_voice_create(44100, ALLEGRO_AUDIO_DEPTH_INT16,
                           ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      fprintf(stderr, "Could not create ALLEGRO_VOICE.\n");
      return 1;
   }
   fprintf(stderr, "Voice created.\n");

#ifndef BYPASS_MIXER
   mixer = al_mixer_create(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                           ALLEGRO_CHANNEL_CONF_2);
   if (!mixer) {
      fprintf(stderr, "Could not create ALLEGRO_MIXER.\n");
      return 1;
   }
   fprintf(stderr, "Mixer created.\n");

   if (al_voice_attach_mixer(voice, mixer) != 0) {
      fprintf(stderr, "al_voice_attach_mixer failed.\n");
      return 1;
   }
#endif

   for (i = 1; i < argc; ++i)
   {
      ALLEGRO_STREAM* stream;
      const char*     filename = argv[i];
      bool playing;

      stream = al_stream_from_file(4, 2048, filename);
      if (!stream) {
         fprintf(stderr, "Could not create an ALLEGRO_STREAM from '%s'!\n",
                 filename);
         continue;
      }
      fprintf(stderr, "Stream created from '%s'.\n", filename);

#ifndef BYPASS_MIXER
      if (al_mixer_attach_stream(mixer, stream) != 0) {
         fprintf(stderr, "al_mixer_attach_stream failed.\n");
         continue;
      }
#else
      if (al_voice_attach_stream(voice, stream) != 0) {
         fprintf(stderr, "al_voice_attach_stream failed.\n");
         return 1;
      }
#endif

      fprintf(stderr, "Playing %s ... Waiting for stream to finish ", filename);
      do {
         al_rest(0.1);
         al_stream_get_bool(stream, ALLEGRO_AUDIOPROP_PLAYING, &playing);
      }
      while (playing);
      fprintf(stderr, "\n");

      al_stream_destroy(stream);
   }

#ifndef BYPASS_MIXER
   al_mixer_destroy(mixer);
#endif
   al_voice_destroy(voice);

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
