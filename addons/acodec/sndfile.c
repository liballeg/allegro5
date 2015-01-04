/*
 * Allegro5 External Audio decoder through libsndfile.
 *
 * Requires libsndfile: http://www.mega-nerd.com/libsndfile/
 *
 * author: pkrcel (aka Andrea Provasi) <pkrcel@gmail.com>
 *
 * Revisions:
 *            2014-12-30 Initial Release - can only load VOC samples
 *            2015-01-03 renamed branch to sndfile
 *
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

#ifndef ALLEGRO_CFG_ACODEC_SNDFILE
   #error configuration problem, ALLEGRO_CFG_ACODEC_SNDFILE not set
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
#ifdef ALLEGRO_CFG_ACODEC_SNDFILE_DLL
static void *cva_dll = NULL;
static bool cva_virgin = true;
#endif

static struct
{
   SNDFILE*    (*sf_open_virtual) (SF_VIRTUAL_IO *sfvirtual, int mode,
                                   SF_INFO *sfinfo, void *user_data);
//   int         (*sf_error)        (SNDFILE *);
//   const char* (*sf_strerror)     (SNDFILE *);
//   const char* (*sf_error_number) (int);
//   int         (*sf_command)      (SNDFILE *, int , void *, int );
//   int         (*sf_command)      (SNDFILE *, int , void *, int );
//   sf_count_t (*sf_seek)          (SNDFILE *sndfile, sf_count_t frames,
//                                   int whence);
   sf_count_t (*sf_readf_short)   (SNDFILE *, short *, sf_count_t );
//   sf_count_t (*sf_writef_short)  (SNDFILE *, const short *, sf_count_t );
//   sf_count_t (*sf_readf_int)     (SNDFILE *, int *, sf_count_t );
//   sf_count_t (*sf_writef_int)    (SNDFILE *, const int *, sf_count_t );
//   sf_count_t (*sf_readf_float)   (SNDFILE *, float *, sf_count_t );
//   sf_count_t (*sf_writef_float)  (SNDFILE *, const float *, sf_count_t );
//   sf_count_t (*sf_readf_double)  (SNDFILE *, double *, sf_count_t );
//   sf_count_t (*sf_writef_double) (SNDFILE *, const double *, sf_count_t );

   sf_count_t (*sf_read_short)    (SNDFILE *, short *, sf_count_t );
//   sf_count_t (*sf_write_short)   (SNDFILE *, const short *, sf_count_t );
//   sf_count_t (*sf_read_int)      (SNDFILE *, int *, sf_count_t );
//   sf_count_t (*sf_write_int)     (SNDFILE *, const int *, sf_count_t );
//   sf_count_t (*sf_read_float)    (SNDFILE *, float *, sf_count_t );
//   sf_count_t (*sf_write_float)   (SNDFILE *, const float *, sf_count_t ) ;
//   sf_count_t (*sf_read_double)   (SNDFILE *, double *, sf_count_t );
//   sf_count_t (*sf_write_double)  (SNDFILE *, const double *, sf_count_t );

   int        (*sf_close)         (SNDFILE *);
} lib;


#ifdef ALLEGRO_CFG_ACODEC_SNDFILE_DLL
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
#ifdef ALLEGRO_CFG_ACODEC_SNDFILE_DLL
   if (cva_dll) {
      return true;
   }

   if (!cva_virgin) {
      return false;
   }

   cva_virgin = false;

   cva_dll = _al_open_library(ALLEGRO_CFG_ACODEC_SNDFILE_DLL);
   if (!cva_dll) {
      ALLEGRO_WARN("Could not load " ALLEGRO_CFG_ACODEC_SNDFILE_DLL "\n");
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
 * Providing SF_VIRTUAL_IO environment
 * these functions have to be passed later to sf_open_virtual
 * For mor information:
 *    <http://www.mega-nerd.com/libsndfile/api.html#open_virtual>
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

/*
 * This is not implemented, current release does not save samples.
 * And the implementation SHOULD NOT save streams or samples with sndfile.
 * This is because a choice should be made on the format saving
 * and standard WAV saving is already available in the library.
 */
static sf_count_t _al_sf_vio_write(const void *ptr, sf_count_t count, void *user_data){
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


ALLEGRO_SAMPLE *_al_load_sndfile(const char *filename)
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

   spl = _al_load_sndfile_f(f);

   al_fclose(f);

   return spl;
}


ALLEGRO_SAMPLE *_al_load_sndfile_f(ALLEGRO_FILE *file)
{
   /* NOTE: The decoding library libsndfile returns "whatever we want"
    * and is transparent to the user.
    * To conform to other acodec handlers, we'll use 16-bit "short" an no other
    * format as the 16Bit PCM data seems to be the most commonly supported.
    *
    * The decoding library uses the native endian format of the host, so no need
    * to check current endianess.
    */

   const int word_size = 2;  /* constant stands for 16-bit */

   short *buffer;            /* the actual PCM data buffer */
   ALLEGRO_SAMPLE *sample;
   int channels;
   long samplerate;
   long total_samples;
   long total_size;          /* unsure about how I handle this */
   AL_SNDFILE_DATA al_sd;
   long read;

   if (!init_dynlib()) {
      return NULL;
   }

   /* Initialize the sndfile object we pass to the decoder*/
   memset(&al_sd, 0, sizeof(al_sd));
   al_sd.al_sd_vio.get_filelen = _al_sf_vio_get_filelen;
   al_sd.al_sd_vio.read = _al_sf_vio_read;
   al_sd.al_sd_vio.write = _al_sf_vio_write;
   al_sd.al_sd_vio.seek = _al_sf_vio_seek;
   al_sd.al_sd_vio.tell = _al_sf_vio_tell;
   al_sd.file = file;


   /* Need to open the file first to get the parameters
    *
    * TODO: inser proper error handling, even thou the ALLEGRO_FILE should
    * already have been properly handled by the caller
    *
    */

   al_sd.sf = lib.sf_open_virtual(&al_sd.al_sd_vio, SFM_READ,
                                  &al_sd.sfinfo, al_sd.file);

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
    * libsndfile allows us to read all the buffer in "frames".
    * Given that 'total_size' is correct, there should not be any overrun.
    */
   read = lib.sf_readf_short(al_sd.sf, buffer, al_sd.sfinfo.frames);

   if (read != al_sd.sfinfo.frames)
      ALLEGRO_DEBUG("Voc Decoder: read %l bytes for %llu samples\n",
                    read, al_sd.sfinfo.frames);

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
 * TODO:
 * Should also provide a STREAM LOADER implementation.
 *
 */
