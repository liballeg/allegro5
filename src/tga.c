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
 *      TGA reader.
 *
 *      By Tim Gunn.
 *
 *      RLE support added by Michal Mertl and Salvador Eduardo Tropea.
 * 
 *	Palette reading improved by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* rle_tga_read:
 *  Helper for reading 256 color RLE data from TGA files.
 */
static void rle_tga_read(unsigned char *b, int w, PACKFILE *f)
{
   unsigned char value;
   int count;
   int c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 count = (count & 0x7F) + 1;
	 c += count;
	 value = pack_getc(f);
	 while (count--)
	    *(b++) = value;
      }
      else {
	 count++;
	 c += count;
	 pack_fread(b, count, f);
	 b += count;
      }
   } while (c < w);
}



/* rle_tga_read32:
 *  Helper for reading 32 bit RLE data from TGA files.
 */
static void rle_tga_read32(unsigned char *b, int w, PACKFILE *f)
{
   unsigned char value[4];
   int count;
   int c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 count = (count & 0x7F) + 1;
	 c += count;
	 pack_fread(value, 4, f);
	 while (count--) {
	    b[_rgb_a_shift_32/8] = value[3];
	    b[_rgb_r_shift_32/8] = value[2];
	    b[_rgb_g_shift_32/8] = value[1];
	    b[_rgb_b_shift_32/8] = value[0];
	    b += 4;
	 }
      }
      else {
	 count++;
	 c += count;
	 while (count--) {
	    pack_fread(value, 4, f);
	    b[_rgb_a_shift_32/8] = value[3];
	    b[_rgb_r_shift_32/8] = value[2];
	    b[_rgb_g_shift_32/8] = value[1];
	    b[_rgb_b_shift_32/8] = value[0];
	    b += 4;
	 }
      }
   } while (c < w);
}



/* rle_tga_read24:
 *  Helper for reading 24 bit RLE data from TGA files.
 */
static void rle_tga_read24(unsigned char *b, int w, PACKFILE *f)
{
   unsigned char value[4];
   int count;
   int c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 count = (count & 0x7F) + 1;
	 c += count;
	 pack_fread(value, 3, f);
	 while (count--) {
	    b[_rgb_r_shift_24/8] = value[2];
	    b[_rgb_g_shift_24/8] = value[1];
	    b[_rgb_b_shift_24/8] = value[0];
	    b += 3;
	 }
      }
      else {
	 count++;
	 c += count;
	 while (count--) {
	    pack_fread(value, 3, f);
	    b[_rgb_r_shift_24/8] = value[2];
	    b[_rgb_g_shift_24/8] = value[1];
	    b[_rgb_b_shift_24/8] = value[0];
	    b += 3;
	 }
      }
   } while (c < w);
}



/* rle_tga_read16:
 *  Helper for reading 16 bit RLE data from TGA files.
 */
static void rle_tga_read16(unsigned short *b, int w, PACKFILE *f)
{
   unsigned int value;
   unsigned short color;
   int count;
   int c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 count = (count & 0x7F) + 1;
	 c += count;
	 value = pack_igetw(f);
	 color = ((((value >> 10) & 0x1F) << _rgb_r_shift_15) |
		  (((value >> 5) & 0x1F) << _rgb_g_shift_15) |
		  ((value & 0x1F) << _rgb_b_shift_15));
	 while (count--)
	    *(b++) = color;
      }
      else {
	 count++;
	 c += count;
	 while (count--) {
	    value = pack_igetw(f);
	    color = ((((value >> 10) & 0x1F) << _rgb_r_shift_15) |
		     (((value >> 5) & 0x1F) << _rgb_g_shift_15) |
		     ((value & 0x1F) << _rgb_b_shift_15));
	    *(b++) = color;
	 }
      }
   } while (c < w);
}



