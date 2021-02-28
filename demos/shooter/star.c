#include "star.h"

/* for the starfield */
#define MAX_STARS       512

volatile struct {
   al_fixed x, y, z;
   int ox, oy;
} star[MAX_STARS];

static int star_count = 0;
static int star_count_count = 0;



void init_starfield_2d(void)
{
   int c;
   for (c = 0; c < MAX_STARS; c++) {
      star[c].ox = AL_RAND() % SCREEN_W;
      star[c].oy = AL_RAND() % SCREEN_H;
      star[c].z = AL_RAND() & 7;
   }
}



void starfield_2d(void)
{
   int c;
   for (c = 0; c < MAX_STARS; c++) {
      if ((star[c].oy += ((int)star[c].z >> 1) + 1) >= SCREEN_H)
         star[c].oy = 0;
   }
}



void scroll_stars(void)
{
   int c;
   for (c = 0; c < MAX_STARS; c++) {
      if (++star[c].oy >= SCREEN_H)
         star[c].oy = 0;
   }
}



void draw_starfield_2d()
{
   int x, y, c;
   for (c = 0; c < MAX_STARS; c++) {
      y = star[c].oy;
      x = ((star[c].ox - SCREEN_W / 2) * (y / (4 - star[c].z / 2) +
                                          SCREEN_H) / SCREEN_H) +
          SCREEN_W / 2;

      al_draw_filled_circle(x, y, 1, get_palette(15 - (int)star[c].z));
   }
}



void init_starfield_3d(void)
{
   int c;
   for (c = 0; c < MAX_STARS; c++) {
      star[c].z = 0;
      star[c].ox = star[c].oy = -1;
   }
}

void starfield_3d(void)
{
   int c;
   al_fixed x, y;
   int ix, iy;

   for (c = 0; c < star_count; c++) {
      if (star[c].z <= itofix(1)) {
         x = itofix(AL_RAND() & 0xff);
         y = itofix(((AL_RAND() & 3) + 1) * SCREEN_W);

         star[c].x = fixmul(fixcos(x), y);
         star[c].y = fixmul(fixsin(x), y);
         star[c].z = itofix((AL_RAND() & 0x1f) + 0x20);
      }

      x = fixdiv(star[c].x, star[c].z);
      y = fixdiv(star[c].y, star[c].z);

      ix = (int)(x >> 16) + SCREEN_W / 2;
      iy = (int)(y >> 16) + SCREEN_H / 2;

      if ((ix >= 0) && (ix < SCREEN_W) && (iy >= 0)
          && (iy <= SCREEN_H)) {
         star[c].ox = ix;
         star[c].oy = iy;
         star[c].z -= 4096;
      }
      else {
         star[c].ox = -1;
         star[c].oy = -1;
         star[c].z = 0;
      }
   }

   /* wake up new star */
   if (star_count < MAX_STARS) {
      if (star_count_count++ >= 8) {
         star_count_count = 0;
         star_count++;
      }
   }
}



void draw_starfield_3d(void)
{
   int c, c2;
   for (c = 0; c < star_count; c++) {
      c2 = 7 - (int)(star[c].z >> 18);
      al_draw_filled_circle(star[c].ox, star[c].oy, 3, get_palette(CLAMP(0, c2, 7)));
   }
}
