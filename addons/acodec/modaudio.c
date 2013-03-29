/*
 * Allegro MOD reader
 * author: Matthew Leverton
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_system.h"
#include "acodec.h"
#include "helper.h"

#ifndef ALLEGRO_CFG_ACODEC_MODAUDIO
   #error configuration problem, ALLEGRO_CFG_ACODEC_MODAUDIO not set
#endif

#include <dumb.h>
#include <stdio.h>

ALLEGRO_DEBUG_CHANNEL("acodec")


/* forward declarations */
static size_t modaudio_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size);
static bool modaudio_stream_rewind(ALLEGRO_AUDIO_STREAM *stream);
static bool modaudio_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time);
static double modaudio_stream_get_position(ALLEGRO_AUDIO_STREAM *stream);
static double modaudio_stream_get_length(ALLEGRO_AUDIO_STREAM *stream);
static bool modaudio_stream_set_loop(ALLEGRO_AUDIO_STREAM *stream,
   double start, double end);
static void modaudio_stream_close(ALLEGRO_AUDIO_STREAM *stream);


typedef struct MOD_FILE
{
   DUH *duh;
   DUH_SIGRENDERER *sig;
   ALLEGRO_FILE *fh;
   double length;
   long loop_start, loop_end;
} MOD_FILE;


static bool libdumb_loaded = false;


/* dynamic loading support (Windows only currently) */
#ifdef ALLEGRO_CFG_ACODEC_DUMB_DLL
static void *dumb_dll = NULL;
#endif

static struct
{
   long (*duh_render)(DUH_SIGRENDERER *, int, int, float, float, long, void *);
   long (*duh_sigrenderer_get_position)(DUH_SIGRENDERER *);
   void (*duh_end_sigrenderer)(DUH_SIGRENDERER *);
   void (*unload_duh)(DUH *);
   DUH_SIGRENDERER *(*duh_start_sigrenderer)(DUH *, int, int, long);
   DUMBFILE *(*dumbfile_open_ex)(void *, DUMBFILE_SYSTEM *);
   long (*duh_get_length)(DUH *);
   void (*dumb_exit)(void);
   void (*register_dumbfile_system)(DUMBFILE_SYSTEM *);
   DUH *(*dumb_read_it)(DUMBFILE *);
   DUH *(*dumb_read_xm)(DUMBFILE *);
   DUH *(*dumb_read_s3m)(DUMBFILE *);
   DUH *(*dumb_read_mod)(DUMBFILE *);
} lib;


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
   /* Don't actually close f here. */
   (void)f;
}


/* Stream Functions */

static size_t modaudio_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   MOD_FILE *const df = stream->extra;
   
   /* the mod files are stereo and 16-bit */
   const int sample_size = 4;
   size_t written;
   size_t i;
   
   written = lib.duh_render(df->sig, 16, 0, 1.0, 65536.0 / 44100.0,
      buf_size / sample_size, data) * sample_size;

   /* Fill the remainder with silence */
   for (i = written; i < buf_size; ++i)
      ((int *)data)[i] = 0x8000;
   
   /* Check to see if a loop is set */
   if (df->loop_start != -1 && 
      df->loop_end < lib.duh_sigrenderer_get_position(df->sig)) {
         modaudio_stream_seek(stream, df->loop_start / 65536.0);
   }   
   
   return written;
}

static void modaudio_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   _al_acodec_stop_feed_thread(stream);
      
   lib.duh_end_sigrenderer(df->sig);
   lib.unload_duh(df->duh);
   if (df->fh)
      al_fclose(df->fh);
}

static bool modaudio_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   lib.duh_end_sigrenderer(df->sig);
   df->sig = lib.duh_start_sigrenderer(df->duh, 0, 2, 0);
   return true;
}

static bool modaudio_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time)
{
   MOD_FILE *const df = stream->extra;
   
   lib.duh_end_sigrenderer(df->sig);
   df->sig = lib.duh_start_sigrenderer(df->duh, 0, 2, time * 65536);
   
   return false;
}

static double modaudio_stream_get_position(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   return lib.duh_sigrenderer_get_position(df->sig) / 65536.0;
}

static double modaudio_stream_get_length(ALLEGRO_AUDIO_STREAM *stream)
{
   MOD_FILE *const df = stream->extra;
   return df->length;
}

