/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Main player control functions.
 */

#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "speed.h"



/* how many lives do we have left? */
int lives;



/* counters for various time delays */
static int init_time;
static int die_time;
static int fire_time;



/* current position and velocity */
static float pos;
static float vel;



/* which segments of health we currently possess */
#define SEGMENTS 16

static int ganja[SEGMENTS];

/* Did you ever come across a DOS shareware game called "Ganja Farmer"?
 * It is really very cool: you have to protect your crop from "Da Man",
 * who is trying to bomb it, spray it with defoliants, etc. Superb
 * reggae soundtrack, and the gameplay concept here is kind of similar,
 * hence my variable names...
 */



/* tell other people where we are */
float player_pos()
{
   return pos;
}



/* tell other people where to avoid us */
float find_target(float x)
{
   int seg = (int)(x * SEGMENTS) % SEGMENTS;
   int i, j;

   if (seg < 0)
      seg += SEGMENTS;

   for (i=0; i<SEGMENTS/2; i++) {
      j = (seg+i) % SEGMENTS;

      if (ganja[j])
	 return (float)j / SEGMENTS + (0.5 / SEGMENTS);

      j = (seg-i) % SEGMENTS;
      if (j < 0)
	 j += SEGMENTS;

      if (ganja[j])
	 return (float)j / SEGMENTS + (0.5 / SEGMENTS);
   }

   if (pos < 0.5)
      return pos + 0.5;
   else
      return pos - 0.5;
}



/* tell other people whether we are healthy */
int player_dying()
{
   return ((die_time != 0) && (lives <= 1));
}



/* called by the badguys when they want to blow us up */
int kill_player(float x, float y)
{
   int ret = FALSE;

   if (y >= 0.97) {
      int seg = (int)(x * SEGMENTS) % SEGMENTS; 

      if (ganja[seg]) {
	 ganja[seg] = FALSE;
	 explode(x, 0.98, 1);
	 sfx_explode_block();
      }

      ret = TRUE;
   }

   if ((y >= 0.95) && (!init_time) && (!die_time) && (!cheat)) {
      float d = pos - x;

      if (d < -0.5)
	 d += 1;
      else if (d > 0.5)
	 d -= 1;

      if (ABS(d) < 0.06) {
	 die_time = 128;

	 explode(x, 0.98, 2);
	 explode(x, 0.98, 4);
	 sfx_explode_player();

	 message("Ship Destroyed");

	 ret = TRUE;
      }
   }

   return ret;
}



/* initialises the player functions */
void init_player()
{
   int i;

   lives = 3;

   init_time = 128;
   die_time = 0;
   fire_time = 0;

   pos = 0.5;
   vel = 0;

   for (i=0; i<SEGMENTS; i++)
      ganja[i] = TRUE;
}



/* closes down the player module */
void shutdown_player()
{
}



/* advances the player to the next attack wave */
void advance_player(int cycle)
{
   char buf[80];
   int bonus = 0;
   int old_score = score;
   int i;

   for (i=0; i<SEGMENTS; i++) {
      if (ganja[i])
	 bonus++;
      else
	 ganja[i] = TRUE;
   }

   if (bonus == SEGMENTS) {
      message("Bonus: 100");
      score += 100;
   }

   if (cycle) {
      message("Bonus: 1000");
      score += 1000;
   }

   sprintf(buf, "Score: %d", bonus);
   message(buf);

   score += bonus;

   if ((get_hiscore() > 0) && (score > get_hiscore()) && (old_score <= get_hiscore())) {
      message("New Record Score!");
      sfx_ping(3);
   }
   else if ((bonus == SEGMENTS) || (cycle)) {
      sfx_ping(1);
   }
}



/* updates the player position */
int update_player()
{
   poll_input();

   /* quit game? */ 
   if (key[ALLEGRO_KEY_ESCAPE])
      return -1;

   /* safe period while initing */
   if (init_time)
      init_time--;

   /* blown up? */
   if (die_time) {
      die_time--;

      if (!die_time) {
	 lives--;
	 if (!lives)
	    return 1;

	 init_time = 128;
	 pos = 0.5;
	 vel = 0;

	 if (lives == 1)
	    message("This Is Your Final Life");
	 else
	    message("One Life Remaining");
      }
   }

   /* handle user left/right input */
   if (!die_time) {
      if ((joy_left) || (key[ALLEGRO_KEY_LEFT]))
	 vel -= 0.005;

      if ((joy_right) || (key[ALLEGRO_KEY_RIGHT]))
	 vel += 0.005;
   }

   /* move left and right */
   pos += vel;

   if (pos >= 1.0)
      pos -= 1.0;

   if (pos < 0.0)
      pos += 1.0;

   vel *= 0.67;

   /* fire bullets */
   if ((!die_time) && (!init_time) && (!fire_time)) {
      if ((key[ALLEGRO_KEY_SPACE]) || (joy_b1)) {
	 fire_bullet();
	 fire_time = 24;
      }
   }

   if (fire_time)
      fire_time--;

   return 0;
}



/* draws the player */
void draw_player(int r, int g, int b, int (*project)(float *f, int *i, int c))
{
   float shape[12];
   int ishape[12];
   int i;

   /* draw health segments */
   for (i=0; i<SEGMENTS; i++) {
      if (ganja[i]) {
	 shape[0] = (float)i / SEGMENTS;
	 shape[1] = 0.98;

	 shape[2] = (float)(i+1) / SEGMENTS;
	 shape[3] = 0.98;

	 shape[4] = (float)(i+1) / SEGMENTS;
	 shape[5] = 1.0;

	 shape[6] = (float)i / SEGMENTS;
	 shape[7] = 1.0;

	 if (project(shape, ishape, 8))
	    polygon(4, ishape, makecol(r/3, g/3, b/3));
      }
   }

   /* flash on and off while initing, don't show while dead */
   if ((init_time & 4) || (die_time))
      return;

   /* draw the ship */
   shape[0] = pos - 0.04;
   shape[1] = 0.98;

   shape[2] = pos - 0.02;
   shape[3] = 0.97;

   shape[4] = pos;
   shape[5] = 0.95;

   shape[6] = pos + 0.02;
   shape[7] = 0.97;

   shape[8] = pos + 0.04;
   shape[9] = 0.98;

   shape[10] = pos;
   shape[11] = 0.98;

   if (project(shape, ishape, 12))
      polygon(6, ishape, makecol(r, g, b));
}


