/*
 * Allegro MOD reader
 * author: Matthew Leverton
 */

#define _FILE_OFFSET_BITS 64
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

// We use the deprecated duh_render in DUMB 2.
#define DUMB_DECLARE_DEPRECATED
#include <dumb.h>
#include <stdio.h>

ALLEGRO_DEBUG_CHANNEL("acodec")

/* In addition to DUMB 0.9.3 at http://dumb.sourceforge.net/,
 * we support kode54's fork of DUMB at https://github.com/kode54/dumb.
 * The newest version before kode54's fork was DUMB 0.9.3, and the first
 * forked version that A5 supports is already 2.0.0.
 */

/* forward declarations */
static bool init_libdumb(void);
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

/*
 * dumb_off_t is introduced in DUMB 2.0, it's a signed 64+ bit integer.
 * dumb_ssize_t is a platform-independent signed size_t.
 * DUMB 0.9.3 expects long wherever dumb_off_t or dumb_ssize_t appear.
 *
 * _al_dumb_size_t takes care of another, less documented, difference
 * between the versions.
 */
#if (DUMB_MAJOR_VERSION) < 2
typedef long dumb_off_t;
typedef long dumb_ssize_t;
typedef long _al_dumb_size_t;
#else
typedef size_t _al_dumb_size_t;
#endif

static struct
{
   long (*duh_sigrenderer_get_position)(DUH_SIGRENDERER *);
   void (*duh_end_sigrenderer)(DUH_SIGRENDERER *);
   void (*unload_duh)(DUH *);
   DUH_SIGRENDERER *(*duh_start_sigrenderer)(DUH *, int, int, long);
   dumb_off_t (*duh_get_length)(DUH *);
   void (*dumb_exit)(void);
   DUMB_IT_SIGRENDERER *(*duh_get_it_sigrenderer)(DUH_SIGRENDERER *);
   void (*dumb_it_set_loop_callback)(DUMB_IT_SIGRENDERER *, int (*)(void *), void *);
   void (*dumb_it_set_xm_speed_zero_callback)(DUMB_IT_SIGRENDERER *, int (*)(void *), void *);
   int (*dumb_it_callback_terminate)(void *);

#if (DUMB_MAJOR_VERSION) >= 2
   /*
    * DUMB 2.0 deprecates duh_render, and suggests instead duh_render_int
    * and duh_render_float. But I (Simon N.) don't know how to use those.
    * I have already written some documentation for DUMB 2.0 that got merged
    * upstream, but I haven't gotten around to this issue yet.
    */
   long (*duh_render)(DUH_SIGRENDERER *,
      int, int, float, float, long, void *); /* deprecated */
   void (*register_dumbfile_system)(const DUMBFILE_SYSTEM *);
   DUMBFILE *(*dumbfile_open_ex)(void *, const DUMBFILE_SYSTEM *);
   DUH *(*dumb_read_any)(DUMBFILE *, int, int);
#else
   long (*duh_render)(DUH_SIGRENDERER *,
      int, int, float, float, long, void *);
   void (*register_dumbfile_system)(DUMBFILE_SYSTEM *);
   DUMBFILE *(*dumbfile_open_ex)(void *, DUMBFILE_SYSTEM *);
   DUH *(*dumb_read_mod)(DUMBFILE *);
   DUH *(*dumb_read_it)(DUMBFILE *);
   DUH *(*dumb_read_xm)(DUMBFILE *);
   DUH *(*dumb_read_s3m)(DUMBFILE *);
#endif
} lib;

/* Set up DUMB's file system */
static DUMBFILE_SYSTEM dfs, dfs_f;

static void *dfs_open(const char *filename)
{
   return al_fopen(filename, "rb");
}

static int dfs_skip(void *f, dumb_off_t n)
{
   return al_fseek(f, n, ALLEGRO_SEEK_CUR) ? 0 : -1;
}

static int dfs_getc(void *f)
{
   return al_fgetc(f);
}

static dumb_ssize_t dfs_getnc(char *ptr, _al_dumb_size_t n, void *f)
{
   return al_fread(f, ptr, n);
}

static void dfs_close(void *f)
{
   /* Don't actually close f here. */
   (void)f;
}

#if (DUMB_MAJOR_VERSION) >= 2
static int dfs_seek(void *f, dumb_off_t n)
{
   return al_fseek(f, n, ALLEGRO_SEEK_SET) ? 0 : -1;
}

