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
 *      Graphics routines: palette fading, circles, etc.
 *
 *      By Shawn Hargreaves.
 *
 *      Optimised line drawer by Michael Bukin.
 *
 *      Bresenham arc routine by Romano Signorelli.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* drawing_mode:
 *  Sets the drawing mode. This only affects routines like putpixel,
 *  lines, rectangles, triangles, etc, not the blitting or sprite
 *  drawing functions.
 */
void drawing_mode(int mode, BITMAP *pattern, int x_anchor, int y_anchor)
{
   _drawing_mode = mode;
   _drawing_pattern = pattern;
   _drawing_x_anchor = x_anchor;
   _drawing_y_anchor = y_anchor;

   if (pattern) {
      _drawing_x_mask = 1; 
      while (_drawing_x_mask < (unsigned)pattern->w)
	 _drawing_x_mask <<= 1;        /* find power of two greater than w */

      if (_drawing_x_mask > (unsigned)pattern->w) {
	 ASSERT(FALSE);
	 _drawing_x_mask >>= 1;        /* round down if required */
      }

      _drawing_x_mask--;               /* convert to AND mask */

      _drawing_y_mask = 1;
      while (_drawing_y_mask < (unsigned)pattern->h)
	 _drawing_y_mask <<= 1;        /* find power of two greater than h */

      if (_drawing_y_mask > (unsigned)pattern->h) {
	 ASSERT(FALSE);
	 _drawing_y_mask >>= 1;        /* round down if required */
      }

      _drawing_y_mask--;               /* convert to AND mask */
   }
   else
      _drawing_x_mask = _drawing_y_mask = 0;

   if ((gfx_driver) && (gfx_driver->drawing_mode) && (!_dispsw_status))
      gfx_driver->drawing_mode();
}



/* set_blender_mode:
 *  Specifies a custom set of blender functions for interpolating between
 *  truecolor pixels. The 24 bit blender is shared between the 24 and 32 bit
 *  modes. Pass a NULL table for unused color depths (you must not draw
 *  translucent graphics in modes without a handler, though!). Your blender
 *  will be passed two 32 bit colors in the appropriate format (5.5.5, 5.6.5,
 *  or 8.8.8), and an alpha value, should return the result of combining them.
 *  In translucent drawing modes, the two colors are taken from the source
 *  and destination images and the alpha is specified by this function. In
 *  lit modes, the alpha is specified when you call the drawing routine, and
 *  the interpolation is between the source color and the RGB values you pass
 *  to this function.
 */
void set_blender_mode(BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, int r, int g, int b, int a)
{
   _blender_func15 = b15;
   _blender_func16 = b16;
   _blender_func24 = b24;
   _blender_func32 = b24;

   _blender_func15x = _blender_black;
   _blender_func16x = _blender_black;
   _blender_func24x = _blender_black;

   _blender_col_15 = makecol15(r, g, b);
   _blender_col_16 = makecol16(r, g, b);
   _blender_col_24 = makecol24(r, g, b);
   _blender_col_32 = makecol32(r, g, b);

   _blender_alpha = a;
}



/* set_blender_mode_ex
 *  Specifies a custom set of blender functions for interpolating between
 *  truecolor pixels, providing a more complete set of routines, which
 *  differentiate between 24 and 32 bit modes, and have special routines
 *  for blending 32 bit RGBA pixels onto a destination of any format.
 */
void set_blender_mode_ex(BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, BLENDER_FUNC b32, BLENDER_FUNC b15x, BLENDER_FUNC b16x, BLENDER_FUNC b24x, int r, int g, int b, int a)
{
   _blender_func15 = b15;
   _blender_func16 = b16;
   _blender_func24 = b24;
   _blender_func32 = b32;

   _blender_func15x = b15x;
   _blender_func16x = b16x;
   _blender_func24x = b24x;

   _blender_col_15 = makecol15(r, g, b);
   _blender_col_16 = makecol16(r, g, b);
   _blender_col_24 = makecol24(r, g, b);
   _blender_col_32 = makecol32(r, g, b);

   _blender_alpha = a;
}



/* xor_mode:
 *  Shortcut function for toggling XOR mode on and off.
 */
void xor_mode(int on)
{
   drawing_mode(on ? DRAW_MODE_XOR : DRAW_MODE_SOLID, NULL, 0, 0);
}



