/*
 * Allegro5 Creative Voice Reader.
 * Based on external libsndfile usage
 * Can only load samples right now
 * author: pkrcel (aka Andrea Provasi) <pkrcel@gmail.com>
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

#ifndef ALLEGRO_CFG_ACODEC_CREATIVE_VOICE
   #error configuration problem, ALLEGRO_CFG_ACODEC_CREATIVE_VOICE not set
#endif

ALLEGRO_DEBUG_CHANNEL("acodec")

#include <sndfile.h>

typedef struct AL_SNDFILE_DATA AL_SNDFILE_DATA;

struct AL_SNDFILE_DATA {
   SNDFILE *sf;
   SF_INFO sfinfo;
   ALLEGRO_FILE *file;
   SF_VIRTUAL_IO al_sd_vio;
};


/* dynamic loading support (Windows only currently) */
#ifdef ALLEGRO_CFG_ACODEC_CREATIVE_VOICE_DLL
static void *cva_dll = NULL;
static bool cva_virgin = true;
#endif

static struct
{
   SNDFILE*    (*sf_open_virtual) (SF_VIRTUAL_IO *sfvirtual, int mode, SF_INFO *sfinfo, void *user_data);
//   int         (*sf_error)        (SNDFILE *);
//   const char* (*sf_strerror)     (SNDFILE *);
//   const char* (*sf_error_number) (int) ;
//   int         (*sf_command)      (SNDFILE *, int , void *, int ) ;

   sf_count_t (*sf_readf_short)   (SNDFILE *, short *, sf_count_t ) ;
//   sf_count_t (*sf_writef_short)  (SNDFILE *, const short *, sf_count_t ) ;
//   sf_count_t (*sf_readf_int)     (SNDFILE *, int *, sf_count_t ) ;
//   sf_count_t (*sf_writef_int)    (SNDFILE *, const int *, sf_count_t ) ;
//   sf_count_t (*sf_readf_float)   (SNDFILE *, float *, sf_count_t ) ;
//   sf_count_t (*sf_writef_float)  (SNDFILE *, const float *, sf_count_t ) ;
//   sf_count_t (*sf_readf_double)  (SNDFILE *, double *, sf_count_t ) ;
//   sf_count_t (*sf_writef_double) (SNDFILE *, const double *, sf_count_t ) ;

   sf_count_t (*sf_read_short)    (SNDFILE *, short *, sf_count_t ) ;
//   sf_count_t (*sf_write_short)   (SNDFILE *, const short *, sf_count_t ) ;
//   sf_count_t (*sf_read_int)      (SNDFILE *, int *, sf_count_t ) ;
//   sf_count_t (*sf_write_int)     (SNDFILE *, const int *, sf_count_t ) ;
//   sf_count_t (*sf_read_float)    (SNDFILE *, float *, sf_count_t ) ;
//   sf_count_t (*sf_write_float)   (SNDFILE *, const float *, sf_count_t ) ;
//   sf_count_t (*sf_read_double)   (SNDFILE *, double *, sf_count_t ) ;
//   sf_count_t (*sf_write_double)  (SNDFILE *, const double *, sf_count_t ) ;

   int        (*sf_close)         (SNDFILE *) ;
} lib;


#ifdef ALLEGRO_CFG_ACODEC_CREATIVE_VOICE_DLL
static void shutdown_dynlib(void)
{
   if (cva_dll) {
      _al_close_library(cva_dll);
      cva_dll = NULL;
      cva_virgin = true;
   }
}
#endif


