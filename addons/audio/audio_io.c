/*
 * Allegro audio codec table.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_vector.h"

ALLEGRO_DEBUG_CHANNEL("audio")


#define MAX_EXTENSION_LENGTH  (32)

typedef struct ACODEC_TABLE ACODEC_TABLE;
struct ACODEC_TABLE
{
   char              ext[MAX_EXTENSION_LENGTH];
   ALLEGRO_SAMPLE *  (*loader)(const char *filename);
   bool              (*saver)(const char *filename, ALLEGRO_SAMPLE *spl);
   ALLEGRO_AUDIO_STREAM *(*stream_loader)(const char *filename,
                        size_t buffer_count, unsigned int samples);
                        
   ALLEGRO_SAMPLE *  (*fs_loader)(ALLEGRO_FILE *fp);
   bool              (*fs_saver)(ALLEGRO_FILE *fp, ALLEGRO_SAMPLE *spl);
   ALLEGRO_AUDIO_STREAM *(*fs_stream_loader)(ALLEGRO_FILE *fp,
                        size_t buffer_count, unsigned int samples);
};


/* globals */
static bool acodec_inited = false;
static _AL_VECTOR acodec_table = _AL_VECTOR_INITIALIZER(ACODEC_TABLE);


static void acodec_shutdown(void)
{
   if (acodec_inited) {
      _al_vector_free(&acodec_table);
      acodec_inited = false;
   }
}


static ACODEC_TABLE *find_acodec_table_entry(const char *ext)
{
   ACODEC_TABLE *ent;
   unsigned i;

   if (!acodec_inited) {
      acodec_inited = true;
      _al_add_exit_func(acodec_shutdown, "acodec_shutdown");
   }

   for (i = 0; i < _al_vector_size(&acodec_table); i++) {
      ent = _al_vector_ref(&acodec_table, i);
      if (0 == _al_stricmp(ent->ext, ext)) {
         return ent;
      }
   }

   return NULL;
}


static ACODEC_TABLE *add_acodec_table_entry(const char *ext)
{
   ACODEC_TABLE *ent;

   ent = _al_vector_alloc_back(&acodec_table);
   strcpy(ent->ext, ext);
   ent->loader = NULL;
   ent->saver = NULL;
   ent->stream_loader = NULL;
   
   ent->fs_loader = NULL;
   ent->fs_saver = NULL;
   ent->fs_stream_loader = NULL;

   return ent;
}


/* Function: al_register_sample_loader
 */
bool al_register_sample_loader(const char *ext,
   ALLEGRO_SAMPLE *(*loader)(const char *filename))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) + 1 >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!loader) {
      if (!ent || !ent->loader) {
         return false; /* Nothing to remove. */
      }
   }
   else if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->loader = loader;

   return true;
}


/* Function: al_register_sample_loader_f
 */
bool al_register_sample_loader_f(const char *ext,
   ALLEGRO_SAMPLE *(*loader)(ALLEGRO_FILE* fp))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) + 1 >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!loader) {
      if (!ent || !ent->fs_loader) {
         return false; /* Nothing to remove. */
      }
   }
   else if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->fs_loader = loader;

   return true;
}


/* Function: al_register_sample_saver
 */
bool al_register_sample_saver(const char *ext,
   bool (*saver)(const char *filename, ALLEGRO_SAMPLE *spl))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) + 1 >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!saver) {
      if (!ent || !ent->saver) {
         return false; /* Nothing to remove. */
      }
   }
   else if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->saver = saver;

   return true;
}


/* Function: al_register_sample_saver_f
 */
bool al_register_sample_saver_f(const char *ext,
   bool (*saver)(ALLEGRO_FILE* fp, ALLEGRO_SAMPLE *spl))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) + 1 >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!saver) {
      if (!ent || !ent->fs_saver) {
         return false; /* Nothing to remove. */
      }
   }
   else if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->fs_saver = saver;

   return true;
}


