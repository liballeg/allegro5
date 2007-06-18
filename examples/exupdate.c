/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to support double buffering,
 *    page flipping, and triple buffering as options within a single
 *    program, and how to make things run at a constant rate no
 *    matter what the speed of your computer. You have to use this
 *    example from the command line to specify as first parameter a
 *    number which represents the type of video update you want: 1
 *    for double buffering with memory bitmaps, 2 for page flipping,
 *    3 for triple buffering and 4 for double buffering with system
 *    bitmaps. After this, a dialog allows you to select a screen
 *    resolution and finally you will see a kaleidoscopic animation,
 *    along with a frames per second counter on the top left of
 *    the screen.
 */


#include <allegro.h>



/* number of video memory pages:
 *  1 = double buffered
 *  2 = page flipping
 *  3 = triple buffered
 */
int num_pages;

int use_system_bitmaps;



/* counters for speed control and frame rate measurement */
volatile int update_count = 0;
volatile int frame_count = 0;
volatile int fps = 0;



/* timer callback for controlling the program speed */
void timer_proc(void)
{
   update_count++;
}

END_OF_FUNCTION(timer_proc)



/* timer callback for measuring the frame rate */
void fps_proc(void)
{
   fps = frame_count;
   frame_count = 0;
}

END_OF_FUNCTION(fps_proc)



/* some rotation values for the graphical effect */
fixed r1 = 0;
fixed r2 = 0;
fixed r3 = 0;
fixed r4 = 0;



/* helper to draw four mirrored versions of a triangle */
void kalid(BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3,
	   int r, int g, int b)
{
   triangle(bmp, SCREEN_W/2+x1, SCREEN_H/2+y1, SCREEN_W/2+x2, SCREEN_H/2+y2,
	    SCREEN_W/2+x3, SCREEN_H/2+y3, makecol(r, g, b));
   triangle(bmp, SCREEN_W/2-x1, SCREEN_H/2+y1, SCREEN_W/2-x2, SCREEN_H/2+y2,
	    SCREEN_W/2-x3, SCREEN_H/2+y3, makecol(r, g, b));
   triangle(bmp, SCREEN_W/2-x1, SCREEN_H/2-y1, SCREEN_W/2-x2, SCREEN_H/2-y2,
	    SCREEN_W/2-x3, SCREEN_H/2-y3, makecol(r, g, b));
   triangle(bmp, SCREEN_W/2+x1, SCREEN_H/2-y1, SCREEN_W/2+x2, SCREEN_H/2-y2,
	    SCREEN_W/2+x3, SCREEN_H/2-y3, makecol(r, g, b));
}



/* draws the current animation frame into the specified bitmap */
void draw_screen(BITMAP *bmp)
{
   fixed c1 = fixcos(r1);
   fixed c2 = fixcos(r2);
   fixed c3 = fixcos(r3);
   fixed c4 = fixcos(r4);
   fixed s1 = fixsin(r1);
   fixed s2 = fixsin(r2);
   fixed s3 = fixsin(r3);
   fixed s4 = fixsin(r4);

   acquire_bitmap(bmp);

   clear_bitmap(bmp);

   xor_mode(TRUE);

   kalid(bmp, fixtoi(c1*SCREEN_W/3), fixtoi(s1*SCREEN_H/3),
	      fixtoi(c2*SCREEN_W/3), fixtoi(s2*SCREEN_H/3),
	      fixtoi(c3*SCREEN_W/3), fixtoi(s3*SCREEN_H/3),
	      127+fixtoi(c1*127), 127+fixtoi(c2*127), 127+fixtoi(c3*127));

   kalid(bmp, fixtoi(s1*SCREEN_W/3), fixtoi(c2*SCREEN_H/3),
	      fixtoi(s3*SCREEN_W/3), fixtoi(c4*SCREEN_H/3),
	      fixtoi(c3*SCREEN_W/3), fixtoi(s4*SCREEN_H/3),
	      127+fixtoi(s1*127), 127+fixtoi(c4*127), 127+fixtoi(s4*127));

   kalid(bmp, fixtoi(fixmul(s2, c3)*SCREEN_W/3), fixtoi(c1*SCREEN_H/3),
	      fixtoi(c4*SCREEN_W/3), fixtoi(fixmul(c2, s3)*SCREEN_H/3),
	      fixtoi(fixmul(c3, s4)*SCREEN_W/3), fixtoi(s1*SCREEN_H/3),
	      127+fixtoi(s2*127), 127+fixtoi(c3*127), 127+fixtoi(s3*127));

   xor_mode(FALSE);

   if (num_pages == 1) {
      if (use_system_bitmaps)
	 textout_ex(bmp, font, "Double buffered (system bitmap)", 0, 0,
		    makecol(255, 255, 255), -1);
      else
	 textout_ex(bmp, font, "Double buffered (memory bitmap)", 0, 0,
		    makecol(255, 255, 255), -1);
   }
   else if (num_pages == 2)
      textout_ex(bmp, font, "Page flipping (two pages of vram)", 0, 0,
		 makecol(255, 255, 255), -1);
   else
      textout_ex(bmp, font, "Triple buffered (three pages of vram)", 0, 0,
		 makecol(255, 255, 255), -1);

   textout_ex(bmp, font, gfx_driver->name, 0, 16, makecol(255, 255, 255), -1);

   textprintf_ex(bmp, font, 0, 32, makecol(255, 255, 255), -1, "FPS: %d", fps);

   release_bitmap(bmp);
}



