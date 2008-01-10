#include "game.h"
#include "data.h"
#include "bullet.h"
#include "expl.h"
#include "star.h"
#include "aster.h"
#include "dirty.h"
#include "display.h"

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

#define MAX_SPEED       32



/* handles fade effects and timing for the ready, steady, go messages */
static int fade_intro_item(int music_pos, int fade_speed)
{
   while (midi_pos < music_pos) {
      if (keypressed())
         return TRUE;

      poll_joystick();

      if ((joy[0].button[0].b) || (joy[0].button[1].b))
         return TRUE;
   }

   set_palette(data[GAME_PAL].dat);
   fade_out(fade_speed);

   return FALSE;
}



/* draws one of the ready, steady, go messages */
static void draw_intro_item(int item, int size)
{
   BITMAP *b = (BITMAP *) data[item].dat;
   int w = MIN(SCREEN_W, (SCREEN_W * 2 / size));
   int h = SCREEN_H / size;

   clear_bitmap(screen);
   stretch_blit(b, screen, 0, 0, b->w, b->h, (SCREEN_W - w) / 2,
                (SCREEN_H - h) / 2, w, h);
}



/* timer callback for controlling the game speed */
static void game_timer(void)
{
   game_time++;
}

END_OF_STATIC_FUNCTION(game_timer);



/* timer callback for measuring the frames per second */
static void fps_proc(void)
{
   fps = frame_count;
   frame_count = 0;
}