/* solid_mode:
 *  Shortcut function for selecting solid drawing mode.
 */
void solid_mode()
{
   drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
}



/* clear_bitmap:
 *  Clears the bitmap to color 0.
 */
void clear_bitmap(BITMAP *bitmap)
{
   clear_to_color(bitmap, 0);
}



/* vsync:
 *  Waits for a retrace.
 */
void vsync()
{
   ASSERT(gfx_driver);

   if (!_dispsw_status)
      gfx_driver->vsync();
}



/* set_color:
 *  Sets a single palette entry.
 */
void set_color(int index, AL_CONST RGB *p)
{
   set_palette_range((struct RGB *)p-index, index, index, FALSE);
}



/* set_palette:
 *  Sets the entire color palette.
 */
void set_palette(AL_CONST PALETTE p)
{
   set_palette_range(p, 0, PAL_SIZE-1, TRUE);
}



/* set_palette_range:
 *  Sets a part of the color palette.
 */
void set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   int c;

   for (c=from; c<=to; c++) {
      _current_palette[c] = p[c];

      if (_color_depth != 8)
	 palette_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
   }

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));

   if (gfx_driver) {
      if ((screen->vtable->color_depth == 8) && (!_dispsw_status))
	 gfx_driver->set_palette(p, from, to, vsync);
   }
   else if ((system_driver) && (system_driver->set_palette_range))
      system_driver->set_palette_range(p, from, to, vsync);
}



/* previous palette, so the image loaders can restore it when they are done */
int _got_prev_current_palette = FALSE;
PALETTE _prev_current_palette;
static int prev_palette_color[256];



/* select_palette:
 *  Sets the aspects of the palette tables that are used for converting
 *  between different image formats, without altering the display settings.
 *  The previous settings are copied onto a one-deep stack, from where they
 *  can be restored by calling unselect_palette().
 */
void select_palette(AL_CONST PALETTE p)
{
   int c;

   for (c=0; c<256; c++) {
      _prev_current_palette[c] = _current_palette[c];
      prev_palette_color[c] = palette_color[c];
   }

   _got_prev_current_palette = TRUE;

   for (c=0; c<256; c++) {
      _current_palette[c] = p[c];

      if (_color_depth != 8)
	 palette_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
   }

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));
}



/* unselect_palette:
 *  Restores palette settings from before the last call to select_palette().
 */
void unselect_palette()
{
   int c;

   for (c=0; c<256; c++) {
      _current_palette[c] = _prev_current_palette[c];

      if (_color_depth != 8)
	 palette_color[c] = prev_palette_color[c];
   }

   _got_prev_current_palette = FALSE;

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));
}



/* _palette_expansion_table:
 *  Creates a lookup table for expanding 256->truecolor.
 */
static int *palette_expansion_table(int bpp)
{
   int *table;
   int c;

   switch (bpp) {
      case 15: table = _palette_color15; break;
      case 16: table = _palette_color16; break;
      case 24: table = _palette_color24; break;
      case 32: table = _palette_color32; break;
      default: ASSERT(FALSE); return NULL;
   }

   if (_current_palette_changed & (1<<(bpp-1))) {
      for (c=0; c<256; c++) {
	 table[c] = makecol_depth(bpp,
				  _rgb_scale_6[_current_palette[c].r], 
				  _rgb_scale_6[_current_palette[c].g], 
				  _rgb_scale_6[_current_palette[c].b]);
      }

      _current_palette_changed &= ~(1<<(bpp-1));
   } 

   return table;
}



/* this has to be called through a function pointer, so MSVC asm can use it */
int *(*_palette_expansion_table)(int) = palette_expansion_table;



/* generate_332_palette:
 *  Used when loading a truecolor image into an 8 bit bitmap, to generate
 *  a 3.3.2 RGB palette.
 */
void generate_332_palette(PALETTE pal)
{
   int c;

   for (c=0; c<256; c++) {
      pal[c].r = ((c>>5)&7) * 63/7;
      pal[c].g = ((c>>2)&7) * 63/7;
      pal[c].b = (c&3) * 63/3;
   }

   pal[0].r = 63;
   pal[0].g = 0;
   pal[0].b = 63;

   pal[254].r = pal[254].g = pal[254].b = 0;
}



