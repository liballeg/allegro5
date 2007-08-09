/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use the 32 bit RGBA
 *    translucency functions to store an alpha channel along with
 *    a bitmap graphic.  Two images are loaded from disk. One will
 *    be used for the background and the other as a sprite. The
 *    example generates an alpha channel for the sprite image,
 *    composing the 32 bit RGBA bitmap during runtime, and draws
 *    it at the position of the mouse cursor.
 */


#include <allegro.h>



int main(int argc, char *argv[])
{
   char buf[256];
   BITMAP *background;
   BITMAP *alpha;
   BITMAP *sprite;
   BITMAP *buffer;
   int bpp = -1;
   int ret = -1;
   int x, y, c, a;

   if (allegro_init() != 0)
      return 1;
   install_keyboard(); 
   install_mouse(); 
   install_timer();

   /* what color depth should we use? */
   if (argc > 1) {
      if ((argv[1][0] == '-') || (argv[1][0] == '/'))
	 argv[1]++;
      bpp = atoi(argv[1]);
      if ((bpp != 15) && (bpp != 16) && (bpp != 24) && (bpp != 32)) {
	 allegro_message("Invalid color depth '%s'\n", argv[1]);
	 return 1;
      }
   }

   if (bpp > 0) {
      /* set a user-requested color depth */
      set_color_depth(bpp);
      ret = set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0);
   }
   else {
      /* autodetect what color depths are available */
      static int color_depths[] = { 16, 15, 32, 24, 0 };
      for (a=0; color_depths[a]; a++) {
	 bpp = color_depths[a];
	 set_color_depth(bpp);
	 ret = set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0);
	 if (ret == 0)
	    break;
      }
   }

   /* did the video mode set properly? */
   if (ret != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting %d bit graphics mode\n%s\n", bpp,
		      allegro_error);
      return 1;
   }

   /* load the background picture */
   replace_filename(buf, argv[0], "allegro.pcx", sizeof(buf));
   background = load_bitmap(buf, NULL);
   if (!background) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s!\n", buf);
      return 1;
   }

   /* make a copy of it */
   set_color_depth(32);
   sprite = create_bitmap(background->w, background->h);
   blit(background, sprite, 0, 0, 0, 0, background->w, background->h);

   /* load the alpha sprite image. Note that we specifically force this
    * to load in a 32 bit format by calling set_color_depth(). That is
    * because the disk file is actually only a 256 color graphic: if it
    * was already a 32 bit RGBA sprite, we would probably want to use
    * set_color_conversion(COLORCONV_NONE) instead.
    */
   replace_filename(buf, argv[0], "mysha.pcx", sizeof(buf));
   alpha = load_bitmap(buf, NULL);
   if (!alpha) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s!\n", buf);
      return 1;
   }

   /* normally we would have loaded an RGBA image directly from disk. Since
    * I don't have one lying around, and am too lazy to draw one (or I could
    * rationalise this by saying that I'm trying to save download size by
    * reusing graphics :-) I'll just have to generate an alpha channel in
    * code. I do this by using greyscale values from the mouse picture as an
    * alpha channel for the Allegro image. Don't worry about this code: you
    * wouldn't normally need to write anything like this, because you'd just
    * get the right graphics directly out of a datafile.
    */
   drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
   set_write_alpha_blender();

   for (y=0; y<sprite->h; y++) {
      for (x=0; x<sprite->w; x++) {
	 c = getpixel(alpha, x, y);
	 a = getr(c) + getg(c) + getb(c);
	 a = CLAMP(0, a/2-128, 255);

	 putpixel(sprite, x, y, a);
      }
   }

   destroy_bitmap(alpha);

   set_color_depth(bpp);

   /* darken the background image down a bit */
   drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
   set_multiply_blender(0, 0, 0, 255);
   rectfill(background, 0, 0, background->w, background->h,
	    makecol(32, 16, 128));
   solid_mode();

   /* create a double buffer bitmap */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   /* scale the background image to be the same size as the screen */
   stretch_blit(background, buffer, 0, 0, background->w, background->h,
		0, 0, SCREEN_W, SCREEN_H);

   textprintf_ex(buffer, font, 0, 0, makecol(255, 255, 255), -1,
		 "%dx%d, %dbpp", SCREEN_W, SCREEN_H, bpp);

   destroy_bitmap(background);
   background = create_bitmap(SCREEN_W, SCREEN_H);
   blit(buffer, background, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

   while (!keypressed()) {
      /* draw the alpha sprite */
      x = mouse_x - sprite->w/2;
      y = mouse_y - sprite->h/2;

      set_alpha_blender();
      draw_trans_sprite(buffer, sprite, x, y);

      /* flip it across to the screen */
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      /* replace the background where we drew the sprite */
      blit(background, buffer, x, y, x, y, sprite->w, sprite->h);
   }

   clear_keybuf();

   destroy_bitmap(background);
   destroy_bitmap(sprite);
   destroy_bitmap(buffer);

   return 0;
}

END_OF_MAIN()
