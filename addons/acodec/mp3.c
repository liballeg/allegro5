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

void mp3_initminimp3();

ALLEGRO_SAMPLE *_al_load_mp3(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

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
	if (filesize == -1)
		return NULL;

	// Allocate buffer and read all the file
	size_t fsize = (size_t)al_fsize(f);
	uint8_t* mp3data = (uint8_t*)al_malloc(fsize);
	size_t readbytes = al_fread(f, mp3data, fsize);
	if (readbytes != fsize)
	{
		al_free(mp3data);
		return NULL;
	}

	// Decode the file contents, and copy to a new buffer
	mp3dec_load_buf(&mp3d, mp3data, filesize, &info, NULL, NULL);
	uint8_t* pcmdata = (uint8_t*)al_malloc(info.samples * sizeof(int16_t));
	memcpy(pcmdata, info.buffer, info.samples * sizeof(int16_t));

	// Free file copy, and buffer (which is copied to pcmdata
	al_free(mp3data);
	al_free(info.buffer);

	// Create sample from info variable
	spl = al_create_sample(pcmdata, info.samples, info.hz, _al_word_size_to_depth_conf(2), _al_count_to_channel_conf(info.channels), true);
	
   return spl;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream(const char *filename, size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   stream = _al_load_mp3_audio_stream_f(f, buffer_count, samples);
   if (!stream) {
      al_fclose(f);
   }

   return stream;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream_f(ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples)
{
	mp3_initminimp3(); // Make sure library is initialised
	return NULL;
}

void mp3_initminimp3()
{
	if(!mp3_libinit)
	{
		mp3dec_init(&mp3d);
		mp3_libinit = true;
	}
}