static dumb_off_t dfs_get_size(void *f)
{
   return al_fsize(f);
}
#endif

/* Stream Functions */

static size_t modaudio_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   MOD_FILE *const df = stream->extra;

   /* the mod files are stereo and 16-bit */
   const int sample_size = 4;
   size_t written = 0;
   size_t i;

   DUMB_IT_SIGRENDERER *it_sig = lib.duh_get_it_sigrenderer(df->sig);
   if (it_sig) {
      lib.dumb_it_set_loop_callback(it_sig,
         stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONCE
         ? lib.dumb_it_callback_terminate : NULL, NULL);
   }

   while (written < buf_size) {
      long size_to_read = (buf_size - written) / sample_size;
      long position = lib.duh_sigrenderer_get_position(df->sig);
      bool loop = false;
      if (stream->spl.loop != _ALLEGRO_PLAYMODE_STREAM_ONCE && df->loop_end != -1 &&
          position + 65536 * size_to_read / 44100 >= df->loop_end) {
         size_to_read = (df->loop_end - position) * 44100 / 65536;
         if (size_to_read < 0)
            size_to_read = 0;
         loop = true;
      }
      written += lib.duh_render(df->sig, 16, 0, 1.0, 65536.0 / 44100.0,
         size_to_read, &(((char *)data)[written])) * sample_size;
      if (loop || (long)written < size_to_read * sample_size) {
         break;
      }
   }

   /* Fill the remainder with silence */
   for (i = written; i < buf_size; ++i)
      ((char *)data)[i] = 0;

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
   df->sig = lib.duh_start_sigrenderer(df->duh, 0, 2, df->loop_start);
   return true;
}

