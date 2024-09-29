#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

bool _al_identify_png(A5O_FILE *f)
{
   uint8_t x[8];
   al_fread(f, x, 8);
   if (memcmp(x, "\x89PNG\r\n\x1a\n", 8) != 0)
      return false;
   return true;
}

bool _al_identify_jpg(A5O_FILE *f)
{
   uint8_t x[4];
   uint16_t y;
   y = al_fread16be(f);
   if (y != 0xffd8)
      return false;
   al_fseek(f, 6 - 2, A5O_SEEK_CUR);
   al_fread(f, x, 4);
   if (memcmp(x, "JFIF", 4) != 0)
      return false;
   return true;
}

bool _al_identify_webp(A5O_FILE *f)
{
   uint8_t x[4];
   al_fread(f, x, 4);
   if (memcmp(x, "RIFF", 4) != 0)
      return false;
   al_fseek(f, 4, A5O_SEEK_CUR);
   al_fread(f, x, 4);
   if (memcmp(x, "WEBP", 4) != 0)
      return false;
   return true;
}