/* get_color:
 *  Retrieves a single color from the palette.
 */
void get_color(int index, RGB *p)
{
   get_palette_range(p-index, index, index);
}



/* get_palette:
 *  Retrieves the entire color palette.
 */
void get_palette(PALETTE p)
{
   get_palette_range(p, 0, PAL_SIZE-1);
}



/* get_palette_range:
 *  Retrieves a part of the color palette.
 */
void get_palette_range(PALETTE p, int from, int to)
{
   int c;

   if ((system_driver) && (system_driver->read_hardware_palette))
      system_driver->read_hardware_palette();

   for (c=from; c<=to; c++)
      p[c] = _current_palette[c];
}



/* fade_interpolate: 
 *  Calculates a palette part way between source and dest, returning it
 *  in output. The pos indicates how far between the two extremes it should
 *  be: 0 = return source, 64 = return dest, 32 = return exactly half way.
 *  Only affects colors between from and to (inclusive).
 */
void fade_interpolate(AL_CONST PALETTE source, AL_CONST PALETTE dest, PALETTE output, int pos, int from, int to)
{
   int c;

   for (c=from; c<=to; c++) { 
      output[c].r = ((int)source[c].r * (63-pos) + (int)dest[c].r * pos) / 64;
      output[c].g = ((int)source[c].g * (63-pos) + (int)dest[c].g * pos) / 64;
      output[c].b = ((int)source[c].b * (63-pos) + (int)dest[c].b * pos) / 64;
   }
}



/* fade_from_range:
 *  Fades from source to dest, at the specified speed (1 is the slowest, 64
 *  is instantaneous). Only affects colors between from and to (inclusive,
 *  pass 0 and 255 to fade the entire palette).
 */
void fade_from_range(AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed, int from, int to)
{
   PALETTE temp;
   int c, start, last;

   for (c=0; c<PAL_SIZE; c++)
      temp[c] = source[c];

   if (_timer_installed) {
      start = retrace_count;
      last = -1;

      while ((c = (retrace_count-start)*speed/2) < 64) {
	 if (c != last) {
	    fade_interpolate(source, dest, temp, c, from, to);
	    set_palette_range(temp, from, to, TRUE);
	    last = c;
	 }
      }
   }
   else {
      for (c=0; c<64; c+=speed) {
	 fade_interpolate(source, dest, temp, c, from, to);
	 set_palette_range(temp, from, to, TRUE);
	 set_palette_range(temp, from, to, TRUE);
      }
   }

   set_palette_range(dest, from, to, TRUE);
}



/* fade_in_range:
 *  Fades from a solid black palette to p, at the specified speed (1 is
 *  the slowest, 64 is instantaneous). Only affects colors between from and 
 *  to (inclusive, pass 0 and 255 to fade the entire palette).
 */
void fade_in_range(AL_CONST PALETTE p, int speed, int from, int to)
{
   fade_from_range(black_palette, p, speed, from, to);
}



/* fade_out_range:
 *  Fades from the current palette to a solid black palette, at the 
 *  specified speed (1 is the slowest, 64 is instantaneous). Only affects 
 *  colors between from and to (inclusive, pass 0 and 255 to fade the 
 *  entire palette).
 */
void fade_out_range(int speed, int from, int to)
{
   PALETTE temp;

   get_palette(temp);
   fade_from_range(temp, black_palette, speed, from, to);
}



/* fade_from:
 *  Fades from source to dest, at the specified speed (1 is the slowest, 64
 *  is instantaneous).
 */
void fade_from(AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed)
{
   fade_from_range(source, dest, speed, 0, PAL_SIZE-1);
}



/* fade_in:
 *  Fades from a solid black palette to p, at the specified speed (1 is
 *  the slowest, 64 is instantaneous).
 */
void fade_in(AL_CONST PALETTE p, int speed)
{
   fade_in_range(p, speed, 0, PAL_SIZE-1);
}



/* fade_out:
 *  Fades from the current palette to a solid black palette, at the 
 *  specified speed (1 is the slowest, 64 is instantaneous).
 */
void fade_out(int speed)
{
   fade_out_range(speed, 0, PAL_SIZE-1);
}



/* rect:
 *  Draws an outline rectangle.
 */
void rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int t;

   if (x2 < x1) {
      t = x1;
      x1 = x2;
      x2 = t;
   }

   if (y2 < y1) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   acquire_bitmap(bmp);

   hline(bmp, x1, y1, x2, color);

   if (y2 > y1)
      hline(bmp, x1, y2, x2, color);

   if (y2-1 >= y1+1) {
      vline(bmp, x1, y1+1, y2-1, color);

      if (x2 > x1)
	 vline(bmp, x2, y1+1, y2-1, color);
   }

   release_bitmap(bmp);
}



/* _normal_rectfill:
 *  Draws a solid filled rectangle, using hfill() to do the work.
 */
void _normal_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int t;

   if (y1 > y2) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   if (bmp->clip) {
      if (x1 > x2) {
	 t = x1;
	 x1 = x2;
	 x2 = t;
      }

      if (x1 < bmp->cl)
	 x1 = bmp->cl;

      if (x2 >= bmp->cr)
	 x2 = bmp->cr-1;

      if (x2 < x1)
	 return;

      if (y1 < bmp->ct)
	 y1 = bmp->ct;

      if (y2 >= bmp->cb)
	 y2 = bmp->cb-1;

      if (y2 < y1)
	 return;

      bmp->clip = FALSE;
      t = TRUE;
   }
   else
      t = FALSE;

   acquire_bitmap(bmp);

   while (y1 <= y2) {
      bmp->vtable->hfill(bmp, x1, y1, x2, color);
      y1++;
   };

   release_bitmap(bmp);

   bmp->clip = t;
}



/* do_line:
 *  Calculates all the points along a line between x1, y1 and x2, y2, 
 *  calling the supplied function for each one. This will be passed a 
 *  copy of the bmp parameter, the x and y position, and a copy of the 
 *  d parameter (so do_line() can be used with putpixel()).
 */