/* load_tga:
 *  Loads a 256 color or 24 bit uncompressed TGA file, returning a bitmap
 *  structure and storing the palette data in the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
BITMAP *load_tga(AL_CONST char *filename, RGB *pal)
{
   unsigned char image_id[256], image_palette[256][3], rgb[4];
   unsigned char id_length, palette_type, image_type, palette_entry_size;
   unsigned char bpp, descriptor_bits;
   short unsigned int first_color, palette_colors;
   short unsigned int left, top, image_width, image_height;
   unsigned int c, i, x, y, yc;
   unsigned short *s;
   int dest_depth;
   int compressed;
   PACKFILE *f;
   BITMAP *bmp;
   PALETTE tmppal;

   if (!pal)
      pal = tmppal;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   id_length = pack_getc(f);
   palette_type = pack_getc(f);
   image_type = pack_getc(f);
   first_color = pack_igetw(f);
   palette_colors  = pack_igetw(f);
   palette_entry_size = pack_getc(f);
   left = pack_igetw(f);
   top = pack_igetw(f);
   image_width = pack_igetw(f);
   image_height = pack_igetw(f);
   bpp = pack_getc(f);
   descriptor_bits = pack_getc(f);

   pack_fread(image_id, id_length, f);

   for (i = 0; i < palette_colors; i++) {

      switch (palette_entry_size) {

	 case 16: 
	    c = pack_igetw(f);
	    image_palette[i][0] = (c & 0x1F) << 3;
	    image_palette[i][1] = ((c >> 5) & 0x1F) << 3;
	    image_palette[i][2] = ((c >> 10) & 0x1F) << 3;
	    break;

	 case 24:
	 case 32:
	    image_palette[i][0] = pack_getc(f);
	    image_palette[i][1] = pack_getc(f);
	    image_palette[i][2] = pack_getc(f);
	    if (palette_entry_size == 32)
	       pack_getc(f);
	    break;
      }
   }

   /* Image type:
    *    0 = no image data
    *    1 = uncompressed color mapped
    *    2 = uncompressed true color
    *    3 = grayscale
    *    9 = RLE color mapped
    *   10 = RLE true color
    *   11 = RLE grayscale
    */
   compressed = (image_type & 8);
   image_type &= 7;

   if ((image_type < 1) || (image_type > 3)) {
      pack_fclose(f);
      return NULL;
   }

   switch (image_type) {

      case 1:
	 /* paletted image */
	 if ((palette_type != 1) || (bpp != 8)) {
	    pack_fclose(f);
	    return NULL;
	 }

	 for(i=0; i<palette_colors; i++) {
	     pal[i].r = image_palette[i][2] >> 2;
	     pal[i].g = image_palette[i][1] >> 2;
	     pal[i].b = image_palette[i][0] >> 2;
	 }

	 dest_depth = _color_load_depth(8, FALSE);
	 break;

      case 2:
	 /* truecolor image */
	 if ((palette_type == 0) && ((bpp == 15) || (bpp == 16))) {
	    bpp = 15;
	    generate_332_palette(pal);
	    dest_depth = _color_load_depth(15, FALSE);
	 }
	 else if ((palette_type == 0) && ((bpp == 24) || (bpp == 32))) {
	    generate_332_palette(pal);
	    dest_depth = _color_load_depth(bpp, (bpp == 32));
	 }
	 else {
	    pack_fclose(f);
	    return NULL;
	 }
	 break;

      case 3:
	 /* grayscale image */
	 if ((palette_type != 0) || (bpp != 8)) {
	    pack_fclose(f);
	    return NULL;
	 }

	 for (i=0; i<256; i++) {
	     pal[i].r = i>>2;
	     pal[i].g = i>>2;
	     pal[i].b = i>>2;
	 }

	 dest_depth = _color_depth;
	 break;

      default:
	 pack_fclose(f);
	 return NULL;
   }

   bmp = create_bitmap_ex(bpp, image_width, image_height);
   if (!bmp) {
      pack_fclose(f);
      return NULL;
   }

   for (y=image_height; y; y--) {
      yc = (descriptor_bits & 0x20) ? image_height-y : y-1;

      switch (image_type) {

	 case 1:
	 case 3:
	    if (compressed)
	       rle_tga_read(bmp->line[yc], image_width, f);
	    else
	       pack_fread(bmp->line[yc], image_width, f);
	    break;

	 case 2:
	    if (bpp == 32) {
	       if (compressed) {
		  rle_tga_read32(bmp->line[yc], image_width, f);
	       }
	       else {
		  for (x=0; x<image_width; x++) {
		     pack_fread(rgb, 4, f);
		     bmp->line[yc][x*4+_rgb_a_shift_32/8] = rgb[3];
		     bmp->line[yc][x*4+_rgb_r_shift_32/8] = rgb[2];
		     bmp->line[yc][x*4+_rgb_g_shift_32/8] = rgb[1];
		     bmp->line[yc][x*4+_rgb_b_shift_32/8] = rgb[0];
		  }
	       }
	    }
	    else if (bpp == 24) {
	       if (compressed) {
		  rle_tga_read24(bmp->line[yc], image_width, f);
	       }
	       else {
		  for (x=0; x<image_width; x++) {
		     pack_fread(rgb, 3, f);
		     bmp->line[yc][x*3+_rgb_r_shift_24/8] = rgb[2];
		     bmp->line[yc][x*3+_rgb_g_shift_24/8] = rgb[1];
		     bmp->line[yc][x*3+_rgb_b_shift_24/8] = rgb[0];
		  }
	       }
	    }
	    else {
	       if (compressed) {
		  rle_tga_read16((unsigned short *)bmp->line[yc], image_width, f);
	       }
	       else {
		  s = (unsigned short *)bmp->line[yc];
		  for (x=0; x<image_width; x++) {
		     c = pack_igetw(f);
		     s[x] = ((((c >> 10) & 0x1F) << _rgb_r_shift_15) |
			     (((c >> 5) & 0x1F) << _rgb_g_shift_15) |
			     ((c & 0x1F) << _rgb_b_shift_15));
		  }
	       }
	    }
	    break;
      }
   }

   pack_fclose(f);

   if (*allegro_errno) {
      destroy_bitmap(bmp);
      return NULL;
   }

   if (dest_depth != bpp)
      bmp = _fixup_loaded_bitmap(bmp, pal, dest_depth);

   return bmp;
}



