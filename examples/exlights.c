/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program shows one way to implement colored lighting effects
 *    in a hicolor video mode. Warning: it is not for the faint of heart!
 *    This is by no means the simplest or easiest to understand method,
 *    I just thought it was a cool concept that would be worth 
 *    demonstrating.
 *
 *    The basic approach is to select a 15 or 16 bit screen mode, but
 *    then draw onto 24 bit memory bitmaps. Since we only need the bottom
 *    5 bits of each 8 bit color in order to store 15 bit data within a
 *    24 bit location, we can fit a light level into the top 3 bits.
 *    The tricky bit is that these aren't actually 24 bit images at all:
 *    they are implemented as 8 bit memory bitmaps, and we just store the
 *    red level in one pixel, green in the next, and blue in the next,
 *    making the total image be three times wider than we really wanted.
 *    This allows us to use all the normal 256 color graphics routines
 *    for drawing onto our memory surfaces, most importantly the lookup
 *    table translucency, which can be used to combine the low 5 bits
 *    of color and the top 3 bits of light in a single drawing operation.
 *    Some trickery is needed to load 24 bit data into this fake 8 bit
 *    format, and of course it needs a custom routine to convert the
 *    resulting image while copying it across to the hardware screen.
 *
 *    This program chugs slightly on my p133, but not significantly 
 *    worse than any double buffering in what amounts to a 1920x640, 
 *    256 color resolution. The light blending doesn't seem to slow 
 *    it down too badly, so I think this technique would be quite usable 
 *    on faster machines and in lower resolution hicolor modes. The 
 *    biggest problem is that although you keep the full 15 bit color 
 *    resolution, you only get 3 bits of light, ie. 8 light levels. 
 *    You can do some nice colored light patches, but smooth gradients 
 *    aren't going to work too well :-)
 */


#include <allegro.h>



/* double buffer bitmap */
BITMAP *buffer;


/* the two moving bitmap images */
BITMAP *image1;
BITMAP *image2;


/* the light map, drawn wherever the mouse pointer is */
BITMAP *lightmap;


/* give images some light of their own. Make this zero for total black */
#define AMBIENT_LIGHT      0x20



/* converts a bitmap from the normal Allegro format into our magic layout */
BITMAP *get_magic_bitmap_format(BITMAP *orig, PALETTE pal)
{
   BITMAP *bmp;
   int c, r, g, b;
   int x, y; 
   int bpp; 

   /* create an 8 bpp image, three times as wide as the original */
   bmp = create_bitmap_ex(8, orig->w*3, orig->h);

   /* store info about the original bitmap format */
   bpp = bitmap_color_depth(orig);
   select_palette(pal);

   /* convert the data */
   for (y=0; y<orig->h; y++) {
      for (x=0; x<orig->w; x++) {
	 c = getpixel(orig, x, y);

	 r = getr_depth(bpp, c);
	 g = getg_depth(bpp, c);
	 b = getb_depth(bpp, c);

	 bmp->line[y][x*3] = r*31/255 | AMBIENT_LIGHT;
	 bmp->line[y][x*3+1] = g*31/255 | AMBIENT_LIGHT;
	 bmp->line[y][x*3+2] = b*31/255 | AMBIENT_LIGHT;
      }
   }

   /* return our new, magic format version of the image */
   return bmp;
}



/* creates the light graphic for the mouse pointer, in our magic format */
BITMAP *create_light_graphic(void)
{
   BITMAP *bmp;
   int x, y;
   int dx, dy;
   int dist, dir;
   int r, g, b;

   bmp = create_bitmap_ex(8, 256*3, 256);

   /* draw the light (a circular color gradient) */
   for (y=0; y<256; y++) {
      for (x=0; x<256; x++) {
	 dx = x-128;
	 dy = y-128;

	 dist = fixtoi(fixsqrt(itofix(CLAMP(-32768, dx*dx+dy*dy, 32767))));

	 dir = fixtoi(fixatan2(itofix(dy), itofix(dx)) + itofix(128));

	 hsv_to_rgb(dir*360.0/256.0, CLAMP(0, dist/128.0, 1), 1, &r, &g, &b);

	 r = r * (128-dist) / 1024;
	 g = g * (128-dist) / 1024;
	 b = b * (128-dist) / 1024;

	 bmp->line[y][x*3] = CLAMP(0, r, 7) << 5;
	 bmp->line[y][x*3+1] = CLAMP(0, g, 7) << 5;
	 bmp->line[y][x*3+2] = CLAMP(0, b, 7) << 5;
      }
   }

   /* draw some solid pixels as well (a cross in the middle ) */
   #define magic_putpix(x, y, r, g, b)       \
   {                                         \
      bmp->line[y][(x)*3] &= 0xE0;           \
      bmp->line[y][(x)*3+1] &= 0xE0;         \
      bmp->line[y][(x)*3+2] &= 0xE0;         \
					     \
      bmp->line[y][(x)*3] |= r;              \
      bmp->line[y][(x)*3+1] |= g;            \
      bmp->line[y][(x)*3+2] |= b;            \
   }

   for (x=-15; x<=15; x++) {
      for (y=-2; y<=2; y++) {
	 magic_putpix(128+x, 128+y, x+15, 15-x, 0);
	 magic_putpix(128+y, 128+x, x+15, x+15, 15-x);
      }
   }

   return bmp;
}



