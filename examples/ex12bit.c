/*
 *    Example program for the Allegro library, by Richard Mitton.
 *
 *    This program sets up a 12-bit mode on any 8-bit card, by
 *    setting up a 256-colour palette that will fool the eye into
 *    grouping two 8-bit pixels into one 12-bit pixel. In order
 *    to do this, you make your 256-colour palette with all the
 *    combinations of blue and green, assuming green ranges from 0-15
 *    and blue from 0-14. This takes up 16x15=240 colours. This
 *    leaves 16 colours to use as red (red ranges from 0-15).
 *    Then you put your green/blue in one pixel, and your red in
 *    the pixel next to it. The eye gets fooled into thinking it's
 *    all one pixel.
 *
 *    The example starts setting a normal 256 color mode, and
 *    construct a special palette for it. But then comes the trick:
 *    you need to write to a set of two adjacent pixels to form a
 *    single 12 bit dot. Two eight bit pixels is the same as one 16
 *    bit pixel, so after setting the video mode you need to hack
 *    the screen bitmap about, halving the width and changing it
 *    to use the 16 bit drawing code. Then, once you have packed a
 *    color into the correct format (using the makecol12() function
 *    below), any of the normal Allegro drawing functions can be
 *    used with this 12 bit display!
 *
 *    Things to note:
 *
 *    <ul><li>The horizontal width is halved, so you get resolutions
 *    like 320x480, 400x600, and 512x768.
 *
 *    <li>Because each dot is spread over two actual pixels, the
 *    display will be darker than in a normal video mode.
 *
 *    <li>Any bitmap data will obviously need converting to the
 *    correct 12 bit format: regular 15 or 16 bit images won't
 *    display correctly...
 *
 *    <li>Although this works like a truecolor mode, it is
 *    actually using a 256 color palette, so palette fades are
 *    still possible!
 *
 *    <li>This code only works in linear screen modes (don't try
 *    Mode-X).</ul>
 */


#include <allegro.h>



/* declare the screen size and mask colour we will use */
#define GFXW            320
#define GFXH            480
#define MASK_COLOR_12   0xFFE0


/* these are specific to this example. They say how big the balls are */
#define BALLW           12
#define BALLH           24
#define BIGBALLW        20
#define BIGBALLH        40
#define MESSAGE_STR     "Allegro"


typedef struct
{
   fixed x, y;
   int c;
} POINT_T;


/* these functions can be used in any 12-bit program */
int makecol12(int r, int g, int b);
void set_12bit_palette(void);
BITMAP *create_bitmap_12(int w, int h);


/* these functions are just for this example */
void blur_12(BITMAP *bmp, BITMAP *back);
void rgb_scales_12(BITMAP *bmp, int ox, int oy, int w, int h);
POINT_T *make_points(int *numpoints, char *msg);
BITMAP *make_ball(int w, int h, int br, int bg, int bb);



/* construct the magic palette that makes it all work */
void set_12bit_palette(void)
{
   int r, g, b;
   PALETTE pal;

   for (b=0; b<15; b++) {
      for (g=0; g<16; g++) {
	 pal[b*16+g].r = 0;
	 pal[b*16+g].g = g*63/15;
	 pal[b*16+g].b = b*63/15;
      }
   }

   for (r=0; r<16; r++) {
      pal[r+240].r = r*63/15;
      pal[r+240].g = 0;
      pal[r+240].b = 0;
   }

   set_palette(pal);
}



/* the other magic routine - use this to make colours instead of makecol */
int makecol12(int r, int g, int b)
{
   /* returns a 16-bit integer - here's the format:
    *
    *    0xARBG - where A=0xf (reserved, if you like),
    *                   R=red (0-15)
    *                   B=blue (0-14)
    *                   G=green (0-15)
    */

   r = r*16/256;
   g = g*16/256;
   b = b*16/256 - 1;

   if (b < 0)
      b = 0;

   return (r << 8) | (b << 4) | g | 0xF000;
}



/* extract red component from color */
int getr12(int color)
{
   return (color >> 4) & 0xF0;
}



/* extract green component from color */
int getg12(int color)
{
   return (color << 4) & 0xF0;
}



/* extract blue component from color */
int getb12(int color)
{
   return (color & 0xF0);
}



/* use this instead of create_bitmap, because the vtable needs changing
 * so that the drawing functions will use the 16-bit functions.
 */
BITMAP *create_bitmap_12(int w, int h)
{
   BITMAP *bmp;

   bmp = create_bitmap_ex(16, w, h);

   if (bmp) {
      bmp->vtable->color_depth = 12;
      bmp->vtable->mask_color = MASK_COLOR_12;
   }

   return bmp;
}



