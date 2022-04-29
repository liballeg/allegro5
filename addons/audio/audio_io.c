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

   bool              (*identifier)(ALLEGRO_FILE *fp);
};


/* globals */
static bool acodec_inited = false;
static _AL_VECTOR acodec_table = _AL_VECTOR_INITIALIZER(ACODEC_TABLE);
static ALLEGRO_AUDIO_STREAM *global_stream;


static void acodec_shutdown(void)
{
   if (acodec_inited) {
      _al_vector_free(&acodec_table);
      acodec_inited = false;
   }
   al_destroy_audio_stream(global_stream);
}


static ACODEC_TABLE *find_acodec_table_entry(const char *ext)
{
   ACODEC_TABLE *ent;
   unsigned i;

   for (i = 0; i < _al_vector_size(&acodec_table); i++) {
      ent = _al_vector_ref(&acodec_table, i);
      if (0 == _al_stricmp(ent->ext, ext)) {
         return ent;
      }
   }

   return NULL;
}


static ACODEC_TABLE *find_acodec_table_entry_for_file(ALLEGRO_FILE *f)
{
   ACODEC_TABLE *ent;
   unsigned i;

   for (i = 0; i < _al_vector_size(&acodec_table); i++) {
      ent = _al_vector_ref(&acodec_table, i);
      if (ent->identifier) {
         int64_t pos = al_ftell(f);
         bool identified = ent->identifier(f);
         al_fseek(f, pos, ALLEGRO_SEEK_SET);
         if (identified)
            return ent;
      }
   }
   return NULL;
}


static ACODEC_TABLE *add_acodec_table_entry(const char *ext)
{
   ACODEC_TABLE *ent;

   ASSERT(ext);

   if (!acodec_inited) {
      acodec_inited = true;
      _al_add_exit_func(acodec_shutdown, "acodec_shutdown");
   }

   ent = _al_vector_alloc_back(&acodec_table);
   strcpy(ent->ext, ext);
   ent->loader = NULL;
   ent->saver = NULL;
   ent->stream_loader = NULL;
   
   ent->fs_loader = NULL;
   ent->fs_saver = NULL;
   ent->fs_stream_loader = NULL;

   ent->identifier = NULL;

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


/* Function: al_register_sample_identifier
 */
bool al_register_sample_identifier(const char *ext,
   bool (*identifier)(ALLEGRO_FILE* fp))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) + 1 >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!identifier) {
      if (!ent || !ent->identifier) {
         return false; /* Nothing to remove. */
      }
   }
   else if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->identifier = identifier;

   return true;
}


/* Function: al_load_sample
 */
ALLEGRO_SAMPLE *al_load_sample(const char *filename)
{
   const char *ext;
   ACODEC_TABLE *ent;

   ASSERT(filename);
   ext = al_identify_sample(filename);
   if (!ext) {
      ext = strrchr(filename, '.');
      if (ext == NULL) {
         ALLEGRO_ERROR("Unable to determine extension for %s.\n", filename);
         return NULL;
      }
   }

   ent = find_acodec_table_entry(ext);
   if (ent && ent->loader) {
      return (ent->loader)(filename);
   }
   else {
      ALLEGRO_ERROR("No handler for audio file extension %s - "
         "therefore not trying to load %s.\n", ext, filename);
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
   else {
      ALLEGRO_ERROR("No handler for audio file extension %s.\n", ident);
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
   ext = al_identify_sample(filename);
   if (!ext) {
      ext = strrchr(filename, '.');
      if (ext == NULL) {
         ALLEGRO_ERROR("Unable to determine extension for %s.\n", filename);
         return NULL;
      }
   }

   ent = find_acodec_table_entry(ext);
   if (ent && ent->stream_loader) {
      return (ent->stream_loader)(filename, buffer_count, samples);
   }
   else {
      ALLEGRO_ERROR("No handler for audio file extension %s - "
         "therefore not trying to load %s.\n", ext, filename);
   }

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
   else {
      ALLEGRO_ERROR("No handler for audio file extension %s.\n", ident);
   }

   return NULL;
}


/* Function: al_play_audio_stream
 */
ALLEGRO_AUDIO_STREAM *al_play_audio_stream(const char *filename)
{
   ASSERT(filename);
   if (!al_get_default_mixer()) {
      ALLEGRO_ERROR("No default mixer\n!");
      return NULL;
   }
   al_destroy_audio_stream(global_stream);
   global_stream = al_load_audio_stream(filename, 4, 2048);
   if (!global_stream) {
      ALLEGRO_ERROR("Could not play audio stream: %s.\n", filename);
      return NULL;
   }
   if (!al_attach_audio_stream_to_mixer(global_stream, al_get_default_mixer())) {
      ALLEGRO_ERROR("Could not attach stream to mixer\n");
      return NULL;
   }
   return global_stream;
}


/* Function: al_play_audio_stream_f
 */
ALLEGRO_AUDIO_STREAM *al_play_audio_stream_f(ALLEGRO_FILE *fp, const char *ident)
{
   ASSERT(fp);
   ASSERT(ident);
   if (!al_get_default_mixer()) {
      ALLEGRO_ERROR("No default mixer\n!");
      return NULL;
   }
   al_destroy_audio_stream(global_stream);
   global_stream = al_load_audio_stream_f(fp, ident, 4, 2048);
   if (!global_stream) {
      ALLEGRO_ERROR("Could not play audio stream.\n");
      return NULL;
   }
   if (!al_attach_audio_stream_to_mixer(global_stream, al_get_default_mixer())) {
      ALLEGRO_ERROR("Could not attach stream to mixer\n");
      return NULL;
   }
   return global_stream;
}


/* Function: al_save_sample
 */
bool al_save_sample(const char *filename, ALLEGRO_SAMPLE *spl)
{
   const char *ext;
   ACODEC_TABLE *ent;

   ASSERT(filename);
   ext = strrchr(filename, '.');
   if (ext == NULL) {
      ALLEGRO_ERROR("Unable to determine extension for %s.\n", filename);
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (ent && ent->saver) {
      return (ent->saver)(filename, spl);
   }
   else {
      ALLEGRO_ERROR("No handler for audio file extension %s - "
         "therefore not trying to load %s.\n", ext, filename);
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
   else {
      ALLEGRO_ERROR("No handler for audio file extension %s.\n", ident);
   }

   return false;
}


/* Function: al_identify_sample_f
 */
char const *al_identify_sample_f(ALLEGRO_FILE *fp)
{
   ACODEC_TABLE *h = find_acodec_table_entry_for_file(fp);
   if (!h)
      return NULL;
   return h->ext;
}


/* Function: al_identify_sample
 */
char const *al_identify_sample(char const *filename)
{
   char const *ext;
   ALLEGRO_FILE *fp = al_fopen(filename, "rb");
   if (!fp)
      return NULL;
   ext = al_identify_sample_f(fp);
   al_fclose(fp);
   return ext;
}

/* vim: set sts=3 sw=3 et: */
