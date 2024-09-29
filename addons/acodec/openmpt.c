/*
 * Allegro OpenMPT wrapper
 * author: Pavel Sountsov
 */

#define _FILE_OFFSET_BITS 64
#include <ctype.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_system.h"
#include "acodec.h"
#include "helper.h"

#ifndef A5O_CFG_ACODEC_OPENMPT
   #error configuration problem, A5O_CFG_ACODEC_OPENMPT not set
#endif

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>

A5O_DEBUG_CHANNEL("acodec")


typedef struct MOD_FILE
{
   openmpt_module *mod;
   A5O_FILE *fh;
   double length;
   double loop_start, loop_end;
} MOD_FILE;


static size_t stream_read_func(void *stream, void *dst, size_t bytes)
{
   return al_fread((A5O_FILE*)stream, dst, bytes);
}


static int stream_seek_func(void *stream, int64_t offset, int whence)
{
   int allegro_whence;
   switch (whence)
   {
      case OPENMPT_STREAM_SEEK_SET:
         allegro_whence = A5O_SEEK_SET;
         break;
      case OPENMPT_STREAM_SEEK_CUR:
         allegro_whence = A5O_SEEK_CUR;
         break;
      case OPENMPT_STREAM_SEEK_END:
         allegro_whence = A5O_SEEK_END;
         break;
      default:
         return -1;
   }

   return al_fseek((A5O_FILE*)stream, offset, allegro_whence) ? 0 : -1;
}


static int64_t stream_tell_func(void *stream)
{
   return al_ftell((A5O_FILE*)stream);
}


static void log_func(const char* message, void *user)
{
   (void)user;
   A5O_DEBUG("OpenMPT: %s", message);
}


static int error_func(int error, void *user)
{
   (void)user;
   const char* error_str = openmpt_error_string(error);
   if (error_str) {
      A5O_ERROR("OpenMPT error: %s\n", error_str);
      openmpt_free_string(error_str);
   }
   else
      A5O_ERROR("OpenMPT error: %d\n", error);
   return OPENMPT_ERROR_FUNC_RESULT_NONE;
}

// /* Stream Functions */
static size_t openmpt_stream_update(A5O_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   MOD_FILE *const modf = stream->extra;

   /* the mod files are stereo and 16-bit */
   const int frame_size = 4;
   size_t written = 0;
   size_t i;

   if (stream->spl.loop == _A5O_PLAYMODE_STREAM_ONCE) {
     openmpt_module_ctl_set_text(modf->mod, "play.at_end", "stop");
     openmpt_module_set_repeat_count(modf->mod, 0);
   } else {
     openmpt_module_ctl_set_text(modf->mod, "play.at_end", "continue");
     openmpt_module_set_repeat_count(modf->mod, -1);
   }

   int count = 0;
   while (written < buf_size) {
      long frames_to_read = (buf_size - written * frame_size) / frame_size;
      double position = openmpt_module_get_position_seconds(modf->mod);
      bool manual_loop = false;
      /* If manual looping is not enabled, then we need to implement
       * short-stopping manually. */
      if (stream->spl.loop != _A5O_PLAYMODE_STREAM_ONCE && modf->loop_end != -1 &&
          position + frames_to_read / 44100 >= modf->loop_end) {
         frames_to_read = (long)((modf->loop_end - position) * 44100);
         if (frames_to_read < 0)
            frames_to_read = 0;
         manual_loop = true;
      }
      written += openmpt_module_read_interleaved_stereo(modf->mod, 44100, frames_to_read,
            (int16_t*)&(((char *)data)[written])) * frame_size;
      if (((long)written < frames_to_read * frame_size) || manual_loop) {
         break;
      }
      count += 1;
   }

   /* Fill the remainder with silence */
   for (i = written; i < buf_size; ++i)
      ((char *)data)[i] = 0;

   return written;
}

static void openmpt_stream_close(A5O_AUDIO_STREAM *stream)
{
   MOD_FILE *const modf = stream->extra;
   _al_acodec_stop_feed_thread(stream);

   openmpt_module_destroy(modf->mod);

   if (modf->fh)
      al_fclose(modf->fh);
}

static bool openmpt_stream_rewind(A5O_AUDIO_STREAM *stream)
{
   MOD_FILE *const modf = stream->extra;
   openmpt_module_set_position_seconds(modf->mod, modf->loop_start);
   return true;
}


static bool openmpt_stream_seek(A5O_AUDIO_STREAM *stream, double time)
{
   MOD_FILE *const modf = stream->extra;

   if (modf->loop_end != -1 && time > modf->loop_end) {
      return false;
   }

   openmpt_module_set_position_seconds(modf->mod, time);

   return false;
}


static double openmpt_stream_get_position(A5O_AUDIO_STREAM *stream)
{
   MOD_FILE *const modf = stream->extra;
   return openmpt_module_get_position_seconds(modf->mod);
}


static double openmpt_stream_get_length(A5O_AUDIO_STREAM *stream)
{
   MOD_FILE *const modf = stream->extra;
   return modf->length;
}


static bool openmpt_stream_set_loop(A5O_AUDIO_STREAM *stream,
   double start, double end)
{
   MOD_FILE *const modf = stream->extra;

   modf->loop_start = start;
   modf->loop_end = end;

   return true;
}