static bool modaudio_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time)
{
   MOD_FILE *const df = stream->extra;

   if (df->loop_end != -1 && (long)(time * 65336) > df->loop_end) {
      return false;
   }

   lib.duh_end_sigrenderer(df->sig);
   df->sig = lib.duh_start_sigrenderer(df->duh, 0, 2, time * 65536);

   return true;
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

static ALLEGRO_AUDIO_STREAM *modaudio_stream_init(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples
#if (DUMB_MAJOR_VERSION) < 2
   /* For DUMB 0.9.3, we must choose a loader function ourselves. */
   , DUH *(loader)(DUMBFILE *)
#endif
)
{
   ALLEGRO_AUDIO_STREAM *stream;
   DUMBFILE *df;
   DUH_SIGRENDERER *sig = NULL;
   DUH *duh = NULL;
   DUMB_IT_SIGRENDERER *it_sig = NULL;
   int64_t start_pos = -1;

   df = lib.dumbfile_open_ex(f, &dfs_f);
   if (!df) {
      ALLEGRO_ERROR("dumbfile_open_ex failed.\n");
      return NULL;
   }

   start_pos = al_ftell(f);

#if (DUMB_MAJOR_VERSION) >= 2
   /*
    * DUMB 2.0 introduces dumb_read_any. It takes two extra int arguments.
    *
    * The first int, restrict_, changes how a MOD module is interpreted.
    * int restrict_ is a two-bit bitfield, and 0 is a good default.
    * For discussion, see: https://github.com/kode54/dumb/issues/53
    *
    * The second int, subsong, matters only for very few formats.
    * A5 doesn't allow to choose a subsong from the 5.2 API anyway, thus 0.
    */
   duh = lib.dumb_read_any(df, 0, 0);
#else
   duh = loader(df);
#endif
   if (!duh) {
      ALLEGRO_ERROR("Failed to create DUH.\n");
      goto Error;
   }

   sig = lib.duh_start_sigrenderer(duh, 0, 2, 0);
   if (!sig) {
      ALLEGRO_ERROR("duh_start_sigrenderer failed.\n");
      goto Error;
   }

   it_sig = lib.duh_get_it_sigrenderer(sig);
   if (it_sig) {
      /* Turn off freezing for XM files. Seems completely pointless. */
      lib.dumb_it_set_xm_speed_zero_callback(it_sig, lib.dumb_it_callback_terminate, NULL);
   }

   stream = al_create_audio_stream(buffer_count, samples, 44100,
      ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);

   if (stream) {
      MOD_FILE *mf = al_malloc(sizeof(MOD_FILE));
      mf->duh = duh;
      mf->sig = sig;
      mf->fh = NULL;
      mf->length = lib.duh_get_length(duh) / 65536.0;
      if (mf->length < 0) {
         mf->length = 0;
         mf->loop_start = -1;
         mf->loop_end = -1;
      }
      else {
         mf->loop_start = 0;
         mf->loop_end = (int)(mf->length * 65536.0);
      }

      stream->extra = mf;
      stream->feeder = modaudio_stream_update;
      stream->unload_feeder = modaudio_stream_close;
      stream->rewind_feeder = modaudio_stream_rewind;
      stream->seek_feeder = modaudio_stream_seek;
      stream->get_feeder_position = modaudio_stream_get_position;
      stream->get_feeder_length = modaudio_stream_get_length;
      stream->set_feeder_loop = modaudio_stream_set_loop;
      _al_acodec_start_feed_thread(stream);
   }
   else {
      ALLEGRO_ERROR("Failed to create stream.\n");
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
      ALLEGRO_ERROR("Could not load " ALLEGRO_CFG_ACODEC_DUMB_DLL "\n");
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

   INITSYM(duh_render); /* see comment on duh_render deprecation above */
   INITSYM(duh_sigrenderer_get_position);
   INITSYM(duh_end_sigrenderer);
   INITSYM(unload_duh);
   INITSYM(duh_start_sigrenderer);
   INITSYM(dumbfile_open_ex);
   INITSYM(duh_get_length);
   INITSYM(dumb_exit);
   INITSYM(register_dumbfile_system);
   INITSYM(duh_get_it_sigrenderer);
   INITSYM(dumb_it_set_loop_callback);
   INITSYM(dumb_it_set_xm_speed_zero_callback);
   INITSYM(dumb_it_callback_terminate);

   dfs.open = dfs_open;
   dfs.skip = dfs_skip;
   dfs.getc = dfs_getc;
   dfs.getnc = dfs_getnc;
   dfs.close = dfs_close;

#if (DUMB_MAJOR_VERSION) >= 2
   INITSYM(dumb_read_any);

   dfs.seek = dfs_seek;
   dfs.get_size = dfs_get_size;
#else
   INITSYM(dumb_read_it);
   INITSYM(dumb_read_xm);
   INITSYM(dumb_read_s3m);
   INITSYM(dumb_read_mod);
#endif

   /* Set up DUMB's default I/O to go through Allegro... */
   lib.register_dumbfile_system(&dfs);

   /* But we'll actually use them through this version: */
   dfs_f = dfs;
   dfs_f.open = NULL;
   dfs_f.close = NULL;

   libdumb_loaded = true;
   return true;
}

#if (DUMB_MAJOR_VERSION) >= 2

static ALLEGRO_AUDIO_STREAM *load_dumb_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;
   return modaudio_stream_init(f, buffer_count, samples);
}

/*
 * DUMB 2.0 figures out the file format for us.
 * Need only one loader from disk file and one loader from DUMBFILE.
 */
static ALLEGRO_AUDIO_STREAM *load_dumb_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   stream = load_dumb_audio_stream_f(f, buffer_count, samples);

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

   ((MOD_FILE *)stream->extra)->fh = f;

   return stream;
}

static bool _al_identify_669(ALLEGRO_FILE *f)
{
   uint8_t x[2];
   if (al_fread(f, x, 2) < 2)
      return false;
   if (memcmp(x, "if", 2) == 0 || memcmp(x, "JN", 2) == 0)
      return true;
   return false;
}

static bool _al_identify_amf(ALLEGRO_FILE *f)
{
   uint8_t x[3];
   if (al_fread(f, x, 3) < 3)
      return false;
   if (memcmp(x, "AMF", 3) == 0)
      return true;
   return false;
}

static bool _al_identify_asy(ALLEGRO_FILE *f)
{
   uint8_t x[24];
   if (al_fread(f, x, 24) < 24)
      return false;
   if (memcmp(x, "ASYLUM Music Format V1.0", 24) == 0)
      return true;
   return false;
}

static bool _al_identify_mtm(ALLEGRO_FILE *f)
{
   uint8_t x[3];
   if (al_fread(f, x, 3) < 3)
      return false;
   if (memcmp(x, "MTM", 3) == 0)
      return true;
   return false;
}