static bool modaudio_stream_set_loop(ALLEGRO_AUDIO_STREAM *stream,
   double start, double end)
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
   DUH_SIGRENDERER *sig = NULL;
   DUH *duh = NULL;
   int64_t start_pos = -1;
   
   df = lib.dumbfile_open_ex(f, &dfs_f);
   if (!df)
      return NULL;
      
   start_pos = al_ftell(f);

   duh = loader(df);
   if (!duh) {
      goto Error;
   }

   sig = lib.duh_start_sigrenderer(duh, 0, 2, 0);
   if (!sig) {
      goto Error;
   }

   stream = al_create_audio_stream(buffer_count, samples, 44100,
      ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2); 

   if (stream) {
      MOD_FILE *mf = al_malloc(sizeof(MOD_FILE));
      mf->duh = duh;
      mf->sig = sig;
      mf->fh = NULL;
      mf->length = lib.duh_get_length(duh) / 65536.0;
      if (mf->length < 0)
         mf->length = 0;
      mf->loop_start = -1;
      mf->loop_end = -1;

      stream->extra = mf;
      stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
      stream->feeder = modaudio_stream_update;
      stream->unload_feeder = modaudio_stream_close;
      stream->rewind_feeder = modaudio_stream_rewind;
      stream->seek_feeder = modaudio_stream_seek;
      stream->get_feeder_position = modaudio_stream_get_position;
      stream->get_feeder_length = modaudio_stream_get_length;
      stream->set_feeder_loop = modaudio_stream_set_loop;
      al_start_thread(stream->feed_thread);
   }
   else {
      goto Error;
   }

   return stream;

Error:

   if (sig) {
      lib.duh_end_sigrenderer(sig);
   }

   if (duh) {
      lib.unload_duh(duh);
   }

   /* try to return back to where we started to load */
   if (start_pos != -1)
      al_fseek(f, start_pos, ALLEGRO_SEEK_SET);

   return NULL;
}

static void shutdown_libdumb(void)
{
   if (libdumb_loaded) {
      lib.dumb_exit();
      libdumb_loaded = false;
   }

#ifdef ALLEGRO_CFG_ACODEC_DUMB_DLL
   if (dumb_dll) {
      _al_close_library(dumb_dll);
      dumb_dll = NULL;
   }
#endif
}

static bool init_libdumb(void)
{
   if (libdumb_loaded) {
      return true;
   }

#ifdef ALLEGRO_CFG_ACODEC_DUMB_DLL
   dumb_dll = _al_open_library(ALLEGRO_CFG_ACODEC_DUMB_DLL);
   if (!dumb_dll) {
      ALLEGRO_WARN("Could not load " ALLEGRO_CFG_ACODEC_DUMB_DLL "\n");
      return false;
   }

   #define INITSYM(x)                                                         \
      do                                                                      \
      {                                                                       \
         lib.x = _al_import_symbol(dumb_dll, #x);                             \
         if (lib.x == 0) {                                                    \
            ALLEGRO_ERROR("undefined symbol in lib structure: " #x "\n");     \
            return false;                                                     \
         }                                                                    \
      } while(0)
#else
   #define INITSYM(x)   (lib.x = (x))
#endif

   _al_add_exit_func(shutdown_libdumb, "shutdown_libdumb");

   memset(&lib, 0, sizeof(lib));

   INITSYM(duh_render);
   INITSYM(duh_sigrenderer_get_position);
   INITSYM(duh_end_sigrenderer);
   INITSYM(unload_duh);
   INITSYM(duh_start_sigrenderer);
   INITSYM(dumbfile_open_ex);
   INITSYM(duh_get_length);
   INITSYM(dumb_exit);
   INITSYM(register_dumbfile_system);
   INITSYM(dumb_read_it);
   INITSYM(dumb_read_xm);
   INITSYM(dumb_read_s3m);
   INITSYM(dumb_read_mod);

   dfs.open = dfs_open;
   dfs.skip = dfs_skip;
   dfs.getc = dfs_getc;
   dfs.getnc = dfs_getnc;
   dfs.close = dfs_close;
   
   /* Set up DUMB's default I/O to go through Allegro... */
   lib.register_dumbfile_system(&dfs);
   
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

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

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
   
   if (!stream) {
      al_fclose(f);
      return NULL;
   }

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

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

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
   
   if (!stream) {
      al_fclose(f);
      return NULL;
   }
      
   ((MOD_FILE *)stream->extra)->fh = f;
   
   return stream;
}

ALLEGRO_AUDIO_STREAM *_al_load_mod_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, lib.dumb_read_mod);
}

ALLEGRO_AUDIO_STREAM *_al_load_it_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, lib.dumb_read_it);
}

ALLEGRO_AUDIO_STREAM *_al_load_xm_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, lib.dumb_read_xm);
}

ALLEGRO_AUDIO_STREAM *_al_load_s3m_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;

   return mod_stream_init(f, buffer_count, samples, lib.dumb_read_s3m);
}

/* vim: set sts=3 sw=3 et: */