#ifdef ALLEGRO_LITTLE_ENDIAN

/* lookup tables for speeding up the color conversion */
unsigned short rgtable[65536];
unsigned long brtable[65536];
unsigned short gbtable[65536];



/* builds some helper tables for doing color conversions */
void generate_conversion_tables(void)
{
   int r, g, b;
   int cr, cg, cb;

   /* this table combines a 16 bit r+g value into a screen pixel */
   for (r=0; r<256; r++) {
      cr = (r&31) * (r>>5) * 255/217;
      for (g=0; g<256; g++) {
	 cg = (g&31) * (g>>5) * 255/217;
	 rgtable[r+g*256] = makecol(cr, cg, 0);
      }
   }

   /* this table combines a 16 bit b+r value into a screen pixel */
   for (b=0; b<256; b++) {
      cb = (b&31) * (b>>5) * 255/217;
      for (r=0; r<256; r++) {
	 cr = (r&31) * (r>>5) * 255/217;
	 brtable[b+r*256] = makecol(0, 0, cb) | (makecol(cr, 0, 0) << 16);
      }
   }

   /* this table combines a 16 bit g+b value into a screen pixel */
   for (g=0; g<256; g++) {
      cg = (g&31) * (g>>5) * 255/217;
      for (b=0; b<256; b++) {
	 cb = (b&31) * (b>>5) * 255/217;
	 gbtable[g+b*256] = makecol(0, cg, cb);
      }
   }
}



/* copies from our magic format data onto a normal Allegro screen bitmap */
void blit_magic_format_to_screen(BITMAP *bmp)
{
   uintptr_t addr;
   uint32_t *data;
   unsigned long in1, in2, in3;
   unsigned long out1, out2;
   int x, y;

   /* Warning: this function contains some rather hairy optimisations :-)
    * The fastest way to copy large amounts of data is in aligned 32 bit 
    * chunks. If we expand it out to process 4 pixels per cycle, we can 
    * do this by reading 12 bytes (4 pixels) from the 24 bit source data, 
    * and writing 8 bytes (4 pixels) to the 16 bit destination. Then the 
    * only problem is how to convert our 12 bytes of data into a suitable 
    * 8 byte format, once we've got it loaded into registers. This is done 
    * by some lookup tables, which are arranged so they can process 2 color 
    * components in a single lookup, and allow me to precalculate the 
    * makecol() operation.
    *
    * Warning #2: this code assumes little-endianess.
    *
    * Here is a (rather confusing) attempt to diagram the logic of the
    * lookup table lighting conversion from 24 to 16 bit format in little-
    * endian format:
    *
    *
    *  inputs: |     (dword 1)     |     (dword 2)     |     (dword 3)     |
    *  pixels: |   (pixel1)   |   (pixel2)   |   (pixel3)   |   (pixel4)   |
    *  bytes:  | r1   g1   b1   r2   g2   b2   r3   g3   b3   r4   g4   b4 |
    *          |    |         |         |         |         |         |    |
    *  lookup: | rgtable   brtable   gbtable   rgtable   brtable   gbtable |
    *          |    |         |         |         |         |         |    |
    *  pixels: |   (pixel1)   |   (pixel2)   |   (pixel3)   |   (pixel4)   |
    *  outputs |          (dword 1)          |          (dword 2)          |
    *
    *
    * For reference, here is the original, non-optimised but much easier
    * to understand version of this routine:
    *
    *    _farsetsel(screen->seg);
    * 
    *    for (y=0; y<SCREEN_H; y++) {
    *       addr = bmp_write_line(screen, y);
    * 
    *       for (x=0; x<SCREEN_W; x++) {
    *          r = bmp->line[y][x*3];
    *          g = bmp->line[y][x*3+1];
    *          b = bmp->line[y][x*3+2];
    * 
    *          r = (r&31) * (r>>5) * 255/217;
    *          g = (g&31) * (g>>5) * 255/217;
    *          b = (b&31) * (b>>5) * 255/217;
    * 
    *          _farnspokew(addr, makecol(r, g, b));
    * 
    *          addr += 2;
    *       }
    *    }
    */

   bmp_select(screen);

   for (y=0; y<SCREEN_H; y++) {
      addr = bmp_write_line(screen, y);
      data = (uint32_t *)bmp->line[y];

      for (x=0; x<SCREEN_W/4; x++) {
	 in1 = *(data++);
	 in2 = *(data++);
	 in3 = *(data++);

	 /* trust me, this does make sense, really :-) */
	 out1 = rgtable[in1&0xFFFF] | 
		brtable[in1>>16] | 
		(gbtable[in2&0xFFFF] << 16);

	 out2 = rgtable[in2>>16] | 
		brtable[in3&0xFFFF] | 
		(gbtable[in3>>16] << 16);

	 bmp_write32(addr, out1);
	 bmp_write32(addr+4, out2);

	 addr += 8;
      }
   }

   bmp_unwrite_line(screen);
}