static bool _al_identify_okt(ALLEGRO_FILE *f)
{
   uint8_t x[8];
   if (al_fread(f, x, 8) < 8)
      return false;
   if (memcmp(x, "OKTASONG", 8) == 0)
      return true;
   return false;
}

static bool _al_identify_psm(ALLEGRO_FILE *f)
{
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "PSM\x00", 4) == 0 || memcmp(x, "PSM\xFE", 4) == 0)
      return true;
   return false;
}

static bool _al_identify_ptm(ALLEGRO_FILE *f)
{
   uint8_t x[4];
   if (!al_fseek(f, 0x2C, ALLEGRO_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "DSMF", 4) == 0)
      return true;
   return false;
}

static bool _al_identify_riff(ALLEGRO_FILE *f)
{
   static const char riff_fmts[][4] = {
      "AM  ", "AMFF", "DSMF"
   };
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "RIFF", 4) != 0)
      return false;
   if (!al_fseek(f, 4, ALLEGRO_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   for (size_t i = 0; i < sizeof(riff_fmts) / 4; ++i) {
      if (memcmp(x, riff_fmts[i], 4) == 0)
         return true;
   }
   return false;
}

static bool _al_identify_stm(ALLEGRO_FILE *f)
{
   static const char stm_fmts[][8] = {
      "!Scream!", "BMOD2STM", "WUZAMOD!"
   };
   uint8_t x[10];
   if (!al_fseek(f, 20, ALLEGRO_SEEK_CUR))
      return false;
   if (al_fread(f, x, 10) < 8)
      return false;
   if (x[9] != 2)
      return false;
   for (size_t i = 0; i < sizeof(stm_fmts) / 8; ++i) {
      if (memcmp(x, stm_fmts[i], 8) == 0)
         return true;
   }
   return false;
}

#else
/*
 * For DUMB 0.9.3:
 *
 * 4 separate loaders for the 4 file formats supported by DUMB 0.9.3,
 * then 4 loaders that take a DUMBFILE. The loaders from file are
 * all identical, except for the function called in the middle.
 */

static ALLEGRO_AUDIO_STREAM *load_mod_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;
   return modaudio_stream_init(f, buffer_count, samples, lib.dumb_read_mod);
}

static ALLEGRO_AUDIO_STREAM *load_it_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;
   return modaudio_stream_init(f, buffer_count, samples, lib.dumb_read_it);
}

static ALLEGRO_AUDIO_STREAM *load_xm_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;
   return modaudio_stream_init(f, buffer_count, samples, lib.dumb_read_xm);
}

static ALLEGRO_AUDIO_STREAM *load_s3m_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   if (!init_libdumb())
      return NULL;
   return modaudio_stream_init(f, buffer_count, samples, lib.dumb_read_s3m);
}

static ALLEGRO_AUDIO_STREAM *load_mod_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   stream = load_mod_audio_stream_f(f, buffer_count, samples);

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

   ((MOD_FILE *)stream->extra)->fh = f;

   return stream;
}

static ALLEGRO_AUDIO_STREAM *load_it_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   stream = load_it_audio_stream_f(f, buffer_count, samples);

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

   ((MOD_FILE *)stream->extra)->fh = f;

   return stream;
}

static ALLEGRO_AUDIO_STREAM *load_xm_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   stream = load_xm_audio_stream_f(f, buffer_count, samples);

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

   ((MOD_FILE *)stream->extra)->fh = f;

   return stream;
}

static ALLEGRO_AUDIO_STREAM *load_s3m_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   stream = load_s3m_audio_stream_f(f, buffer_count, samples);

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

   ((MOD_FILE *)stream->extra)->fh = f;

   return stream;
}

#endif // DUMB_MAJOR_VERSION

static bool _al_identify_it(ALLEGRO_FILE *f)
{
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "IMPM", 4) == 0)
      return true;
   return false;
}

