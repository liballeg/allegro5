/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates the use of triple buffering and vertical
 *    retrace interrupt simulation.
 */


#include "allegro.h"



#define NUM_SHAPES   16


typedef struct SHAPE
{
   int color;                          /* color of the shape */
   fixed x, y;                         /* centre of the shape */
   fixed dir1, dir2, dir3;             /* directions to the three corners */
   fixed dist1, dist2, dist3;          /* distances to the three corners */
   fixed xc, yc, ac;                   /* position and angle change values */
} SHAPE;


SHAPE shapes[NUM_SHAPES];



/* randomly initialises a shape structure */
void init_shape(SHAPE *shape)
{
   shape->color = 1+(rand()%15);

   /* randomly position the corners */
   shape->dir1 = itofix(rand()%256);
   shape->dir2 = itofix(rand()%256);
   shape->dir3 = itofix(rand()%256);

   shape->dist1 = itofix(rand()%64);
   shape->dist2 = itofix(rand()%64);
   shape->dist3 = itofix(rand()%64);

   /* rand centre position and movement speed/direction */
   shape->x = itofix(rand() % SCREEN_W);
   shape->y = itofix(rand() % SCREEN_H);
   shape->ac = itofix((rand()%9)-4);
   shape->xc = itofix((rand()%7)-2);
   shape->yc = itofix((rand()%7)-2);
}



/* updates the position of a shape structure */
void move_shape(SHAPE *shape)
{
   shape->x += shape->xc;
   shape->y += shape->yc;

   shape->dir1 += shape->ac;
   shape->dir2 += shape->ac;
   shape->dir3 += shape->ac;

   if (((shape->x <= 0) && (shape->xc < 0)) ||
       ((shape->x >= itofix(SCREEN_W)) && (shape->xc > 0))) {
      shape->xc = -shape->xc;
      shape->ac = itofix((rand()%9)-4);
   }

   if (((shape->y <= 0) && (shape->yc < 0)) ||
       ((shape->y >= itofix(SCREEN_H)) && (shape->yc > 0))) {
      shape->yc = -shape->yc;
      shape->ac = itofix((rand()%9)-4);
   }
}



/* draws a frame of the animation */
void draw(BITMAP *b)
{
   int c;

   acquire_bitmap(b);

   clear_bitmap(b);

   for (c=0; c<NUM_SHAPES; c++) {
      triangle(b, 
	       fixtoi(shapes[c].x+fixmul(shapes[c].dist1, fixcos(shapes[c].dir1))),
	       fixtoi(shapes[c].y+fixmul(shapes[c].dist1, fixsin(shapes[c].dir1))),
	       fixtoi(shapes[c].x+fixmul(shapes[c].dist2, fixcos(shapes[c].dir2))),
	       fixtoi(shapes[c].y+fixmul(shapes[c].dist2, fixsin(shapes[c].dir2))),
	       fixtoi(shapes[c].x+fixmul(shapes[c].dist3, fixcos(shapes[c].dir3))),
	       fixtoi(shapes[c].y+fixmul(shapes[c].dist3, fixsin(shapes[c].dir3))),
	       shapes[c].color);

      move_shape(shapes+c);
   } 

   if (retrace_proc)
      textout(b, font, "Triple buffering with retrace sync", 0, 0, 255);
   else
      textout(b, font, "Triple buffering", 0, 0, 255);

   release_bitmap(b);
}



int fade_color = 63;

/* vertical retrace callback function for doing the palette fade */
void fade(void)
{
   int c = (fade_color < 64) ? fade_color : 127 - fade_color;
   RGB rgb;

   rgb.r = rgb.g = rgb.b = c;

   _set_color(0, &rgb);

   fade_color++;
   if (fade_color >= 128)
      fade_color = 0;
}

END_OF_FUNCTION(fade);



/* main animation control loop */
void triple_buffer(BITMAP *page1, BITMAP *page2, BITMAP *page3)
{
   BITMAP *active_page = page1;
   int page = 0;

   do {
      /* draw a frame */
      draw(active_page);

      /* make sure the last flip request has actually happened */
      do {
      } while (poll_scroll());

      /* post a request to display the page we just drew */
      request_video_bitmap(active_page);

      /* update counters to point to the next page */
      switch (page) {
	 case 0:  page = 1;  active_page = page2;  break;
	 case 1:  page = 2;  active_page = page3;  break;
	 case 2:  page = 0;  active_page = page1;  break;
      }

   } while (!keypressed());

   clear_keybuf();
}



int main()
{
   BITMAP *page1, *page2, *page3;
   int c;

   allegro_init();
   install_timer();
   install_keyboard();
   install_mouse();

   /* see comments in exflip.c */
   if (set_gfx_mode(GFX_AUTODETECT, 320, 240, 0, 720) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 240, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }
   set_palette(desktop_palette);

   /* if triple buffering isn't available, try to enable it */
   if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER))
      enable_triple_buffer();

   /* if that didn't work, give up */
   if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

      #ifdef ALLEGRO_DOS
	 allegro_message("Triple buffering not available, sorry\n\nTry again in clean DOS mode (not under Windows)\n");
      #else
	 allegro_message("Triple buffering not available, sorry\n");
      #endif

      return 1;
   }

   /* allocate three sub bitmaps to access pages of the screen */
   page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
   page2 = create_video_bitmap(SCREEN_W, SCREEN_H);
   page3 = create_video_bitmap(SCREEN_W, SCREEN_H);

   if ((!page1) || (!page2) || (!page3)) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to create three video memory pages\n");
      return 1;
   }

   /* initialise the shapes */
   for (c=0; c<NUM_SHAPES; c++)
      init_shape(shapes+c);

   text_mode(-1);

   LOCK_VARIABLE(fade_color);
   LOCK_FUNCTION(fade);

   if (timer_can_simulate_retrace()) {
      timer_simulate_retrace(TRUE);
      retrace_proc = fade;
   }
   else {
      alert("Retrace sync is not available in",
	    "this environment, so you won't get",
	    "the funky smooth palette fading", 
	    "Damn, that's a shame", NULL, 13, 0);
   }

   triple_buffer(page1, page2, page3);

   destroy_bitmap(page1);
   destroy_bitmap(page2);
   destroy_bitmap(page3);

   retrace_proc = NULL;

   return 0;
}

END_OF_MAIN();
