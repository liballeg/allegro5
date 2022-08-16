/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 * Allegro5 Creative Voice Audio Reader.
 *
 * Loosely based on A4 voc loader and tightly based on specs of the soundblaster
 * hardware programming manual:
 * <http://pdos.csail.mit.edu/6.828/2007/readings/hardware/SoundBlaster.pdf#1254>
 * also specs are available at
 * <http://sox.sourceforge.net/AudioFormats-11.html> section 11.5
 *
 * List of VOC audio codecs supported:
 * supported 0x0000  8-bit unsigned PCM
 *           0x0001  Creative 8-bit to 4-bit ADPCM    *HW implemented on the SB
 *           0x0002  Creative 8-bit to 3-bit ADPCM    *HW implemented on the SB
 *           0x0003  Creative 8-bit to 2-bit ADPCM    *HW implemented on the SB
 * supported 0x0004  16-bit signed PCM
 *
 * these are unsupported and present only in VOC files version 1.20 and above
 *           0x0006  CCITT a-Law                      * not really used
 *           0x0007  CCITT u-Law                      * not really used
 *           0x0200  Creative 16-bit to 4-bit ADPCM   *HW implemented on the SB
 *
 *
 * author: pkrcel (aka Andrea Provasi) <pkrcel@gmail.com>
 *
 * Revisions:
 *            2015-01-03 Source derived from previous sndfile implementation
 *                       cleaned up and put decoder into voc.c
 *            2015-01-08 Clean up ISO C90 related warnings and tried to get
 *                       a consistent code style and removed some comments.
 *            2015-01-15 Corrected style and enriched header.
 */


#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "acodec.h"
#include "helper.h"

ALLEGRO_DEBUG_CHANNEL("voc")

typedef struct AL_VOC_DATA AL_VOC_DATA;

struct AL_VOC_DATA {
   ALLEGRO_FILE   *file;
   size_t         datapos;
   int            samplerate;
   short          bits;        /* 8 (unsigned char) or 16 (signed short) */
   short          channels;    /* 1 (mono) or 2 (stereo) */
   int            sample_size; /* channels * bits/8 */
   int            samples;     /* # of samples. size = samples * sample_size */
};

/*
 * Fills in a VOC data header with information obtained  by opening an
 * caller-provided ALLEGRO_FILE.
 * The datapos index will be the first data byte of the first data block which
 * contains ACTUAL data.
 */

#define READNBYTES(f, data, n, retv)                                           \
   do {                                                                        \
      if (al_fread(f, &data, n) != n) {                                        \
         ALLEGRO_WARN("voc_open: Bad Number of bytes read in last operation"); \
         return retv;                                                          \
      }                                                                        \
   } while(0)


