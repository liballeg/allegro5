/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      A simple game demonstrating the use of the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "demo.h"



/* different ways to update the screen */
#define DOUBLE_BUFFER      1
#define PAGE_FLIP          2
#define RETRACE_FLIP       3
#define TRIPLE_BUFFER      4
#define DIRTY_RECTANGLE    5

int animation_type = 0;


/* command line options */
int cheat = FALSE;
int turbo = FALSE;
int jumpstart = FALSE;


/* our graphics, samples, etc. */
DATAFILE *data;

BITMAP *s;

BITMAP *page1, *page2, *page3;
int current_page = 0;

int use_retrace_proc = FALSE;

PALETTE title_palette;


/* the current score */
volatile int score;
char score_buf[80];


/* info about the state of the game */
int dead;
int pos;
int xspeed, yspeed, ycounter;
int ship_state;
int ship_burn;
int shot;
int skip_speed, skip_count;
volatile int frame_count, fps;
volatile int game_time;

#define MAX_SPEED       32
#define SPEED_SHIFT     3


/* for the starfield */
#define MAX_STARS       128

volatile struct {
   fixed x, y, z;
   int ox, oy;
} star[MAX_STARS];


/* info about the aliens (they used to be aliens at least,
 * even if the current graphics look more like asteroids :-) 
 */
#define MAX_ALIENS      50

volatile struct {
   int x, y;
   int d;
   int state;
   int shot;
} alien[MAX_ALIENS];

volatile int alien_count;
volatile int new_alien_count;


/* explosion graphics */
#define EXPLODE_FLAG    100
#define EXPLODE_FRAMES  64
#define EXPLODE_SIZE    80

RLE_SPRITE *explosion[EXPLODE_FRAMES];


/* position of the bullet */
#define BULLET_SPEED    6

volatile int bullet_flag;
volatile int bullet_x, bullet_y; 


/* dirty rectangle list */
typedef struct DIRTY_RECT {
   int x, y, w, h;
} DIRTY_RECT;

typedef struct DIRTY_LIST {
   int count;
   DIRTY_RECT rect[2*(MAX_ALIENS+MAX_STARS+512)];
} DIRTY_LIST;

DIRTY_LIST dirty, old_dirty;


/* text messages (loaded from readme.txt) */
char *title_text;
int title_size;
int title_alloced;

char *end_text;


/* for doing stereo sound effects */
#define PAN(x)       (((x) * 256) / SCREEN_W)



/* timer callback for controlling the game speed */
void game_timer(void)
{
   game_time++;
}

END_OF_FUNCTION(game_timer);



/* timer callback for measuring the frames per second */
void fps_proc(void)
{
   fps = frame_count;
   frame_count = 0;
   score++;
}

END_OF_FUNCTION(fps_proc);



