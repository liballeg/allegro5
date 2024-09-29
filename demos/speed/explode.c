/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Explosion graphics effect.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "speed.h"



/* explosion info */
typedef struct EXPLOSION
{
   float x;
   float y;
   int big;
   float time;
   struct EXPLOSION *next;
} EXPLOSION;


static EXPLOSION *explosions;



/* initialises the explode functions */
void init_explode()
{
   explosions = NULL;
}



/* closes down the explode module */
void shutdown_explode()
{
   EXPLOSION *e;

   while (explosions) {
      e = explosions;
      explosions = explosions->next;
      free(e);
   }
}



/* triggers a new explosion */
void explode(float x, float y, int big)
{
   EXPLOSION *e = malloc(sizeof(EXPLOSION));

   e->x = x;
   e->y = y;

   e->big = big;

   e->time = 0;

   e->next = explosions;
   explosions = e;
}



/* updates the explode position */
void update_explode()
{
   EXPLOSION **p = &explosions;
   EXPLOSION *e = explosions;
   EXPLOSION *tmp;

   while (e) {
      e->time += 1.0 / (e->big/2.0 + 1);

      if (e->time > 32) {
	 *p = e->next;
	 tmp = e;
	 e = e->next;
	 free(tmp);
      }
      else {
	 p = &e->next;
	 e = e->next;
      }
   }
}



/* draws explosions */
void draw_explode(int r, int g, int b, int (*project)(float *f, int *i, int c))
{
   EXPLOSION *e = explosions;
   float size = view_size();
   float pos[2];
   int ipos[2];
   int rr, gg, bb, c, s;
   A5O_COLOR col;

   while (e) {
      pos[0] = e->x;
      pos[1] = e->y;

      if (project(pos, ipos, 2)) {
	 s = e->time * size / (512 / (e->big + 1));

	 if ((!low_detail) && (e->time < 24)) {
	    c = (24 - e->time) * 255 / 24;
	    col = makecol(c, c, c);

	    circle(ipos[0], ipos[1], s*2, col);
	    circle(ipos[0], ipos[1], s*s/8, col);
	 }

	 if (e->time < 32) {
	    rr = (32 - e->time) * r / 32;
	    gg = (32 - e->time) * g / 32;
	    bb = (32 - e->time) * b / 32;

	    c = MAX((24 - e->time) * 255 / 24, 0);

	    rr = MAX(rr, c);
	    gg = MAX(gg, c);
	    bb = MAX(bb, c);

	    col = makecol(rr, gg, bb);

	    circlefill(ipos[0], ipos[1], s, col);
	 }
      }

      e = e->next;
   }
}

