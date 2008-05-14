/*
 * Allegro5 Ogg Vorbis reader.
 * Can load samples and do streaming
 * author: Ryan Dickie (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"

#ifdef ALLEGRO_CFG_ACODEC_VORBIS

#include <vorbis/vorbisfile.h>

/* note: decoding library returns floats..
 * so i always return 16-bit (most commonly supported)
 * TODO: figure out a way for 32-bit float or 24-bit
 * support in the al framework while keeping API simple to use
 */
ALLEGRO_SAMPLE* al_load_sample_oggvorbis(const char *filename)
{
   const int endian = 0; /* 0 for Little-Endian, 1 for Big-Endian */
   int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   int signedness = 1; /* 0  for unsigned, 1 for signed */
   const int packet_size = 4096; /* suggestion for size to read at a time */
   OggVorbis_File vf;
   vorbis_info* vi;
   FILE* file;
   char* buffer;
   long pos;
   ALLEGRO_SAMPLE* sample;
   int channels;
   long rate;
   long total_samples;
   int bitStream;
   long total_size;

   file = fopen(filename, "rb");
   if (file == NULL)
      return NULL;


   if(ov_open_callbacks(file, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
   {
      fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
      fclose(file);
      return NULL;
   }

   vi = ov_info(&vf, -1);

   channels = vi->channels;
   rate = vi->rate;
   total_samples = ov_pcm_total(&vf,-1);
   bitStream = -1;
   total_size = total_samples * channels * word_size;

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);
 
   /* al_sample_destroy will clean it! */
   buffer = (char*) malloc(total_size);
   pos = 0;
   while (true)
   {
      long read = ov_read(&vf, buffer + pos, packet_size, endian, word_size, signedness, &bitStream);
      pos += read;
      if (read == 0)
         break;
   }
   ov_clear(&vf);
   fclose(file);

   sample = al_sample_create(
            buffer,
            total_samples,
            rate,
            _al_word_size_to_depth_conf(word_size),
            _al_count_to_channel_conf(channels),
            TRUE
         );

   return sample;
}

/* Used in the stream callback */
typedef struct AL_OV_DATA {
   OggVorbis_File* vf;
   vorbis_info* vi;
   FILE* file;
   int bitStream;
} AL_OV_DATA;

/* to be called when stream is destroyed */
void _ogg_stream_close(ALLEGRO_STREAM* stream)
{
   AL_OV_DATA* ex_data = (AL_OV_DATA*) stream->ex_data;
   ov_clear(ex_data->vf);
   fclose(ex_data->file);
   free(ex_data->vf);
   free(ex_data);
   stream->ex_data = NULL;
}

bool _ogg_stream_update(ALLEGRO_STREAM* stream, void* data, unsigned long buf_size)
{
   AL_OV_DATA* ex_data = (AL_OV_DATA*) stream->ex_data;

   const int endian = 0; /* 0 for Little-Endian, 1 for Big-Endian */
   const int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   const int signedness = 1; /* 0  for unsigned, 1 for signed */
   int channels = ex_data->vi->channels;

   unsigned long pos = 0;
   while (pos < buf_size)
   {
      unsigned long read = ov_read(ex_data->vf, (char*)data + pos,
                                   buf_size - pos, endian, word_size,
                                   signedness, &ex_data->bitStream);
      pos += read;

      /* if nothing read then now to silence from here to the end. */
      if (read == 0)
      {
         memset((char*)data + pos, 0, buf_size - pos);
         return false;
      }
   }
   return true;
}

ALLEGRO_STREAM* al_load_stream_oggvorbis(const char *filename)
{
   const int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   const int signedness = 1; /* 0  for unsigned, 1 for signed */
   FILE* file = NULL;
   OggVorbis_File* vf;
   vorbis_info* vi;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   AL_OV_DATA* ex_data;
   ALLEGRO_STREAM* stream;

   file = fopen(filename, "rb");
   if (file == NULL)
      return NULL;

   vf = (OggVorbis_File*) malloc(sizeof(OggVorbis_File));
   if(ov_open_callbacks(file, vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
   {
      fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
      fclose(file);
      return NULL;
   }

   vi = ov_info(vf, -1);
   channels = vi->channels;
   rate = vi->rate;
   total_samples = ov_pcm_total(vf,-1);
   total_size = total_samples * channels * word_size;


   ex_data = (AL_OV_DATA*) malloc(sizeof(AL_OV_DATA));
   ex_data->vf = vf;
   ex_data->vi = vi;
   ex_data->file = file;
   ex_data->bitStream = -1;

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);
 
   stream = al_stream_create(
            rate,
            _al_word_size_to_depth_conf(word_size),
            _al_count_to_channel_conf(channels),
            _ogg_stream_update,
            _ogg_stream_close
         );

   stream->ex_data = ex_data;

   return stream;
}

#endif /* ALLEGRO_CFG_ACODEC_VORBIS */
