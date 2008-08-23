/* Check that we can play some samples. */
/* Currently requires libsndfile. */

#include <stdio.h>
#include <sndfile.h>

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"

#define MAX_SAMPLES  32


ALLEGRO_SAMPLE *ex_load_sample_sndfile(const char *filename);


int main(int argc, const char *argv[])
{
   ALLEGRO_VOICE *voice;
   ALLEGRO_MIXER *mixer;
   ALLEGRO_SAMPLE *spl[MAX_SAMPLES];
   char *buf = NULL;
   int i;
   int nsamples;

   if (argc < 2) {
      TRACE("usage: ex_kcm_play files ...\n");
      return 1;
   }

   al_init();

   if (al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT) != 0) {
      fprintf(stderr, "al_audio_init failed\n");
      return 1;
   }

   voice = al_voice_create(44100, ALLEGRO_AUDIO_16_BIT_INT,
      ALLEGRO_AUDIO_2_CH);
   if (!voice) {
      fprintf(stderr, "al_voice_create failed.\n");
      return 1;
   }

   mixer = al_mixer_create(44100, ALLEGRO_AUDIO_32_BIT_FLOAT,
      ALLEGRO_AUDIO_2_CH);
   if (!mixer) {
      fprintf(stderr, "al_mixer_create failed.\n");
      return 1;
   }

   nsamples = 0;
   for (i = 1; i < argc && nsamples < MAX_SAMPLES; i++) {
      spl[nsamples] = ex_load_sample_sndfile(argv[i]);
      if (!spl[nsamples]) {
         TRACE("load sample failed.\n");
         return 1;
      }
      nsamples++;
   }

   if (al_voice_attach_mixer(voice, mixer) != 0) {
      TRACE("al_voice_attach_mixer failed.\n");
      return 1;
   }

   for (i = 0; i < nsamples; i++) {
      /* XXX currently crashes if a sample is attached to a mixer
       * which is not itself attached; is that supposed to be allowed?
       */
      if (al_mixer_attach_sample(mixer, spl[i]) != 0) {
         fprintf(stderr, "al_mixer_attach_sample failed.\n");
         return 1;
      }
      if (al_sample_set_bool(spl[i], ALLEGRO_AUDIO_PLAYING, true) != 0) {
         fprintf(stderr, "al_sample_set_bool playing failed.\n");
         return 1;
      }
   }

   al_rest(3.0);

   for (i = 0; i < nsamples; i++) {
      al_sample_destroy(spl[i]);
   }
   al_mixer_destroy(mixer);
   al_voice_destroy(voice);
   al_audio_deinit();

   return 0;
}
END_OF_MAIN()


/* Copied from acodec for now, while acodec is designed to work with the
 * `audio' addon.
 */

ALLEGRO_AUDIO_ENUM _get_depth_enum( int format , int* word_size)
{
   switch (format&0xFFFF)
   {
      case SF_FORMAT_PCM_U8:
         *word_size = 1;
         return ALLEGRO_AUDIO_8_BIT_UINT; 

      case SF_FORMAT_PCM_16:
         *word_size = 2;
         return ALLEGRO_AUDIO_16_BIT_INT;
   
      case SF_FORMAT_PCM_24:
         *word_size = 3;
         return ALLEGRO_AUDIO_24_BIT_INT;
   
      case SF_FORMAT_FLOAT:
         *word_size = 4;
         return ALLEGRO_AUDIO_32_BIT_FLOAT;
    
      default:
         fprintf(stderr, "Unsupported sndfile depth format (%X)\n",format);
         *word_size = 0;
         return 0;
   }
}

/* FIXME: use the allegro provided helpers */
ALLEGRO_AUDIO_ENUM _al_count_to_channel_conf(int num_channels)
{
   switch (num_channels)
   {
      case 1:
         return ALLEGRO_AUDIO_1_CH;
      case 2:
         return ALLEGRO_AUDIO_2_CH;
      case 3:
         return ALLEGRO_AUDIO_3_CH;
      case 4:
         return ALLEGRO_AUDIO_4_CH;
      case 6:
         return ALLEGRO_AUDIO_5_1_CH;
      case 7:
         return ALLEGRO_AUDIO_6_1_CH;
      case 8:
         return ALLEGRO_AUDIO_7_1_CH;
      default:
         return 0;
   }
}

ALLEGRO_SAMPLE *ex_load_sample_sndfile(const char *filename)
{
   ALLEGRO_AUDIO_ENUM depth; 
   SF_INFO sfinfo;
   SNDFILE *sndfile;
   int word_size;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   void* buffer;
   ALLEGRO_SAMPLE *sample;

   sfinfo.format = 0;
   sndfile = sf_open(filename, SFM_READ, &sfinfo); 

   if (sndfile == NULL)
      return NULL;

   word_size = 0;
   depth = _get_depth_enum(sfinfo.format,&word_size);  
   channels = sfinfo.channels;
   rate = sfinfo.samplerate;
   total_samples = sfinfo.frames;
   total_size = total_samples * channels * word_size; 

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);

   buffer = malloc(total_size);
   if (buffer == NULL) {
      sf_close(sndfile);
      return NULL;
   }

   if (depth == ALLEGRO_AUDIO_16_BIT_INT) {
      sf_readf_short(sndfile, buffer, total_samples);
   }
   else if (depth == ALLEGRO_AUDIO_32_BIT_FLOAT) {
      sf_readf_float(sndfile, buffer, total_samples);
   }
   else {
      sf_read_raw(sndfile, buffer, total_samples);
   }

   sf_close(sndfile);

   sample = al_sample_create(buffer, total_samples, rate, depth,
         _al_count_to_channel_conf(channels));

   return sample;
}

/* vim: set sts=3 sw=3 et: */
