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
 *      Datafile reading routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



static void unload_midi(MIDI *m);



/* hack to let the grabber prevent compiled sprites from being compiled */
int _compile_sprites = TRUE;

static void (*datafile_callback)(DATAFILE *) = NULL;



/* load_st_data:
 *  I'm not using this format any more, but files created with the old
 *  version of Allegro might have some bitmaps stored like this. It is 
 *  the 4bpp planar system used by the Atari ST low resolution mode.
 */
static void load_st_data(unsigned char *pos, long size, PACKFILE *f)
{
   int c;
   unsigned int d1, d2, d3, d4;

   size /= 8;           /* number of 4 word planes to read */

   while (size) {
      d1 = pack_mgetw(f);
      d2 = pack_mgetw(f);
      d3 = pack_mgetw(f);
      d4 = pack_mgetw(f);
      for (c=0; c<16; c++) {
	 *(pos++) = ((d1 & 0x8000) >> 15) + ((d2 & 0x8000) >> 14) +
		    ((d3 & 0x8000) >> 13) + ((d4 & 0x8000) >> 12);
	 d1 <<= 1;
	 d2 <<= 1;
	 d3 <<= 1;
	 d4 <<= 1; 
      }
      size--;
   }
}



/* read_block:
 *  Reads a block of size bytes from a file, allocating memory to store it.
 */
static void *read_block(PACKFILE *f, int size, int alloc_size)
{
   void *p;

   p = malloc(MAX(size, alloc_size));
   if (!p) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   pack_fread(p, size, f);

   if (pack_ferror(f)) {
      free(p);
      return NULL;
   }

   return p;
}



/* read_bitmap:
 *  Reads a bitmap from a file, allocating memory to store it.
 */
