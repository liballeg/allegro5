/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use an offscreen part of
 *    the video memory to store source graphics for a hardware
 *    accelerated graphics driver. The example loads the `mysha.pcx'
 *    file and then blits it several times on the screen. Depending
 *    on whether you have enough video memory and Allegro supports
 *    the hardware acceleration features of your card, your success
 *    running this example may be none at all, sluggish performance
 *    due to software emulation, or flicker free smooth hardware
 *    accelerated animation.
 */


#include <allegro.h>



#define MAX_IMAGES      256


/* structure to hold the current position and velocity of an image */
typedef struct IMAGE
{
   float x, y;
   float dx, dy;
} IMAGE;



/* initialises an image structure to a random position and velocity */
void init_image(IMAGE *image)
{
   image->x = (float)(AL_RAND() % 704);
   image->y = (float)(AL_RAND() % 568);
   image->dx = (float)((AL_RAND() % 255) - 127) / 32.0;
   image->dy = (float)((AL_RAND() % 255) - 127) / 32.0;
}



/* called once per frame to bounce an image around the screen */
void update_image(IMAGE *image)
{
   image->x += image->dx;
   image->y += image->dy;

   if (((image->x < 0) && (image->dx < 0)) ||
       ((image->x > 703) && (image->dx > 0)))
      image->dx *= -1;

   if (((image->y < 0) && (image->dy < 0)) ||
       ((image->y > 567) && (image->dy > 0)))
      image->dy *= -1;
}



int main(int argc, char *argv[])
{
   char buf[256];
   PALETTE pal;
   BITMAP *image;
   BITMAP *page[2];
   BITMAP *vimage;
   IMAGE images[MAX_IMAGES];
   int num_images = 4;
   int page_num = 1;
   int done = FALSE;
   int i;

   if (allegro_init() != 0)
      return 1;
   install_keyboard(); 
   install_timer();

   /* see comments in exflip.c */
#ifdef ALLEGRO_VRAM_SINGLE_SURFACE
   if (set_gfx_mode(GFX_AUTODETECT, 1024, 768, 0, 2 * 768 + 200) != 0) {
#else
   if (set_gfx_mode(GFX_AUTODETECT, 1024, 768, 0, 0) != 0) {
#endif
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   /* read in the source graphic */
   replace_filename(buf, argv[0], "mysha.pcx", sizeof(buf));
   image = load_bitmap(buf, pal);
   if (!image) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s!\n", buf);
      return 1;
   }

   set_palette(pal);

   /* initialise the images to random positions */
   for (i=0; i<MAX_IMAGES; i++)
      init_image(images+i);

   /* create two video memory bitmaps for page flipping */
   page[0] = create_video_bitmap(SCREEN_W, SCREEN_H);
   page[1] = create_video_bitmap(SCREEN_W, SCREEN_H);

   /* create a video memory bitmap to store our picture */
   vimage = create_video_bitmap(image->w, image->h);

   if ((!page[0]) || (!page[1]) || (!vimage)) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Not enough video memory (need two 1024x768 pages "
		      "and a 320x200 image)\n");
      return 1;
   }

   /* copy the picture into offscreen video memory */
   blit(image, vimage, 0, 0, 0, 0, image->w, image->h);

   while (!done) {
      acquire_bitmap(page[page_num]);

      /* clear the screen */
      clear_bitmap(page[page_num]);

      /* draw onto it */
      for (i=0; i<num_images; i++)
	 blit(vimage, page[page_num], 0, 0, images[i].x, images[i].y,
	      vimage->w, vimage->h);

      textprintf_ex(page[page_num], font, 0, 0, 255, -1,
		    "Images: %d (arrow keys to change)", num_images);

      /* tell the user which functions are being done in hardware */
      if (gfx_capabilities & GFX_HW_FILL)
	 textout_ex(page[page_num], font, "Clear: hardware accelerated",
		    0, 16, 255, -1);
      else
	 textout_ex(page[page_num], font, "Clear: software (urgh, this "
		    "is not good!)", 0, 16, 255, -1);

      if (gfx_capabilities & GFX_HW_VRAM_BLIT)
	 textout_ex(page[page_num], font, "Blit: hardware accelerated",
		    0, 32, 255, -1);
      else
	 textout_ex(page[page_num], font, "Blit: software (urgh, this program "
		    "will run too sloooooowly without hardware acceleration!)",
		    0, 32, 255, -1);

      release_bitmap(page[page_num]);

      /* page flip */
      show_video_bitmap(page[page_num]);
      page_num = 1-page_num;

      /* deal with keyboard input */
      while (keypressed()) {
	 switch (readkey()>>8) {

	    case KEY_UP:
	    case KEY_RIGHT:
	       if (num_images < MAX_IMAGES)
		  num_images++;
	       break;

	    case KEY_DOWN:
	    case KEY_LEFT:
	       if (num_images > 0)
		  num_images--;
	       break;

	    case KEY_ESC:
	       done = TRUE;
	       break;
	 }
      }

      /* bounce the images around the screen */
      for (i=0; i<num_images; i++)
	 update_image(images+i);
   }

   destroy_bitmap(image);
   destroy_bitmap(vimage);
   destroy_bitmap(page[0]);
   destroy_bitmap(page[1]);

   return 0;
}

END_OF_MAIN()