#elif defined ALLEGRO_BIG_ENDIAN


/* lookup tables for speeding up the color conversion */
unsigned short bgtable[65536];
unsigned long rbtable[65536];
unsigned short grtable[65536];



/* builds some helper tables for doing color conversions */
void generate_conversion_tables(void)
{
   int r, g, b;
   int cr, cg, cb;

   /* this table combines a 16 bit b+g value into a screen pixel */
   for (b=0; b<256; b++) {
      cb = (b&31) * (b>>5) * 255/217;
      for (g=0; g<256; g++) {
	 cg = (g&31) * (g>>5) * 255/217;
	 bgtable[b+g*256] = makecol(0, cg, cb);
      }
   }

   /* this table combines a 16 bit g+r value into a screen pixel */
   for (r=0; r<256; r++) {
      cr = (r&31) * (r>>5) * 255/217;
      for (b=0; b<256; b++) {
	 cb = (b&31) * (b>>5) * 255/217;
	 rbtable[r+b*256] = makecol(cr, 0, 0) | (makecol(0, 0, cb) << 16);
      }
   }

   /* this table combines a 16 bit r+r value into a screen pixel */
   for (g=0; g<256; g++) {
      cg = (g&31) * (g>>5) * 255/217;
      for (r=0; r<256; r++) {
	 cr = (r&31) * (r>>5) * 255/217;
	 grtable[g+r*256] = makecol(cr, cg, 0);
      }
   }
}



/* copies from our magic format data onto a normal Allegro screen bitmap */
void blit_magic_format_to_screen(BITMAP *bmp)
{
   uintptr_t addr;
   uint32_t *data;
   unsigned long in1, in2, in3, temp1, temp2, temp3;
   unsigned long out1, out2;
   int x, y;

   /* Warning: this is the big-endian version of the routine above. We need
    * a different lookup tables arrangement and we also need to shuffle the
    * bytes order before doing the lookup.
    *
    * Here is a (rather confusing) attempt to diagram the logic of the
    * lookup table lighting conversion from 24 to 16 bit format in big-
    * endian format:
    *
    *
    *  inputs: |     (dword 1)     |     (dword 2)     |     (dword 3)     |
    *  pixels: |   (pixel1)   |   (pixel2)   |   (pixel3)   |   (pixel4)   |
    *  bytes:  | r2   b1   g1   r1   g3   r3   b2   g2   b4   g4   r4   b3 |
    *  bytes2: | b2   g2   r2   b1   g1   r1   b4   g4   r4   b3   g3   r3 |
    *          |    |         |         |         |         |         |    |
    *  lookup: | bgtable   rbtable   grtable   bgtable   rbtable   grtable |
    *          |    |         |         |         |         |         |    |
    *  pixels: |   (pixel2)   |   (pixel1)   |   (pixel4)   |   (pixel3)   |
    *  outputs |          (dword 1)          |          (dword 2)          |
    */

   bmp_select(screen);

   for (y=0; y<SCREEN_H; y++) {
      addr = bmp_write_line(screen, y);
      data = (uint32_t *)bmp->line[y];

      for (x=0; x<SCREEN_W/4; x++) {
	 in1 = *(data++);
	 in2 = *(data++);
	 in3 = *(data++);

	 /* trust me, this does make sense, really :-) */
	 temp1 = (in1 << 16) | (in2 >> 16);
	 temp2 = (in1 >> 16) | (in3 << 16);
	 temp3 = (in3 >> 16) | (in2 << 16);

	 out1 = bgtable[temp1&0xFFFF] |
	        rbtable[temp1>>16] |
		(grtable[temp2&0xFFFF] << 16);

	 out2 = bgtable[temp2>>16] |
	        rbtable[temp3&0xFFFF] |
		(grtable[temp3>>16] << 16);

	 bmp_write32(addr, out1);
	 bmp_write32(addr+4, out2);

	 addr += 8;
      }
   }

   bmp_unwrite_line(screen);
}


