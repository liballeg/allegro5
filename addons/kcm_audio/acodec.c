/*
 * Allegro audio codec loader.
 */

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_vector.h"


#define MAX_EXTENSION_LENGTH  (32)

typedef struct ACODEC_TABLE ACODEC_TABLE;
struct ACODEC_TABLE
{
   char              ext[MAX_EXTENSION_LENGTH];
   ALLEGRO_SAMPLE *  (*loader)(const char *filename);
   bool              (*saver)(const char *filename, ALLEGRO_SAMPLE *spl);
   ALLEGRO_STREAM *  (*stream_loader)(const char *filename,
                        size_t buffer_count, unsigned int samples);
};


/* forward declarations */
static void acodec_ensure_init(void);
static void acodec_shutdown(void);
static ACODEC_TABLE *find_acodec_table_entry(const char *ext);


/* globals */
static bool acodec_inited = false;
static _AL_VECTOR acodec_table = _AL_VECTOR_INITIALIZER(ACODEC_TABLE);


static void acodec_ensure_init(void)
{
   if (acodec_inited) {
      return;
   }

   /* Must be before register calls to avoid recursion. */
   acodec_inited = true;

   al_register_sample_loader(".wav", al_load_sample_wav);
   al_register_sample_saver(".wav", al_save_sample_wav);
   al_register_stream_loader(".wav", al_load_stream_wav);

   _al_add_exit_func(acodec_shutdown, "acodec_shutdown");
}


static void acodec_shutdown(void)
{
   _al_vector_free(&acodec_table);
   acodec_inited = false;
}


static ACODEC_TABLE *find_acodec_table_entry(const char *ext)
{
   ACODEC_TABLE *ent;
   unsigned i;

   acodec_ensure_init();

   for (i = 0; i < _al_vector_size(&acodec_table); i++) {
      ent = _al_vector_ref(&acodec_table, i);
      if (0 == stricmp(ent->ext, ext)) {
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

   return ent;
}


/* Function: al_register_sample_loader
 */
bool al_register_sample_loader(const char *ext,
   ALLEGRO_SAMPLE *(*loader)(const char *filename))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->loader = loader;

   return true;
}


/* Function: al_register_sample_saver
 */
bool al_register_sample_saver(const char *ext,
   bool (*saver)(const char *filename, ALLEGRO_SAMPLE *spl))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->saver = saver;

   return true;
}


/* Function: al_register_stream_loader
 */
bool al_register_stream_loader(const char *ext,
   ALLEGRO_STREAM *(*stream_loader)(const char *filename,
      size_t buffer_count, unsigned int samples))
{
   ACODEC_TABLE *ent;

   if (strlen(ext) >= MAX_EXTENSION_LENGTH) {
      return false;
   }

   ent = find_acodec_table_entry(ext);
   if (!ent) {
      ent = add_acodec_table_entry(ext);
   }

   ent->stream_loader = stream_loader;

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


/* Function: al_stream_from_file
 */
ALLEGRO_STREAM *al_stream_from_file(const char *filename, size_t buffer_count,
   unsigned int samples)
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

   TRACE("Error creating ALLEGRO_STREAM from '%s'.\n", filename);

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
   ext++;   /* skip '.' */

   ent = find_acodec_table_entry(ext);
   if (ent && ent->saver) {
      return (ent->saver)(filename, spl);
   }

   return false;
}


/* FIXME: use the allegro provided helpers */
ALLEGRO_CHANNEL_CONF _al_count_to_channel_conf(int num_channels)
{
   switch (num_channels) {
      case 1:
         return ALLEGRO_CHANNEL_CONF_1;
      case 2:
         return ALLEGRO_CHANNEL_CONF_2;
      case 3:
         return ALLEGRO_CHANNEL_CONF_3;
      case 4:
         return ALLEGRO_CHANNEL_CONF_4;
      case 6:
         return ALLEGRO_CHANNEL_CONF_5_1;
      case 7:
         return ALLEGRO_CHANNEL_CONF_6_1;
      case 8:
         return ALLEGRO_CHANNEL_CONF_7_1;
      default:
         return 0;
   }
}


/* Note: assumes 8-bit is unsigned, and all others are signed. */
ALLEGRO_AUDIO_DEPTH _al_word_size_to_depth_conf(int word_size)
{
   switch (word_size) {
      case 1:
         return ALLEGRO_AUDIO_DEPTH_UINT8;
      case 2:
         return ALLEGRO_AUDIO_DEPTH_INT16;
      case 3:
         return ALLEGRO_AUDIO_DEPTH_INT24;
      case 4:
         return ALLEGRO_AUDIO_DEPTH_FLOAT32;
      default:
         return 0;
   }
}

/* vim: set sts=3 sw=3 et: */