static bool init_dynlib(void)
{
#ifdef ALLEGRO_CFG_ACODEC_CREATIVE_VOICE_DLL
   if (cva_dll) {
      return true;
   }

   if (!cva_virgin) {
      return false;
   }

   cva_virgin = false;

   cva_dll = _al_open_library(ALLEGRO_CFG_ACODEC_CREATIVE_VOICE_DLL);
   if (!cva_dll) {
      ALLEGRO_WARN("Could not load " ALLEGRO_CFG_ACODEC_CREATIVE_VOICE_DLL "\n");
      return false;
   }

   _al_add_exit_func(shutdown_dynlib, "shutdown_dynlib");

   #define INITSYM(x)                                                         \
      do                                                                      \
      {                                                                       \
         lib.x = _al_import_symbol(cva_dll, #x);                               \
         if (lib.x == 0) {                                                    \
            ALLEGRO_ERROR("undefined symbol in lib structure: " #x "\n");     \
            return false;                                                     \
         }                                                                    \
      } while(0)
#else
   #define INITSYM(x)   (lib.x = (x))
#endif

   memset(&lib, 0, sizeof(lib));

   INITSYM(sf_open_virtual);
   INITSYM(sf_readf_short);
   INITSYM(sf_read_short);
   INITSYM(sf_close);

   return true;

#undef INITSYM
}

/*
 * Providing SF_VIRTUAL_IO functions to later be called on sf_open_virtual
 */

static sf_count_t _al_sf_vio_get_filelen(void *user_data){
   ALLEGRO_FILE *f = (ALLEGRO_FILE*)user_data;
   return (sf_count_t)al_fsize(f);
}

static sf_count_t _al_sf_vio_seek (sf_count_t offset, int whence, void *user_data){
   ALLEGRO_FILE *f = (ALLEGRO_FILE*)user_data;

   switch(whence) {
      case SEEK_SET: whence = ALLEGRO_SEEK_SET; break;
      case SEEK_CUR: whence = ALLEGRO_SEEK_CUR; break;
      case SEEK_END: whence = ALLEGRO_SEEK_END; break;
   }
   if (!al_fseek(f, offset, whence)) {
      return -1;
   } else {
      return (sf_count_t)al_ftell(f);
      }
}

static sf_count_t _al_sf_vio_read(void *ptr, sf_count_t count, void *user_data){
   ALLEGRO_FILE *f = (ALLEGRO_FILE *)user_data;
   size_t nrbytes = 0;

   nrbytes = al_fread(f, ptr, count);

   return nrbytes;
}

static sf_count_t _al_sf_vio_write(const void *ptr, sf_count_t count, void *user_data){
   // won't implement, we do not need to write to a virtual IO so far
   return 0;
}

static sf_count_t _al_sf_vio_tell(void *user_data){
   ALLEGRO_FILE *f = (ALLEGRO_FILE *)user_data;
   sf_count_t ret = 0;

   ret = al_ftell(f);
   if (ret == -1)
      return -1;

   return ret;
}


ALLEGRO_SAMPLE *_al_load_creative_voice(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);
   
   ALLEGRO_INFO("Loading sample %s.\n", filename);
   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Failed reading %s.\n", filename);
      return NULL;
   }

   spl = _al_load_creative_voice_f(f);

   al_fclose(f);

   return spl;
}


ALLEGRO_SAMPLE *_al_load_creative_voice_f(ALLEGRO_FILE *file)
{
   /* Note: decoding library returns "whatever we want" as far it is transparent
    * To conform to other acodec handlers, we'll use 16-bit "short" an no other
    * Might change in the future.
    */

   //endinanees check might be superfluos
#ifdef ALLEGRO_LITTLE_ENDIAN
   //const int endian = 0; /* 0 for Little-Endian, 1 for Big-Endian */
#else
   //const int endian = 1; /* 0 for Little-Endian, 1 for Big-Endian */
#endif

   //const int packet_size = 4096; /* suggestion for size to read at a time */
   const int word_size = 2;  /* constant for 16-bit */

   short *buffer;
   long pos;
   ALLEGRO_SAMPLE *sample;
   int channels;
   long samplerate;
   long total_samples;
   long total_size;
   AL_SNDFILE_DATA al_sd;
   long read;

   if (!init_dynlib()) {
      return NULL;
   }

   //Initialize the sndfile object
   memset(&al_sd, 0, sizeof(al_sd));
   al_sd.al_sd_vio.get_filelen = _al_sf_vio_get_filelen;
   al_sd.al_sd_vio.read = _al_sf_vio_read;
   al_sd.al_sd_vio.write = _al_sf_vio_write;
   al_sd.al_sd_vio.seek = _al_sf_vio_seek;
   al_sd.al_sd_vio.tell = _al_sf_vio_tell;
   al_sd.file = file;


   // We need to open the file to get the parameters
   al_sd.sf = lib.sf_open_virtual(&al_sd.al_sd_vio, SFM_READ, &al_sd.sfinfo, al_sd.file);


   channels = al_sd.sfinfo.channels;
   samplerate = al_sd.sfinfo.samplerate;
   total_samples = al_sd.sfinfo.frames;

   total_size = total_samples * channels * word_size;

   ALLEGRO_DEBUG("channels %d\n", channels);
   ALLEGRO_DEBUG("word_size %d\n", word_size);
   ALLEGRO_DEBUG("rate %ld\n", samplerate);
   ALLEGRO_DEBUG("total_samples %ld\n", total_samples);
   ALLEGRO_DEBUG("total_size %ld\n", total_size);

   buffer = al_malloc(total_size);
   if (!buffer) {
      return NULL;
   }

   /*
    * libsndfile allows us to not read in chunks but this is STILL safer than
    * an all-or-nothing whole file read, I guess
    */

//   pos = 0;
//   while (pos < total_size) {
//      const int read_size = _ALLEGRO_MIN(packet_size, total_size - pos);
//      ASSERT(pos + read_size <= total_size);

//      /* TODO: lacks error handling, implement sooner than later*/
//      read = lib.sf_read_short(al_sd.sf, buffer + pos, read_size);

//      pos += read;
//      if (read == 0)
//         break;
//   }

   read = lib.sf_readf_short(al_sd.sf, buffer, al_sd.sfinfo.frames);
   sample = al_create_sample(buffer, total_samples, samplerate,
      _al_word_size_to_depth_conf(word_size),
      _al_count_to_channel_conf(channels), true);

   if (!sample) {
      al_free(buffer);
   }

   lib.sf_close(al_sd.sf);

   return sample;
}

/*
 *
 * Should Implement also stream seek.
 *
 */