static BITMAP *read_bitmap(PACKFILE *f, int bits, int allowconv)
{
   int x, y, w, h, c, r, g, b, a;
   int destbits, dither8, prev_drawmode, rgba;
   unsigned short *p16;
   unsigned long *p32;
   BITMAP *bmp;

   if (bits < 0) {
      bits = -bits;
      rgba = TRUE;
   }
   else
      rgba = FALSE;

   if (allowconv)
      destbits = _color_load_depth(bits, rgba);
   else
      destbits = 8;

   if ((destbits == 8) && (bits > 8) && (_color_conv & COLORCONV_DITHER_PAL)) {
      destbits = bits;
      dither8 = TRUE;
   }
   else
      dither8 = FALSE;

   w = pack_mgetw(f);
   h = pack_mgetw(f);

   bmp = create_bitmap_ex(destbits, w, h);
   if (!bmp) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   prev_drawmode = _drawing_mode;
   _drawing_mode = DRAW_MODE_SOLID;

   switch (bits) {

      case 4:
	 /* old format ST data */
	 load_st_data(bmp->dat, w*h/2, f);
	 break;


      case 8:
	 /* 256 color bitmap */
	 if (destbits == 8) {
	    pack_fread(bmp->dat, w*h, f);
	 }
	 else {
	    /* expand 256 colors into truecolor */
	    for (y=0; y<h; y++) {
	       for (x=0; x<w; x++) {
		  c = pack_getc(f);
		  r = getr8(c);
		  g = getg8(c);
		  b = getb8(c);
		  c = makecol_depth(destbits, r, g, b);
		  putpixel(bmp, x, y, c);
	       }
	    }
	 }
	 break;


      case 15:
      case 16:
	 /* hicolor */
	 switch (destbits) {

	 #ifdef ALLEGRO_COLOR8

	    case 8:
	       /* reduce hicolor to 256 colors */
	       for (y=0; y<h; y++) {
		  for (x=0; x<w; x++) {
		     c = pack_igetw(f);
		     r = _rgb_scale_5[(c >> 11) & 0x1F];
		     g = _rgb_scale_6[(c >> 5) & 0x3F];
		     b = _rgb_scale_5[c & 0x1F];
		     bmp->line[y][x] = makecol8(r, g, b);
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	       /* load hicolor to a 15 bit hicolor bitmap */
	       for (y=0; y<h; y++) {
		  p16 = (unsigned short *)bmp->line[y];

		  for (x=0; x<w; x++) {
		     c = pack_igetw(f);
		     r = _rgb_scale_5[(c >> 11) & 0x1F];
		     g = _rgb_scale_6[(c >> 5) & 0x3F];
		     b = _rgb_scale_5[c & 0x1F];
		     p16[x] = makecol15(r, g, b);
		  }
	       }
	       break;

	    case 16:
	       /* load hicolor to a 16 bit hicolor bitmap */
	       for (y=0; y<h; y++) {
		  p16 = (unsigned short *)bmp->line[y];

		  for (x=0; x<w; x++) {
		     c = pack_igetw(f);
		     r = _rgb_scale_5[(c >> 11) & 0x1F];
		     g = _rgb_scale_6[(c >> 5) & 0x3F];
		     b = _rgb_scale_5[c & 0x1F];
		     p16[x] = makecol16(r, g, b);
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       /* load hicolor to a 24 bit truecolor bitmap */
	       for (y=0; y<h; y++) {
		  for (x=0; x<w; x++) {
		     c = pack_igetw(f);
		     r = _rgb_scale_5[(c >> 11) & 0x1F];
		     g = _rgb_scale_6[(c >> 5) & 0x3F];
		     b = _rgb_scale_5[c & 0x1F];
		     bmp->line[y][x*3+_rgb_r_shift_24/8] = r;
		     bmp->line[y][x*3+_rgb_g_shift_24/8] = g;
		     bmp->line[y][x*3+_rgb_b_shift_24/8] = b;
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       /* load hicolor to a 32 bit truecolor bitmap */
	       for (y=0; y<h; y++) {
		  p32 = (unsigned long *)bmp->line[y];

		  for (x=0; x<w; x++) {
		     c = pack_igetw(f);
		     r = _rgb_scale_5[(c >> 11) & 0x1F];
		     g = _rgb_scale_6[(c >> 5) & 0x3F];
		     b = _rgb_scale_5[c & 0x1F];
		     p32[x] = makecol32(r, g, b);
		  }
	       }
	       break;

	 #endif

	 }
	 break;


      case 24:
      case 32:
	 /* truecolor */
	 switch (destbits) {

	 #ifdef ALLEGRO_COLOR8

	    case 8:
	       /* reduce truecolor to 256 colors */
	       for (y=0; y<h; y++) {
		  for (x=0; x<w; x++) {
		     r = pack_getc(f);
		     g = pack_getc(f);
		     b = pack_getc(f);
		     if (rgba)
			pack_getc(f);
		     bmp->line[y][x] = makecol8(r, g, b);
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	       /* load truecolor to a 15 bit hicolor bitmap */
	       for (y=0; y<h; y++) {
		  p16 = (unsigned short *)bmp->line[y];

		  if (_color_conv & COLORCONV_DITHER_HI) {
		     for (x=0; x<w; x++) {
			r = pack_getc(f);
			g = pack_getc(f);
			b = pack_getc(f);

			if (rgba)
			   pack_getc(f);

			p16[x] = makecol15_dither(r, g, b, x, y);
		     }
		  }
		  else {
		     for (x=0; x<w; x++) {
			r = pack_getc(f);
			g = pack_getc(f);
			b = pack_getc(f);

			if (rgba)
			   pack_getc(f);

			p16[x] = makecol15(r, g, b);
		     }
		  }
	       }
	       break;

	    case 16:
	       /* load truecolor to a 16 bit hicolor bitmap */
	       for (y=0; y<h; y++) {
		  p16 = (unsigned short *)bmp->line[y];

		  if (_color_conv & COLORCONV_DITHER_HI) {
		     for (x=0; x<w; x++) {
			r = pack_getc(f);
			g = pack_getc(f);
			b = pack_getc(f);

			if (rgba)
			   pack_getc(f);

			p16[x] = makecol16_dither(r, g, b, x, y);
		     }
		  }
		  else {
		     for (x=0; x<w; x++) {
			r = pack_getc(f);
			g = pack_getc(f);
			b = pack_getc(f);

			if (rgba)
			   pack_getc(f);

			p16[x] = makecol16(r, g, b);
		     }
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       /* load truecolor to a 24 bit truecolor bitmap */
	       for (y=0; y<h; y++) {
		  for (x=0; x<w; x++) {
		     r = pack_getc(f);
		     g = pack_getc(f);
		     b = pack_getc(f);

		     if (rgba)
			pack_getc(f);

		     bmp->line[y][x*3+_rgb_r_shift_24/8] = r;
		     bmp->line[y][x*3+_rgb_g_shift_24/8] = g;
		     bmp->line[y][x*3+_rgb_b_shift_24/8] = b;
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       /* load truecolor to a 32 bit truecolor bitmap */
	       for (y=0; y<h; y++) {
		  p32 = (unsigned long *)bmp->line[y];

		  for (x=0; x<w; x++) {
		     r = pack_getc(f);
		     g = pack_getc(f);
		     b = pack_getc(f);

		     if (rgba)
			a = pack_getc(f);
		     else
			a = 0;

		     p32[x] = makeacol32(r, g, b, a);
		  }
	       }
	       break;

	 #endif

	 }
	 break;
   }

   _drawing_mode = prev_drawmode;

   /* postprocess for paletted dithering */
   if (dither8) {
      BITMAP *tmp = create_bitmap_ex(8, bmp->w, bmp->h);
      if (!tmp) {
	 destroy_bitmap(bmp);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }

      blit(bmp, tmp, 0, 0, 0, 0, bmp->w, bmp->h);
      destroy_bitmap(bmp);
      bmp = tmp;
   }

   return bmp;
}



/* read_font_fixed:
 *  Reads a fixed pitch font from a file. This format is no longer used.
 */
static FONT *read_font_fixed(PACKFILE *pack, int height, int maxchars)
{
   FONT *f = NULL;
   FONT_MONO_DATA *mf = NULL;
   FONT_GLYPH **gl = NULL;
   
   int i = 0;
   
   f = malloc(sizeof(FONT));
   mf = malloc(sizeof(FONT_MONO_DATA));
   gl = malloc(sizeof(FONT_GLYPH *) * maxchars);
   
   if (!f || !mf || !gl) {
      free(f);
      free(mf);
      free(gl);
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   f->data = mf;
   f->height = height;
   f->vtable = font_vtable_mono;
   
   mf->begin = ' ';
   mf->end = ' ' + maxchars;
   mf->next = NULL;
   mf->glyphs = gl;
   
   memset(gl, 0, sizeof(FONT_GLYPH *) * maxchars);
   
   for (i = 0; i < maxchars; i++) {
      FONT_GLYPH *g = malloc(sizeof(FONT_GLYPH) + height);
      
      if (!g) {
	 destroy_font(f);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }
      
      gl[i] = g;
      g->w = 8;
      g->h = height;
      
      pack_fread(g->dat, height, pack);
   }
   
   return f;
}



/* read_font_prop:
 *  Reads a proportional font from a file. This format is no longer used.
 */
static FONT *read_font_prop(PACKFILE *pack, int maxchars)
{
   FONT *f = NULL;
   FONT_COLOR_DATA *cf = NULL;
   BITMAP **bits = NULL;
   int i = 0, h = 0;
   
   f = malloc(sizeof(FONT));
   cf = malloc(sizeof(FONT_COLOR_DATA));
   bits = malloc(sizeof(BITMAP *) * maxchars);
   
   if (!f || !cf || !bits) {
      free(f);
      free(cf);
      free(bits);
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   f->data = cf;
   f->vtable = font_vtable_color;
   
   cf->begin = ' ';
   cf->end = ' ' + maxchars;
   cf->next = NULL;
   cf->bitmaps = bits;
   
   memset(bits, 0, sizeof(BITMAP *) * maxchars);
   
   for (i = 0; i < maxchars; i++) {
      if (pack_feof(pack)) break;
      
      bits[i] = read_bitmap(pack, 8, FALSE);
      if (!bits[i]) {
	 destroy_font(f);
	 return NULL;
      }
      
      if (bits[i]->h > h) h = bits[i]->h;
   }
   
   while (i < maxchars) {
      bits[i] = create_bitmap_ex(8, 8, h);
      if (!bits[i]) {
	 destroy_font(f);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }
      
      clear_bitmap(bits[i]);
      i++;
   }
   
   f->height = h;
   
   return f;
}



/* read_font_mono:
 *  Helper for read_font, below
 */
static FONT_MONO_DATA *read_font_mono(PACKFILE *f, int *hmax)
{
   FONT_MONO_DATA *mf = NULL;
   int max = 0, i = 0;
   FONT_GLYPH **gl = NULL;
   
   mf = malloc(sizeof(FONT_MONO_DATA));
   if (!mf) {
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   mf->begin = pack_mgetl(f);
   mf->end = pack_mgetl(f) + 1;
   mf->next = NULL;
   max = mf->end - mf->begin;
   
   mf->glyphs = gl = malloc(sizeof(FONT_GLYPH *) * max);
   if (!gl) {
      free(mf);
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   for (i = 0; i < max; i++) {
      int w, h, sz;
      
      w = pack_mgetw(f);
      h = pack_mgetw(f);
      sz = ((w + 7) / 8) * h;
      
      if (h > *hmax) *hmax = h;
      
      gl[i] = malloc(sizeof(FONT_GLYPH) + sz);
      if (!gl[i]) {
	 while (i) {
	    i--;
	    free(mf->glyphs[i]);
	 }
	 free(mf);
	 free(mf->glyphs);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }
      
      gl[i]->w = w;
      gl[i]->h = h;
      pack_fread(gl[i]->dat, sz, f);
   }
   
   return mf;
}



/* read_font_color:
 *  Helper for read_font, below.
 */
static FONT_COLOR_DATA *read_font_color(PACKFILE *pack, int *hmax)
{
   FONT_COLOR_DATA *cf = NULL;
   int max = 0, i = 0;
   BITMAP **bits = NULL;
   
   cf = malloc(sizeof(FONT_COLOR_DATA));
   if (!cf) {
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   cf->begin = pack_mgetl(pack);
   cf->end = pack_mgetl(pack) + 1;
   cf->next = NULL;
   max = cf->end - cf->begin;

   cf->bitmaps = bits = malloc(sizeof(BITMAP *) * max);
   if (!bits) {
      free(cf);
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   for (i = 0; i < max; i++) {
      bits[i] = read_bitmap(pack, 8, FALSE);
      if (!bits[i]) {
	 while (i) {
	    i--;
	    destroy_bitmap(bits[i]);
	 }
	 free(bits);
	 free(cf);
	 *allegro_errno = ENOMEM;
	 return 0;
      }
      
      if (bits[i]->h > *hmax) 
	 *hmax = bits[i]->h;
   }

   return cf;
}



/* read_font:
 *  Reads a new style, Unicode format font from a file.
 */
static FONT *read_font(PACKFILE *pack)
{
   FONT *f = NULL;
   int num_ranges = 0;
   int height = 0;

   f = malloc(sizeof(FONT));
   if (!f) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   f->data = NULL;

   num_ranges = pack_mgetw(pack);
   while (num_ranges--) {
      if (pack_getc(pack)) {
	 FONT_MONO_DATA *mf = 0, *iter = (FONT_MONO_DATA *)f->data;
	 
	 f->vtable = font_vtable_mono;

	 mf = read_font_mono(pack, &height);
	 if (!mf) {
	    destroy_font(f);
	    return NULL;
	 }

	 if (!iter)
	    f->data = mf;
	 else {
	    while (iter->next) iter = iter->next;
	    iter->next = mf;
	 }
      } 
      else {
	 FONT_COLOR_DATA *cf = NULL, *iter = (FONT_COLOR_DATA *)f->data;

	 f->vtable = font_vtable_color;

	 cf = read_font_color(pack, &height);
	 if (!cf) {
	    destroy_font(f);
	    return NULL;
	 }

	 if (!iter) 
	    f->data = cf;
	 else {
	    while (iter->next) iter = iter->next;
	    iter->next = cf;
	 }
      }
   }

   f->height = height;
   return f;
}



/* read_palette:
 *  Reads a palette from a file.
 */
static RGB *read_palette(PACKFILE *f, int size)
{
   RGB *p;
   int c, x;

   p = malloc(sizeof(PALETTE));
   if (!p) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c<size; c++) {
      p[c].r = pack_getc(f) >> 2;
      p[c].g = pack_getc(f) >> 2;
      p[c].b = pack_getc(f) >> 2;
   }

   x = 0;
   while (c < PAL_SIZE) {
      p[c] = p[x];
      c++;
      x++;
      if (x >= size)
	 x = 0;
   }

   return p;
}



/* read_sample:
 *  Reads a sample from a file.
 */
static SAMPLE *read_sample(PACKFILE *f)
{
   signed short bits;
   SAMPLE *s;

   s = malloc(sizeof(SAMPLE));
   if (!s) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   bits = pack_mgetw(f);

   if (bits < 0) {
      s->bits = -bits;
      s->stereo = TRUE;
   }
   else {
      s->bits = bits;
      s->stereo = FALSE;
   }

   s->freq = pack_mgetw(f);
   s->len = pack_mgetl(f);
   s->priority = 128;
   s->loop_start = 0;
   s->loop_end = s->len;
   s->param = 0;

   if (s->bits == 8) {
      s->data = read_block(f, s->len * ((s->stereo) ? 2 : 1), 0);
   }
   else {
      s->data = malloc(s->len * sizeof(short) * ((s->stereo) ? 2 : 1));
      if (s->data) {
	 int i;

	 for (i=0; i < (int)s->len * ((s->stereo) ? 2 : 1); i++) {
	    ((unsigned short *)s->data)[i] = pack_igetw(f);
	 }

	 if (pack_ferror(f)) {
	    free(s->data);
	    s->data = NULL;
	 }
      }
   }
   if (!s->data) {
      free(s);
      return NULL;
   }

   LOCK_DATA(s, sizeof(SAMPLE));
   LOCK_DATA(s->data, s->len * ((s->bits==8) ? 1 : sizeof(short)) * ((s->stereo) ? 2 : 1));

   return s;
}



/* read_midi:
 *  Reads MIDI data from a datafile (this is not the same thing as the 
 *  standard midi file format).
 */
static MIDI *read_midi(PACKFILE *f)
{
   MIDI *m;
   int c;

   m = malloc(sizeof(MIDI));
   if (!m) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c<MIDI_TRACKS; c++) {
      m->track[c].len = 0;
      m->track[c].data = NULL;
   }

   m->divisions = pack_mgetw(f);

   for (c=0; c<MIDI_TRACKS; c++) {
      m->track[c].len = pack_mgetl(f);
      if (m->track[c].len > 0) {
	 m->track[c].data = read_block(f, m->track[c].len, 0);
	 if (!m->track[c].data) {
	    unload_midi(m);
	    return NULL;
	 }
      }
   }

   LOCK_DATA(m, sizeof(MIDI));

   for (c=0; c<MIDI_TRACKS; c++) {
      if (m->track[c].data) {
	 LOCK_DATA(m->track[c].data, m->track[c].len);
      }
   }

   return m;
}



/* read_rle_sprite:
 *  Reads an RLE compressed sprite from a file, allocating memory for it. 
 */
static RLE_SPRITE *read_rle_sprite(PACKFILE *f, int bits)
{
   int x, y, w, h, c, r, g, b, a;
   int destbits, size, rgba;
   RLE_SPRITE *s;
   BITMAP *b1, *b2;
   unsigned int eol_marker;
   signed short s16;
   signed short *p16;
   signed long *p32;

   if (bits < 0) {
      bits = -bits;
      rgba = TRUE;
   }
   else
      rgba = FALSE;

   w = pack_mgetw(f);
   h = pack_mgetw(f);
   size = pack_mgetl(f);

   s = malloc(sizeof(RLE_SPRITE) + size);
   if (!s) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   s->w = w;
   s->h = h;
   s->color_depth = bits;
   s->size = size;

   switch (bits) {

      case 8:
	 /* easy! */
	 pack_fread(s->dat, size, f);
	 break;

      case 15:
      case 16:
	 /* read hicolor data */
	 p16 = (signed short *)s->dat;
	 eol_marker = (bits == 15) ? MASK_COLOR_15 : MASK_COLOR_16;

	 for (y=0; y<h; y++) {
	    s16 = pack_igetw(f);

	    while ((unsigned short)s16 != MASK_COLOR_16) {
	       if (s16 < 0) {
		  /* skip count */
		  *p16 = s16;
		  p16++;
	       }
	       else {
		  /* solid run */
		  x = s16;
		  *p16 = s16;
		  p16++;

		  while (x-- > 0) {
		     c = pack_igetw(f);
		     r = _rgb_scale_5[(c >> 11) & 0x1F];
		     g = _rgb_scale_6[(c >> 5) & 0x3F];
		     b = _rgb_scale_5[c & 0x1F];
		     *p16 = makecol_depth(bits, r, g, b);
		     p16++;
		  }
	       }

	       s16 = pack_igetw(f);
	    }

	    /* end of line */
	    *p16 = eol_marker;
	    p16++;
	 }
	 break;

      case 24:
      case 32:
	 /* read truecolor data */
	 p32 = (signed long *)s->dat;
	 eol_marker = (bits == 24) ? MASK_COLOR_24 : MASK_COLOR_32;

	 for (y=0; y<h; y++) {
	    c = pack_igetl(f);

	    while ((unsigned long)c != MASK_COLOR_32) {
	       if (c < 0) {
		  /* skip count */
		  *p32 = c;
		  p32++;
	       }
	       else {
		  /* solid run */
		  x = c;
		  *p32 = c;
		  p32++;

		  while (x-- > 0) {
		     r = pack_getc(f);
		     g = pack_getc(f);
		     b = pack_getc(f);

		     if (rgba)
			a = pack_getc(f);
		     else
			a = 0;

		     *p32 = makeacol_depth(bits, r, g, b, a);

		     p32++;
		  }
	       }

	       c = pack_igetl(f);
	    }

	    /* end of line */
	    *p32 = eol_marker;
	    p32++;
	 }
	 break;
   }

   destbits = _color_load_depth(bits, rgba);

   if (destbits != bits) {
      b1 = create_bitmap_ex(bits, s->w, s->h);
      if (!b1) {
	 destroy_rle_sprite(s);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }

      clear_to_color(b1, bitmap_mask_color(b1));
      draw_rle_sprite(b1, s, 0, 0);

      b2 = create_bitmap_ex(destbits, s->w, s->h);
      if (!b2) {
	 destroy_rle_sprite(s);
	 destroy_bitmap(b1);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }

      blit(b1, b2, 0, 0, 0, 0, s->w, s->h);

      destroy_rle_sprite(s);
      s = get_rle_sprite(b2);

      destroy_bitmap(b1);
      destroy_bitmap(b2);
   }

   return s;
}



/* read_compiled_sprite:
 *  Reads a compiled sprite from a file, allocating memory for it.
 */
static COMPILED_SPRITE *read_compiled_sprite(PACKFILE *f, int planar, int bits)
{
   BITMAP *b;
   COMPILED_SPRITE *s;

   b = read_bitmap(f, bits, TRUE);
   if (!b)
      return NULL;

   if (!_compile_sprites)
      return (COMPILED_SPRITE *)b;

   s = get_compiled_sprite(b, planar);
   if (!s)
      *allegro_errno = ENOMEM;

   destroy_bitmap(b);

   return s;
}



/* read_old_datafile:
 *  Helper for reading old-format datafiles (only needed for backward
 *  compatibility).
 */
static DATAFILE *read_old_datafile(PACKFILE *f, void (*callback)(DATAFILE *))
{
   DATAFILE *dat;
   int size;
   int type;
   int c;

   size = pack_mgetw(f);
   if (*allegro_errno) {
      pack_fclose(f);
      return NULL;
   }

   dat = malloc(sizeof(DATAFILE)*(size+1));
   if (!dat) {
      pack_fclose(f);
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c<=size; c++) {
      dat[c].type = DAT_END;
      dat[c].dat = NULL;
      dat[c].size = 0;
      dat[c].prop = NULL;
   }

   for (c=0; c<size; c++) {

      type = pack_mgetw(f);

      switch (type) {

	 case V1_DAT_FONT: 
	 case V1_DAT_FONT_8x8: 
	    dat[c].type = DAT_FONT;
	    dat[c].dat = read_font_fixed(f, 8, OLD_FONT_SIZE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_FONT_PROP:
	    dat[c].type = DAT_FONT;
	    dat[c].dat = read_font_prop(f, OLD_FONT_SIZE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_BITMAP:
	 case V1_DAT_BITMAP_256:
	    dat[c].type = DAT_BITMAP;
	    dat[c].dat = read_bitmap(f, 8, TRUE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_BITMAP_16:
	    dat[c].type = DAT_BITMAP;
	    dat[c].dat = read_bitmap(f, 4, FALSE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_SPRITE_256:
	    dat[c].type = DAT_BITMAP;
	    pack_mgetw(f);
	    dat[c].dat = read_bitmap(f, 8, TRUE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_SPRITE_16:
	    dat[c].type = DAT_BITMAP;
	    pack_mgetw(f);
	    dat[c].dat = read_bitmap(f, 4, FALSE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_PALETTE:
	 case V1_DAT_PALETTE_256:
	    dat[c].type = DAT_PALETTE;
	    dat[c].dat = read_palette(f, PAL_SIZE);
	    dat[c].size = sizeof(PALETTE);
	    break;

	 case V1_DAT_PALETTE_16:
	    dat[c].type = DAT_PALETTE;
	    dat[c].dat = read_palette(f, 16);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_SAMPLE:
	    dat[c].type = DAT_SAMPLE;
	    dat[c].dat = read_sample(f);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_MIDI:
	    dat[c].type = DAT_MIDI;
	    dat[c].dat = read_midi(f);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_RLE_SPRITE:
	    dat[c].type = DAT_RLE_SPRITE;
	    dat[c].dat = read_rle_sprite(f, 8);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_FLI:
	    dat[c].type = DAT_FLI;
	    dat[c].size = pack_mgetl(f);
	    dat[c].dat = read_block(f, dat[c].size, 0);
	    break;

	 case V1_DAT_C_SPRITE:
	    dat[c].type = DAT_C_SPRITE;
	    dat[c].dat = read_compiled_sprite(f, FALSE, 8);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_XC_SPRITE:
	    dat[c].type = DAT_XC_SPRITE;
	    dat[c].dat = read_compiled_sprite(f, TRUE, 8);
	    dat[c].size = 0;
	    break;

	 default:
	    dat[c].type = DAT_DATA;
	    dat[c].size = pack_mgetl(f);
	    dat[c].dat = read_block(f, dat[c].size, 0);
	    break;
      }

      if (*allegro_errno) {
	 if (!dat[c].dat)
	    dat[c].type = DAT_END;
	 unload_datafile(dat);
	 pack_fclose(f);
	 return NULL;
      }

      if (callback)
	 callback(dat+c);
   }

   return dat;
}



/* load_data_object:
 *  Loads a binary data object from a datafile.
 */
static void *load_data_object(PACKFILE *f, long size)
{
   return read_block(f, size, 0);
}



/* load_font_object:
 *  Loads a font object from a datafile.
 */
static void *load_font_object(PACKFILE *f, long size)
{
   short height = pack_mgetw(f);

   if (height > 0)
      return read_font_fixed(f, height, LESS_OLD_FONT_SIZE);
   else if (height < 0)
      return read_font_prop(f, LESS_OLD_FONT_SIZE);
   else
      return read_font(f);
}



/* load_sample_object:
 *  Loads a sample object from a datafile.
 */
static void *load_sample_object(PACKFILE *f, long size)
{
   return read_sample(f);
}



/* load_midi_object:
 *  Loads a midifile object from a datafile.
 */
static void *load_midi_object(PACKFILE *f, long size)
{
   return read_midi(f);
}



/* load_bitmap_object:
 *  Loads a bitmap object from a datafile.
 */
static void *load_bitmap_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_bitmap(f, bits, TRUE);
}



/* load_rle_sprite_object:
 *  Loads an RLE sprite object from a datafile.
 */
static void *load_rle_sprite_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_rle_sprite(f, bits);
}



/* load_compiled_sprite_object:
 *  Loads a compiled sprite object from a datafile.
 */
static void *load_compiled_sprite_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_compiled_sprite(f, FALSE, bits);
}



/* load_xcompiled_sprite_object:
 *  Loads a mode-X compiled object from a datafile.
 */
static void *load_xcompiled_sprite_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_compiled_sprite(f, TRUE, bits);
}



/* unload_sample: 
 *  Destroys a sample object.
 */
static void unload_sample(SAMPLE *s)
{
   if (s) {
      if (s->data) {
	 UNLOCK_DATA(s->data, s->len * ((s->bits==8) ? 1 : sizeof(short)) * ((s->stereo) ? 2 : 1));
	 free(s->data);
      }

      UNLOCK_DATA(s, sizeof(SAMPLE));
      free(s);
   }
}



/* unload_midi: 
 *  Destroys a MIDI object.
 */
static void unload_midi(MIDI *m)
{
   int c;

   if (m) {
      for (c=0; c<MIDI_TRACKS; c++) {
	 if (m->track[c].data) {
	    UNLOCK_DATA(m->track[c].data, m->track[c].len);
	    free(m->track[c].data);
	 }
      }

      UNLOCK_DATA(m, sizeof(MIDI));
      free(m);
   }
}



/* load_object:
 *  Helper to load an object from a datafile.
 */
static void *load_object(PACKFILE *f, int type, long size)
{
   int i;

   /* look for a load function */
   for (i=0; i<MAX_DATAFILE_TYPES; i++)
      if (_datafile_type[i].type == type)
	 return _datafile_type[i].load(f, size);

   /* if not found, load binary data */
   return load_data_object(f, size);
}



/* load_file_object:
 *  Loads a datafile object.
 */
static void *load_file_object(PACKFILE *f, long size)
{
   #define MAX_PROPERTIES  64

   DATAFILE_PROPERTY prop[MAX_PROPERTIES];
   DATAFILE *dat;
   PACKFILE *ff;
   int prop_count, count, type, c, d;
   char *p;

   count = pack_mgetl(f);

   dat = malloc(sizeof(DATAFILE)*(count+1));
   if (!dat) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c<=count; c++) {
      dat[c].type = DAT_END;
      dat[c].dat = NULL;
      dat[c].size = 0;
      dat[c].prop = NULL;
   }

   for (c=0; c<MAX_PROPERTIES; c++)
      prop[c].dat = NULL;

   c = 0;
   prop_count = 0;

   while (c < count) {
      type = pack_mgetl(f);

      if (type == DAT_PROPERTY) {
	 /* load an object property */
	 type = pack_mgetl(f);
	 d = pack_mgetl(f);
	 if (prop_count < MAX_PROPERTIES) {
	    prop[prop_count].type = type;
	    prop[prop_count].dat = malloc(d+1);
	    if (prop[prop_count].dat != NULL) {
	       pack_fread(prop[prop_count].dat, d, f);
	       prop[prop_count].dat[d] = 0;
	       if (need_uconvert(prop[prop_count].dat, U_UTF8, U_CURRENT)) {
		  p = malloc(uconvert_size(prop[prop_count].dat, U_UTF8, U_CURRENT));
		  if (p)
		     do_uconvert(prop[prop_count].dat, U_UTF8, p, U_CURRENT, -1);
		  free(prop[prop_count].dat);
		  prop[prop_count].dat = p;
	       }
	       prop_count++;
	       d = 0;
	    }
	 }
	 while (d-- > 0)
	    pack_getc(f);
      }
      else {
	 /* load actual data */
	 ff = pack_fopen_chunk(f, FALSE);

	 if (ff) {
	    d = ff->todo;

	    dat[c].dat = load_object(ff, type, d);

	    if (dat[c].dat) {
	       dat[c].type = type;
	       dat[c].size = d;

	       if (prop_count > 0) {
		  dat[c].prop = malloc(sizeof(DATAFILE_PROPERTY)*(prop_count+1));
		  if (dat[c].prop != NULL) {
		     for (d=0; d<prop_count; d++) {
			dat[c].prop[d].dat = prop[d].dat;
			dat[c].prop[d].type = prop[d].type;
			prop[d].dat = NULL;
		     }
		     dat[c].prop[d].dat = NULL;
		     dat[c].prop[d].type = DAT_END;
		  }
		  else {
		     for (d=0; d<prop_count; d++) {
			free(prop[d].dat);
			prop[d].dat = NULL;
		     }
		  }
		  prop_count = 0;
	       }
	       else
		  dat[c].prop = NULL;
	    }

	    f = pack_fclose_chunk(ff);

	    if (datafile_callback)
	       datafile_callback(dat+c);

	    c++;
	 }
      }

      if (*allegro_errno) {
	 unload_datafile(dat);

	 for (c=0; c<MAX_PROPERTIES; c++)
	    if (prop[c].dat)
	       free(prop[c].dat);

	 return NULL;
      }
   }

   for (c=0; c<MAX_PROPERTIES; c++)
      if (prop[c].dat)
	 free(prop[c].dat);

   return dat;
}



/* load_datafile:
 *  Loads an entire data file into memory, and returns a pointer to it. 
 *  On error, sets errno and returns NULL.
 */
DATAFILE *load_datafile(AL_CONST char *filename)
{
   return load_datafile_callback(filename, NULL);
}



/* load_datafile_callback:
 *  Loads an entire data file into memory, and returns a pointer to it. 
 *  On error, sets errno and returns NULL.
 */
DATAFILE *load_datafile_callback(AL_CONST char *filename, void (*callback)(DATAFILE *))
{
   PACKFILE *f;
   DATAFILE *dat;
   int type;

   f = pack_fopen(filename, F_READ_PACKED);
   if (!f)
      return NULL;

   if ((f->flags & PACKFILE_FLAG_CHUNK) && (!(f->flags & PACKFILE_FLAG_EXEDAT)))
      type = (_packfile_type == DAT_FILE) ? DAT_MAGIC : 0;
   else
      type = pack_mgetl(f);

   if (type == V1_DAT_MAGIC) {
      dat = read_old_datafile(f, callback);
   }
   else if (type == DAT_MAGIC) {
      datafile_callback = callback;
      dat = load_file_object(f, 0);
      datafile_callback = NULL;
   }
   else
      dat = NULL;

   pack_fclose(f);
   return dat; 
}



/* load_datafile_object:
 *  Loads a single object from a datafile.
 */
DATAFILE *load_datafile_object(AL_CONST char *filename, AL_CONST char *objectname)
{
   PACKFILE *f;
   DATAFILE *dat;
   void *object;
   char buf[1024], tmp[16];
   int size;

   ustrzcpy(buf, sizeof(buf), filename);

   if (ustrcmp(buf, uconvert_ascii("#", tmp)) != 0)
      ustrzcat(buf, sizeof(buf), uconvert_ascii("#", tmp));

   ustrzcat(buf, sizeof(buf), objectname);

   f = pack_fopen(buf, F_READ_PACKED);
   if (!f)
      return NULL;

   size = f->todo;

   dat = malloc(sizeof(DATAFILE));
   if (!dat) {
      pack_fclose(f);
      return NULL;
   }

   object = load_object(f, _packfile_type, size);

   pack_fclose(f);

   if (!object) {
      free(dat);
      return NULL;
   }

   dat->dat = object;
   dat->type = _packfile_type;
   dat->size = size;
   dat->prop = NULL;

   return dat; 
}



/* _unload_datafile_object:
 *  Helper to destroy a datafile object.
 */
void _unload_datafile_object(DATAFILE *dat)
{
   int i;

   /* free the property list */
   if (dat->prop) {
      for (i=0; dat->prop[i].type != DAT_END; i++)
	 if (dat->prop[i].dat)
	    free(dat->prop[i].dat);

      free(dat->prop);
   }

   /* look for a destructor function */
   for (i=0; i<MAX_DATAFILE_TYPES; i++) {
      if (_datafile_type[i].type == dat->type) {
	 if (dat->dat) {
	    if (_datafile_type[i].destroy)
	       _datafile_type[i].destroy(dat->dat);
	    else
	       free(dat->dat);
	 }
	 return;
      }
   }

   /* if not found, just free the data */
   if (dat->dat)
      free(dat->dat);
}



/* unload_datafile:
 *  Frees all the objects in a datafile.
 */
void unload_datafile(DATAFILE *dat)
{
   int i;

   if (dat) {
      for (i=0; dat[i].type != DAT_END; i++)
	 _unload_datafile_object(dat+i);

      free(dat);
   }
}



/* unload_datafile_object:
 *  Unloads a single datafile object, returned by load_datafile_object().
 */
void unload_datafile_object(DATAFILE *dat)
{
   if (dat) {
      _unload_datafile_object(dat);
      free(dat);
   }
}



/* find_datafile_object:
 *  Returns a pointer to the datafile object with the given name
 */
DATAFILE *find_datafile_object(AL_CONST DATAFILE *dat, AL_CONST char *objectname)
{
   char name[512];
   int recurse = FALSE;
   int pos, c;

   /* split up the object name */
   pos = 0;

   while ((c = ugetxc(&objectname)) != 0) {
      if ((c == '#') || (c == '/') || (c == OTHER_PATH_SEPARATOR)) {
	 recurse = TRUE;
	 break;
      }
      pos += usetc(name+pos, c);
   }

   usetc(name+pos, 0);

   /* search for the requested object */
   for (pos=0; dat[pos].type != DAT_END; pos++) {
      if (ustricmp(name, get_datafile_property(dat+pos, DAT_NAME)) == 0) {
	 if (recurse) {
	    if (dat[pos].type == DAT_FILE)
	       return find_datafile_object(dat[pos].dat, objectname);
	    else
	       return NULL;
	 }
	 else
	    return (DATAFILE*)dat+pos;
      }
   }

   /* oh dear, the object isn't there... */
   return NULL; 
}



/* get_datafile_property:
 *  Returns the specified property string for the datafile object, or
 *  an empty string if the property does not exist.
 */
AL_CONST char *get_datafile_property(AL_CONST DATAFILE *dat, int type)
{
   DATAFILE_PROPERTY *prop = dat->prop;

   if (prop) {
      while (prop->type != DAT_END) {
	 if (prop->type == type)
	    return (prop->dat) ? prop->dat : empty_string;

	 prop++;
      }
   }

   return empty_string;
}



/* fixup_datafile:
 *  For use with compiled datafiles, to convert truecolor images into the
 *  appropriate pixel format.
 */
void fixup_datafile(DATAFILE *data)
{
   BITMAP *bmp;
   RLE_SPRITE *rle;
   int i, c, r, g, b, a, x, y;
   int bpp, depth;
   unsigned short *p16;
   unsigned long *p32;
   signed short *s16;
   signed long *s32;
   int eol_marker;

   _construct_datafile(data);

   for (i=0; data[i].type != DAT_END; i++) {

      switch (data[i].type) {

	 case DAT_BITMAP:
	    /* fix up a bitmap object */
	    bmp = data[i].dat;
	    bpp = bitmap_color_depth(bmp);

	    switch (bpp) {

	    #ifdef ALLEGRO_COLOR16

	       case 15:
		  /* fix up a 15 bit hicolor bitmap */
		  if (_color_depth == 16) {
		     GFX_VTABLE *vtable = _get_vtable(16);

		     if (vtable != NULL) {
			depth = 16;
			bmp->vtable = vtable;
		     }
		     else
			depth = 15;
		  }
		  else
		     depth = 15;

		  for (y=0; y<bmp->h; y++) {
		     p16 = (unsigned short *)bmp->line[y];

		     for (x=0; x<bmp->w; x++) {
			c = p16[x];
			r = _rgb_scale_5[c & 0x1F];
			g = _rgb_scale_5[(c >> 5) & 0x1F];
			b = _rgb_scale_5[(c >> 10) & 0x1F];
			p16[x] = makecol_depth(depth, r, g, b);
		     }
		  }
		  break;


	       case 16:
		  /* fix up a 16 bit hicolor bitmap */
		  if (_color_depth == 15) {
		     GFX_VTABLE *vtable = _get_vtable(15);

		     if (vtable != NULL) {
			depth = 15;
			bmp->vtable = vtable;
		     }
		     else
			depth = 16;
		  }
		  else
		     depth = 16;

		  for (y=0; y<bmp->h; y++) {
		     p16 = (unsigned short *)bmp->line[y];

		     for (x=0; x<bmp->w; x++) {
			c = p16[x];
			r = _rgb_scale_5[c & 0x1F];
			g = _rgb_scale_6[(c >> 5) & 0x3F];
			b = _rgb_scale_5[(c >> 11) & 0x1F];
			p16[x] = makecol_depth(depth, r, g, b);
		     }
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR24

	       case 24:
		  /* fix up a 24 bit truecolor bitmap */
		  for (y=0; y<bmp->h; y++) {
		     for (x=0; x<bmp->w; x++) {
			r = bmp->line[y][x*3];
			g = bmp->line[y][x*3+1];
			b = bmp->line[y][x*3+2];
			bmp->line[y][x*3+_rgb_r_shift_24/8] = r;
			bmp->line[y][x*3+_rgb_r_shift_24/8] = g;
			bmp->line[y][x*3+_rgb_r_shift_24/8] = b;
		     }
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR32

	       case 32:
		  /* fix up a 32 bit truecolor bitmap */
		  for (y=0; y<bmp->h; y++) {
		     p32 = (unsigned long *)bmp->line[y];

		     for (x=0; x<bmp->w; x++) {
			c = p32[x];
			r = (c & 0xFF);
			g = (c >> 8) & 0xFF;
			b = (c >> 16) & 0xFF;
			a = (c >> 24) & 0xFF;
			p32[x] = makeacol32(r, g, b, a);
		     }
		  }
		  break;

	    #endif

	    }
	    break;


	 case DAT_RLE_SPRITE:
	    /* fix up an RLE sprite object */
	    rle = data[i].dat;
	    bpp = rle->color_depth;

	    switch (bpp) {

	    #ifdef ALLEGRO_COLOR16

	       case 15:
		  /* fix up a 15 bit hicolor RLE sprite */
		  if (_color_depth == 16) {
		     depth = 16;
		     rle->color_depth = 16;
		     eol_marker = MASK_COLOR_16;
		  }
		  else {
		     depth = 15;
		     eol_marker = MASK_COLOR_15;
		  }

		  s16 = (signed short *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((unsigned short)*s16 != MASK_COLOR_15) {
			if (*s16 > 0) {
			   x = *s16;
			   s16++;
			   while (x-- > 0) {
			      c = *s16;
			      r = _rgb_scale_5[c & 0x1F];
			      g = _rgb_scale_5[(c >> 5) & 0x1F];
			      b = _rgb_scale_5[(c >> 10) & 0x1F];
			      *s16 = makecol_depth(depth, r, g, b);
			      s16++;
			   }
			}
			else
			   s16++;
		     }
		     *s16 = eol_marker;
		     s16++;
		  }
		  break;


	       case 16:
		  /* fix up a 16 bit hicolor RLE sprite */
		  if (_color_depth == 15) {
		     depth = 15;
		     rle->color_depth = 15;
		     eol_marker = MASK_COLOR_15;
		  }
		  else {
		     depth = 16;
		     eol_marker = MASK_COLOR_16;
		  }

		  s16 = (signed short *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((unsigned short)*s16 != MASK_COLOR_16) {
			if (*s16 > 0) {
			   x = *s16;
			   s16++;
			   while (x-- > 0) {
			      c = *s16;
			      r = _rgb_scale_5[c & 0x1F];
			      g = _rgb_scale_6[(c >> 5) & 0x3F];
			      b = _rgb_scale_5[(c >> 11) & 0x1F];
			      *s16 = makecol_depth(depth, r, g, b);
			      s16++;
			   }
			}
			else
			   s16++;
		     }
		     *s16 = eol_marker;
		     s16++;
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR24

	       case 24:
		  /* fix up a 24 bit truecolor RLE sprite */
		  if (_color_depth == 32) {
		     depth = 32;
		     rle->color_depth = 32;
		     eol_marker = MASK_COLOR_32;
		  }
		  else {
		     depth = 24;
		     eol_marker = MASK_COLOR_24;
		  }

		  s32 = (signed long *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((unsigned long)*s32 != MASK_COLOR_24) {
			if (*s32 > 0) {
			   x = *s32;
			   s32++;
			   while (x-- > 0) {
			      c = *s32;
			      r = (c & 0xFF);
			      g = (c>>8) & 0xFF;
			      b = (c>>16) & 0xFF;
			      *s32 = makecol_depth(depth, r, g, b);
			      s32++;
			   }
			}
			else
			   s32++;
		     }
		     *s32 = eol_marker;
		     s32++;
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR32

	       case 32:
		  /* fix up a 32 bit truecolor RLE sprite */
		  if (_color_depth == 24) {
		     depth = 24;
		     rle->color_depth = 24;
		     eol_marker = MASK_COLOR_24;
		  }
		  else {
		     depth = 32;
		     eol_marker = MASK_COLOR_32;
		  }

		  s32 = (signed long *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((unsigned long)*s32 != MASK_COLOR_32) {
			if (*s32 > 0) {
			   x = *s32;
			   s32++;
			   while (x-- > 0) {
			      c = *s32;
			      r = (c & 0xFF);
			      g = (c>>8) & 0xFF;
			      b = (c>>16) & 0xFF;
			      if (depth == 32) {
				 a = (c>>24) & 0xFF;
				 *s32 = makeacol32(r, g, b, a);
			      }
			      else
				 *s32 = makecol24(r, g, b);
			      s32++;
			   }
			}
			else
			   s32++;
		     }
		     *s32 = eol_marker;
		     s32++;
		  }
		  break;

	    #endif

	    }
	    break;
      }
   }
}



/* _construct_datafile:
 *  Helper used by the output from dat2s, for fixing up parts of
 *  the data that can't be statically initialised, such as locking
 *  samples and MIDI files, and setting the segment selectors in the 
 *  BITMAP structures.
 */
void _construct_datafile(DATAFILE *data)
{
   int c, c2;
   FONT *f;
   SAMPLE *s;
   MIDI *m;

   for (c=0; data[c].type != DAT_END; c++) {

      switch (data[c].type) {

	 case DAT_FILE:
	    _construct_datafile(data[c].dat);
	    break;

	 case DAT_BITMAP:
	    ((BITMAP *)data[c].dat)->seg = _default_ds();
	    break;

	 case DAT_FONT:
	    f = data[c].dat;
	    if (f->vtable == font_vtable_color) {
	       FONT_COLOR_DATA *cf = (FONT_COLOR_DATA *)f->data;
	       while (cf) {
		  for (c2 = cf->begin; c2 < cf->end; c2++)
		     cf->bitmaps[c2 - cf->begin]->seg = _default_ds();
		  cf = cf->next;
	       }
	    }
	    break;

	 case DAT_SAMPLE:
	    s = data[c].dat;
	    LOCK_DATA(s, sizeof(SAMPLE));
	    LOCK_DATA(s->data, s->len * ((s->bits==8) ? 1 : sizeof(short)) * ((s->stereo) ? 2 : 1));
	    break;

	 case DAT_MIDI:
	    m = data[c].dat;
	    LOCK_DATA(m, sizeof(MIDI));
	    for (c2=0; c2<MIDI_TRACKS; c2++) {
	       if (m->track[c2].data) {
		  LOCK_DATA(m->track[c2].data, m->track[c2].len);
	       }
	    }
	    break;
      }
   }
}



/* _initialize_datafile_types:
 *  Register my loader functions with the code in dataregi.c.
 */
#ifdef CONSTRUCTOR_FUNCTION
   CONSTRUCTOR_FUNCTION(void _initialize_datafile_types());
#endif

void _initialize_datafile_types()
{
   register_datafile_object(DAT_FILE,         load_file_object,             (void (*)(void *data))unload_datafile        );
   register_datafile_object(DAT_FONT,         load_font_object,             (void (*)(void *data))destroy_font           );
   register_datafile_object(DAT_SAMPLE,       load_sample_object,           (void (*)(void *data))unload_sample          );
   register_datafile_object(DAT_MIDI,         load_midi_object,             (void (*)(void *data))unload_midi            );
   register_datafile_object(DAT_BITMAP,       load_bitmap_object,           (void (*)(void *data))destroy_bitmap         );
   register_datafile_object(DAT_RLE_SPRITE,   load_rle_sprite_object,       (void (*)(void *data))destroy_rle_sprite     );
   register_datafile_object(DAT_C_SPRITE,     load_compiled_sprite_object,  (void (*)(void *data))destroy_compiled_sprite);
   register_datafile_object(DAT_XC_SPRITE,    load_xcompiled_sprite_object, (void (*)(void *data))destroy_compiled_sprite);
}