/* the main game update function */
void move_everyone(void)
{
   int c;

   if (shot) {
      /* player dead */
      if (skip_count <= 0) {
	 if (ship_state >= EXPLODE_FLAG+EXPLODE_FRAMES-1)
	    dead = TRUE;
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
      pos += xspeed;
      if (pos < (32 << SPEED_SHIFT)) {
	 pos = (32 << SPEED_SHIFT);
	 xspeed = 0;
      }
      else if (pos >= ((SCREEN_W-32) << SPEED_SHIFT)) {
	 pos = ((SCREEN_W-32) << SPEED_SHIFT);
	 xspeed = 0;
      }

      if (skip_count <= 0) {
	 if ((key[KEY_UP]) || (joy[0].stick[0].axis[1].d1) || (turbo)) {
	    /* firing thrusters */
	    if (yspeed < MAX_SPEED) {
	       if (yspeed == 0) {
		  play_sample(data[ENGINE_SPL].dat, 0, PAN(pos>>SPEED_SHIFT), 1000, TRUE);
	       }
	       else {
		  /* fade in sample while speeding up */
		  adjust_sample(data[ENGINE_SPL].dat, yspeed*64/MAX_SPEED, PAN(pos>>SPEED_SHIFT), 1000, TRUE);
	       }
	       yspeed++;
	    }
	    else {
	       /* adjust pan while the sample is looping */
	       adjust_sample(data[ENGINE_SPL].dat, 64, PAN(pos>>SPEED_SHIFT), 1000, TRUE);
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
		  adjust_sample(data[ENGINE_SPL].dat, yspeed*64/MAX_SPEED, PAN(pos>>SPEED_SHIFT), 500+yspeed*500/MAX_SPEED, TRUE);
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
	 if (bullet_flag)
	    bullet_y++;

	 for (c=0; c<MAX_STARS; c++) {
	    if (++star[c].oy >= SCREEN_H)
	       star[c].oy = 0;
	 }

	 for (c=0; c<alien_count; c++)
	    alien[c].y++;

	 ycounter -= (1 << SPEED_SHIFT);
      }
   }

   /* move bullet */
   if (bullet_flag) {
      bullet_y -= BULLET_SPEED;

      if (bullet_y < 8) {
	 bullet_flag = FALSE;
      }
      else {
	 /* shot an alien? */
	 for (c=0; c<alien_count; c++) {
	    if ((ABS(bullet_y - alien[c].y) < 20) && (ABS(bullet_x - alien[c].x) < 20) && (!alien[c].shot)) {
	       alien[c].shot = TRUE;
	       alien[c].state = EXPLODE_FLAG;
	       bullet_flag = FALSE;
	       score += 10;
	       play_sample(data[BOOM_SPL].dat, 255, PAN(bullet_x), 1000, FALSE);
	       break; 
	    }
	 }
      }
   }

   /* move stars */
   for (c=0; c<MAX_STARS; c++) {
      if ((star[c].oy += ((int)star[c].z>>1)+1) >= SCREEN_H)
	 star[c].oy = 0;
   }

   /* fire bullet? */
   if (!shot) {
      if ((key[KEY_SPACE]) || (joy[0].button[0].b) || (joy[0].button[1].b)) {
	 if (!bullet_flag) {
	    bullet_x = (pos>>SPEED_SHIFT)-2;
	    bullet_y = SCREEN_H-64;
	    bullet_flag = TRUE;
	    play_sample(data[SHOOT_SPL].dat, 100, PAN(bullet_x), 1000, FALSE);
	 } 
      }
   }

   /* move aliens */
   for (c=0; c<alien_count; c++) {
      if (alien[c].shot) {
	 /* dying alien */
	 if (skip_count <= 0) {
	    if (alien[c].state < EXPLODE_FLAG+EXPLODE_FRAMES-1) {
	       alien[c].state++;
	       if (alien[c].state & 1)
		  alien[c].x += alien[c].d;
	    }
	    else {
	       alien[c].x = 16 + rand() % (SCREEN_W-32);
	       alien[c].y = -60 - (rand() & 0x3f);
	       alien[c].d = (rand()&1) ? 1 : -1;
	       alien[c].shot = FALSE;
	       alien[c].state = -1;
	    }
	 }
      }
      else {
	 /* move alien sideways */
	 alien[c].x += alien[c].d;
	 if (alien[c].x < -60)
	    alien[c].x = SCREEN_W;
	 else if (alien[c].x > SCREEN_W)
	    alien[c].x = -60;
      }

      /* move alien vertically */
      alien[c].y += 1;

      if (alien[c].y > SCREEN_H+30) {
	 if (!alien[c].shot) {
	    alien[c].x = rand() % (SCREEN_W-32);
	    alien[c].y = -32 - (rand() & 0x3f);
	    alien[c].d = (rand()&1) ? 1 : -1;
	 }
      }
      else {
	 /* alien collided with player? */
	 if ((ABS(alien[c].x - (pos>>SPEED_SHIFT)) < 48) && (ABS(alien[c].y - (SCREEN_H-42)) < 32)) {
	    if (!shot) {
	       if (!cheat) {
		  ship_state = EXPLODE_FLAG;
		  shot = TRUE;
		  stop_sample(data[ENGINE_SPL].dat);
	       }
	       if ((!cheat) || (!alien[c].shot))
		  play_sample(data[DEATH_SPL].dat, 255, PAN(pos>>SPEED_SHIFT), 1000, FALSE);
	    }
	    if (!alien[c].shot) {
	       alien[c].shot = TRUE;
	       alien[c].state = EXPLODE_FLAG;
	    }
	 }
      }
   }

   if (skip_count <= 0) {
      skip_count = skip_speed;

      /* make a new alien? */
      new_alien_count++;

      if ((new_alien_count > 600) && (alien_count < MAX_ALIENS)) {
	 alien_count++;
	 new_alien_count = 0;
      }
   }
   else
      skip_count--;

   /* yes, I know that isn't a very pretty piece of code :-) */
}



/* adds a new screen area to the dirty rectangle list */
void add_to_list(DIRTY_LIST *list, int x, int y, int w, int h)
{
   list->rect[list->count].x = x;
   list->rect[list->count].y = y;
   list->rect[list->count].w = w;
   list->rect[list->count].h = h;
   list->count++; 
}



/* qsort() callback for sorting the dirty rectangle list */
int rect_cmp(const void *p1, const void *p2)
{
   DIRTY_RECT *r1 = (DIRTY_RECT *)p1;
   DIRTY_RECT *r2 = (DIRTY_RECT *)p2;

   return (r1->y - r2->y);
}



/* main screen update function */
void draw_screen(void)
{
   int c;
   int i, j;
   int x, y;
   BITMAP *bmp;
   RLE_SPRITE *spr;
   char *animation_type_str;

   if (animation_type == DOUBLE_BUFFER) {
      /* for double buffering, draw onto the memory bitmap. The first step 
       * is to clear it...
       */
      animation_type_str = "double buffered";
      bmp = s;
      clear_bitmap(bmp);
   }
   else if ((animation_type == PAGE_FLIP) || (animation_type == RETRACE_FLIP)) {
      /* for page flipping we draw onto one of the sub-bitmaps which
       * describe different parts of the large virtual screen.
       */ 
      if (animation_type == PAGE_FLIP) 
	 animation_type_str = "page flipping";
      else
	 animation_type_str = "synced flipping";

      if (current_page == 0) {
	 bmp = page2;
	 current_page = 1;
      }
      else {
	 bmp = page1;
	 current_page = 0;
      }
      clear_bitmap(bmp);
   }
   else if (animation_type == TRIPLE_BUFFER) {
      /* triple buffering works kind of like page flipping, but with three
       * pages (obvious, really :-) The benefit of this is that we can be
       * drawing onto page a, while displaying page b and waiting for the
       * retrace that will flip across to page c, hence there is no need
       * to waste time sitting around waiting for retraces.
       */ 
      animation_type_str = "triple buffered";

      if (current_page == 0) {
	 bmp = page2; 
	 current_page = 1;
      }
      else if (current_page == 1) {
	 bmp = page3; 
	 current_page = 2; 
      }
      else {
	 bmp = page1; 
	 current_page = 0;
      }
      clear_bitmap(bmp);
   }
   else {
      /* for dirty rectangle animation we draw onto the memory bitmap, but 
       * we can use information saved during the last draw operation to
       * only clear the areas that have something on them.
       */
      animation_type_str = "dirty rectangles";
      bmp = s;

      for (c=0; c<dirty.count; c++) {
	 if ((dirty.rect[c].w == 1) && (dirty.rect[c].h == 1)) {
	    putpixel(bmp, dirty.rect[c].x, dirty.rect[c].y, 0);
	 }
	 else {
	    rectfill(bmp, dirty.rect[c].x, dirty.rect[c].y, 
			dirty.rect[c].x + dirty.rect[c].w, 
			dirty.rect[c].y+dirty.rect[c].h, 0);
	 }
      }

      old_dirty = dirty;
      dirty.count = 0;
   }

   acquire_bitmap(bmp);

   /* draw the stars */
   for (c=0; c<MAX_STARS; c++) {
      y = star[c].oy;
      x = ((star[c].ox-SCREEN_W/2)*(y/(4-star[c].z/2)+SCREEN_H)/SCREEN_H)+SCREEN_W/2;

      putpixel(bmp, x, y, 15-(int)star[c].z);

      if (animation_type == DIRTY_RECTANGLE)
	 add_to_list(&dirty, x, y, 1, 1);
   }

   /* draw the player */
   x = pos>>SPEED_SHIFT;

   if ((ship_burn) && (ship_state < EXPLODE_FLAG)) {
      spr = (RLE_SPRITE *)data[ENGINE1+(retrace_count/4)%7].dat;
      draw_rle_sprite(bmp, spr, x-spr->w/2, SCREEN_H-24); 

      if (animation_type == DIRTY_RECTANGLE)
	 add_to_list(&dirty, x-spr->w/2, SCREEN_H-24, spr->w, spr->h);
   }

   if (ship_state >= EXPLODE_FLAG)
      spr = explosion[ship_state-EXPLODE_FLAG];
   else
      spr = (RLE_SPRITE *)data[ship_state].dat;

   draw_rle_sprite(bmp, spr, x-spr->w/2, SCREEN_H-42-spr->h/2); 

   if (animation_type == DIRTY_RECTANGLE)
      add_to_list(&dirty, x-spr->w/2, SCREEN_H-42-spr->h/2, spr->w, spr->h);

   /* draw the aliens */
   for (c=0; c<alien_count; c++) {
      x = alien[c].x;
      y = alien[c].y;

      if (alien[c].state >= EXPLODE_FLAG) {
	 spr = explosion[alien[c].state-EXPLODE_FLAG];
      }
      else {
	 switch (c%3) {
	    case 0: i = ASTA01; break;
	    case 1: i = ASTB01; break;
	    case 2: i = ASTC01; break;
	    default: i = 0; break;
	 }
	 j = (retrace_count/(6-(c&3))+c)%15;
	 if (c&1)
	    spr = (RLE_SPRITE *)data[i+14-j].dat;
	 else
	    spr = (RLE_SPRITE *)data[i+j].dat;
      }

      draw_rle_sprite(bmp, spr, x-spr->w/2, y-spr->h/2);

      if (animation_type == DIRTY_RECTANGLE)
	 add_to_list(&dirty, x-spr->h/2, y-spr->h/2, spr->w, spr->h);
   }

   /* draw the bullet */
   if (bullet_flag) {
      x = bullet_x;
      y = bullet_y;

      spr = (RLE_SPRITE *)data[ROCKET].dat;
      draw_rle_sprite(bmp, spr, x-spr->w/2, y-spr->h/2);

      if (animation_type == DIRTY_RECTANGLE)
	 add_to_list(&dirty, x-spr->w/2, y-spr->h/2, spr->w, spr->h);
   }

   /* draw the score and fps information */
   if (fps)
      sprintf(score_buf, "Score: %ld - (%s, %ld fps)", (long)score, animation_type_str, (long)fps);
   else
      sprintf(score_buf, "Score: %ld", (long)score);

   textout(bmp, font, score_buf, 0, 0, 7);

   if (animation_type == DIRTY_RECTANGLE)
      add_to_list(&dirty, 0, 0, text_length(font, score_buf), 8);

   release_bitmap(bmp);

   if (animation_type == DOUBLE_BUFFER) {
      /* when double buffering, just copy the memory bitmap to the screen */
      blit(s, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }
   else if ((animation_type == PAGE_FLIP) || (animation_type == RETRACE_FLIP)) {
      /* for page flipping we scroll to display the image */ 
      show_video_bitmap(bmp);
   }
   else if (animation_type == TRIPLE_BUFFER) {
      /* make sure the last flip request has actually happened */
      do {
      } while (poll_scroll());

      /* post a request to display the page we just drew */
      request_video_bitmap(bmp);
   }
   else {
      /* for dirty rectangle animation, only copy the areas that changed */
      for (c=0; c<dirty.count; c++)
	 add_to_list(&old_dirty, dirty.rect[c].x, dirty.rect[c].y, dirty.rect[c].w, dirty.rect[c].h);

      /* sorting the objects really cuts down on bank switching */
      if (!gfx_driver->linear)
	 qsort(old_dirty.rect, old_dirty.count, sizeof(DIRTY_RECT), rect_cmp);

      acquire_screen();

      for (c=0; c<old_dirty.count; c++) {
	 if ((old_dirty.rect[c].w == 1) && (old_dirty.rect[c].h == 1)) {
	    putpixel(screen, old_dirty.rect[c].x, old_dirty.rect[c].y, 
		     getpixel(bmp, old_dirty.rect[c].x, old_dirty.rect[c].y));
	 }
	 else {
	    blit(bmp, screen, old_dirty.rect[c].x, old_dirty.rect[c].y, 
			    old_dirty.rect[c].x, old_dirty.rect[c].y, 
			    old_dirty.rect[c].w, old_dirty.rect[c].h);
	 }
      }

      release_screen();
   }
}



/* timer callback for controlling the speed of the scrolling text */
volatile int scroll_count;

void scroll_counter(void)
{
   scroll_count++;
}

END_OF_FUNCTION(scroll_counter);



/* draws one of the ready, steady, go messages */
void draw_intro_item(int item, int size)
{
   BITMAP *b = (BITMAP *)data[item].dat;
   int w = MIN(SCREEN_W, (SCREEN_W * 2 / size));
   int h = SCREEN_H / size;

   clear_bitmap(screen);
   stretch_blit(b, screen, 0, 0, b->w, b->h, (SCREEN_W-w)/2, (SCREEN_H-h)/2, w, h);
}



/* handles fade effects and timing for the ready, steady, go messages */
int fade_intro_item(int music_pos, int fade_speed)
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



/* sets up and plays the game */
void play_game(void)
{
   int c;
   BITMAP *b, *b2;
   int esc = FALSE;

   stop_midi();

   /* set up a load of globals */
   dead = shot = FALSE;
   pos = (SCREEN_W/2)<<SPEED_SHIFT;
   xspeed = yspeed = ycounter = 0;
   ship_state = SHIP3;
   ship_burn = FALSE;
   frame_count = fps = 0;
   bullet_flag = FALSE;
   score = 0;
   old_dirty.count = dirty.count = 0;

   skip_count = 0;
   if (SCREEN_W < 400)
      skip_speed = 0;
   else if (SCREEN_W < 700)
      skip_speed = 1;
   else if (SCREEN_W < 1000)
      skip_speed = 2;
   else
      skip_speed = 3;

   for (c=0; c<MAX_STARS; c++) {
      star[c].ox = rand() % SCREEN_W;
      star[c].oy = rand() % SCREEN_H;
      star[c].z = rand() & 7;
   }

   for (c=0; c<MAX_ALIENS; c++) {
      alien[c].x = 16 + rand() % (SCREEN_W-32);
      alien[c].y = -60 - (rand() & 0x3f);
      alien[c].d = (rand()&1) ? 1 : -1;
      alien[c].state = -1;
      alien[c].shot = FALSE;
   }
   alien_count = 2;
   new_alien_count = 0;

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

   text_mode(0);
   clear_bitmap(screen);
   clear_bitmap(s);
   draw_screen();

   if ((!keypressed()) && (!joy[0].button[0].b) && (!joy[0].button[1].b)) {
      do {
      } while (midi_pos < 17);
   }

   clear_keybuf();

   set_palette(data[GAME_PAL].dat);
   position_mouse(SCREEN_W/2, SCREEN_H/2);

   /* set up the interrupt routines... */
   install_int(fps_proc, 1000);

   if (use_retrace_proc)
      retrace_proc = game_timer;
   else
      install_int(game_timer, 6400/SCREEN_W);

   game_time = 0;

   /* main game loop */
   while (!dead) {
      poll_keyboard();
      poll_joystick();

      while (game_time > 0) {
	 move_everyone();
	 game_time--;
      }

      draw_screen();
      frame_count++;

      if (key[KEY_ESC])
	 esc = dead = TRUE;
   }

   /* cleanup */
   remove_int(fps_proc);

   if (use_retrace_proc)
      retrace_proc = NULL;
   else
      remove_int(game_timer);

   stop_sample(data[ENGINE_SPL].dat);

   if (esc) {
      while (current_page != 0)
	 draw_screen();

      show_video_bitmap(screen);

      do {
	 poll_keyboard();
      } while (key[KEY_ESC]);

      fade_out(5);
      return;
   }

   fps = 0;
   draw_screen();

   if ((animation_type == PAGE_FLIP) || (animation_type == RETRACE_FLIP) || (animation_type == TRIPLE_BUFFER)) {
      while (current_page != 0)
	 draw_screen();

      show_video_bitmap(screen);

      blit(screen, s, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }

   /* display the score */
   b = create_bitmap(160, 160);
   b2 = create_bitmap(160, 160);
   clear_bitmap(b);
   textout_centre(b, data[END_FONT].dat, "GAME OVER", 80, 50, 2);
   textout_centre(b, data[END_FONT].dat, score_buf, 80, 82, 2);
   clear_keybuf();
   scroll_count = -160;
   c = 0;
   install_int(scroll_counter, 6000/SCREEN_W);

   while ((!keypressed()) && (!joy[0].button[0].b) && (!joy[0].button[1].b)) {
      if (scroll_count > c+8)
	 c = scroll_count = c+8;
      else
	 c = scroll_count;

      blit(s, b2, c, c, 0, 0, 160, 160);
      rotate_sprite(b2, b, 0, 0, itofix(c)/4);
      blit(b2, screen, 0, 0, c, c, 160, 160);

      if ((c > SCREEN_W) || (c > SCREEN_H))
	 scroll_count = -160;

      poll_joystick();
   }

   remove_int(scroll_counter);
   destroy_bitmap(b);
   destroy_bitmap(b2);
   clear_keybuf();
   fade_out(5);
}



/* the explosion graphics are pregenerated, using a simple particle system */
void generate_explosions(void)
{
   BITMAP *bmp = create_bitmap(EXPLODE_SIZE, EXPLODE_SIZE);
   unsigned char *p;
   int c, c2;
   int x, y;
   int xx, yy;
   int color;

   #define HOTSPOTS  64

   struct HOTSPOT
   {
      int x, y;
      int xc, yc;
   } hot[HOTSPOTS];

   for (c=0; c<HOTSPOTS; c++) {
      hot[c].x = hot[c].y = (EXPLODE_SIZE/2)<<16;
      hot[c].xc = (rand()&0xFFFF)-0x7FFF;
      hot[c].yc = (rand()&0xFFFF)-0x7FFF;
   }

   for (c=0; c<EXPLODE_FRAMES; c++) {
      clear_bitmap(bmp);

      color = ((c<16) ? c*4 : (80-c)) >> 2;

      for (c2=0; c2<HOTSPOTS; c2++) {
	 for (x=-6; x<=6; x++) {
	    for (y=-6; y<=6; y++) {
	       xx = (hot[c2].x>>16) + x;
	       yy = (hot[c2].y>>16) + y;
	       if ((xx>0) && (yy>0) && (xx<EXPLODE_SIZE) && (yy<EXPLODE_SIZE)) {
		  p = bmp->line[yy] + xx;
		  *p += (color >> ((ABS(x)+ABS(y))/3));
		  if (*p > 63)
		     *p = 63;
	       }
	    }
	 }
	 hot[c2].x += hot[c2].xc;
	 hot[c2].y += hot[c2].yc;
      }

      for (x=0; x<EXPLODE_SIZE; x++) {
	 for (y=0; y<EXPLODE_SIZE; y++) {
	    c2 = bmp->line[y][x];
	    if (c2 < 8)
	       bmp->line[y][x] = 0;
	    else
	       bmp->line[y][x] = 16+c2/4;
	 }
      }

      explosion[c] = get_rle_sprite(bmp);
   }

   destroy_bitmap(bmp);
}



/* modifies the palette to give us nice colors for the GUI dialogs */
void set_gui_colors(void)
{
   static RGB black = { 0,  0,  0,  0 };
   static RGB grey  = { 48, 48, 48, 0 };
   static RGB white = { 63, 63, 63, 0 };

   set_color(0, &black);
   set_color(16, &black);
   set_color(1, &grey); 
   set_color(255, &white); 

   gui_fg_color = palette_color[0];
   gui_bg_color = palette_color[1];
}



/* d_list_proc() callback for the animation mode dialog */
char *anim_list_getter(int index, int *list_size)
{
   static char *s[] =
   {
      "Double buffered",
      "Page flipping",
      "Synced flips",
      "Triple buffered",
      "Dirty rectangles"
   };

   if (index < 0) {
      *list_size = 5;
      return NULL;
   }

   return s[index];
}



/* dialog procedure for the animation type dialog */
extern DIALOG anim_type_dlg[];


int anim_list_proc(int msg, DIALOG *d, int c)
{
   int sel, ret;

   sel = d->d1;

   ret = d_list_proc(msg, d, c);

   if (sel != d->d1)
      ret |= D_REDRAW;

   return ret;
}



/* dialog procedure for the animation type dialog */
int anim_desc_proc(int msg, DIALOG *d, int c)
{
   static char *double_buffer_desc[] = 
   {
      "Draws onto a memory bitmap,",
      "and then uses a brute-force",
      "blit to copy the entire",
      "image across to the screen.",
      NULL
   };

   static char *page_flip_desc[] = 
   {
      "Uses two pages of video",
      "memory, and flips back and",
      "forth between them. It will",
      "only work if there is enough",
      "video memory to set up dual",
      "pages.",
      NULL
   };

   static char *retrace_flip_desc[] = 
   {
      "This is basically the same",
      "as page flipping, but it uses",
      "the vertical retrace interrupt",
      "simulator instead of retrace",
      "polling. Only works in mode-X,",
      "and not under win95.",
      NULL
   };

   static char *triple_buffer_desc[] = 
   {
      "Uses three pages of video",
      "memory, to avoid wasting time",
      "waiting for retraces. Only",
      "some drivers and hardware",
      "support this, eg. VBE 3.0 or",
      "mode-X with retrace sync.",
      NULL
   };

   static char *dirty_rectangle_desc[] = 
   {
      "This is similar to double",
      "buffering, but stores a list",
      "of which parts of the screen",
      "have changed, to minimise the",
      "amount of drawing that needs",
      "to be done.",
      NULL
   };

   static char **descs[] =
   {
      double_buffer_desc,
      page_flip_desc,
      retrace_flip_desc,
      triple_buffer_desc,
      dirty_rectangle_desc
   };

   char **p;
   int y;

   if (msg == MSG_DRAW) {
      rectfill(screen, d->x, d->y, d->x+d->w, d->y+d->h, d->bg);
      text_mode(d->bg);

      p = descs[anim_type_dlg[2].d1];
      y = d->y;

      while (*p) {
	 textout(screen, font, *p, d->x, y, d->fg);
	 y += 8;
	 p++;
      } 
   }

   return D_O_K;
}



DIALOG anim_type_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)     (d1)  (d2)  (dp)                 (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,    281,  151,  0,    1,    0,    0,          0,    0,    NULL,                NULL, NULL  },
   { d_ctext_proc,      140,  8,    1,    1,    0,    1,    0,    0,          0,    0,    "Animation Method",  NULL, NULL  },
   { anim_list_proc,    16,   28,   153,  44,   0,    1,    0,    D_EXIT,     4,    0,    anim_list_getter,    NULL, NULL  },
   { anim_desc_proc,    16,   90,   248,  48,   0,    1,    0,    0,          0,    0,    0,                   NULL, NULL  },
   { d_button_proc,     184,  28,   80,   16,   0,    1,    13,   D_EXIT,     0,    0,    "OK",                NULL, NULL  },
   { d_button_proc,     184,  50,   80,   16,   0,    1,    27,   D_EXIT,     0,    0,    "Cancel",            NULL, NULL  },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,    0,          0,    0,    NULL,                NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,          0,    0,    NULL,                NULL, NULL  }
};



/* allows the user to choose an animation type */
int pick_animation_type(int *type)
{
   int ret;

   centre_dialog(anim_type_dlg);

   clear_bitmap(screen);

   /* we set up colors to match screen color depth */
   for (ret = 0; anim_type_dlg[ret].proc; ret++) {
      anim_type_dlg[ret].fg = palette_color[0];
      anim_type_dlg[ret].bg = palette_color[1];
   }

   ret = do_dialog(anim_type_dlg, 2);

   *type = anim_type_dlg[2].d1 + 1;

   return (ret == 5) ? -1 : ret;
}



/* for parsing readme.txt */
typedef struct TEXT_LIST
{
   char *text;
   struct TEXT_LIST *next;
} TEXT_LIST;


typedef struct README_SECTION
{
   TEXT_LIST *head;
   TEXT_LIST *tail;
   char *flat;
   char *desc;
} README_SECTION;



/* formats a list of TEXT_LIST structure into a single string */
char *format_text(TEXT_LIST *head, char *eol, char *gap)
{
   TEXT_LIST *l;
   int size = 0;
   char *s;

   l = head;
   while (l) {
      if (l->text[0])
	 size += strlen(l->text) + strlen(eol);
      else
	 size += strlen(gap) + strlen(eol);
      l = l->next;
   }

   s = malloc(size+1);
   s[0] = 0;

   l = head;
   while (l) {
      if (l->text[0])
	 strcat(s, l->text);
      else
	 strcat(s, gap);
      strcat(s, eol);
      l = l->next;
   }

   return s;
}



/* loads the scroller message from readme.txt */
void load_text(void)
{
   README_SECTION sect[] =
   {
      { NULL, NULL, NULL, "Introduction" },
      { NULL, NULL, NULL, "Features"     },
      { NULL, NULL, NULL, "Copyright"    },
      { NULL, NULL, NULL, "Contact info" }
   };

   #define SPLITTER  "                                "

   static char intro_msg[] =
      "Welcome to the Allegro demonstration game, by Shawn Hargreaves."
      SPLITTER
      "Your mission: to go where no man has gone before, to seek out strange new life, and to boldly blast it to smithereens."
      SPLITTER
      "Your controls: the arrow keys to move left and right, the up arrow to accelerate (the faster you go, the more score you get), and the space bar to fire."
      SPLITTER
      "What complexity!"
      SPLITTER
      "What subtlety."
      SPLITTER
      "What originality."
      SPLITTER
      "But enough of that. On to the serious stuff..."
      SPLITTER;

   static char splitter[] = SPLITTER;
   static char marker[] = "--------";
   char buf[256], buf2[256];
   README_SECTION *sec = NULL;
   TEXT_LIST *l, *p;
   PACKFILE *f;
   int inblank = TRUE;
   char *s;
   int i;

   get_executable_name(buf, sizeof(buf));

   replace_filename(buf2, buf, "readme.txt", sizeof(buf2));
   f = pack_fopen(buf2, F_READ);

   if (!f) {
      replace_filename(buf2, buf, "../readme.txt", sizeof(buf2));
      f = pack_fopen(buf2, F_READ);

      if (!f) {
	 title_text = "Can't find readme.txt, so this scroller is empty.                ";
	 title_size = strlen(title_text);
	 title_alloced = FALSE;
	 end_text = NULL;
	 return;
      }
   }

   while (pack_fgets(buf, sizeof(buf)-1, f) != 0) {
      if (buf[0] == '=') {
	 s = strchr(buf, ' ');
	 if (s) {
	    for (i=strlen(s)-1; (uisspace(s[i])) || (s[i] == '='); i--)
	       s[i] = 0;

	    s++;

	    sec = NULL;
	    inblank = TRUE;

	    for (i=0; i<(int)(sizeof(sect)/sizeof(sect[0])); i++) {
	       if (stricmp(s, sect[i].desc) == 0) {
		  sec = &sect[i];
		  break;
	       }
	    }
	 }
      }
      else if (sec) {
	 s = buf;

	 while ((*s) && (uisspace(*s)))
	    s++;

	 for (i=strlen(s)-1; (i >= 0) && (uisspace(s[i])); i--)
	    s[i] = 0;

	 if ((s[0]) || (!inblank)) {
	    l = malloc(sizeof(TEXT_LIST));
	    l->next = NULL;
	    l->text = malloc(strlen(s)+1);
	    strcpy(l->text, s);

	    if (sec->tail)
	       sec->tail->next = l;
	    else
	       sec->head = l;

	    sec->tail = l;
	 }

	 inblank = (s[0] == 0);
      }
   }

   pack_fclose(f);

   if (sect[2].head)
      end_text = format_text(sect[2].head, "\n", "");
   else
      end_text = NULL;

   title_size = strlen(intro_msg);

   for (i=0; i<(int)(sizeof(sect)/sizeof(sect[0])); i++) {
      if (sect[i].head) {
	 sect[i].flat = format_text(sect[i].head, " ", splitter);
	 title_size += strlen(sect[i].flat) + strlen(sect[i].desc) + 
		       strlen(splitter) + strlen(marker)*2 + 2;
      }
   }

   title_text = malloc(title_size+1);
   title_alloced = TRUE;

   strcpy(title_text, intro_msg);

   for (i=0; i<(int)(sizeof(sect)/sizeof(sect[0])); i++) {
      if (sect[i].flat) {
	 strcat(title_text, marker);
	 strcat(title_text, " ");
	 strcat(title_text, sect[i].desc);
	 strcat(title_text, " ");
	 strcat(title_text, marker);
	 strcat(title_text, splitter);
	 strcat(title_text, sect[i].flat);
      }
   }

   for (i=0; i<(int)(sizeof(sect)/sizeof(sect[0])); i++) {
      l = sect[i].head;
      while (l) {
	 free(l->text);
	 p = l;
	 l = l->next;
	 free(p);
      }
      if (sect[i].flat)
	 free(sect[i].flat);
   }
}



/* for parsing thanks._tx and the various source files */
typedef struct CREDIT_NAME
{
   char *name;
   char *text;
   TEXT_LIST *files;
   struct CREDIT_NAME *next;
} CREDIT_NAME;



CREDIT_NAME *credits = NULL;



/* reads credit info from a source file */
void parse_source(AL_CONST char *filename, int attrib, int param)
{
   char buf[256];
   PACKFILE *f;
   CREDIT_NAME *c;
   TEXT_LIST *d;
   char *p;

   if (attrib & FA_DIREC) {
      p = get_filename(filename);

      if ((stricmp(p, ".") != 0) && (stricmp(p, "..") != 0) &&
	  (stricmp(p, "lib") != 0) && (stricmp(p, "obj") != 0)) {

	 /* recurse inside a directory */
	 strcpy(buf, filename);
	 put_backslash(buf);
	 strcat(buf, "*.*");

	 for_each_file(buf, FA_ARCH | FA_RDONLY | FA_DIREC, parse_source, param);
      }
   }
   else {
      p = get_extension(filename);

      if ((stricmp(p, "c") == 0) || (stricmp(p, "cpp") == 0) ||
	  (stricmp(p, "h") == 0) || (stricmp(p, "inc") == 0) ||
	  (stricmp(p, "s") == 0) || (stricmp(p, "asm") == 0)) {

	 /* parse a source file */
	 f = pack_fopen(filename, F_READ);
	 if (!f)
	    return;

	 textprintf_centre(screen, font, SCREEN_W/2, SCREEN_H/2+8, makecol(160, 160, 160), "                %s                ", filename+param);

	 while (pack_fgets(buf, sizeof(buf)-1, f) != 0) {
	    if (strstr(buf, "*/"))
	       break;

	    c = credits;

	    while (c) {
	       if (strstr(buf, c->name)) {
		  for (d=c->files; d; d=d->next) {
		     if (strcmp(d->text, filename+param) == 0)
			break;
		  }

		  if (!d) {
		     d = malloc(sizeof(TEXT_LIST));
		     d->text = malloc(strlen(filename+param)+1);
		     strcpy(d->text, filename+param);
		     d->next = c->files;
		     c->files = d;
		  }
	       }

	       c = c->next;
	    }
	 }

	 pack_fclose(f);
      }
   }
}



/* sorts a list of text strings */
void sort_text_list(TEXT_LIST **head)
{
   TEXT_LIST **prev, *p;
   int done;

   do {
      done = TRUE;

      prev = head;
      p = *head;

      while ((p) && (p->next)) {
	 if (stricmp(p->text, p->next->text) > 0) {
	    *prev = p->next;
	    p->next = p->next->next;
	    (*prev)->next = p;
	    p = *prev;

	    done = FALSE;
	 }

	 prev = &p->next;
	 p = p->next;
      }

   } while (!done);
}



/* sorts a list of credit strings */
void sort_credit_list(void)
{
   CREDIT_NAME **prev, *p;
   TEXT_LIST *t;
   int n, done;

   do {
      done = TRUE;

      prev = &credits;
      p = credits;

      while ((p) && (p->next)) {
	 n = 0;

	 for (t=p->files; t; t=t->next)
	    n--;

	 for (t=p->next->files; t; t=t->next)
	    n++;

	 if (n == 0)
	    n = stricmp(p->name, p->next->name);

	 if (n > 0) {
	    *prev = p->next;
	    p->next = p->next->next;
	    (*prev)->next = p;
	    p = *prev;

	    done = FALSE;
	 }

	 prev = &p->next;
	 p = p->next;
      }

   } while (!done);
}



/* reads credit info from various places */
void load_credits(void)
{
   char buf[256], buf2[256], *p, *p2;
   CREDIT_NAME *c = NULL;
   PACKFILE *f;

   /* parse thanks._tx */
   get_executable_name(buf, sizeof(buf));
   replace_filename(buf2, buf, "../docs/src/thanks._tx", sizeof(buf2));

   f = pack_fopen(buf2, F_READ);
   if (!f)
      return;

   while (pack_fgets(buf, sizeof(buf)-1, f) != 0) {
      if (stricmp(buf, "Thanks!") == 0)
	 break;

      while ((p = strstr(buf, "&lt")) != NULL) {
	 *p = '<';
	 memmove(p+1, p+3, strlen(p+2));
      }

      while ((p = strstr(buf, "&gt")) != NULL) {
	 *p = '>';
	 memmove(p+1, p+3, strlen(p+2));
      }

      p = buf;

      while ((*p) && (uisspace(*p)))
	 p++;

      p2 = p;

      while ((*p2) && ((!uisspace(*p2)) || (*(p2+1) != '(')))
	 p2++;

      if (strncmp(p2, " (<email>", 9) == 0) {
	 *p2 = 0;

	 c = malloc(sizeof(CREDIT_NAME));

	 c->name = malloc(strlen(p)+1);
	 strcpy(c->name, p);

	 c->text = NULL;
	 c->files = NULL;

	 c->next = credits;
	 credits = c;
      }
      else if (*p) {
	 if (c) {
	    p2 = p + strlen(p) - 1;
	    while ((p2 > p) && (uisspace(*p2)))
	       *(p2--) = 0;

	    if (c->text) {
	       c->text = realloc(c->text, strlen(c->text)+strlen(p)+2);
	       strcat(c->text, " ");
	       strcat(c->text, p);
	    }
	    else {
	       c->text = malloc(strlen(p)+1);
	       strcpy(c->text, p);
	    }
	 }
      }
      else
	 c = NULL;
   }

   pack_fclose(f);

   /* parse source files */
   get_executable_name(buf, sizeof(buf));
   replace_filename(buf2, buf, "../*.*", sizeof(buf2));

   for_each_file(buf2, FA_ARCH | FA_RDONLY | FA_DIREC, parse_source, strlen(buf2)-3);

   /* sort the lists */
   sort_credit_list();

   for (c=credits; c; c=c->next) {
      sort_text_list(&c->files);
   }
}



/* displays the title screen */
int title_screen(void)
{
   static int color = 0;
   int c, c2, c3, n, n2;
   BITMAP *buffer;
   RGB rgb;

   /* for the starfield */
   fixed x, y, z;
   int ix, iy;
   int star_count = 0;
   int star_count_count = 0;

   /* for the text scroller */
   char buf[2] = " ";
   int text_char = 0xFFFF;
   int text_width = 0;
   int text_pix = 0;
   int text_scroll = 0;
   BITMAP *text_bmp;

   /* for the credits display */
   char cbuf[2] = " ";
   CREDIT_NAME *cred = NULL;
   TEXT_LIST *tl;
   int credit_width = 0;
   int credit_scroll = 0;
   int credit_offset = 0;
   int credit_age = 0;
   int credit_speed = 32;
   int credit_skip = 1;
   char *p;

   play_midi(data[TITLE_MUSIC].dat, TRUE);
   play_sample(data[WELCOME_SPL].dat, 255, 127, 1000, FALSE);

   for (c=0; c<MAX_STARS; c++) {
      star[c].z = 0;
      star[c].ox = star[c].oy = -1;
   }

   for (c=0; c<8; c++)
      title_palette[c] = ((RGB *)data[TITLE_PAL].dat)[c];

   /* set up the colors differently each time we display the title screen */
   for (c=8; c<PAL_SIZE/2; c++) {
      rgb = ((RGB *)data[TITLE_PAL].dat)[c];
      switch (color) {
	 case 0:
	    rgb.b = rgb.r;
	    rgb.r = 0;
	    break;
	 case 1:
	    rgb.g = rgb.r;
	    rgb.r = 0;
	    break;
	 case 3:
	    rgb.g = rgb.r;
	    break;
      }
      title_palette[c] = rgb;
   }

   for (c=PAL_SIZE/2; c<PAL_SIZE; c++)
      title_palette[c] = ((RGB *)data[TITLE_PAL].dat)[c];

   color++;
   if (color > 3)
      color = 0;

   clear_bitmap(screen);
   set_palette(title_palette);

   scroll_count = 1;
   install_int(scroll_counter, 5);

   while ((c=scroll_count) < 160)
      stretch_blit(data[TITLE_BMP].dat, screen, 0, 0, 320, 128, SCREEN_W/2-c, SCREEN_H/2-c*64/160-32, c*2, c*128/160);

   remove_int(scroll_counter);

   blit(data[TITLE_BMP].dat, screen, 0, 0, SCREEN_W/2-160, SCREEN_H/2-96, 320, 128);

   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   clear_bitmap(buffer);

   text_bmp = create_bitmap(SCREEN_W, 24);
   clear_bitmap(text_bmp);

   old_dirty.count = dirty.count = 0;

   clear_keybuf();

   scroll_count = 0;

   if (use_retrace_proc)
      retrace_proc = scroll_counter;
   else
      install_int(scroll_counter, 6);

   do {
      do {
      } while (scroll_count < 0);

      while (scroll_count > 0) {
	 /* animate the starfield */
	 for (c=0; c<star_count; c++) {
	    if (star[c].z <= itofix(1)) {
	       x = itofix(rand()&0xff);
	       y = itofix(((rand()&3)+1)*SCREEN_W);

	       star[c].x = fixmul(fixcos(x), y);
	       star[c].y = fixmul(fixsin(x), y);
	       star[c].z = itofix((rand() & 0x1f) + 0x20);
	    }

	    x = fixdiv(star[c].x, star[c].z);
	    y = fixdiv(star[c].y, star[c].z);

	    ix = (int)(x>>16) + SCREEN_W/2;
	    iy = (int)(y>>16) + SCREEN_H/2;

	    if ((ix >= 0) && (ix < SCREEN_W) && (iy >= 0) && (iy <= SCREEN_H)) {
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

	 /* wake up new stars */
	 if (star_count < MAX_STARS) {
	    if (star_count_count++ >= 32) {
	       star_count_count = 0;
	       star_count++;
	    }
	 }

	 /* move the scroller */
	 text_scroll += (use_retrace_proc ? 2 : 1);

	 /* update the credits position */
	 if (credit_scroll <= 0) {
	    if (cred)
	       cred = cred->next;

	    if (!cred)
	       cred = credits;

	    if (cred) {
	       credit_width = text_length(data[TITLE_FONT].dat, cred->name) + 24;

	       if (cred->text)
		  credit_scroll = strlen(cred->text)*8 + SCREEN_W - credit_width + 64;
	       else
		  credit_scroll = 256;

	       tl = cred->files;
	       n = 0;

	       while (tl) {
		  n++;
		  tl = tl->next;
	       }

	       credit_offset = 0;

	       if (n) {
		  credit_skip = 1 + n/50;
		  credit_speed = 8 + fixtoi(fixdiv(itofix(512), itofix(n)));
		  if (credit_speed > 200)
		     credit_speed = 200;
		  c = 1024 + (n-1)*credit_speed;
		  if (credit_scroll < c) {
		     credit_offset = credit_scroll - c;
		     credit_scroll = c;
		  }
	       }

	       credit_age = 0;
	    }
	 }
	 else {
	    credit_scroll--;
	    credit_age++;
	 }

	 scroll_count--;
      }

      /* clear dirty rectangle list */
      for (c=0; c<dirty.count; c++) {
	 if ((dirty.rect[c].w == 1) && (dirty.rect[c].h == 1)) {
	    putpixel(buffer, dirty.rect[c].x, dirty.rect[c].y, 0);
	 }
	 else {
	    rectfill(buffer, dirty.rect[c].x, dirty.rect[c].y, 
			     dirty.rect[c].x + dirty.rect[c].w, 
			     dirty.rect[c].y+dirty.rect[c].h, 0);
	 }
      }

      old_dirty = dirty;
      dirty.count = 0;

      /* draw the text scroller */
      blit(text_bmp, text_bmp, text_scroll, 0, 0, 0, SCREEN_W, 24);
      rectfill(text_bmp, SCREEN_W-text_scroll, 0, SCREEN_W, 24, 0);

      while (text_scroll > 0) {
	 text_pix += text_scroll;
	 textout(text_bmp, data[TITLE_FONT].dat, buf, SCREEN_W-text_pix, 0, -1);
	 if (text_pix >= text_width) {
	    text_scroll = text_pix - text_width;
	    text_char++;
	    if (text_char >= title_size)
	       text_char = 0;
	    buf[0] = title_text[text_char];
	    text_pix = 0;
	    text_width = text_length(data[TITLE_FONT].dat, buf);
	 }
	 else
	    text_scroll = 0;
      }

      blit(text_bmp, buffer, 0, 0, 0, SCREEN_H-24, SCREEN_W, 24);
      add_to_list(&dirty, 0, SCREEN_H-24, SCREEN_W, 24);

      /* draw author file credits */
      if (cred) {
	 tl = cred->files;
	 n = credit_width;
	 n2 = 0;

	 while (tl) {
	    c = 1024 + n2*credit_speed - credit_age;

	    if ((c > 0) && (c < 1024) && ((n2 % credit_skip) == 0)) {
	       x = itofix(SCREEN_W/2);
	       y = itofix(SCREEN_H/2-32);

	       c2 = c * ((n/13)%17 + 1) / 32;
	       if (n&1)
		  c2 = -c2;

	       c2 -= 96;

	       c3 = (32 + fixtoi(ABS(fixsin(itofix(c/(15+n%42)+n)))*128)) * SCREEN_W / 640;

	       x += fixsin(itofix(c2)) * c3;
	       y += fixcos(itofix(c2)) * c3;

	       if (c < 512) {
		  z = fixsqrt(itofix(c)/512);

		  x = fixmul(itofix(32), itofix(1)-z) + fixmul(x, z);
		  y = fixmul(itofix(16), itofix(1)-z) + fixmul(y, z);
	       }
	       else if (c > 768) {
		  z = fixsqrt(itofix(1024-c)/256);

		  if (n&2)
		     x = fixmul(itofix(128), itofix(1)-z) + fixmul(x, z);
		  else
		     x = fixmul(itofix(SCREEN_W-128), itofix(1)-z) + fixmul(x, z);

		  y = fixmul(itofix(SCREEN_H-128), itofix(1)-z) + fixmul(y, z);
	       }

	       c = 128 + (512 - ABS(512-c)) / 24;

	       ix = fixtoi(x);
	       iy = fixtoi(y);

	       c2 = strlen(tl->text);
	       ix -= c2*4;

	       textout(buffer, font, tl->text, ix, iy, c);
	       add_to_list(&dirty, ix, iy, c2*8, 8);
	    }

	    tl = tl->next;
	    n += 1234567;
	    n2++;
	 }
      }

      /* draw the starfield */
      for (c=0; c<star_count; c++) {
	 c2 = 7-(int)(star[c].z>>18);
	 putpixel(buffer, star[c].ox, star[c].oy, MID(0, c2, 7));
	 add_to_list(&dirty, star[c].ox, star[c].oy, 1, 1);
      }

      /* draw author name/desc credits */
      if (cred) {
	 if (cred->text) {
	    c = credit_scroll + credit_offset;
	    p = cred->text;
	    c2 = strlen(p);

	    if (c > 0) {
	       if (c2 > c/8) {
		  p += c2 - c/8;
		  c &= 7;
	       }
	       else {
		  c -= c2*8;
	       }

	       c += credit_width;

	       while ((*p) && (c < SCREEN_W-32)) {
		  if (c < credit_width+96)
		     c2 = 128 + (c-credit_width-32)*127/64;
		  else if (c > SCREEN_W-96)
		     c2 = 128 + (SCREEN_W-32-c)*127/64;
		  else
		     c2 = 255;

		  if ((c2 > 128) && (c2 <= 255)) {
		     cbuf[0] = *p;
		     textout(buffer, font, cbuf, c, 16, c2);
		  }

		  p++;
		  c += 8;
	       }
	    }
	 }

	 c = 4;

	 if (credit_age < 100)
	    c -= (100-credit_age) * (100-credit_age) * credit_width / 10000;

	 if (credit_scroll < 150)
	    c += (150-credit_scroll) * (150-credit_scroll) * SCREEN_W / 22500;

	 textprintf(buffer, data[TITLE_FONT].dat, c, 4, -1, "%s:", cred->name);
	 add_to_list(&dirty, 0, 4, SCREEN_W, text_height(data[TITLE_FONT].dat));
      }

      /* draw the Allegro logo over the top */
      draw_sprite(buffer, data[TITLE_BMP].dat, SCREEN_W/2-160, SCREEN_H/2-96);

      /* dirty rectangle screen update */
      for (c=0; c<dirty.count; c++)
	 add_to_list(&old_dirty, dirty.rect[c].x, dirty.rect[c].y, dirty.rect[c].w, dirty.rect[c].h);

      if (!gfx_driver->linear)
	 qsort(old_dirty.rect, old_dirty.count, sizeof(DIRTY_RECT), rect_cmp);

      acquire_screen();

      for (c=0; c<old_dirty.count; c++) {
	 if ((old_dirty.rect[c].w == 1) && (old_dirty.rect[c].h == 1)) {
	    putpixel(screen, old_dirty.rect[c].x, old_dirty.rect[c].y, 
		     getpixel(buffer, old_dirty.rect[c].x, old_dirty.rect[c].y));
	 }
	 else {
	    blit(buffer, screen, old_dirty.rect[c].x, old_dirty.rect[c].y, 
				 old_dirty.rect[c].x, old_dirty.rect[c].y, 
				 old_dirty.rect[c].w, old_dirty.rect[c].h);
	 }
      }

      release_screen();

      poll_joystick();

   } while ((!keypressed()) && (!joy[0].button[0].b) && (!joy[0].button[1].b));

   if (use_retrace_proc)
      retrace_proc = NULL;
   else
      remove_int(scroll_counter);

   fade_out(5);

   destroy_bitmap(buffer);
   destroy_bitmap(text_bmp);

   while (keypressed())
      if ((readkey() & 0xff) == 27)
	 return FALSE;

   return TRUE;
}



int main(int argc, char *argv[])
{
   int c, w, h;
   char buf[256], buf2[256];
   BITMAP *bmp;
   CREDIT_NAME *cred;
   TEXT_LIST *tl;

   for (c=1; c<argc; c++) {
      if (stricmp(argv[c], "-cheat") == 0)
	 cheat = TRUE;

      if (stricmp(argv[c], "-turbo") == 0)
	 turbo = TRUE;

      if (stricmp(argv[c], "-jumpstart") == 0)
	 jumpstart = TRUE;
   }

   allegro_init();
   install_keyboard();
   install_timer();
   install_mouse();

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, argv[0]) != 0) {
      allegro_message("Error initialising sound\n%s\n", allegro_error);
      install_sound (DIGI_NONE, MIDI_NONE, NULL);
   }

   if (install_joystick(JOY_TYPE_AUTODETECT) != 0) {
      allegro_message("Error initialising joystick\n%s\n", allegro_error);
      install_joystick (JOY_TYPE_NONE);
   }

   #ifdef ALLEGRO_CONSOLE_OK
      if (!jumpstart)
	 fade_out(4);
   #endif

   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }

   get_executable_name(buf, sizeof(buf));
   replace_filename(buf2, buf, "demo.dat", sizeof(buf2));
   set_color_conversion(COLORCONV_NONE);
   data = load_datafile(buf2);
   if (!data) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s\n", buf2);
      exit(1);
   }

   load_text();

   if (!jumpstart) {
      /* horrible hack to use MIDI music if we have a nice MIDI driver, or
       * play a sample otherwise, but still compile on platforms that don't
       * know about the MIDI drivers we are looking for. Urgh, I'm quite
       * ashamed to admit that I actually wrote this :-)
       */
      #ifdef ALLEGRO_WINDOWS

 	 play_midi(data[INTRO_MUSIC].dat, FALSE);

      #else

	 if ((0) ||
	    #ifdef MIDI_DIGMID
	       (midi_driver->id == MIDI_DIGMID) ||
	    #endif
	    #ifdef MIDI_AWE32
	       (midi_driver->id == MIDI_AWE32) ||
	    #endif
	    #ifdef MIDI_MPU
	       (midi_driver->id == MIDI_MPU) ||
	    #endif
	    #ifdef MIDI_SB_OUT
	       (midi_driver->id == MIDI_SB_OUT) ||
	    #endif
	    (0))
	 play_midi(data[INTRO_MUSIC].dat, FALSE);
      else 
	 play_sample(data[INTRO_SPL].dat, 255, 128, 1000, FALSE);

      #endif

      bmp = create_sub_bitmap(screen, SCREEN_W/2-160, SCREEN_H/2-100, 320, 200);
      play_memory_fli(data[INTRO_ANIM].dat, bmp, FALSE, NULL);
      destroy_bitmap(bmp);
   }

   c = retrace_count;

   if (!jumpstart) {
      do {
      } while (retrace_count-c < 120);

      fade_out(1);
   }

   clear_bitmap(screen);
   set_gui_colors();
   set_mouse_sprite(NULL);

   if (!gfx_mode_select(&c, &w, &h))
      exit(1);

   if (pick_animation_type(&animation_type) < 0)
      exit(1);

   set_color_depth(8);
   if (set_gfx_mode(c, w, h, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting 8bpp graphics mode\n%s\n", allegro_error);
      exit(1);
   }

   generate_explosions();

   switch (animation_type) {

      case PAGE_FLIP:
	 /* set up page flipping bitmaps */
	 page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
	 page2 = create_video_bitmap(SCREEN_W, SCREEN_H);

	 if ((!page1) || (!page2)) {
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	    allegro_message("Not enough video memory for page flipping\n");
	    exit(1);
	 }
	 break;

      case RETRACE_FLIP: 
	 /* set up retrace-synced page flipping bitmaps */
	 if (!timer_can_simulate_retrace()) {
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	    allegro_message("Retrace syncing is not possible in this environment\n");
	    exit(1);
	 }

	 timer_simulate_retrace(TRUE);

	 page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
	 page2 = create_video_bitmap(SCREEN_W, SCREEN_H);

	 if ((!page1) || (!page2)) {
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	    allegro_message("Not enough video memory for page flipping\n");
	    exit(1);
	 }
	 break;

      case TRIPLE_BUFFER: 
	 /* set up triple buffered bitmaps */
	 if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER))
	    enable_triple_buffer();

	 if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)) {
	    strcpy(buf, gfx_driver->name);
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

	    #ifdef ALLEGRO_DOS
	       allegro_message("The %s driver does not support triple buffering\n\nTry using mode-X in clean DOS mode (not under Windows)\n", buf);
	    #else
	       allegro_message("The %s driver does not support triple buffering\n", buf);
	    #endif

	    exit(1);
	 }

	 page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
	 page2 = create_video_bitmap(SCREEN_W, SCREEN_H);
	 page3 = create_video_bitmap(SCREEN_W, SCREEN_H);

	 if ((!page1) || (!page2) || (!page3)) {
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	    allegro_message("Not enough video memory for triple buffering\n");
	    exit(1);
	 }
	 break;
   }

   use_retrace_proc = timer_is_using_retrace();

   LOCK_VARIABLE(game_time);
   LOCK_FUNCTION(game_timer);

   LOCK_VARIABLE(scroll_count);
   LOCK_FUNCTION(scroll_counter);

   LOCK_VARIABLE(score);
   LOCK_VARIABLE(frame_count);
   LOCK_VARIABLE(fps);
   LOCK_FUNCTION(fps_proc);

   s = create_bitmap(SCREEN_W, SCREEN_H);

   stop_sample(data[INTRO_SPL].dat);

   text_mode(0);
   clear_bitmap(screen);
   textout_centre(screen, font, "Scanning for author credits...", SCREEN_W/2, SCREEN_H/2-16, makecol(160, 160, 160));

   if (SCREEN_W >= 640)
      load_credits();

   while (title_screen())
      play_game(); 

   destroy_bitmap(s);

   if ((animation_type == PAGE_FLIP) || (animation_type == RETRACE_FLIP)) {
      destroy_bitmap(page1);
      destroy_bitmap(page2);
   }
   else if (animation_type == TRIPLE_BUFFER) {
      destroy_bitmap(page1);
      destroy_bitmap(page2);
      destroy_bitmap(page3);
   }

   for (c=0; c<EXPLODE_FRAMES; c++)
      destroy_rle_sprite(explosion[c]);

   stop_midi();

   set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

   if (end_text) {
      allegro_message("%shttp://alleg.sourceforge.net/\n\n", end_text);
      free(end_text);
   }

   if ((title_text) && (title_alloced))
      free(title_text);

   while (credits) {
      cred = credits;
      credits = cred->next;

      if (cred->name)
	 free(cred->name);

      if (cred->text)
	 free(cred->text);

      while (cred->files) {
	 tl = cred->files;
	 cred->files = tl->next;

	 if (tl->text)
	    free(tl->text);

	 free(tl);
      }

      free(cred);
   }

   unload_datafile(data);

   allegro_exit();

   return 0;
}

END_OF_MAIN();