/* called at a regular speed to update the program state */
void do_update(void)
{
   r1 += ftofix(0.5);
   r2 += ftofix(0.6);
   r3 += ftofix(0.11);
   r4 += ftofix(0.13);
}



int main(int argc, char *argv[])
{ 
   PALETTE pal;
   BITMAP *bmp[3];
   int card = GFX_AUTODETECT;
   int w, h, bpp, page, i;

   if (allegro_init() != 0)
      return 1;

   /* check command line arguments */
   if (argc == 2)
      num_pages = atoi(argv[1]);
   else
      num_pages = 0;

   if (num_pages == 4) {
      num_pages = 1;
      use_system_bitmaps = TRUE;
   }
   else
      use_system_bitmaps = FALSE;

   if ((num_pages < 1) || (num_pages > 3)) {
      allegro_message("\nUsage: 'exupdate num_pages', where num_pages is one of:\n\n"
		      "1 = double buffered (memory bitmap)\n\n"
		      "    + easy, reliable\n"
		      "    + drawing onto a memory bitmap is very fast\n"
		      "    - blitting the finished image to the screen can be quite slow\n\n"
		      "2 = page flipping (two pages of video memory)\n\n"
		      "    + avoids the need for a memory to screen blit of the completed image\n"
		      "    + can use hardware acceleration when it is available\n"
		      "    - drawing directly to vram can be slower than using a memory bitmap\n"
		      "    - requires a card with enough video memory for two screen pages\n\n"
		      "3 = triple buffered (three pages of video memory)\n\n"
		      "    + like page flipping, but faster and smoother\n"
		      "    - requires a card with enough video memory for three screen pages\n"
		      "    - only possible with some graphics drivers\n\n"
		      "4 = double buffered (system bitmap)\n\n"
		      "    + as easy as normal double buffering\n"
		      "    + system bitmaps can be hardware accelerated on some platforms\n"
		      "    - drawing to system bitmaps is sometimes slower than memory bitmaps\n\n");
      return 1;
   }

   /* set up Allegro */
   install_keyboard();
   install_timer();
   install_mouse();
   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);

   w = SCREEN_W;
   h = SCREEN_H;
   bpp = bitmap_color_depth(screen);
   if (!gfx_mode_select_ex(&card, &w, &h, &bpp))
      return 0;

   set_color_depth(bpp);

#ifdef ALLEGRO_VRAM_SINGLE_SURFACE
   if (set_gfx_mode(card, w, h, w, num_pages*h) != 0) {
#else
   if (set_gfx_mode(card, w, h, 0, 0) != 0) {
#endif
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   generate_332_palette(pal);
   pal[0].r = pal[0].g = pal[0].b = 0;
   set_palette(pal);

   switch (num_pages) {

      case 1:
	 /* double buffering setup */
	 if (use_system_bitmaps)
	    bmp[0] = create_system_bitmap(SCREEN_W, SCREEN_H);
	 else
	    bmp[0] = create_bitmap(SCREEN_W, SCREEN_H);
	 break;

      case 2:
	 /* page flipping setup */
	 bmp[0] = create_video_bitmap(SCREEN_W, SCREEN_H);
	 bmp[1] = create_video_bitmap(SCREEN_W, SCREEN_H);
	 break;

      case 3:
	 /* triple buffering setup */
	 if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER))
	    enable_triple_buffer();

	 if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)) {
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

	    #ifdef ALLEGRO_DOS
	    allegro_message("This driver does not support triple buffering\n"
			    "\nTry using mode-X in clean DOS mode (not under "
			    "Windows)\n");
	    #else
	    allegro_message("This driver does not support triple buffering\n");
	    #endif

	    return 1;
	 }

	 bmp[0] = create_video_bitmap(SCREEN_W, SCREEN_H);
	 bmp[1] = create_video_bitmap(SCREEN_W, SCREEN_H);
	 bmp[2] = create_video_bitmap(SCREEN_W, SCREEN_H);
	 break;
   }

   for (i=0; i<num_pages; i++) {
      if (!bmp[i]) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to create %d video memory pages\n",
			 num_pages);
	 return 1;
      }
   }

   /* install timer handlers to control and measure the program speed */
   LOCK_VARIABLE(update_count);
   LOCK_VARIABLE(frame_count);
   LOCK_VARIABLE(fps);
   LOCK_FUNCTION(timer_proc);
   LOCK_FUNCTION(fps_proc);

   install_int_ex(timer_proc, BPS_TO_TIMER(60));
   install_int_ex(fps_proc, BPS_TO_TIMER(1));

   page = 1;

   /* main program loop */
   while (!keypressed()) {

      /* draw the next frame of graphics */
      switch (num_pages) {

	 case 1:
	    /* draw onto a memory bitmap, then blit to the screen */
	    draw_screen(bmp[0]);
	    vsync();
	    blit(bmp[0], screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
	    break;

	 case 2:
	    /* flip back and forth between two pages of video memory */
	    draw_screen(bmp[page]);
	    show_video_bitmap(bmp[page]);
	    page = 1-page;
	    break;

	 case 3:
	    /* triple buffering */
	    draw_screen(bmp[page]);

	    do {
	    } while (poll_scroll());

	    request_video_bitmap(bmp[page]);
	    page = (page+1)%3;
	    break;
      }

      /* update the program state */
      while (update_count > 0) {
	 do_update();
	 update_count--;
      }

      frame_count++;
   }

   for (i=0; i<num_pages; i++)
      destroy_bitmap(bmp[i]);

   return 0;
}

END_OF_MAIN()