static AL_VOC_DATA *voc_open(ALLEGRO_FILE *fp)
{
   AL_VOC_DATA *vocdata;
   char hdrbuf[0x16];
   size_t readcount = 0;

   uint8_t blocktype = 0;
   uint8_t x = 0;

   uint16_t timeconstant = 0;
   uint16_t format = 0;    // can be 16-bit in Blocktype 9 (voc version > 1.20
   uint16_t vocversion = 0;
   uint16_t checkver = 0;  // must be  1's complement of vocversion + 0x1234

   uint32_t blocklength = 0; //length is stored in 3 bites LE, gd byteorder.

   ASSERT(fp);

   /* init VOC data */
   vocdata = al_malloc(sizeof(AL_VOC_DATA));
   memset(vocdata, 0, sizeof(*vocdata));
   memset(hdrbuf, 0, sizeof(hdrbuf));
   vocdata->file = fp;

   /* Begin checking the Header info */
   readcount = al_fread(fp, hdrbuf, 0x16);
   if (readcount != 0x16                                       /*short header*/
       || memcmp(hdrbuf, "Creative Voice File\x1A", 0x14) != 0 /*wrong id */
       || memcmp(hdrbuf+0x14, "\x1A\x00", 0x2) != 0) {         /*wrong offset */
      ALLEGRO_ERROR("voc_open: File does not appear to be a valid VOC file");
      return NULL;
   }

   READNBYTES(fp, vocversion, 2, NULL);
   if (vocversion != 0x10A && vocversion != 0x114) {   // known ver 1.10 -1.20
      ALLEGRO_ERROR("voc_open: File is of unknown version");
      return NULL;
   }
   /* checksum version check */
   READNBYTES(fp, checkver, 2, NULL);
   if (checkver != ~vocversion + 0x1234) {
      ALLEGRO_ERROR("voc_open: Bad VOC Version Identification Number");
      return NULL;
   }
   /*
    * We're at the first datablock, we shall check type and set all the relevant
    * info in the vocdata structure, including finally the datapos index to the
    * first valid data byte.
    */
   READNBYTES(fp, blocktype, 1, NULL);
   READNBYTES(fp, blocklength, 2, NULL);
   READNBYTES(fp, x, 1, NULL);
   blocklength += x<<16;
   switch (blocktype) {
      case 1:
         /* blocktype 1 is the most basic header with 1byte format, time
          * constant and length equal to (datalength + 2).
          */
         blocklength -= 2;
         READNBYTES(fp, timeconstant, 1, NULL);
         READNBYTES(fp, format, 1, NULL);
         vocdata->bits = 8; /* only possible codec for Blocktype 1 */
         vocdata->channels = 1;  /* block 1 alone means MONO */
         vocdata->samplerate = 1000000 / (256 - timeconstant);
         vocdata->sample_size = 1; /* or better: 1 * 8 / 8 */
         /*
          * Expected number of samples is at LEAST what is in this block.
          * IIF lentgh 0xFFF there will be a following blocktype 2.
          * We will deal with this later in load_voc.
          */
         vocdata->samples = blocklength / vocdata->sample_size;
         vocdata->datapos = al_ftell(fp);
         break;
      case 8:
         /* Blocktype 8 is enhanced data block (mainly for Stereo samples I
          * guess) that precedes a Blocktype 1, of which the sound infor have
          * to be ignored.
          * We skip to the end of the following Blocktype 1 once we get all the
          * required header info.
          */
         if (blocklength != 4) {
            ALLEGRO_ERROR("voc_open: Got opening Blocktype 8 of wrong length");
            return NULL;
         }
         READNBYTES(fp, timeconstant, 2, NULL);
         READNBYTES(fp, format, 1, NULL);
         READNBYTES(fp, vocdata->channels, 1, NULL);
         vocdata->channels += 1; /* was 0 for mono, 1 for stereo */
         vocdata->bits = 8; /* only possible codec for Blocktype 8 */
         vocdata->samplerate = 1000000 / (256 - timeconstant);
         vocdata->samplerate /= vocdata->channels;
         vocdata->sample_size = vocdata->channels * vocdata->bits / 8;
         /*
          * Now following there is a blocktype 1 which tells us the length of
          * the data block and all other info are discarded.
          */
         READNBYTES(fp, blocktype, 1, NULL);
         if (blocktype != 1) {
            ALLEGRO_ERROR("voc_open: Blocktype following type 8 is not 1");
            return NULL;
         }
         READNBYTES(fp, blocklength, 2, NULL);
         READNBYTES(fp, x, 1, NULL);
         blocklength += x<<16;
         blocklength -= 2;
         READNBYTES(fp, x, 2, NULL);
         vocdata->samples = blocklength / vocdata->sample_size;
         vocdata->datapos = al_ftell(fp);
         break;
      case 9:
         /*
          * Blocktype 9 is available only for VOC version 1.20 and above.
          * Deals with 16-bit codecs and stereo and is richier than blocktype 1
          * or the BLocktype 8+1 combo
          * Length is 12 bytes more than actual data.
          */
         blocklength -= 12;
         READNBYTES(fp, vocdata->samplerate, 4, NULL);   // actual samplerate
         READNBYTES(fp, vocdata->bits, 1, NULL);         // actual bits
                                                         // after compression
         READNBYTES(fp, vocdata->channels, 1, NULL);     // actual channels
         READNBYTES(fp, format, 2, NULL);
         if ((vocdata->bits != 8 && vocdata->bits != 16) ||
             (format != 0 && format != 4)) {
            ALLEGRO_ERROR("voc_open: unsupported CODEC in voc data");
            return NULL;
         }
         READNBYTES(fp, x, 4, NULL);         // just skip 4 reserved bytes
         vocdata->datapos = al_ftell(fp);
         break;
      case 2:               //
      case 3:               // these cases are just
      case 4:               // ignored in this span
      case 5:               // and wll not return a
      case 6:               // valid VOC data.
      case 7:               //
      default:
         ALLEGRO_ERROR("voc_open: opening Block is of unsupported type");
         return NULL;
         break;
   }
   return vocdata;
}

