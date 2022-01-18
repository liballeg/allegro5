#include "game.h"
#include "data.h"
#include "bullet.h"
#include "expl.h"
#include "star.h"
#include "aster.h"

#define BULLET_DELAY   20

/* the current score */
int score;
static char score_buf[80];
int player_x_pos;
int player_hit;
int ship_state;
int skip_count;

/* info about the state of the game */
static int dead;
static int xspeed, yspeed, ycounter;
static int ship_burn;
static int ship_count;
static int skip_speed;
static volatile int frame_count, fps;
static volatile unsigned int game_time;
static unsigned int prev_bullet_time;
static int new_asteroid_time;
static int score_pos;
static ALLEGRO_SAMPLE_ID engine;

#define MAX_SPEED       32



/* handles fade effects and timing for the ready, steady, go messages */
static int fade_intro_item(int music_pos, int fade_speed)
{
   (void)music_pos;
   set_palette(data[GAME_PAL].dat);
   fade_out(fade_speed);
   return keypressed();
}



/* draws one of the ready, steady, go messages */
static void draw_intro_item(int item, int size)
{
   BITMAP *b = (BITMAP *) data[item].dat;
   int w = MIN(SCREEN_W, (SCREEN_W * 2 / size));
   int h = SCREEN_H / size;

   //clear_bitmap(screen);
   int bw = al_get_bitmap_width(b);
   int bh = al_get_bitmap_height(b);
   al_clear_to_color(makecol(0, 0, 0));
   stretch_blit(b, 0, 0, bw, bh, (SCREEN_W - w) / 2,
                (SCREEN_H - h) / 2, w, h);
   al_flip_display();
}



