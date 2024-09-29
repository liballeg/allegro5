/*
 * Allegro MOD file common routines
 * author: Matthew Leverton
 */


#define _FILE_OFFSET_BITS 64
#include <ctype.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"
#include "acodec.h"


A5O_DEBUG_CHANNEL("acodec")


bool _al_identify_it(A5O_FILE *f)
{
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "IMPM", 4) == 0)
      return true;
   return false;
}

bool _al_identify_669(A5O_FILE *f)
{
   uint8_t x[2];
   if (al_fread(f, x, 2) < 2)
      return false;
   if (memcmp(x, "if", 2) == 0 || memcmp(x, "JN", 2) == 0)
      return true;
   return false;
}

bool _al_identify_amf(A5O_FILE *f)
{
   uint8_t x[3];
   if (al_fread(f, x, 3) < 3)
      return false;
   if (memcmp(x, "AMF", 3) == 0)
      return true;
   return false;
}

bool _al_identify_asy(A5O_FILE *f)
{
   uint8_t x[24];
   if (al_fread(f, x, 24) < 24)
      return false;
   if (memcmp(x, "ASYLUM Music Format V1.0", 24) == 0)
      return true;
   return false;
}

bool _al_identify_mtm(A5O_FILE *f)
{
   uint8_t x[3];
   if (al_fread(f, x, 3) < 3)
      return false;
   if (memcmp(x, "MTM", 3) == 0)
      return true;
   return false;
}

bool _al_identify_okt(A5O_FILE *f)
{
   uint8_t x[8];
   if (al_fread(f, x, 8) < 8)
      return false;
   if (memcmp(x, "OKTASONG", 8) == 0)
      return true;
   return false;
}

bool _al_identify_psm(A5O_FILE *f)
{
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "PSM\x00", 4) == 0 || memcmp(x, "PSM\xFE", 4) == 0)
      return true;
   return false;
}

bool _al_identify_ptm(A5O_FILE *f)
{
   uint8_t x[4];
   if (!al_fseek(f, 0x2C, A5O_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "DSMF", 4) == 0)
      return true;
   return false;
}

bool _al_identify_riff(A5O_FILE *f)
{
   const char riff_fmts[][4] = {
      "AM  ", "AMFF", "DSMF"
   };
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "RIFF", 4) != 0)
      return false;
   if (!al_fseek(f, 4, A5O_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   for (size_t i = 0; i < sizeof(riff_fmts) / 4; ++i) {
      if (memcmp(x, riff_fmts[i], 4) == 0)
         return true;
   }
   return false;
}

bool _al_identify_stm(A5O_FILE *f)
{
   const char stm_fmts[][8] = {
      "!Scream!", "BMOD2STM", "WUZAMOD!"
   };
   uint8_t x[10];
   if (!al_fseek(f, 20, A5O_SEEK_CUR))
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

bool _al_identify_mod(A5O_FILE *f)
{
   const char mod_sigs[][4] = {
      "M.K.", "M!K!", "M&K!", "N.T.",
      "NSMS", "FLT4", "M\0\0\0", "8\0\0\0",
      "FEST", "FLT8", "CD81", "OCTA",
      "OKTA", "16CN", "32CN"
   };
   uint8_t x[4];
   if (!al_fseek(f, 0x438, A5O_SEEK_CUR))
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

bool _al_identify_s3m(A5O_FILE *f)
{
   uint8_t x[4];
   if (!al_fseek(f, 0x2C, A5O_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "SCRM", 4) == 0)
      return true;
   return false;
}

bool _al_identify_xm(A5O_FILE *f)
{
   uint8_t x[16];
   if (al_fread(f, x, 16) < 16)
      return false;
   if (memcmp(x, "Extended Module:", 16) == 0)
      return true;
   return false;
}

/* vim: set sts=3 sw=3 et: */
