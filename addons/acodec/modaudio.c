/*
 * Allegro MOD reader
 * author: Matthew Leverton
 */


#include "allegro5/allegro5.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_memory.h"
#include "acodec.h"

#include <dumb.h>
#include <stdio.h>

static size_t duh_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size);
static bool duh_stream_rewind(ALLEGRO_AUDIO_STREAM *stream);
static bool duh_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time);
static double duh_stream_get_position(ALLEGRO_AUDIO_STREAM *stream);
static double duh_stream_get_length(ALLEGRO_AUDIO_STREAM *stream);
static bool duh_stream_set_loop(ALLEGRO_AUDIO_STREAM *stream, double start, double end);
static void duh_stream_close(ALLEGRO_AUDIO_STREAM *stream);

typedef struct MOD_FILE
{
   DUH *duh;
   DUH_SIGRENDERER *sig;
   ALLEGRO_FILE *fh;
   double length;
   long loop_start, loop_end;
} MOD_FILE;

static bool libdumb_loaded = false;

/* Set up DUMB's file system */
static DUMBFILE_SYSTEM dfs, dfs_f;

static void *dfs_open(const char *filename)
{
   return al_fopen(filename, "rb");
}

static int dfs_skip(void *f, long n)
{
   return al_fseek(f, n, ALLEGRO_SEEK_CUR) ? 0 : -1;
}

static int dfs_getc(void *f)
{
   return al_fgetc(f);
}

static long dfs_getnc(char *ptr, long n, void *f)
{
   return al_fread(f, ptr, n);
}

static void dfs_close(void *f)
{
   al_fclose(f);
}

/* Stream Functions */

static size_t duh_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   MOD_FILE *const df = stream->extra;
   
   /* the mod files are stereo and 16-bit */
   const int sample_size = 4;
   size_t written;
   size_t i;
   
   written = duh_render(df->sig, 16, 0, 1.0, 65536.0 / 44100.0,
      buf_size / sample_size, data) * sample_size;

   /* Fill the remainder with silence */
   for (i = written; i < buf_size; ++i)
      ((int *)data)[i] = 0x8000;
   
   /* Check to see if a loop is set */
   if (df->loop_start != -1 && 
      df->loop_end < duh_sigrenderer_get_position(df->sig)) {
         duh_stream_seek(stream, df->loop_start / 65536.0);
   }   
   
   return written;
}

static void duh_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   
   duh_end_sigrenderer(df->sig);
   unload_duh(df->duh);
   if (df->fh)
      al_fclose(df->fh);
}

static bool duh_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   duh_end_sigrenderer(df->sig);
   df->sig = duh_start_sigrenderer(df->duh, 0, 2, 0);   
   return true;
}

static bool duh_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time)
{
   MOD_FILE *const df = stream->extra;
   
   duh_end_sigrenderer(df->sig);
   df->sig = duh_start_sigrenderer(df->duh, 0, 2, time * 65536);
   
   return false;
}

static double duh_stream_get_position(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   return duh_sigrenderer_get_position(df->sig) / 65536.0;
}

static double duh_stream_get_length(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   return df->length;
}

static bool duh_stream_set_loop(ALLEGRO_AUDIO_STREAM *stream, double start, double end)
{
   MOD_FILE *const df = stream->extra;
   
   df->loop_start = start * 65536;
   df->loop_end = end * 65536;
   
   return true;
}

/* Create the Allegro stream */

static ALLEGRO_AUDIO_STREAM *mod_stream_init(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples, DUH *(loader)(DUMBFILE *))
{
   ALLEGRO_AUDIO_STREAM *stream;
   DUMBFILE *df;
   DUH_SIGRENDERER *sig;
   DUH *duh;
   int64_t start_pos = -1;
   
   df = dumbfile_open_ex(f, &dfs_f);
   if (!df)
      return NULL;
      
   start_pos = al_ftell(f);

   duh = loader(df);
   if (!duh) {
      /* try to return back to where we started to load */
      if (start_pos != -1)
         al_fseek(f, start_pos, ALLEGRO_SEEK_SET);
      return NULL;
   }

   sig = duh_start_sigrenderer(duh, 0, 2, 0);
   if (!sig) {
      unload_duh(duh);
      return NULL;
   }

   stream = al_create_audio_stream(buffer_count, samples, 44100,
      ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2); 

   if (stream) {
      MOD_FILE *mf = malloc(sizeof(MOD_FILE));
      mf->duh = duh;
      mf->sig = sig;
      mf->fh = NULL;
      mf->length = duh_get_length(duh) / 65536.0;
      if (mf->length < 0)
         mf->length = 0;
      mf->loop_start = -1;
      mf->loop_end = -1;

      stream->extra = mf;
      stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
      stream->feeder = duh_stream_update;
      stream->unload_feeder = duh_stream_close;
      stream->rewind_feeder = duh_stream_rewind;
      stream->seek_feeder = duh_stream_seek;
      stream->get_feeder_position = duh_stream_get_position;
      stream->get_feeder_length = duh_stream_get_length;
      stream->set_feeder_loop = duh_stream_set_loop;
      al_start_thread(stream->feed_thread);
   }

   return stream;
}

static void shutdown_libdumb(void)
{
   if (libdumb_loaded) {
      dumb_exit();
      libdumb_loaded = false;
   }
}

static bool init_libdumb(void)
{
   if (libdumb_loaded) {
      return true;
   }

   _al_add_exit_func(shutdown_libdumb, "shutdown_libdumb");
    
   dfs.open = dfs_open;
   dfs.skip = dfs_skip;
   dfs.getc = dfs_getc;
   dfs.getnc = dfs_getnc;
   dfs.close = dfs_close;
   
   /* Set up DUMB's default I/O to go through Allegro... */
   register_dumbfile_system(&dfs);
   
   /* But we'll actually use them through this version: */
   dfs_f = dfs;
   dfs_f.open = NULL;
   dfs_f.close = NULL;

   libdumb_loaded = true;
   return true;
}

ALLEGRO_AUDIO_STREAM *_al_load_mod_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);
              
   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   stream = _al_load_mod_audio_stream_f(f, buffer_count, samples);
   
   if (!stream)
      al_fclose(f);
      
   ((MOD_FILE *)stream->extra)->fh = f;
   
   return stream;
}

ALLEGRO_AUDIO_STREAM *_al_load_it_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);
              
   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   stream = _al_load_it_audio_stream_f(f, buffer_count, samples);
   
   if (!stream)
      al_fclose(f);
      
   ((MOD_FILE *)stream->extra)->fh = f;
   
   return stream;
}

ALLEGRO_AUDIO_STREAM *_al_load_xm_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);
              
   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   stream = _al_load_xm_audio_stream_f(f, buffer_count, samples);
   
   if (!stream)
      al_fclose(f);
      
   ((MOD_FILE *)stream->extra)->fh = f;
   
   return stream;
}

ALLEGRO_AUDIO_STREAM *_al_load_s3m_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);
              
   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   stream = _al_load_s3m_audio_stream_f(f, buffer_count, samples);
   
   if (!stream)
      al_fclose(f);
      
   ((MOD_FILE *)stream->extra)->fh = f;
   
   return stream;
}

ALLEGRO_AUDIO_STREAM *_al_load_mod_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, dumb_read_mod);
}

ALLEGRO_AUDIO_STREAM *_al_load_it_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, dumb_read_it);
}

ALLEGRO_AUDIO_STREAM *_al_load_xm_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, dumb_read_xm);
}

ALLEGRO_AUDIO_STREAM *_al_load_s3m_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, dumb_read_s3m);
}

/* vim: set sts=3 sw=3 et: */