/* the main game update function */
static void move_everyone(void)
{
   BULLET *bullet;

   score++;

   if (player_hit) {
      /* player dead */
      if (skip_count <= 0) {
         if (ship_state >= EXPLODE_FLAG + EXPLODE_FRAMES - 1) {
            if (--ship_count <= 0)
               dead = TRUE;
            else
               player_hit = FALSE;
         }
         else
            ship_state++;

         if (yspeed)
            yspeed--;
      }
   }
   else {
      if (skip_count <= 0) {
         if ((key[ALLEGRO_KEY_LEFT])) { // || (joy[0].stick[0].axis[0].d1)) {
            /* moving left */
            if (xspeed > -MAX_SPEED)
               xspeed -= 2;
         }
         else if ((key[ALLEGRO_KEY_RIGHT])) { // || (joy[0].stick[0].axis[0].d2)) {
            /* moving right */
            if (xspeed < MAX_SPEED)
               xspeed += 2;
         }
         else {
            /* decelerate */
            if (xspeed > 0)
               xspeed -= 2;
            else if (xspeed < 0)
               xspeed += 2;
         }
      }

      /* which ship sprite to use? */
      if (xspeed < -24)
         ship_state = SHIP1;
      else if (xspeed < -2)
         ship_state = SHIP2;
      else if (xspeed <= 2)
         ship_state = SHIP3;
      else if (xspeed <= 24)
         ship_state = SHIP4;
      else
         ship_state = SHIP5;

      /* move player */
      player_x_pos += xspeed;
      if (player_x_pos < (32 << SPEED_SHIFT)) {
         player_x_pos = (32 << SPEED_SHIFT);
         xspeed = 0;
      }
      else if (player_x_pos >= ((SCREEN_W - 32) << SPEED_SHIFT)) {
         player_x_pos = ((SCREEN_W - 32) << SPEED_SHIFT);
         xspeed = 0;
      }

      if (skip_count <= 0) {
         if ((key[ALLEGRO_KEY_UP])) { // || (joy[0].stick[0].axis[1].d1)) {
            /* firing thrusters */
            if (yspeed < MAX_SPEED) {
               if (yspeed == 0) {
                  al_stop_sample(&engine);
                  al_play_sample(data[ENGINE_SPL].dat, 0.9, -1 + 2 * PAN(player_x_pos >> SPEED_SHIFT) / 255.0,
                     1.0, ALLEGRO_PLAYMODE_LOOP, &engine);
               }
               else {
                  /* fade in sample while speeding up */
                  ALLEGRO_SAMPLE_INSTANCE *si = al_lock_sample_id(&engine);
                  al_set_sample_instance_gain(si, yspeed * 64 / MAX_SPEED / 255.0);
                  al_set_sample_instance_pan(si, -1 + 2 * PAN(player_x_pos >> SPEED_SHIFT) / 255.0);
                  al_unlock_sample_id(&engine);
               }
               yspeed++;
            }
            else {
               /* adjust pan while the sample is looping */
               ALLEGRO_SAMPLE_INSTANCE *si = al_lock_sample_id(&engine);
               al_set_sample_instance_gain(si, 64 / 255.0);
               al_set_sample_instance_pan(si, -1 + 2 * PAN(player_x_pos >> SPEED_SHIFT) / 255.0);
               al_unlock_sample_id(&engine);
            }

            ship_burn = TRUE;
            score++;
         }
         else {
            /* not firing thrusters */
            if (yspeed) {
               yspeed--;
               if (yspeed == 0) {
                  al_stop_sample(&engine);
               }
               else {
                  /* fade out and reduce frequency when slowing down */
                  ALLEGRO_SAMPLE_INSTANCE *si = al_lock_sample_id(&engine);
                  al_set_sample_instance_gain(si, yspeed * 64 / MAX_SPEED / 255.0);
                  al_set_sample_instance_pan(si, -1 + 2 * PAN(player_x_pos >> SPEED_SHIFT) / 255.0);
                  al_set_sample_instance_speed(si, (500 + yspeed * 500 / MAX_SPEED) / 1000.0);
                  al_unlock_sample_id(&engine);
               }
            }

            ship_burn = FALSE;
         }
      }
   }

   /* if going fast, move everyone else down to compensate */
   if (yspeed) {
      ycounter += yspeed;

      while (ycounter >= (1 << SPEED_SHIFT)) {
         for (bullet = bullet_list; bullet; bullet = bullet->next)
            bullet->y++;

         scroll_stars();

         scroll_asteroids();

         ycounter -= (1 << SPEED_SHIFT);
      }
   }

   move_bullets();

   starfield_2d();

   /* fire bullet? */
   if (!player_hit) {
      if ((key[ALLEGRO_KEY_SPACE] || key[ALLEGRO_KEY_ENTER] || key[ALLEGRO_KEY_LCTRL] || key[ALLEGRO_KEY_RCTRL])) {
         // ||(joy[0].button[0].b) || (joy[0].button[1].b)) {
         if (prev_bullet_time + BULLET_DELAY < game_time) {
            bullet = add_bullet((player_x_pos >> SPEED_SHIFT) - 2, SCREEN_H - 64);
            if (bullet) {
               play_sample(data[SHOOT_SPL].dat, 100, PAN(bullet->x), 1000,
                           FALSE);
               prev_bullet_time = game_time;
            }
         }
      }
   }

   move_asteroids();

   if (skip_count <= 0) {
      skip_count = skip_speed;

      /* make a new alien? */
      new_asteroid_time++;

      if (new_asteroid_time > 600) {
         add_asteroid();
         new_asteroid_time = 0;
      }
   }
   else
      skip_count--;
}



/* main screen update function */
static void draw_screen(void)
{
   int x;
   RLE_SPRITE *spr;

   //acquire_bitmap(bmp);
   al_clear_to_color(makecol(0, 0, 0));

   draw_starfield_2d();

   /* draw the player */
   x = player_x_pos >> SPEED_SHIFT;

   if ((ship_burn) && (ship_state < EXPLODE_FLAG)) {
      spr = data[ENGINE1 + (retrace_count() / 4) % 7].dat;
      int sprw = al_get_bitmap_width(spr);
      draw_sprite(spr, x - sprw / 2, SCREEN_H - 24);
   }

   if (ship_state >= EXPLODE_FLAG)
      spr = explosion[ship_state - EXPLODE_FLAG];
   else
      spr = (RLE_SPRITE *) data[ship_state].dat;

   int sprw = al_get_bitmap_width(spr);
   int sprh = al_get_bitmap_height(spr);
   draw_sprite(spr, x - sprw / 2, SCREEN_H - 42 - sprh / 2);
   
   /* draw the asteroids */
   draw_asteroids();

   /* draw the bullets */
   draw_bullets();

   /* draw the score and fps information */
   if (fps)
      sprintf(score_buf, "Lives: %d - Score: %d - (%d fps)",
              ship_count, score, fps);
   else
      sprintf(score_buf, "Lives: %d - Score: %d", ship_count, score);

   textout(data[TITLE_FONT].dat, score_buf, 0, 0, get_palette(7));
  
   //release_bitmap(bmp);
}