#elif !defined SCAN_DEPEND
#error Unknown endianess!
#endif



int main(int argc, char *argv[])
{
   BITMAP *tmp;
   PALETTE pal;
   char buf[256];
   int x, y, xc, yc, xl, yl, c, l;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_timer();
   install_mouse();
   set_color_conversion(COLORCONV_NONE);

   /* set a 15 or 16 bpp video mode */
   set_color_depth(16);
   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      set_color_depth(15);
      if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Error setting a 15 or 16 bpp 640x480 video mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   /* create the double buffer, 8 bpp and three times as wide as the screen */
   buffer = create_bitmap_ex(8, SCREEN_W*3, SCREEN_H);

   /* load the first picture */
   replace_filename(buf, argv[0], "allegro.pcx", sizeof(buf));
   tmp = load_bitmap(buf, pal);
   if (!tmp) {
      destroy_bitmap(buffer);
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s!\n", buf);
      return 1;
   }

   /* convert it into our special format */
   image1 = get_magic_bitmap_format(tmp, pal);
   destroy_bitmap(tmp);

   /* load the second picture */
   replace_filename(buf, argv[0], "mysha.pcx", sizeof(buf));
   tmp = load_bitmap(buf, pal);
   if (!tmp) {
      destroy_bitmap(buffer);
      destroy_bitmap(image1);
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error reading %s!\n", buf);
      return 1;
   }

   /* convert it into our special format */
   image2 = get_magic_bitmap_format(tmp, pal);
   destroy_bitmap(tmp);

   /* create the light map image */
   lightmap = create_light_graphic();

   /* create our custom color blending map, which does translucency in the
    * bottom five bits and adds the light levels in the top three bits.
    * This version just does a 50% translucency if you are drawing sprites
    * with it, but you could easily make other color maps for different 
    * alpha levels, or for doing additive color, which can work happily
    * in parallel with the light blending.
    */
   color_map = malloc(sizeof(COLOR_MAP));

   for (x=0; x<256; x++) {
      for (y=0; y<256; y++) {
	 xc = x&31;
	 yc = y&31;

	 xl = x>>5;
	 yl = y>>5;

	 if (xc)
	    c = (xc+yc)/2;
	 else
	    c = yc;

	 l = xl+yl;
	 if (l > 7)
	    l = 7;

	 color_map->data[x][y] = c | (l<<5);
      }
   }

   /* generate tables for converting pixels from magic->screen format */
   generate_conversion_tables();

   /* display the animation */
   while (!keypressed()) {
      poll_mouse();

      clear_bitmap(buffer);

      /* we can draw the graphics using normal calls, just as if they were
       * regular 256 color images. Everything is just three times as wide 
       * as it would usually be, so we need to make sure that we only ever
       * draw to an x coordinate that is a multiple of three (otherwise
       * all the colors would get shifted out of phase with each other).
       */
      blit(image1, buffer, 0, 0, 0, retrace_count%(SCREEN_H+image1->h)-image1->h, image1->w, image1->h);
      blit(image2, buffer, 0, 0, buffer->w-image2->w, buffer->h-(retrace_count*2/3)%(SCREEN_H+image2->h), image2->w, image2->h);

      /* now we overlay translucent graphics and lights. Having set up a
       * suitable color blending table, these can be done at the same time,
       * either drawing a series of images some of which are translucent
       * sprites and some of which are light maps, or if we want, we can
       * just draw a single bitmap containing both color and light data 
       * with a single call, like this! You could also use graphics 
       * primitives like circlefill() to draw the lights, as long as you
       * do it in a translucent mode.
       */
      draw_trans_sprite(buffer, lightmap, (mouse_x-lightmap->w/6)*3, mouse_y-lightmap->h/2);

      /* this function is the key to the whole thing, converting from our
       * weird 5+3 interleaved format into a regular hicolor pixel that
       * can be displayed on your monitor.
       */
      blit_magic_format_to_screen(buffer);
   }

   clear_keybuf();

   destroy_bitmap(buffer);
   destroy_bitmap(image1);
   destroy_bitmap(image2);
   destroy_bitmap(lightmap);

   free(color_map);

   return 0;
}

END_OF_MAIN()