/* this merges 'bmp' into 'back'. This is how the trails work */
void blur_12(BITMAP *bmp, BITMAP *back)
{
   int x, y, r1, g1, b1, r2, g2, b2, c1, c2;

   for (y=0; y<bmp->h; y++) {
      unsigned short *backline = (unsigned short *)(back->line[y]);
      unsigned short *bmpline = (unsigned short *)(bmp->line[y]);

      for (x=0; x<bmp->w; x++) {
	 /* first get the pixel from each bitmap, then move the first
	  * colour value slightly towards the second.
	  */
	 c1 = bmpline[x];
	 c2 = backline[x];
	 r1 = c1 & 0xF00;
	 r2 = c2 & 0xF00;
	 if (r1 < r2)
	    c1 += 0x100;
	 else if (r1 > r2)
	    c1 -= 0x100;

	 b1 = c1 & 0xF0;
	 b2 = c2 & 0xF0;
	 if (b1 < b2)
	    c1 += 0x10;
	 else if (b1 > b2)
	    c1 -= 0x10;

	 g1 = c1 & 0x0F;
	 g2 = c2 & 0x0F;
	 if (g1 < g2)
	    c1 += 0x01;
	 else if (g1 > g2)
	    c1 -= 0x01;

	 /* then put it back in the bitmap */
	 bmpline[x] = c1;
      }
   }
}



/* generates some nice RGB scales onto the specified bitmap */
void rgb_scales_12(BITMAP *bmp, int ox, int oy, int w, int h)
{
   int x, y;

   for (y=0; y<h; y++)
      for (x=0; x<w; x++)
	 putpixel(bmp, ox+x, oy+y, makecol12(x*256/w, y*256/h, 0));

   for (y=0; y<h; y++)
      for (x=0; x<w; x++)
	 putpixel(bmp, ox+x+w, oy+y, makecol12(x*256/w, 0, y*256/h));

   for (y=0; y<h; y++)
      for (x=0; x<w; x++)
	 putpixel(bmp, ox+x, oy+y+h, makecol12(0, x*256/w, y*256/h));

   for (y=0; y<h; y++)
      for (x=0; x<w; x++)
	 putpixel(bmp, ox+x+w, oy+y+h, makecol12(x*128/w+y*128/h,
						 x*128/w+y*128/h,
						 x*128/w+y*128/h));
}



/* turns the string in 'msg' into a series of 2D points. These can then
 * be drawn with the vector balls.
 */
POINT_T *make_points(int *numpoints, char *msg)
{
   BITMAP *bmp;
   POINT_T *points;
   int n, x, y;

   bmp = create_bitmap_ex(8, text_length(font, msg), text_height(font));
   clear_bitmap(bmp);
   textout_ex(bmp, font, msg, 0, 0, 1, -1);

   /* first, count how much memory we will need to reserve */
   n = 0;

   for (y=0; y<bmp->h; y++)
      for (x=0; x<bmp->w; x++)
	 if (getpixel(bmp, x, y))
	    n++;

   points = malloc(n * sizeof(POINT_T));

   /* then redo it all, but actually store the points this time */
   n = 0;

   for (y=0; y<bmp->h; y++) {
      for (x=0; x<bmp->w; x++) {
	 if (getpixel(bmp, x, y)) {
	    points[n].x = itofix(x - bmp->w/2) * 6;
	    points[n].y = itofix(y - bmp->h/2) * 12;
	    points[n].c = AL_RAND() % 4;
	    n++;
	 }
      }
   }

   *numpoints = n;

   destroy_bitmap(bmp);

   return points;
}



/* this draws a vector ball. br/bg/bg is the colour of the brightest spot */
BITMAP *make_ball(int w, int h, int br, int bg, int bb)
{
   BITMAP *bmp;
   int r, rx, ry;
   bmp = create_bitmap_12(w, h); 

   clear_to_color(bmp, MASK_COLOR_12);

   for (r=0; r<16; r++) {
      rx = w*(15-r)/32;
      ry = h*(15-r)/32;
      ellipsefill(bmp, w/2, h/2, rx, ry, makecol12(br*r/15, bg*r/15, bb*r/15));
   }

   return bmp;
}



