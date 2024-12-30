/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Enemy control routines (attack waves).
 */

#include <math.h>
#include <allegro5/allegro.h>

#include "speed.h"



/* description of an attack wave */
typedef struct WAVEINFO
{
   int count;              /* how many to create */
   float delay;            /* how long to delay them */
   float delay_rand;       /* random delay factor */
   float speed;            /* how fast they move */
   float speed_rand;       /* speed random factor */
   float move;             /* movement speed */
   float move_rand;        /* movement random factor */
   float sin_depth;        /* depth of sine wave motion */
   float sin_depth_rand;   /* sine depth random factor */
   float sin_speed;        /* speed of sine wave motion */
   float sin_speed_rand;   /* sine speed random factor */
   int split;              /* split into multiple dudes? */
   int aggro;              /* attack the player? */
   int evade;              /* evade the player? */
} WAVEINFO;



#define END_WAVEINFO       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }



/* attack wave #1 (straight downwards) */
static WAVEINFO wave1[] =
{
   /* c  del  rnd  speed   r  mv r  dp r  sd r  sp ag ev */
   {  4, 0.0, 1.0, 0.0015, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0  },
   {  2, 0.7, 0.0, 0.0055, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
   {  2, 0.5, 0.0, 0.0045, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
   {  4, 0.0, 1.0, 0.0035, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
   {  4, 0.0, 1.0, 0.0025, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
   END_WAVEINFO
};



/* attack wave #2 (diagonal motion) */
static WAVEINFO wave2[] =
{
   /* c  del  rnd  speed   rnd     move    rnd    dp r  sd r  sp ag ev */
   {  6, 2.0, 1.0, 0.0025, 0.002,  0.0000, 0.020, 0, 0, 0, 0, 0, 0, 0  },
   {  6, 1.2, 1.0, 0.0025, 0.001,  0.0000, 0.010, 0, 0, 0, 0, 0, 0, 0  },
   {  6, 0.5, 1.0, 0.0025, 0.000,  0.0000, 0.005, 0, 0, 0, 0, 0, 0, 0  },
   {  4, 0.0, 1.0, 0.0025, 0.000, -0.0025, 0.000, 0, 0, 0, 0, 0, 0, 0  },
   {  4, 0.0, 1.0, 0.0025, 0.000,  0.0025, 0.000, 0, 0, 0, 0, 0, 0, 0  },
   END_WAVEINFO
};



/* attack wave #3 (sine wave motion) */
static WAVEINFO wave3[] =
{
   /* c  del  rnd  speed   rnd     mv r  sdepth rnd    sspd   rnd   sp ag ev */
   {  4, 0.0, 2.0, 0.0020, 0.0000, 0, 0, 0.005, 0.000, 0.020, 0.00, 1, 0, 0  },
   {  4, 1.5, 1.0, 0.0016, 0.0024, 0, 0, 0.002, 0.006, 0.010, 0.03, 0, 0, 0  },
   {  4, 1.0, 1.0, 0.0019, 0.0016, 0, 0, 0.003, 0.004, 0.015, 0.02, 0, 0, 0  },
   {  4, 0.5, 1.0, 0.0022, 0.0008, 0, 0, 0.004, 0.002, 0.020, 0.01, 0, 0, 0  },
   {  4, 0.0, 1.0, 0.0025, 0.0000, 0, 0, 0.005, 0.000, 0.025, 0.00, 0, 0, 0  },
   END_WAVEINFO
};



/* attack wave #4 (evade you) */
static WAVEINFO wave4[] =
{
   /* c  del  rnd  speed   rnd     mv rnd    dp r  sd r  sp ag ev */
   {  4, 0.0, 2.0, 0.0020, 0.0000, 0, 0.000, 0, 0, 0, 0, 1, 0, 1  },
   {  3, 1.5, 1.0, 0.0016, 0.0024, 0, 0.010, 0, 0, 0, 0, 0, 0, 1  },
   {  3, 1.0, 1.0, 0.0019, 0.0016, 0, 0.001, 0, 0, 0, 0, 0, 0, 1  },
   {  4, 0.5, 1.0, 0.0022, 0.0008, 0, 0.000, 0, 0, 0, 0, 0, 0, 1  },
   {  4, 0.0, 1.0, 0.0025, 0.0000, 0, 0.000, 0, 0, 0, 0, 0, 0, 1  },
   END_WAVEINFO
};



/* attack wave #5 (attack you) */
static WAVEINFO wave5[] =
{
   /* c  del  rnd  speed   rnd     mv rnd    sd r  sd r  sp ag ev */
   {  4, 0.0, 2.0, 0.0010, 0.0000, 0, 0.000, 0, 0, 0, 0, 1, 1, 0  },
   {  3, 1.5, 1.0, 0.0016, 0.0024, 0, 0.010, 0, 0, 0, 0, 0, 1, 0  },
   {  3, 1.0, 1.0, 0.0019, 0.0016, 0, 0.001, 0, 0, 0, 0, 0, 1, 0  },
   {  3, 0.5, 1.0, 0.0022, 0.0008, 0, 0.000, 0, 0, 0, 0, 0, 1, 0  },
   {  4, 0.0, 1.0, 0.0025, 0.0000, 0, 0.000, 0, 0, 0, 0, 0, 1, 0  },
   END_WAVEINFO
};



/* attack wave #6 (the boss wave, muahaha) */
static WAVEINFO wave6[] =
{
   /* c  del  rnd  speed  rnd    mv rnd   dp rnd    sd rnd   sp ag ev */
   {  8, 6.0, 2.0, 0.002, 0.001, 0, 0.00, 0, 0,     0, 0,    1, 1, 0  },
   {  8, 4.5, 2.0, 0.002, 0.001, 0, 0.00, 0, 0,     0, 0,    1, 0, 1  },
   {  8, 3.0, 2.0, 0.002, 0.001, 0, 0.00, 0, 0.006, 0, 0.03, 1, 0, 0  },
   {  8, 1.5, 2.0, 0.002, 0.001, 0, 0.01, 0, 0,     0, 0,    1, 0, 0  },
   {  8, 0.0, 2.0, 0.002, 0.001, 0, 0.00, 0, 0,     0, 0,    1, 0, 0  },
   END_WAVEINFO
};



/* list of available attack waves */
static WAVEINFO *waveinfo[] =
{
   wave1+4, wave2+4, wave3+4, wave4+4, wave5+4,
   wave1+3, wave2+3, wave3+3, wave4+3, wave5+3,
   wave1+2, wave2+2, wave3+2, wave4+2, wave5+2,
   wave1+1, wave2+1, wave3+1, wave4+1, wave5+1,
   wave1+0, wave2+0, wave3+0, wave4+0, wave5+0,
   wave6
};



/* info about someone nasty */
typedef struct BADGUY
{
   float x;                /* x position */
   float y;                /* y position */
   float speed;            /* vertical speed */
   float move;             /* horizontal motion */
   float sin_depth;        /* depth of sine motion */
   float sin_speed;        /* speed of sine motion */
   int split;              /* whether to split ourselves */
   int aggro;              /* whether to attack the player */
   int evade;              /* whether to evade the player */
   float v;                /* horizontal velocity */
   int t;                  /* integer counter */
   struct BADGUY *next;
} BADGUY;


static BADGUY *evildudes;

static int finished_counter;

static int wavenum;



/* creates a new swarm of badguys */
void lay_attack_wave(int reset)
{
   WAVEINFO *info;
   BADGUY *b;
   int i;

   if (reset) {
      wavenum = 0;
   }
   else {
      wavenum++;
      if (wavenum >= (int)(sizeof(waveinfo)/sizeof(WAVEINFO *)))
         wavenum = 0;
   }

   info = waveinfo[wavenum];

   while (info->count) {
      for (i=0; i<info->count; i++) {
         b = malloc(sizeof(BADGUY));

         #define URAND  ((float)(rand() & 255) / 255.0)
         #define SRAND  (((float)(rand() & 255) / 255.0) - 0.5)

         b->x = URAND;
         b->y = -info->delay - URAND * info->delay_rand;
         b->speed = info->speed + URAND * info->speed_rand;
         b->move = info->move + SRAND * info->move_rand;
         b->sin_depth = info->sin_depth + URAND * info->sin_depth_rand;
         b->sin_speed = info->sin_speed + URAND * info->sin_speed_rand;
         b->split = info->split;
         b->aggro = info->aggro;
         b->evade = info->evade;
         b->v = 0;
         b->t = rand() & 255;

         b->next = evildudes;
         evildudes = b;
      }

      info++;
   }

   finished_counter = 0;
}



/* initialises the badguy functions */
void init_badguys()
{
   evildudes = NULL;

   lay_attack_wave(TRUE);
}



/* closes down the badguys module */
void shutdown_badguys()
{
   BADGUY *b;

   while (evildudes) {
      b = evildudes;
      evildudes = evildudes->next;
      free(b);
   }
}



/* updates the badguy position */
int update_badguys()
{
   BADGUY **p = &evildudes;
   BADGUY *b = evildudes;
   BADGUY *tmp1, *tmp2;
   void *bullet;
   float x, y, d;
   int dead;

   /* testcode: enter clears the level */
   if ((cheat) && (key[ALLEGRO_KEY_ENTER])) {
      shutdown_badguys();
      b = NULL;

      while (key[ALLEGRO_KEY_ENTER])
         poll_input_wait();
   }

   while (b) {
      dead = FALSE;

      if (b->aggro) {
         /* attack the player */
         d = player_pos() - b->x;

         if (d < -0.5)
            d += 1;
         else if (d > 0.5)
            d -= 1;

         if (b->y < 0.5)
            d = -d;

         b->v *= 0.99;
         b->v += SGN(d) * 0.00025;
      }
      else if (b->evade) {
         /* evade the player */
         if (b->y < 0.75)
            d = player_pos() + 0.5;
         else
            d = b->x;

         if (b->move)
            d += SGN(b->move) / 16.0;

         d = find_target(d) - b->x;

         if (d < -0.5)
            d += 1;
         else if (d > 0.5)
            d -= 1;

         b->v *= 0.96;
         b->v += SGN(d) * 0.0004;
      }

      /* horizontal move */
      b->x += b->move + sin(b->t * b->sin_speed) * b->sin_depth + b->v;

      if (b->x < 0)
         b->x += 1;
      else if (b->x > 1)
         b->x -= 1;

      /* vertical move */
      b->y += b->speed;

      if ((b->y > 0.5) && (b->y - b->speed <= 0.5) && (b->split)) {
         /* split ourselves */
         tmp1 = malloc(sizeof(BADGUY));
         tmp2 = malloc(sizeof(BADGUY));

         *tmp1 = *tmp2 = *b;

         tmp1->move -= 0.001;
         tmp2->move += 0.001;

         b->speed += 0.001;

         tmp1->t = rand() & 255;
         tmp2->t = rand() & 255;

         tmp1->next = tmp2;
         tmp2->next = evildudes;
         evildudes = tmp1;
      }

      b->t++;

      if (b->y > 0) {
         if (kill_player(b->x, b->y)) {
            /* did we hit someone? */
            dead = TRUE;
         }
         else {
            /* or did someone else hit us? */
            bullet = get_first_bullet(&x, &y);

            while (bullet) {
               x = x - b->x;

               if (x < -0.5)
                  x += 1;
               else if (x > 0.5)
                  x -= 1;

               x = ABS(x);

               y = ABS(y - b->y);

               if (x < y)
                  d = y/2 + x;
               else
                  d = x/2 + y;

               if (d < 0.025) {
                  kill_bullet(bullet);
                  explode(b->x, b->y, 0);
                  sfx_explode_alien();
                  dead = TRUE;
                  break;
               }

               bullet = get_next_bullet(bullet, &x, &y);
            }
         }
      }

      /* advance to the next dude */
      if (dead) {
         *p = b->next;
         tmp1 = b;
         b = b->next;
         free(tmp1);
      }
      else {
         p = &b->next;
         b = b->next;
      }
   }

   if ((!evildudes) && (!player_dying())) {
      if (!finished_counter) {
         message("Wave Complete");
         sfx_ping(0);
      }

      finished_counter++;

      if (finished_counter > 64)
         return TRUE;
   }

   return FALSE;
}



/* draws the badguys */
void draw_badguys(int r, int g, int b, int (*project)(float *f, int *i, int c))
{
   BADGUY *bad = evildudes;
   ALLEGRO_COLOR c = makecol(r, g, b);
   float shape[12];
   int ishape[12];

   while (bad) {
      if (bad->y > 0) {
         shape[0] = bad->x - 0.02;
         shape[1] = bad->y + 0.01;

         shape[2] = bad->x;
         shape[3] = bad->y + 0.02;

         shape[4] = bad->x + 0.02;
         shape[5] = bad->y + 0.01;

         shape[6] = bad->x + 0.01;
         shape[7] = bad->y + 0.005;

         shape[8] = bad->x;
         shape[9] = bad->y - 0.015;

         shape[10] = bad->x - 0.01;
         shape[11] = bad->y + 0.005;

         if (project(shape, ishape, 12))
            polygon(6, ishape, c);
      }

      bad = bad->next;
   }
}

