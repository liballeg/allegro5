/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates the use of triple buffering. Several
 *    triangles are displayed rotating and bouncing on the screen
 *    until you press a key. Note that on some platforms you
 *    can't get real hardware triple buffering.  The Allegro code
 *    remains the same, but most likely the graphic driver will
 *    emulate it. Unfortunately, in these cases you can't expect
 *    the animation to be completely smooth and flicker free.
 */


#include <allegro.h>



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
int triplebuffer_not_available = 0;



/* randomly initialises a shape structure */
void init_shape(SHAPE *shape)
{
   shape->color = 1+(AL_RAND()%15);

   /* randomly position the corners */
   shape->dir1 = itofix(AL_RAND()%256);
   shape->dir2 = itofix(AL_RAND()%256);
   shape->dir3 = itofix(AL_RAND()%256);

   shape->dist1 = itofix(AL_RAND()%64);
   shape->dist2 = itofix(AL_RAND()%64);
   shape->dist3 = itofix(AL_RAND()%64);

   /* rand centre position and movement speed/direction */
   shape->x = itofix(AL_RAND() % SCREEN_W);
   shape->y = itofix(AL_RAND() % SCREEN_H);
   shape->ac = itofix((AL_RAND()%9)-4);
   shape->xc = itofix((AL_RAND()%7)-2);
   shape->yc = itofix((AL_RAND()%7)-2);
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
      shape->ac = itofix((AL_RAND()%9)-4);
   }

   if (((shape->y <= 0) && (shape->yc < 0)) ||
       ((shape->y >= itofix(SCREEN_H)) && (shape->yc > 0))) {
      shape->yc = -shape->yc;
      shape->ac = itofix((AL_RAND()%9)-4);
   }
}



/* draws a frame of the animation */
void draw(BITMAP *b)
{
   int c;
   char message[1024];

   acquire_bitmap(b);

   clear_bitmap(b);

   for (c=0; c<NUM_SHAPES; c++) {
      triangle(b, 
	       fixtoi(shapes[c].x+fixmul(shapes[c].dist1,
					 fixcos(shapes[c].dir1))),
	       fixtoi(shapes[c].y+fixmul(shapes[c].dist1,
					 fixsin(shapes[c].dir1))),
	       fixtoi(shapes[c].x+fixmul(shapes[c].dist2,
					 fixcos(shapes[c].dir2))),
	       fixtoi(shapes[c].y+fixmul(shapes[c].dist2,
					 fixsin(shapes[c].dir2))),
	       fixtoi(shapes[c].x+fixmul(shapes[c].dist3,
					 fixcos(shapes[c].dir3))),
	       fixtoi(shapes[c].y+fixmul(shapes[c].dist3,
					 fixsin(shapes[c].dir3))),
	       shapes[c].color);

      move_shape(shapes+c);
   } 

   if (triplebuffer_not_available)
      ustrzcpy(message, sizeof message, "Simulated triple buffering");
   else
      ustrzcpy(message, sizeof message, "Real triple buffering");

   textout_ex(b, font, message, 0, 0, 255, -1);

   release_bitmap(b);
}



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



int main(void)
{
   BITMAP *page1, *page2, *page3;
   int c;
   int w, h;

#ifdef ALLEGRO_DOS
   w = 320;
   h = 240;
#else
   w = 640;
   h = 480;
#endif

   if (allegro_init() != 0)
      return 1;
   install_timer();
   install_keyboard();
   install_mouse();

   /* see comments in exflip.c */
#ifdef ALLEGRO_VRAM_SINGLE_SURFACE
   if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, h * 3) != 0) {
#else
   if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0) != 0) {
#endif
      if (set_gfx_mode(GFX_SAFE, w, h, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n",
			 allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);

   /* if triple buffering isn't available, try to enable it */
   if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER))
      enable_triple_buffer();

   /* if that didn't work, give up */
   if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)) {
      triplebuffer_not_available = TRUE;
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

   triple_buffer(page1, page2, page3);

   destroy_bitmap(page1);
   destroy_bitmap(page2);
   destroy_bitmap(page3);

   return 0;
}

END_OF_MAIN()