static bool _al_identify_mod(ALLEGRO_FILE *f)
{
   static const char mod_sigs[][4] = {
      "M.K.", "M!K!", "M&K!", "N.T.",
      "NSMS", "FLT4", "M\0\0\0", "8\0\0\0",
      "FEST", "FLT8", "CD81", "OCTA",
      "OKTA", "16CN", "32CN"
   };
   uint8_t x[4];
   if (!al_fseek(f, 0x438, ALLEGRO_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   for (size_t i = 0; i < sizeof(mod_sigs) / 4; ++i) {
      if (memcmp(x, mod_sigs[i], 4) == 0)
         return true;
   }
   if (memcmp(x + 2, "CH", 2) == 0 && isdigit(x[0]) && isdigit(x[1])) /* ##CH */
      return true;
   if (memcmp(x + 1, "CHN", 3) == 0 && isdigit(x[0])) /* #CHN */
      return true;
   if (memcmp(x, "TDZ", 3) == 0 && isdigit(x[3])) /* TDZ? */
      return true;
   return false;
}

static bool _al_identify_s3m(ALLEGRO_FILE *f)
{
   uint8_t x[4];
   if (!al_fseek(f, 0x2C, ALLEGRO_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "SCRM", 4) == 0)
      return true;
   return false;
}

static bool _al_identify_xm(ALLEGRO_FILE *f)
{
   uint8_t x[16];
   if (al_fread(f, x, 16) < 16)
      return false;
   if (memcmp(x, "Extended Module:", 16) == 0)
      return true;
   return false;
}

bool _al_register_dumb_loaders(void)
{
   bool ret = true;
#if (DUMB_MAJOR_VERSION) >= 2
   /*
    * DUMB 2.0 offers a single loader for at least 13 formats, see their
    * readme. Amiga NoiseTracker isn't listed there, but it's DUMB-supported.
    * It merely has no common extensions, see:
    * https://github.com/kode54/dumb/issues/53
    */
   ret &= al_register_audio_stream_loader(".669", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".669", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".669", _al_identify_669);
   ret &= al_register_audio_stream_loader(".amf", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".amf", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".amf", _al_identify_amf);
   ret &= al_register_audio_stream_loader(".asy", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".asy", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".asy", _al_identify_asy);
   ret &= al_register_audio_stream_loader(".it", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".it", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".it", _al_identify_it);
   ret &= al_register_audio_stream_loader(".mod", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mod", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".mod", _al_identify_mod);
   ret &= al_register_audio_stream_loader(".mtm", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mtm", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".mtm", _al_identify_mtm);
   ret &= al_register_audio_stream_loader(".okt", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".okt", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".okt", _al_identify_okt);
   ret &= al_register_audio_stream_loader(".psm", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".psm", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".psm", _al_identify_psm);
   ret &= al_register_audio_stream_loader(".ptm", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".ptm", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".ptm", _al_identify_ptm);
   ret &= al_register_audio_stream_loader(".riff", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".riff", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".riff", _al_identify_riff);
   ret &= al_register_audio_stream_loader(".s3m", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".s3m", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".s3m", _al_identify_s3m);
   ret &= al_register_audio_stream_loader(".stm", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".stm", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".stm", _al_identify_stm);
   ret &= al_register_audio_stream_loader(".xm", load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".xm", load_dumb_audio_stream_f);
   ret &= al_register_sample_identifier(".xm", _al_identify_xm);
#else
   /*
    * DUMB 0.9.3 supported only these 4 formats and had no *_any loader.
    * Avoid DUMB 1.0 because of versioning problems: dumb.h from git tag 1.0
    * reports 0.9.3 in its version numbers.
    */
   ret &= al_register_audio_stream_loader(".xm", load_xm_audio_stream);
   ret &= al_register_audio_stream_loader_f(".xm", load_xm_audio_stream_f);
   ret &= al_register_sample_identifier(".xm", _al_identify_xm);
   ret &= al_register_audio_stream_loader(".it", load_it_audio_stream);
   ret &= al_register_audio_stream_loader_f(".it", load_it_audio_stream_f);
   ret &= al_register_sample_identifier(".it", _al_identify_it);
   ret &= al_register_audio_stream_loader(".mod", load_mod_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mod", load_mod_audio_stream_f);
   ret &= al_register_sample_identifier(".mod", _al_identify_mod);
   ret &= al_register_audio_stream_loader(".s3m", load_s3m_audio_stream);
   ret &= al_register_audio_stream_loader_f(".s3m", load_s3m_audio_stream_f);
   ret &= al_register_sample_identifier(".s3m", _al_identify_s3m);
#endif // DUMB_MAJOR_VERSION
   return ret;
}

/* vim: set sts=3 sw=3 et: */