END_OF_STATIC_FUNCTION(fps_proc);



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
         if ((key[KEY_LEFT]) || (joy[0].stick[0].axis[0].d1)) {
            /* moving left */
            if (xspeed > -MAX_SPEED)
               xspeed -= 2;
         }
         else if ((key[KEY_RIGHT]) || (joy[0].stick[0].axis[0].d2)) {
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
         if ((key[KEY_UP]) || (joy[0].stick[0].axis[1].d1)) {
            /* firing thrusters */
            if (yspeed < MAX_SPEED) {
               if (yspeed == 0) {
                  play_sample(data[ENGINE_SPL].dat, 0,
                              PAN(player_x_pos >> SPEED_SHIFT), 1000, TRUE);
               }
               else {
                  /* fade in sample while speeding up */
                  adjust_sample(data[ENGINE_SPL].dat,
                                yspeed * 64 / MAX_SPEED,
                                PAN(player_x_pos >> SPEED_SHIFT), 1000, TRUE);
               }
               yspeed++;
            }
            else {
               /* adjust pan while the sample is looping */
               adjust_sample(data[ENGINE_SPL].dat, 64,
                             PAN(player_x_pos >> SPEED_SHIFT), 1000, TRUE);
            }

            ship_burn = TRUE;
            score++;
         }
         else {
            /* not firing thrusters */
            if (yspeed) {
               yspeed--;
               if (yspeed == 0) {
                  stop_sample(data[ENGINE_SPL].dat);
               }
               else {
                  /* fade out and reduce frequency when slowing down */
                  adjust_sample(data[ENGINE_SPL].dat,
                                yspeed * 64 / MAX_SPEED,
                                PAN(player_x_pos >> SPEED_SHIFT),
                                500 + yspeed * 500 / MAX_SPEED, TRUE);
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
      if ((key[KEY_SPACE] || key[KEY_ENTER] || key[KEY_LCONTROL] || key[KEY_RCONTROL]) ||
          (joy[0].button[0].b) || (joy[0].button[1].b)) {
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
static void draw_screen(BITMAP *bmp)
{
   int x;
   RLE_SPRITE *spr;
   char *animation_type_str = NULL;

   switch (animation_type) {
      case DOUBLE_BUFFER:
	 animation_type_str = "double buffered";
	 break;
      case PAGE_FLIP:
	 animation_type_str = "page flipping";
	 break;
      case TRIPLE_BUFFER:
	 animation_type_str = "triple buffered";
	 break;
      case DIRTY_RECTANGLE:
	 animation_type_str = "dirty rectangles";
	 break;
   }

   acquire_bitmap(bmp);

   draw_starfield_2d(bmp);

   /* draw the player */
   x = player_x_pos >> SPEED_SHIFT;

   if ((ship_burn) && (ship_state < EXPLODE_FLAG)) {
      spr = (RLE_SPRITE *) data[ENGINE1 + (retrace_count / 4) % 7].dat;
      draw_rle_sprite(bmp, spr, x - spr->w / 2, SCREEN_H - 24);

      if (animation_type == DIRTY_RECTANGLE)
         dirty_rectangle(x - spr->w / 2, SCREEN_H - 24, spr->w, spr->h);
   }

   if (ship_state >= EXPLODE_FLAG)
      spr = explosion[ship_state - EXPLODE_FLAG];
   else
      spr = (RLE_SPRITE *) data[ship_state].dat;

   draw_rle_sprite(bmp, spr, x - spr->w / 2, SCREEN_H - 42 - spr->h / 2);
   if (animation_type == DIRTY_RECTANGLE)
      dirty_rectangle(x - spr->w / 2, SCREEN_H - 42 - spr->h / 2, spr->w, spr->h);

   /* draw the asteroids */
   draw_asteroids(bmp);

   /* draw the bullets */
   draw_bullets(bmp);

   /* draw the score and fps information */
   if (fps)
      sprintf(score_buf, "Lives: %d - Score: %d - (%s, %d fps)",
              ship_count, score, animation_type_str, fps);
   else
      sprintf(score_buf, "Lives: %d - Score: %d", ship_count, score);

   textout_ex(bmp, font, score_buf, 0, 0, 7, 0);
   if (animation_type == DIRTY_RECTANGLE)
      dirty_rectangle(0, 0, text_length(font, score_buf), 8);

   release_bitmap(bmp);
}



static void move_score(void)
{
   score_pos++;
   if (score_pos > SCREEN_H)
      score_pos = -160;
}



static void draw_score(BITMAP *bmp)
{
   static BITMAP *b = NULL, *b2 = NULL;
   if (!b)
      b = create_bitmap(160, 160);
   if (!b2)
      b2 = create_bitmap(160, 160);

   blit(bmp, b2, score_pos, score_pos, 0, 0, 160, 160);

   clear_bitmap(b);
   textout_centre_ex(b, data[END_FONT].dat, "GAME OVER", 80, 50, 2, 0);
   sprintf(score_buf, "Score: %d", score);
   textout_centre_ex(b, data[END_FONT].dat, score_buf, 80, 82, 2, 0);

   rotate_sprite(b2, b, 0, 0, itofix(score_pos) / 4);

   blit(b2, bmp, 0, 0, score_pos, score_pos, 160, 160);
   if (animation_type == DIRTY_RECTANGLE)
      dirty_rectangle(score_pos, score_pos, 160, 160);
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
   score_pos = -160;

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

   draw_intro_item(GO_BMP, 1);

   if (fade_intro_item(13, 16))
      goto skip;

   if (fade_intro_item(14, 16))
      goto skip;

   if (fade_intro_item(15, 16))
      goto skip;

   if (fade_intro_item(16, 16))
      goto skip;

 skip:

   clear_display();

   if ((!keypressed()) && (!joy[0].button[0].b) && (!joy[0].button[1].b)) {
      do {
      } while (midi_pos < 17);
   }

   clear_keybuf();

   set_palette(data[GAME_PAL].dat);
   position_mouse(SCREEN_W / 2, SCREEN_H / 2);

   /* set up the interrupt routines... */
   LOCK_VARIABLE(score);
   LOCK_VARIABLE(frame_count);
   LOCK_VARIABLE(fps);
   LOCK_FUNCTION(fps_proc);
   install_int(fps_proc, 1000);

   LOCK_VARIABLE(game_time);
   LOCK_FUNCTION(game_timer);
   install_int(game_timer, 6400 / SCREEN_W);

   game_time = 0;
   prev_bullet_time = 0;
   prev_update_time = 0;

   /* main game loop */
   while (!esc) {
      int updated = 0;

      poll_keyboard();
      poll_joystick();

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
         BITMAP *bmp = prepare_display();

         draw_screen(bmp);
         if (dead) {
            draw_score(bmp);
         }

         flip_display();

         frame_count++;
      }

      /* rest for a short while if we're not in CPU-hog mode and too fast */
      if (!max_fps && !updated) {
         rest(1);
      }

      if (key[KEY_ESC] || (dead && (
         keypressed() || joy[0].button[0].b || joy[0].button[1].b)))
         esc = TRUE;
   }

   /* cleanup */
   remove_int(fps_proc);

   remove_int(game_timer);

   stop_sample(data[ENGINE_SPL].dat);

   while (bullet_list)
      delete_bullet(bullet_list);

   if (esc) {
      do {
         poll_keyboard();
      } while (key[KEY_ESC]);

      fade_out(5);
      return;
   }

   clear_keybuf();
   fade_out(5);
}