/* Function: al_register_audio_stream_loader
 */
bool al_register_audio_stream_loader(const char *ext,
   ALLEGRO_AUDIO_STREAM *(*stream_loader)(const char *filename,
      size_t buffer_count, unsigned int samples))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) + 1 >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!stream_loader) {
      if (!ent || !ent->stream_loader) {
         return false; /* Nothing to remove. */
      }
   }
   else if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->stream_loader = stream_loader;

   return true;
}


/* Function: al_register_audio_stream_loader_f
 */
bool al_register_audio_stream_loader_f(const char *ext,
   ALLEGRO_AUDIO_STREAM *(*stream_loader)(ALLEGRO_FILE* fp,
      size_t buffer_count, unsigned int samples))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) + 1 >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!stream_loader) {
      if (!ent || !ent->fs_stream_loader) {
         return false; /* Nothing to remove. */
      }
   }
   else if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->fs_stream_loader = stream_loader;

   return true;
}


/* Function: al_load_sample
 */
ALLEGRO_SAMPLE *al_load_sample(const char *filename)
{
   const char *ext;
   ACODEC_TABLE *ent;

   ASSERT(filename);
   ext = strrchr(filename, '.');
   if (ext == NULL)
      return NULL;

   ent = find_acodec_table_entry(ext);
   if (ent && ent->loader) {
      return (ent->loader)(filename);
   }

   return NULL;
}


/* Function: al_load_sample_f
 */
ALLEGRO_SAMPLE *al_load_sample_f(ALLEGRO_FILE* fp, const char *ident)
{
   ACODEC_TABLE *ent;

   ASSERT(fp);
   ASSERT(ident);

   ent = find_acodec_table_entry(ident);
   if (ent && ent->fs_loader) {
      return (ent->fs_loader)(fp);
   }

   return NULL;
}


/* Function: al_load_audio_stream
 */
ALLEGRO_AUDIO_STREAM *al_load_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   const char *ext;
   ACODEC_TABLE *ent;

   ASSERT(filename);
   ext = strrchr(filename, '.');
   if (ext == NULL)
      return NULL;

   ent = find_acodec_table_entry(ext);
   if (ent && ent->stream_loader) {
      return (ent->stream_loader)(filename, buffer_count, samples);
   }

   ALLEGRO_ERROR("Error creating ALLEGRO_AUDIO_STREAM from '%s'.\n", filename);

   return NULL;
}


/* Function: al_load_audio_stream_f
 */
ALLEGRO_AUDIO_STREAM *al_load_audio_stream_f(ALLEGRO_FILE* fp, const char *ident,
   size_t buffer_count, unsigned int samples)
{
   ACODEC_TABLE *ent;

   ASSERT(fp);
   ASSERT(ident);
   
   ent = find_acodec_table_entry(ident);
   if (ent && ent->fs_stream_loader) {
      return (ent->fs_stream_loader)(fp, buffer_count, samples);
   }

   return NULL;
}


/* Function: al_save_sample
 */
bool al_save_sample(const char *filename, ALLEGRO_SAMPLE *spl)
{
   const char *ext;
   ACODEC_TABLE *ent;

   ASSERT(filename);
   ext = strrchr(filename, '.');
   if (ext == NULL)
      return false;

   ent = find_acodec_table_entry(ext);
   if (ent && ent->saver) {
      return (ent->saver)(filename, spl);
   }

   return false;
}


/* Function: al_save_sample_f
 */
bool al_save_sample_f(ALLEGRO_FILE *fp, const char *ident, ALLEGRO_SAMPLE *spl)
{
   ACODEC_TABLE *ent;

   ASSERT(fp);
   ASSERT(ident);
   
   ent = find_acodec_table_entry(ident);
   if (ent && ent->fs_saver) {
      return (ent->fs_saver)(fp, spl);
   }

   return false;
}


/* vim: set sts=3 sw=3 et: */