void do_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int d, void (*proc)(BITMAP *, int, int, int))
{
   int dx = x2-x1;
   int dy = y2-y1;
   int i1, i2;
   int x, y;
   int dd;

   /* worker macro */
   #define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond)     \
   {                                                                         \
      if (d##pri_c == 0) {                                                   \
	 proc(bmp, x1, y1, d);                                               \
	 return;                                                             \
      }                                                                      \
									     \
      i1 = 2 * d##sec_c;                                                     \
      dd = i1 - (sec_sign (pri_sign d##pri_c));                              \
      i2 = dd - (sec_sign (pri_sign d##pri_c));                              \
									     \
      x = x1;                                                                \
      y = y1;                                                                \
									     \
      while (pri_c pri_cond pri_c##2) {                                      \
	 proc(bmp, x, y, d);                                                 \
									     \
	 if (dd sec_cond 0) {                                                \
	    sec_c sec_sign##= 1;                                             \
	    dd += i2;                                                        \
	 }                                                                   \
	 else                                                                \
	    dd += i1;                                                        \
									     \
	 pri_c pri_sign##= 1;                                                \
      }                                                                      \
   }

   if (dx >= 0) {
      if (dy >= 0) {
	 if (dx >= dy) {
	    /* (x1 <= x2) && (y1 <= y2) && (dx >= dy) */
	    DO_LINE(+, x, <=, +, y, >=);
	 }
	 else {
	    /* (x1 <= x2) && (y1 <= y2) && (dx < dy) */
	    DO_LINE(+, y, <=, +, x, >=);
	 }
      }
      else {
	 if (dx >= -dy) {
	    /* (x1 <= x2) && (y1 > y2) && (dx >= dy) */
	    DO_LINE(+, x, <=, -, y, <=);
	 }
	 else {
	    /* (x1 <= x2) && (y1 > y2) && (dx < dy) */
	    DO_LINE(-, y, >=, +, x, >=);
	 }
      }
   }
   else {
      if (dy >= 0) {
	 if (-dx >= dy) {
	    /* (x1 > x2) && (y1 <= y2) && (dx >= dy) */
	    DO_LINE(-, x, >=, +, y, >=);
	 }
	 else {
	    /* (x1 > x2) && (y1 <= y2) && (dx < dy) */
	    DO_LINE(+, y, <=, -, x, <=);
	 }
      }
      else {
	 if (-dx >= -dy) {
	    /* (x1 > x2) && (y1 > y2) && (dx >= dy) */
	    DO_LINE(-, x, >=, -, y, <=);
	 }
	 else {
	    /* (x1 > x2) && (y1 > y2) && (dx < dy) */
	    DO_LINE(-, y, >=, -, x, <=);
	 }
      }
   }
}



/* _normal_line:
 *  Draws a line from x1, y1 to x2, y2, using putpixel() to do the work.
 */
void _normal_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int sx, sy, dx, dy, t;

   if (x1 == x2) {
      vline(bmp, x1, y1, y2, color);
      return;
   }

   if (y1 == y2) {
      hline(bmp, x1, y1, x2, color);
      return;
   }

   /* use a bounding box to check if the line needs clipping */
   if (bmp->clip) {
      sx = x1;
      sy = y1;
      dx = x2;
      dy = y2;

      if (sx > dx) {
	 t = sx;
	 sx = dx;
	 dx = t;
      }

      if (sy > dy) {
	 t = sy;
	 sy = dy;
	 dy = t;
      }

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      t = TRUE;
   }
   else
      t= FALSE;

   acquire_bitmap(bmp);

   do_line(bmp, x1, y1, x2, y2, color, bmp->vtable->putpixel);

   release_bitmap(bmp);

   bmp->clip = t;
}



/* do_circle:
 *  Helper function for the circle drawing routines. Calculates the points
 *  in a circle of radius r around point x, y, and calls the specified 
 *  routine for each one. The output proc will be passed first a copy of
 *  the bmp parameter, then the x, y point, then a copy of the d parameter
 *  (so putpixel() can be used as the callback).
 */
void do_circle(BITMAP *bmp, int x, int y, int radius, int d, void (*proc)(BITMAP *, int, int, int))
{
   int cx = 0;
   int cy = radius;
   int df = 1 - radius; 
   int d_e = 3;
   int d_se = -2 * radius + 5;

   do {
      proc(bmp, x+cx, y+cy, d); 

      if (cx) 
	 proc(bmp, x-cx, y+cy, d); 

      if (cy) 
	 proc(bmp, x+cx, y-cy, d);

      if ((cx) && (cy)) 
	 proc(bmp, x-cx, y-cy, d); 

      if (cx != cy) {
	 proc(bmp, x+cy, y+cx, d); 

	 if (cx) 
	    proc(bmp, x+cy, y-cx, d);

	 if (cy) 
	    proc(bmp, x-cy, y+cx, d); 

	 if (cx && cy) 
	    proc(bmp, x-cy, y-cx, d); 
      }

      if (df < 0)  {
	 df += d_e;
	 d_e += 2;
	 d_se += 2;
      }
      else { 
	 df += d_se;
	 d_e += 2;
	 d_se += 4;
	 cy--;
      } 

      cx++; 

   } while (cx <= cy);
}



/* circle:
 *  Draws a circle.
 */
void circle(BITMAP *bmp, int x, int y, int radius, int color)
{
   int clip, sx, sy, dx, dy;

   if (bmp->clip) {
      sx = x-radius-1;
      sy = y-radius-1;
      dx = x+radius+1;
      dy = y+radius+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   do_circle(bmp, x, y, radius, color, bmp->vtable->putpixel);

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* circlefill:
 *  Draws a filled circle.
 */
void circlefill(BITMAP *bmp, int x, int y, int radius, int color)
{
   int cx = 0;
   int cy = radius;
   int df = 1 - radius; 
   int d_e = 3;
   int d_se = -2 * radius + 5;
   int clip, sx, sy, dx, dy;

   if (bmp->clip) {
      sx = x-radius-1;
      sy = y-radius-1;
      dx = x+radius+1;
      dy = y+radius+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   do {
      bmp->vtable->hfill(bmp, x-cy, y-cx, x+cy, color);

      if (cx)
	 bmp->vtable->hfill(bmp, x-cy, y+cx, x+cy, color);

      if (df < 0)  {
	 df += d_e;
	 d_e += 2;
	 d_se += 2;
      }
      else { 
	 if (cx != cy) {
	    bmp->vtable->hfill(bmp, x-cx, y-cy, x+cx, color);

	    if (cy)
	       bmp->vtable->hfill(bmp, x-cx, y+cy, x+cx, color);
	 }

	 df += d_se;
	 d_e += 2;
	 d_se += 4;
	 cy--;
      } 

      cx++; 

   } while (cx <= cy);

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* do_ellipse:
 *  Helper function for the ellipse drawing routines. Calculates the points
 *  in an ellipse of radius rx and ry around point x, y, and calls the 
 *  specified routine for each one. The output proc will be passed first a 
 *  copy of the bmp parameter, then the x, y point, then a copy of the d 
 *  parameter (so putpixel() can be used as the callback).
 */
void do_ellipse(BITMAP *bmp, int x, int y, int rx, int ry, int d, void (*proc)(BITMAP *, int, int, int))
{
   int ix, iy;
   int h, i, j, k;
   int oh, oi, oj, ok;

   if (rx < 1) 
      rx = 1; 

   if (ry < 1) 
      ry = 1;

   h = i = j = k = 0xFFFF;

   if (rx > ry) {
      ix = 0; 
      iy = rx * 64;

      do {
	 oh = h;
	 oi = i;
	 oj = j;
	 ok = k;

	 h = (ix + 32) >> 6; 
	 i = (iy + 32) >> 6;
	 j = (h * ry) / rx; 
	 k = (i * ry) / rx;

	 if (((h != oh) || (k != ok)) && (h < oi)) {
	    proc(bmp, x+h, y+k, d); 
	    if (h) 
	       proc(bmp, x-h, y+k, d);
	    if (k) {
	       proc(bmp, x+h, y-k, d); 
	       if (h)
		  proc(bmp, x-h, y-k, d);
	    }
	 }

	 if (((i != oi) || (j != oj)) && (h < i)) {
	    proc(bmp, x+i, y+j, d); 
	    if (i)
	       proc(bmp, x-i, y+j, d);
	    if (j) {
	       proc(bmp, x+i, y-j, d); 
	       if (i)
		  proc(bmp, x-i, y-j, d);
	    }
	 }

	 ix = ix + iy / rx; 
	 iy = iy - ix / rx;

      } while (i > h);
   } 
   else {
      ix = 0; 
      iy = ry * 64;

      do {
	 oh = h;
	 oi = i;
	 oj = j;
	 ok = k;

	 h = (ix + 32) >> 6; 
	 i = (iy + 32) >> 6;
	 j = (h * rx) / ry; 
	 k = (i * rx) / ry;

	 if (((j != oj) || (i != oi)) && (h < i)) {
	    proc(bmp, x+j, y+i, d); 
	    if (j)
	       proc(bmp, x-j, y+i, d);
	    if (i) {
	       proc(bmp, x+j, y-i, d); 
	       if (j)
		  proc(bmp, x-j, y-i, d);
	    }
	 }

	 if (((k != ok) || (h != oh)) && (h < oi)) {
	    proc(bmp, x+k, y+h, d); 
	    if (k)
	       proc(bmp, x-k, y+h, d);
	    if (h) {
	       proc(bmp, x+k, y-h, d); 
	       if (k)
		  proc(bmp, x-k, y-h, d);
	    }
	 }

	 ix = ix + iy / ry; 
	 iy = iy - ix / ry;

      } while(i > h);
   }
}



/* ellipse:
 *  Draws an ellipse.
 */
void ellipse(BITMAP *bmp, int x, int y, int rx, int ry, int color)
{
   int clip, sx, sy, dx, dy;

   if (bmp->clip) {
      sx = x-rx-1;
      sy = y-ry-1;
      dx = x+rx+1;
      dy = y+ry+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   do_ellipse(bmp, x, y, rx, ry, color, bmp->vtable->putpixel);

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* ellipsefill:
 *  Draws a filled ellipse.
 */
void ellipsefill(BITMAP *bmp, int x, int y, int rx, int ry, int color)
{
   int ix, iy;
   int a, b, c, d;
   int da, db, dc, dd;
   int na, nb, nc, nd;
   int clip, sx, sy, dx, dy;

   if (bmp->clip) {
      sx = x-rx-1;
      sy = y-ry-1;
      dx = x+rx+1;
      dy = y+ry+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   if (rx < 1)
      rx = 1;

   if (ry < 1) 
      ry = 1;

   acquire_bitmap(bmp);

   if (rx > ry) {
      dc = -1;
      dd = 0xFFFF;
      ix = 0; 
      iy = rx * 64;
      na = 0; 
      nb = (iy + 32) >> 6;
      nc = 0; 
      nd = (nb * ry) / rx;

      do {
	 a = na; 
	 b = nb; 
	 c = nc; 
	 d = nd;

	 ix = ix + (iy / rx);
	 iy = iy - (ix / rx);
	 na = (ix + 32) >> 6; 
	 nb = (iy + 32) >> 6;
	 nc = (na * ry) / rx; 
	 nd = (nb * ry) / rx;

	 if ((c > dc) && (c < dd)) {
	    bmp->vtable->hfill(bmp, x-b, y+c, x+b, color);
	    if (c)
	       bmp->vtable->hfill(bmp, x-b, y-c, x+b, color);
	    dc = c;
	 }

	 if ((d < dd) && (d > dc)) { 
	    bmp->vtable->hfill(bmp, x-a, y+d, x+a, color);
	    bmp->vtable->hfill(bmp, x-a, y-d, x+a, color);
	    dd = d;
	 }

      } while(b > a);
   } 
   else {
      da = -1;
      db = 0xFFFF;
      ix = 0; 
      iy = ry * 64; 
      na = 0; 
      nb = (iy + 32) >> 6;
      nc = 0; 
      nd = (nb * rx) / ry;

      do {
	 a = na; 
	 b = nb; 
	 c = nc; 
	 d = nd; 

	 ix = ix + (iy / ry); 
	 iy = iy - (ix / ry);
	 na = (ix + 32) >> 6; 
	 nb = (iy + 32) >> 6;
	 nc = (na * rx) / ry; 
	 nd = (nb * rx) / ry;

	 if ((a > da) && (a < db)) {
	    bmp->vtable->hfill(bmp, x-d, y+a, x+d, color); 
	    if (a)
	       bmp->vtable->hfill(bmp, x-d, y-a, x+d, color);
	    da = a;
	 }

	 if ((b < db) && (b > da)) { 
	    bmp->vtable->hfill(bmp, x-c, y+b, x+c, color);
	    bmp->vtable->hfill(bmp, x-c, y-b, x+c, color);
	    db = b;
	 }

      } while(b > a);
   }

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* do_arc:
 *  Helper function for the arc function. Calculates the points in an arc
 *  of radius r around point x, y, going anticlockwise from fixed point
 *  binary angle ang1 to ang2, and calls the specified routine for each one. 
 *  The output proc will be passed first a copy of the bmp parameter, then 
 *  the x, y point, then a copy of the d parameter (so putpixel() can be 
 *  used as the callback).
 */
void do_arc(BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int d, void (*proc)(BITMAP *, int, int, int))
{
   unsigned long rr = r*r;
   unsigned long rr1, rr2, rr3;
   int px, py;
   int ex, ey;
   int px1, px2, px3;
   int py1, py2, py3;
   int d1, d2, d3;
   int ax, ay;
   int q, qe;
   long tg_cur, tg_end;
   int done = FALSE;

   rr1 = r;
   rr2 = itofix(x);
   rr3 = itofix(y);

   /* evaluate the start point and the end point */
   px = fixtoi(rr2 + rr1 * fixcos(ang1));
   py = fixtoi(rr3 - rr1 * fixsin(ang1));
   ex = fixtoi(rr2 + rr1 * fixcos(ang2));
   ey = fixtoi(rr3 - rr1 * fixsin(ang2));

   /* start quadrant */
   if (px >= x) {
      if (py <= y)
	 q = 1;                           /* quadrant 1 */
      else
	 q = 4;                           /* quadrant 4 */
   }
   else {
      if (py < y)
	 q = 2;                           /* quadrant 2 */
      else
	 q = 3;                           /* quadrant 3 */
   }

   /* end quadrant */
   if (ex >= x) {
      if (ey <= y)
	 qe = 1;                          /* quadrant 1 */
      else
	 qe = 4;                          /* quadrant 4 */
   }
   else {
      if (ey < y)
	 qe = 2;                          /* quadrant 2 */
      else
	 qe = 3;                          /* quadrant 3 */
   }

   #define loc_tg(_y, _x)  (_x-x) ? itofix(_y-y)/(_x-x) : itofix(_y-y)

   tg_end = loc_tg(ey, ex);

   while (!done) {
      proc(bmp, px, py, d);

      /* from here, we have only 3 possible direction of movement, eg.
       * for the first quadrant:
       *
       *    OOOOOOOOO
       *    OOOOOOOOO
       *    OOOOOO21O
       *    OOOOOO3*O
       */

      /* evaluate the 3 possible points */
      switch (q) {

	 case 1:
	    px1 = px;
	    py1 = py-1;
	    px2 = px-1;
	    py2 = py-1;
	    px3 = px-1;
	    py3 = py;

	    /* 2nd quadrant check */
	    if (px != x) {
	       break;
	    }
	    else {
	       /* we were in the end quadrant, changing is illegal. Exit. */
	       if (qe == q)
		  done = TRUE;
	       q++;
	    }
	    /* fall through */

	 case 2:
	    px1 = px-1;
	    py1 = py;
	    px2 = px-1;
	    py2 = py+1;
	    px3 = px;
	    py3 = py+1;

	    /* 3rd quadrant check */
	    if (py != y) {
	       break;
	    }
	    else {
	       /* we were in the end quadrant, changing is illegal. Exit. */
	       if (qe == q)
		  done = TRUE;
	       q++;
	    }
	    /* fall through */

	 case 3:
	    px1 = px;
	    py1 = py+1;
	    px2 = px+1;
	    py2 = py+1;
	    px3 = px+1;
	    py3 = py;

	    /* 4th quadrant check */
	    if (px != x) {
	       break;
	    }
	    else {
	       /* we were in the end quadrant, changing is illegal. Exit. */
	       if (qe == q)
		  done = TRUE;
	       q++;
	    }
	    /* fall through */

	 case 4:
	    px1 = px+1;
	    py1 = py;
	    px2 = px+1;
	    py2 = py-1;
	    px3 = px;
	    py3 = py-1;

	    /* 1st quadrant check */
	    if (py == y) {
	       /* we were in the end quadrant, changing is illegal. Exit. */
	       if (qe == q)
		  done = TRUE;

	       q = 1;
	       px1 = px;
	       py1 = py-1;
	       px2 = px-1;
	       py2 = py-1;
	       px3 = px-1;
	       py3 = py;
	    }
	    break;

	 default:
	    return;
      }

      /* now, we must decide which of the 3 points is the right point.
       * We evaluate the distance from center and, then, choose the
       * nearest point.
       */
      ax = x-px1;
      ay = y-py1;
      rr1 = ax*ax + ay*ay;

      ax = x-px2;
      ay = y-py2;
      rr2 = ax*ax + ay*ay;

      ax = x-px3;
      ay = y-py3;
      rr3 = ax*ax + ay*ay;

      /* difference from the main radius */
      if (rr1 > rr)
	 d1 = rr1-rr;
      else
	 d1 = rr-rr1;
      if (rr2 > rr)
	 d2 = rr2-rr;
      else
	 d2 = rr-rr2;
      if (rr3 > rr)
	 d3 = rr3-rr;
      else
	 d3 = rr-rr3;

      /* what is the minimum? */
      if (d1 <= d2) {
	 px = px1;
	 py = py1;
      }
      else if (d2 <= d3) {
	 px = px2;
	 py = py2;
      }
      else {
	 px = px3;
	 py = py3;
      }

      /* are we in the final quadrant? */
      if (qe == q) {
	 tg_cur = loc_tg(py, px);

	 /* is the arc finished? */
	 switch (q) {

	    case 1:
	       /* end quadrant = 1? */
	       if (tg_cur <= tg_end)
		  done = TRUE;
	       break;

	    case 2:
	       /* end quadrant = 2? */
	       if (tg_cur <= tg_end)
		  done = TRUE;
	       break;

	    case 3:
	       /* end quadrant = 3? */
	       if (tg_cur <= tg_end)
		  done = TRUE;
	       break;

	    case 4:
	       /* end quadrant = 4? */
	       if (tg_cur <= tg_end)
		  done = TRUE;
	       break;
	 }
      }
   }

   /* draw the last evaluated point */
   proc(bmp, px, py, d);
}



/* arc:
 *  Draws an arc.
 */
void arc(BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color)
{
   acquire_bitmap(bmp);

   do_arc(bmp, x, y, ang1, ang2, r, color, bmp->vtable->putpixel);

   release_bitmap(bmp);
}