static void move_score(void)
{
   score_pos++;
   if (score_pos > SCREEN_H)
      score_pos = 0;
}



static void draw_score(void)
{
   ALLEGRO_TRANSFORM tr;
   al_identity_transform(&tr);
   al_rotate_transform(&tr, score_pos * ALLEGRO_PI / 300);
   al_translate_transform(&tr, score_pos, score_pos);
   al_use_transform(&tr);
   textout_centre(data[END_FONT].dat, "GAME OVER", 0, -24, get_palette(2));
   sprintf(score_buf, "Score: %d", score);
   textout_centre(data[END_FONT].dat, score_buf, 0, 24, get_palette(2));
   al_identity_transform(&tr);
   al_use_transform(&tr);
}


/* sets up and plays the game */
void play_game(void)
{
   int esc = FALSE;
   unsigned int prev_update_time;

   stop_midi();

   /* set up a load of globals */
   dead = player_hit = FALSE;
   player_x_pos = (SCREEN_W / 2) << SPEED_SHIFT;
   xspeed = yspeed = ycounter = 0;
   ship_state = SHIP3;
   ship_burn = FALSE;
   ship_count = 3;              /* 3 lives instead of one... */
   frame_count = fps = 0;
   bullet_list = NULL;
   score = 0;
   score_pos = 0;

   skip_count = 0;
   if (SCREEN_W < 400)
      skip_speed = 0;
   else if (SCREEN_W < 700)
      skip_speed = 1;
   else if (SCREEN_W < 1000)
      skip_speed = 2;
   else
      skip_speed = 3;

   init_starfield_2d();

   init_asteroids();
   new_asteroid_time = 0;

   /* introduction synced to the music */
   draw_intro_item(INTRO_BMP_1, 5);
   play_midi(data[GAME_MUSIC].dat, TRUE);
   clear_keybuf();

   if (fade_intro_item(-1, 2))
      goto skip;

   draw_intro_item(INTRO_BMP_2, 4);
   if (fade_intro_item(5, 2))
      goto skip;

   draw_intro_item(INTRO_BMP_3, 3);
   if (fade_intro_item(9, 4))
      goto skip;

   draw_intro_item(INTRO_BMP_4, 2);
   if (fade_intro_item(11, 4))
      goto skip;

   draw_intro_item(GO_BMP, 4);
   if (fade_intro_item(13, 16))
      goto skip;

   draw_intro_item(GO_BMP, 4);
   if (fade_intro_item(14, 16))
      goto skip;

   draw_intro_item(GO_BMP, 4);
   if (fade_intro_item(15, 16))
      goto skip;

   draw_intro_item(GO_BMP, 4);
   if (fade_intro_item(16, 16))
      goto skip;

 skip:

   al_clear_to_color(makecol(0, 0, 0));

   clear_keybuf();

   set_palette(data[GAME_PAL].dat);

   game_time = 0;
   prev_bullet_time = 0;
   prev_update_time = 0;
   int speed = 6400 / SCREEN_W;
   double t0 = al_get_time();
   double fps_t0 = t0;

   if (speed < 4) speed = 4;

   /* main game loop */
   while (!esc) {
      int updated = 0;
      double t = al_get_time();

      poll_input();
      //poll_joystick();

      game_time = (t - t0) * 1000 / speed;
      if (t > fps_t0 + 1) {
         fps_t0 = t;
         fps = frame_count;
         frame_count = 0;
      }

      while (prev_update_time < game_time) {
         if (dead) {
            move_score();
         }
         else {
            move_everyone();
            if (dead) {
               clear_keybuf();
            }
         }
         prev_update_time++;
         updated = 1;
      }

      if (max_fps || updated) {
         draw_screen();
         if (dead) {
            draw_score();
         }

         al_flip_display();

         frame_count++;
      }

      /* rest for a short while if we're not in CPU-hog mode and too fast */
      if (!max_fps && !updated) {
         rest(1);
      }

      poll_input();

      if (key[ALLEGRO_KEY_ESCAPE] || (dead && (
         keypressed()))) // || joy[0].button[0].b || joy[0].button[1].b)))
         esc = TRUE;
   }

   /* cleanup */
   al_stop_sample(&engine);

   while (bullet_list)
      delete_bullet(bullet_list);

   if (esc) {
      do {
        poll_input();
      } while (key[ALLEGRO_KEY_ESCAPE]);

      fade_out(5);
      return;
   }

   clear_keybuf();
   fade_out(5);
}
