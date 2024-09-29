#include "expl.h"

RLE_SPRITE *explosion[EXPLODE_FRAMES];

/* the explosion graphics are pregenerated, using a simple particle system */
void generate_explosions(void)
{
   uint8_t bmp[EXPLODE_SIZE * EXPLODE_SIZE];
   unsigned char *p;
   int c, c2;
   int x, y;
   int xx, yy;
   int color;

#define HOTSPOTS  128

   struct HOTSPOT {
      int x, y;
      int xc, yc;
   } hot[HOTSPOTS];

   for (c = 0; c < HOTSPOTS; c++) {
      hot[c].x = hot[c].y = (EXPLODE_SIZE / 2) << 16;
      hot[c].xc = (AL_RAND() & 0x1FFFF) - 0xFFFF;
      hot[c].yc = (AL_RAND() & 0x1FFFF) - 0xFFFF;
   }

   for (c = 0; c < EXPLODE_FRAMES; c++) {
      memset(bmp, 0, EXPLODE_SIZE * EXPLODE_SIZE);

      color = ((c < 16) ? c * 4 : (80 - c)) >> 2;

      for (c2 = 0; c2 < HOTSPOTS; c2++) {
         for (x = -12; x <= 12; x++) {
            for (y = -12; y <= 12; y++) {
               xx = (hot[c2].x >> 16) + x;
               yy = (hot[c2].y >> 16) + y;
               if ((xx > 0) && (yy > 0) && (xx < EXPLODE_SIZE)
                   && (yy < EXPLODE_SIZE)) {
                  p = bmp + yy * EXPLODE_SIZE + xx;
                  *p += (color >> ((ABS(x) + ABS(y)) / 3));
                  if (*p > 63)
                     *p = 63;
               }
            }
         }
         hot[c2].x += hot[c2].xc;
         hot[c2].y += hot[c2].yc;
      }

      explosion[c] = al_create_bitmap(EXPLODE_SIZE, EXPLODE_SIZE);
      A5O_LOCKED_REGION *locked = al_lock_bitmap(explosion[c], A5O_PIXEL_FORMAT_ABGR_8888_LE, A5O_LOCK_WRITEONLY);

      PALETTE *pal = data[GAME_PAL].dat;
      for (x = 0; x < EXPLODE_SIZE; x++) {
         for (y = 0; y < EXPLODE_SIZE; y++) {
            c2 = bmp[y * EXPLODE_SIZE + x];
            if (c2 < 8)
               c2 = 0;
            else
               c2 = 16 + c2 / 4;

            uint8_t *lp = (uint8_t *)locked->data + y * locked->pitch + x * 4;
            lp[0] = pal->rgb[c2].r * 255;
            lp[1] = pal->rgb[c2].g * 255;
            lp[2] = pal->rgb[c2].b * 255;
            if (c2 > 0)
               lp[3] = pal->rgb[c2].a * 127;
            else
               lp[3] = 0;
         }
      }

      
      al_unlock_bitmap(explosion[c]);
   }
}



void destroy_explosions(void)
{
   int c;
   for (c = 0; c < EXPLODE_FRAMES; c++)
      al_destroy_bitmap(explosion[c]);
}