/* save_tga:
 *  Writes a bitmap into a TGA file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
int save_tga(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal)
{
   unsigned char image_palette[256][3];
   int x, y, c, r, g, b;
   int depth;
   PACKFILE *f;
   PALETTE tmppal;

   if (!pal) {
      get_palette(tmppal);
      pal = tmppal;
   }

   depth = bitmap_color_depth(bmp);

   if (depth == 15)
      depth = 16;

   f = pack_fopen(filename, F_WRITE);
   if (!f)
      return *allegro_errno;

   pack_putc(0, f);                          /* id length (no id saved) */
   pack_putc((depth == 8) ? 1 : 0, f);       /* palette type */
   pack_putc((depth == 8) ? 1 : 2, f);       /* image type */
   pack_iputw(0, f);                         /* first colour */
   pack_iputw((depth == 8) ? 256 : 0, f);    /* number of colours */
   pack_putc((depth == 8) ? 24 : 0, f);      /* palette entry size */
   pack_iputw(0, f);                         /* left */
   pack_iputw(0, f);                         /* top */
   pack_iputw(bmp->w, f);                    /* width */
   pack_iputw(bmp->h, f);                    /* height */
   pack_putc(depth, f);                      /* bits per pixel */
   pack_putc(0, f);                          /* descriptor (bottom to top) */

   if (depth == 8) {
      for (y=0; y<256; y++) {
	 image_palette[y][2] = _rgb_scale_6[pal[y].r];
	 image_palette[y][1] = _rgb_scale_6[pal[y].g];
	 image_palette[y][0] = _rgb_scale_6[pal[y].b];
      }

      pack_fwrite(image_palette, 768, f);
   }

   switch (bitmap_color_depth(bmp)) {

      #ifdef ALLEGRO_COLOR8

	 case 8:
	    for (y=bmp->h; y; y--)
	       for (x=0; x<bmp->w; x++)
		  pack_putc(getpixel(bmp, x, y-1), f);
	    break;

      #endif

      #ifdef ALLEGRO_COLOR16

	 case 15:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  r = getr15(c);
		  g = getg15(c);
		  b = getb15(c);
		  c = ((r<<7)&0x7C00) | ((g<<2)&0x3E0) | ((b>>3)&0x1F);
		  pack_iputw(c, f);
	       }
	    }
	    break;

	 case 16:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  r = getr16(c);
		  g = getg16(c);
		  b = getb16(c);
		  c = ((r<<7)&0x7C00) | ((g<<2)&0x3E0) | ((b>>3)&0x1F);
		  pack_iputw(c, f);
	       }
	    }
	    break;

      #endif

      #ifdef ALLEGRO_COLOR24

	 case 24:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  pack_putc(getb24(c), f);
		  pack_putc(getg24(c), f);
		  pack_putc(getr24(c), f);
	       }
	    }
	    break;

      #endif

      #ifdef ALLEGRO_COLOR32

	 case 32:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  pack_putc(getb32(c), f);
		  pack_putc(getg32(c), f);
		  pack_putc(getr32(c), f);
		  pack_putc(geta32(c), f);
	       }
	    }
	    break;

      #endif
   }

   pack_fclose(f);
   return *allegro_errno;
}