int main(void)
{
   BITMAP *rgbpic, *ball[4], *buffer, *bigball;
   int x, r=0, g=0, b=0, numpoints, thispoint;
   fixed xangle, yangle, zangle, newx, newy, newz;
   GFX_VTABLE *orig_vtable;
   POINT_T *points;
   MATRIX m;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();

   /* first set your graphics mode as normal, except twice as wide because
    * we are using 2-bytes per pixel, but the graphics card doesn't know this.
    */
   if (set_gfx_mode(GFX_AUTODETECT, GFXW * 2, GFXH, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting %ix%ix12 (really %ix%ix8, but we fake "
		      "it):\n%s\n", GFXW, GFXH, GFXW* 2, GFXH,
		      allegro_error);
      return 1;
   }

   /* then set your magic palette. From now on you can't use the set_color
    * or set_palette functions or they will mess up this palette. You can
    * still use the fade routines, if you make sure you fade back into this
    * palette.
    */
   set_12bit_palette();

   /* then hack the vtable so it uses the 16-bit functions */
   orig_vtable = screen->vtable;

   #ifdef ALLEGRO_COLOR16
      screen->vtable = &__linear_vtable16;
   #endif

   screen->vtable->color_depth = 12;
   screen->vtable->mask_color = MASK_COLOR_12;
   screen->vtable->unwrite_bank = orig_vtable->unwrite_bank;
   screen->w /= sizeof(short);

   /* reset the clip window to it's new parameters */
   set_clip_rect(screen, 0, 0, screen->w-1, screen->h-1);

   /* then generate 4 vector balls of different colours */
   for (x=0; x<4; x++) {
      switch (x) {
	 case 0: r = 255; g = 0;   b = 0;   break;
	 case 1: r = 0;   g = 255; b = 0;   break;
	 case 2: r = 0;   g = 0;   b = 255; break;
	 case 3: r = 255; g = 255; b = 0;   break;
      }

      ball[x] = make_ball(BALLW, BALLH, r, g, b);
   }

   /* also make one big red vector ball */
   bigball = make_ball(BIGBALLW, BIGBALLH, 255, 0, 0);

   /* make the off-screen buffer that everything will be drawn onto */
   buffer = create_bitmap_12(GFXW, GFXH);

   /* convert the text message into the coordinates of the vector balls */
   points = make_points(&numpoints, MESSAGE_STR);

   /* create the background picture */
   rgbpic = create_bitmap_12(GFXW, GFXH);

   rgb_scales_12(rgbpic, 0, 0, GFXW/2, GFXH/2);

   /* copy the background into the buffer */
   blit(rgbpic, buffer, 0, 0, 0, 0, GFXW, GFXH);

   xangle = yangle = zangle = 0;

   /* put a message in the top-left corner */
   textprintf_ex(rgbpic, font, 3, 3, makecol12(255, 255, 255), -1,
		 "%ix%i 12-bit colour on an 8-bit card", GFXW, GFXH);
   textprintf_ex(rgbpic, font, 3, 13, makecol12(255, 255, 255), -1,
		 "(3840 colours at once!)");

   while (!keypressed()) {
      /* first, draw some vector balls moving in a circle round the edge */
      for (x=0; x<itofix(256); x += itofix(32)) {
	 masked_blit(bigball, buffer, 0, 0,
		     fixtoi(150 * fixcos(xangle+x)) + GFXW/2 - BALLW/2,
		     fixtoi(200 * fixsin(xangle+x)) + GFXH/2 - BALLH/2,
		     BIGBALLW, BIGBALLH);
      }

      /* rotate the vector balls */

      get_rotation_matrix(&m, xangle, yangle, zangle);

      for (thispoint=0; thispoint<numpoints; thispoint++) {
	 apply_matrix(&m, points[thispoint].x,
			  points[thispoint].y,
			  0,
			  &newx, &newy, &newz);

	 masked_blit(ball[points[thispoint].c], buffer, 0,0,
		     fixtoi(newx) + GFXW/2,
		     fixtoi(newy) + GFXH/2, BALLW, BALLH);
      }

      /* then blur the buffer so it fades into the background picture */
      blur_12(buffer, rgbpic);

      /* finally copy everything to the screen */
      blit(buffer, screen, 0, 0, 0, 0, GFXW,GFXH);

      /* rotate it a bit more */
      xangle += itofix(1);
      yangle += itofix(1);
      zangle += itofix(1);

      /* slow down the animation for modern machines */
      rest(1);
   }

   clear_keybuf();

   /* clean it all up */
   for (x=0; x<4; x++)
      destroy_bitmap(ball[x]);

   destroy_bitmap(bigball);
   free(points);

   destroy_bitmap(rgbpic);
   destroy_bitmap(buffer);
   
   screen->vtable = orig_vtable;
   fade_out(4);

   return 0;
}

END_OF_MAIN()