static void voc_close(AL_VOC_DATA *vocdata)
{
   ASSERT(vocdata);

   al_free(vocdata);
}


ALLEGRO_SAMPLE *_al_load_voc(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);
   
   ALLEGRO_INFO("Loading VOC sample %s.\n", filename);
   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   spl = _al_load_voc_f(f);

   al_fclose(f);

   return spl;
}


ALLEGRO_SAMPLE *_al_load_voc_f(ALLEGRO_FILE *file)
{
   AL_VOC_DATA *vocdata;
   ALLEGRO_SAMPLE *sample = NULL;
   size_t pos = 0; /* where to write in the buffer */
   size_t read = 0; /*bytes read during last operation */
   char* buffer;

   size_t bytestoread = 0;
   bool endofvoc = false;

   /*
    * Open file and populate VOC DATA, then create a buffer for the number of
    * samples of the frst block.
    * Iterate on the following blocks till EOF or terminator block
    */
   vocdata = voc_open(file);
   if (!vocdata) return NULL;

   ALLEGRO_DEBUG("channels %d\n", vocdata->channels);
   ALLEGRO_DEBUG("word_size %d\n", vocdata->sample_size);
   ALLEGRO_DEBUG("rate %d\n", vocdata->samplerate);
   ALLEGRO_DEBUG("first_block_samples %d\n", vocdata->samples);
   ALLEGRO_DEBUG("first_block_size %d\n", vocdata->samples * vocdata->sample_size);

   /*
    * Let's allocate at least the first block's bytes;
    */
   buffer = al_malloc(vocdata->samples * vocdata->sample_size);
   if (!buffer) {
      return NULL;
   }
   /*
    * We now need to iterate over data blocks till either we hit end of file
    * or we find a terminator block.
    */
   bytestoread = vocdata->samples * vocdata->sample_size;
   while(!endofvoc && !al_feof(vocdata->file)) {
      uint32_t blocktype = 0;
      uint32_t x = 0, len = 0;
      read = al_fread(vocdata->file, buffer, bytestoread);
      pos += read;
      READNBYTES(vocdata->file, blocktype, 1, NULL);   // read next block type
      if (al_feof(vocdata->file)) break;
      switch (blocktype) {
         case 0:{  /* we found a terminator block */
            endofvoc = true;
            break;
            }
         case 2:{  /*we found a continuation block: unlikely but handled */
            x = 0;
            bytestoread = 0;
            READNBYTES(vocdata->file, bytestoread, 2, NULL);
            READNBYTES(vocdata->file, x, 1, NULL);
            bytestoread += x<<16;
            /* increase subsequently storage */
            buffer = al_realloc(buffer, sizeof(buffer) + bytestoread);
            break;
            }
         case 1:   // we found a NEW data block starter, I assume this is wrong
         case 8:   // and let the so far read sample data correctly in the
         case 9:{   // already allocated buffer.
            endofvoc = true;
            break;
            }
         case 3:     /* we found a pause block */
         case 4:     /* we found a marker block */
         case 5:     /* we found an ASCII c-string block */
         case 6:     /* we found a repeat block */
         case 7:{    /* we found an end repeat block */
                     /* all these blocks will be skipped */
            unsigned int ii;
            len = 0;
            x = 0;
            READNBYTES(vocdata->file, len, 2, NULL);
            READNBYTES(vocdata->file, x, 1, NULL);
            len += x<<16;  // this is the length what's left to skip */
            for (ii = 0; ii < len ; ++ii) {
               al_fgetc(vocdata->file);
            }
            bytestoread = 0;  //should let safely check for the next block */
            break;
            }
         default:
            break;
      }
   }

   sample = al_create_sample(buffer, pos, vocdata->samplerate,
                             _al_word_size_to_depth_conf(vocdata->sample_size),
                             _al_count_to_channel_conf(vocdata->channels),
                             true);
   if (!sample)
      al_free(buffer);

   voc_close(vocdata);

   return sample;
}

/*
 * So far a stream implementation is not provided, since it is deemed unlikely
 * that this format will ever be used as such.
 */


bool _al_identify_voc(ALLEGRO_FILE *f)
{
   uint8_t x[22];
   if (al_fread(f, x, 22) < 22)
      return false;
   if (memcmp(x, "Creative Voice File\x1A\x1A\x00", 22) == 0)
      return true;
   return false;
}
