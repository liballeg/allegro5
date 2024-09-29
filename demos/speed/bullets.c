/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Bullet animation and display routines.
 */

#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "speed.h"



/* bullet info */
typedef struct BULLET
{
   float x;
   float y;
   struct BULLET *next;
} BULLET;


static BULLET *bullets;



/* interface for the aliens to query bullet positions */
void *get_first_bullet(float *x, float *y)
{
   if (!bullets)
      return NULL;

   *x = bullets->x;
   *y = bullets->y;

   return bullets;
}



/* interface for the aliens to query bullet positions */
void *get_next_bullet(void *b, float *x, float *y)
{
   BULLET *bul = ((BULLET *)b)->next;

   if (!bul)
      return NULL;

   *x = bul->x;
   *y = bul->y;

   return bul;
}



/* interface for the aliens to get rid of bullets when they explode */
void kill_bullet(void *b)
{
   ((BULLET *)b)->y = -65536;
}



/* initialises the bullets functions */
void init_bullets()
{
   bullets = NULL;
}



/* closes down the bullets module */
void shutdown_bullets()
{
   BULLET *b;

   while (bullets) {
      b = bullets;
      bullets = bullets->next;
      free(b);
   }
}



/* fires a new bullet */
void fire_bullet()
{
   BULLET *b = malloc(sizeof(BULLET));

   b->x = player_pos();
   b->y = 0.96;

   b->next = bullets;
   bullets = b;

   sfx_shoot();
}



/* updates the bullets position */
void update_bullets()
{
   BULLET **p = &bullets;
   BULLET *b = bullets;
   BULLET *tmp;

   while (b) {
      b->y -= 0.025;

      if (b->y < 0) {
	 *p = b->next;
	 tmp = b;
	 b = b->next;
	 free(tmp);
      }
      else {
	 p = &b->next;
	 b = b->next;
      }
   }
}



/* draws the bullets */
void draw_bullets(int r, int g, int b, int (*project)(float *f, int *i, int c))
{
   BULLET *bul = bullets;
   A5O_COLOR c1 = makecol(128+r/2, 128+g/2, 128+b/2);
   A5O_COLOR c2 = (g) ? makecol(r/5, g/5, b/5) : makecol(r/4, g/4, b/4);
   float shape[6];
   int ishape[6];
   int i;

   while (bul) {
      if (bul->y > 0) {
	 shape[0] = bul->x - 0.005;
	 shape[1] = bul->y + 0.01;

	 shape[2] = bul->x + 0.005;
	 shape[3] = bul->y + 0.01;

	 shape[4] = bul->x;
	 shape[5] = bul->y - 0.015;

	 if (project(shape, ishape, 6)) {
	    polygon(3, ishape, c1);

	    if (!low_detail) {
	       float cx = (ishape[0] + ishape[2] + ishape[4]) / 3;
	       float cy = (ishape[1] + ishape[3] + ishape[5]) / 3;

	       float boxx[4] = { -1, -1,  1,  1 };
	       float boxy[4] = { -1,  1,  1, -1 };

	       for (i=0; i<4; i++) {
		  float rot = ((int)(bul->x*256) & 1) ? bul->y : -bul->y;

		  float tx = cos(rot)*boxx[i] + sin(rot)*boxy[i];
		  float ty = sin(rot)*boxx[i] - cos(rot)*boxy[i];

		  boxx[i] = tx * bul->y * view_size() / 8;
		  boxy[i] = ty * bul->y * view_size() / 8;
	       }

	       line(cx+boxx[0], cy+boxy[0], cx+boxx[1], cy+boxy[1], c2);
	       line(cx+boxx[1], cy+boxy[1], cx+boxx[2], cy+boxy[2], c2);
	       line(cx+boxx[2], cy+boxy[2], cx+boxx[3], cy+boxy[3], c2);
	       line(cx+boxx[3], cy+boxy[3], cx+boxx[0], cy+boxy[0], c2);
	    }
	 }
      }

      bul = bul->next;
   }
}