static A5O_AUDIO_STREAM *openmpt_stream_init(A5O_FILE* f,
   size_t buffer_count, unsigned int samples
)
{
   int64_t start_pos = al_ftell(f);
   A5O_AUDIO_STREAM *stream = NULL;

   openmpt_stream_callbacks callbacks = {
      stream_read_func,
      stream_seek_func,
      stream_tell_func,
   };

   openmpt_module *mod = openmpt_module_create2(
         callbacks, f,
         &log_func, NULL,
         &error_func, NULL,
         NULL, NULL,
         NULL);

   // HACK: For whatever reason, OpenMPT is slightly quieter than DUMB.
   // For many modules, this is *very* close to sqrt(2) which is suggestive of
   // some stereo gain correction, but I could not find the code responsible.
   //
   // Also, for some modules it's actually louder by 2x rather than 1.44x,
   // which is confusing as well.
   openmpt_module_set_render_param(mod, OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL, 300);

   if (!mod)
      return NULL;

   // TODO: OpenMPT recommends 48000 and float
   // TODO: OpenMPT supports quad channels too
   stream = al_create_audio_stream(buffer_count, samples, 44100,
      A5O_AUDIO_DEPTH_INT16, A5O_CHANNEL_CONF_2);

   if (stream) {
      MOD_FILE *mf = al_malloc(sizeof(MOD_FILE));
      mf->mod = mod;
      mf->fh = NULL;
      mf->length = openmpt_module_get_duration_seconds(mod);
      if (mf->length < 0) {
         mf->length = 0;
      }
      /*
       * Set these to -1, so that we can default to the internal loop
       * points.
       */
      mf->loop_start = -1;
      mf->loop_end = -1;

      stream->extra = mf;
      stream->feeder = openmpt_stream_update;
      stream->unload_feeder = openmpt_stream_close;
      stream->rewind_feeder = openmpt_stream_rewind;
      stream->seek_feeder = openmpt_stream_seek;
      stream->get_feeder_position = openmpt_stream_get_position;
      stream->get_feeder_length = openmpt_stream_get_length;
      stream->set_feeder_loop = openmpt_stream_set_loop;
      _al_acodec_start_feed_thread(stream);
   }
   else {
      A5O_ERROR("Failed to create stream.\n");
      goto Error;
   }

   return stream;

Error:

   openmpt_module_destroy(mod);

   /* try to return back to where we started to load */
   if (start_pos != -1)
      al_fseek(f, start_pos, A5O_SEEK_SET);

   return NULL;
}


static A5O_AUDIO_STREAM *load_audio_stream_f(A5O_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   return openmpt_stream_init(f, buffer_count, samples);
}

static A5O_AUDIO_STREAM *load_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   A5O_FILE *f;
   A5O_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      A5O_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   stream = load_audio_stream_f(f, buffer_count, samples);

   if (!stream) {
      al_fclose(f);
      return NULL;
   }

   ((MOD_FILE *)stream->extra)->fh = f;

   return stream;
}

bool _al_register_openmpt_loaders(void)
{
   bool ret = true;
   ret &= al_register_audio_stream_loader(".669", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".669", load_audio_stream_f);
   ret &= al_register_sample_identifier(".669", _al_identify_669);
   ret &= al_register_audio_stream_loader(".amf", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".amf", load_audio_stream_f);
   ret &= al_register_sample_identifier(".amf", _al_identify_amf);
   ret &= al_register_audio_stream_loader(".it", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".it", load_audio_stream_f);
   ret &= al_register_sample_identifier(".it", _al_identify_it);
   ret &= al_register_audio_stream_loader(".mod", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mod", load_audio_stream_f);
   ret &= al_register_sample_identifier(".mod", _al_identify_mod);
   ret &= al_register_audio_stream_loader(".mtm", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mtm", load_audio_stream_f);
   ret &= al_register_sample_identifier(".mtm", _al_identify_mtm);
   ret &= al_register_audio_stream_loader(".okt", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".okt", load_audio_stream_f);
   ret &= al_register_sample_identifier(".okt", _al_identify_okt);
   ret &= al_register_audio_stream_loader(".psm", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".psm", load_audio_stream_f);
   ret &= al_register_sample_identifier(".psm", _al_identify_psm);
   ret &= al_register_audio_stream_loader(".ptm", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".ptm", load_audio_stream_f);
   ret &= al_register_sample_identifier(".ptm", _al_identify_ptm);
   ret &= al_register_audio_stream_loader(".s3m", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".s3m", load_audio_stream_f);
   ret &= al_register_sample_identifier(".s3m", _al_identify_s3m);
   ret &= al_register_audio_stream_loader(".stm", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".stm", load_audio_stream_f);
   ret &= al_register_sample_identifier(".stm", _al_identify_stm);
   ret &= al_register_audio_stream_loader(".xm", load_audio_stream);
   ret &= al_register_audio_stream_loader_f(".xm", load_audio_stream_f);
   ret &= al_register_sample_identifier(".xm", _al_identify_xm);
   return ret;
}

/* vim: set sts=3 sw=3 et: */
