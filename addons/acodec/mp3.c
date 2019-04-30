/*
 * Allegro5 MP3 reader.
 * Requires MiniMP3 from https://github.com/lieff/minimp3
 * author: Mark Watkin (pmprog) 2019
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_system.h"
#include "acodec.h"
#include "helper.h"

#define MINIMP3_IMPLEMENTATION
#include <minimp3.h>
#include <minimp3_ex.h>

ALLEGRO_DEBUG_CHANNEL("acodec")

static bool mp3_libinit = false;
static mp3dec_t mp3d;

void mp3_initminimp3(void);

ALLEGRO_SAMPLE *_al_load_mp3(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Could not open file '%s'.\n", filename);
      return NULL;
   }

   spl = _al_load_mp3_f(f);

   al_fclose(f);

   return spl;
}

ALLEGRO_SAMPLE *_al_load_mp3_f(ALLEGRO_FILE *f)
{
   mp3_initminimp3(); // Make sure library is initialised
   
   mp3dec_file_info_t info;
   ALLEGRO_SAMPLE *spl = NULL;

   // Read our file size
   int64_t filesize = al_fsize(f);
   if (filesize == -1) {
      ALLEGRO_WARN("Could not determine file size.\n");
      return NULL;
   }

   // Allocate buffer and read all the file
   uint8_t* mp3data = (uint8_t*)al_malloc(filesize);
   size_t readbytes = al_fread(f, mp3data, filesize);
   if (readbytes != (size_t)filesize) {
      ALLEGRO_WARN("Failed to read file into memory.\n");
      al_free(mp3data);
      return NULL;
   }

   // Decode the file contents, and copy to a new buffer
   mp3dec_load_buf(&mp3d, mp3data, filesize, &info, NULL, NULL);
   al_free(mp3data);

   if (info.buffer == NULL) {
      ALLEGRO_WARN("Could not decode MP3.\n");
      return NULL;
   }

   // Create sample from info variable
   spl = al_create_sample(info.buffer, info.samples, info.hz, _al_word_size_to_depth_conf(2), _al_count_to_channel_conf(info.channels), true);
   
   return spl;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream(const char *filename, size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Could not open file '%s'.\n", filename);
      return NULL;
   }

   stream = _al_load_mp3_audio_stream_f(f, buffer_count, samples);
   if (!stream) {
      al_fclose(f);
   }

   return stream;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream_f(ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples)
{
   (void)f;
   (void)buffer_count;
   (void)samples;
   mp3_initminimp3(); // Make sure library is initialised
   ALLEGRO_WARN("Streaming for MP3 files not implemented.\n");
   return NULL;
}

void mp3_initminimp3()
{
   if(!mp3_libinit) {
      mp3dec_init(&mp3d);
      mp3_libinit = true;
   }
}
