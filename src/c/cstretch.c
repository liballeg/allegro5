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
 *      Bitmap stretching functions.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



/* Information for stretching line (saved state of Bresenham line algorithm).  */
static struct
{
   int i1, i2;
   int dd, dw;
   int sinc;
} _al_stretch;


/*
 * Line stretchers.
 */
#define DECLARE_STRETCHER(type, size, set, get)                           \
int dd = _al_stretch.dd;                                                  \
type *s = (type*) sptr;                                                   \
unsigned long d = dptr;                                                   \
unsigned long dend = d + _al_stretch.dw;                                  \
for (; d < dend; d += (size), (unsigned char*) s += _al_stretch.sinc) {   \
   set(d, get(s));                                                        \
   if (dd >= 0)                                                           \
      (unsigned char*) s += (size), dd += _al_stretch.i2;                 \
   else                                                                   \
      dd += _al_stretch.i1;                                               \
}

#ifdef GFX_MODEX
/*
 * Mode-X line stretcher.
 */
static void stretch_linex(unsigned long dptr, unsigned char *sptr)
{
   int plane;
   int dw = _al_stretch.dw;
   int first_dd = _al_stretch.dd;

   for (plane = 0; plane < 4; plane++) {
      int dd = first_dd;
      unsigned char *s = sptr;
      unsigned long d = dptr / 4;
      unsigned long dend = (dptr + dw) / 4;

      outportw(0x3C4, (0x100 << (dptr & 3)) | 2);
      for (; d < dend; d++, s += 4 * _al_stretch.sinc) {
	 bmp_write8(d, *s);
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
      }

      /* Move to the beginning of next plane.  */
      if (first_dd >= 0)
	 sptr++, first_dd += _al_stretch.i2;
      else
	 first_dd += _al_stretch.i1;
      dptr++;
      sptr += _al_stretch.sinc;
      dw--;
   }
}
#endif

#ifdef ALLEGRO_COLOR8
static void stretch_line8(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_STRETCHER(unsigned char, 1, bmp_write8, *);
}
#endif

#ifdef ALLEGRO_COLOR16
static void stretch_line15(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_STRETCHER(unsigned short, sizeof(unsigned short), bmp_write15, *);
}

static void stretch_line16(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_STRETCHER(unsigned short, sizeof(unsigned short), bmp_write16, *);
}
#endif

#ifdef ALLEGRO_COLOR24
static long bmp_memory_read24(unsigned char *p)
{
   return ((*(p)) | ((*(p + 1)) << 8) | ((*(p + 2)) << 16));
}

static void stretch_line24(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_STRETCHER(unsigned char, 3, bmp_write24, bmp_memory_read24);
}
#endif

#ifdef ALLEGRO_COLOR32
static void stretch_line32(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_STRETCHER(unsigned long, sizeof(unsigned long), bmp_write32, *);
}
#endif



/*
 * Masked line stretchers.
 */
#define DECLARE_MASKED_STRETCHER(type, size, set, get, mask)              \
int dd = _al_stretch.dd;                                                  \
type *s = (type*) sptr;                                                   \
unsigned long d = dptr;                                                   \
unsigned long dend = d + _al_stretch.dw;                                  \
for (; d < dend; d += (size), (unsigned char*) s += _al_stretch.sinc) {   \
   unsigned long color = get(s);                                          \
   if (color != (mask))                                                   \
   set(d, color);                                                         \
   if (dd >= 0)                                                           \
      (unsigned char*) s += (size), dd += _al_stretch.i2;                 \
   else                                                                   \
      dd += _al_stretch.i1;                                               \
}

#ifdef GFX_MODEX
/*
 * Mode-X masked line stretcher.
 */
static void stretch_masked_linex(unsigned long dptr, unsigned char *sptr)
{
   int plane;
   int dw = _al_stretch.dw;
   int first_dd = _al_stretch.dd;

   for (plane = 0; plane < 4; plane++) {
      int dd = first_dd;
      unsigned char *s = sptr;
      unsigned long d = dptr / 4;
      unsigned long dend = (dptr + dw) / 4;

      outportw(0x3C4, (0x100 << (dptr & 3)) | 2);
      for (; d < dend; d++, s += 4 * _al_stretch.sinc) {
	 unsigned long color = *s;
	 if (color != 0)
	    bmp_write8(d, color);
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
	 if (dd >= 0) s++, dd += _al_stretch.i2;
	 else dd += _al_stretch.i1;
      }

      /* Move to the beginning of next plane.  */
      if (first_dd >= 0)
	 sptr++, first_dd += _al_stretch.i2;
      else
	 first_dd += _al_stretch.i1;
      dptr++;
      sptr += _al_stretch.sinc;
      dw--;
   }
}
#endif

#ifdef ALLEGRO_COLOR8
static void stretch_masked_line8(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned char, 1, bmp_write8, *, 0);
}
#endif

#ifdef ALLEGRO_COLOR16
static void stretch_masked_line15(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned short, sizeof(unsigned short),
			    bmp_write15, *, MASK_COLOR_15);
}

static void stretch_masked_line16(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned short, sizeof(unsigned short),
			    bmp_write16, *, MASK_COLOR_16);
}
#endif

#ifdef ALLEGRO_COLOR24
static void stretch_masked_line24(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned char, 3, bmp_write24, bmp_memory_read24, MASK_COLOR_24);
}
#endif

#ifdef ALLEGRO_COLOR32
static void stretch_masked_line32(unsigned long dptr, unsigned char *sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned long, sizeof(unsigned long), bmp_write32, *, MASK_COLOR_32);
}
#endif



/*
 * Stretch blit work-horse.
 */
static void _al_stretch_blit(BITMAP *src, BITMAP *dst,
			     int sx, int sy, int sw, int sh,
			     int dx, int dy, int dw, int dh,
			     int masked)
{
   int x, y, fixup;
   int i1, i2, dd;
   int xinc, yinc;
   int dxbeg, dxend;
   int dybeg, dyend;
   int sxofs, dxofs;
   void (*stretch_line)(unsigned long dptr, unsigned char *sptr);

   if ((sw <= 0) || (sh <= 0) || (dw <= 0) || (dh <= 0))
      return;

   if (dst->clip) {
      dybeg = ((dy > dst->ct) ? dy : dst->ct);
      dyend = (((dy + dh) < dst->cb) ? (dy + dh) : dst->cb);
      if (dybeg >= dyend)
	 return;

      dxbeg = ((dx > dst->cl) ? dx : dst->cl);
      dxend = (((dx + dw) < dst->cr) ? (dx + dw) : dst->cr);
      if (dxbeg >= dxend)
	 return;
   }
   else {
      dxbeg = dx;
      dxend = dx + dw;
      dybeg = dy;
      dyend = dy + dh;
   }

   /* Bresenham algorithm uses difference between points, not number of points.  */
   sw--;
   sh--;
   dw--;
   dh--;

   /* How much to add to src-coordinates, when moving to the next dst-coordinate.  */
   if (dw == 0)
      xinc = 0;
   else {
      xinc = sw / dw;
      sw %= dw;
   }

   if (dh == 0)
      yinc = 0;
   else {
      yinc = sh / dh;
      sh %= dh;
   }

   /* Walk in x direction until dxbeg and save Bresenham state there.  */
   i2 = (dd = (i1 = 2 * sw) - dw) - dw;
   for (x = dx, y = sx; x < dxbeg; x++, y += xinc) {
      if (dd >= 0)
	 y++, dd += i2;
      else
	 dd += i1;
   }

   /* Save Bresenham algorithm state with offset fixups.  */
   switch (bitmap_color_depth(dst)) {
      case 15:
      case 16:
	 fixup = sizeof(short);
	 break;
      case 24:
	 fixup = 3;
	 break;
      case 32:
	 fixup = sizeof(long);
	 break;
      default:
	 fixup = 1;
	 break;
   }
   sxofs = y * fixup;
   dxofs = x * fixup;
   _al_stretch.i1 = i1;
   _al_stretch.i2 = i2;
   _al_stretch.dd = dd;
   _al_stretch.dw = (dxend - dxbeg) * fixup;
   _al_stretch.sinc = xinc * fixup;

   /* Find out which stretcher should be used.  */
   if (masked) {
      switch (bitmap_color_depth(dst)) {
#ifdef ALLEGRO_COLOR8
	 case 8:
	    if (is_linear_bitmap(dst))
	       stretch_line = stretch_masked_line8;
	    else {
#ifdef GFX_MODEX
	       stretch_line = stretch_masked_linex;
#else
	       return;
#endif
	    }
	    break;
#endif
#ifdef ALLEGRO_COLOR16
	 case 15:
	    stretch_line = stretch_masked_line15;
	    break;
	 case 16:
	    stretch_line = stretch_masked_line16;
	    break;
#endif
#ifdef ALLEGRO_COLOR24
	 case 24:
	    stretch_line = stretch_masked_line24;
	    break;
#endif
#ifdef ALLEGRO_COLOR32
	 case 32:
	    stretch_line = stretch_masked_line32;
	    break;
#endif
	 default:
	    return;
      }
   }
   else {
      switch (bitmap_color_depth(dst)) {
#ifdef ALLEGRO_COLOR8
	 case 8:
	    if (is_linear_bitmap(dst))
	       stretch_line = stretch_line8;
	    else {
#ifdef GFX_MODEX
	       stretch_line = stretch_linex;
#else
	       return;
#endif
	    }
	    break;
#endif
#ifdef ALLEGRO_COLOR16
	 case 15:
	    stretch_line = stretch_line15;
	    break;
	 case 16:
	    stretch_line = stretch_line16;
	    break;
#endif
#ifdef ALLEGRO_COLOR24
	 case 24:
	    stretch_line = stretch_line24;
	    break;
#endif
#ifdef ALLEGRO_COLOR32
	 case 32:
	    stretch_line = stretch_line32;
	    break;
#endif
	 default:
	    return;
      }
   }

   /* Walk in y direction until we reach first non-clipped line.  */
   i2 = (dd = (i1 = 2 * sh) - dh) - dh;
   for (x = dy, y = sy; x < dybeg; x++, y += yinc) {
      if (dd >= 0)
	 y++, dd += i2;
      else
	 dd += i1;
   }

   /* Stretch all non-clipped lines.  */
   bmp_select(dst);
   for (; x < dyend; x++, y += yinc) {
      (*stretch_line)(bmp_write_line(dst, x) + dxofs, src->line[y] + sxofs);
      if (dd >= 0)
	 y++, dd += i2;
      else
	 dd += i1;
   }
   bmp_unwrite_line(dst);
}



/* stretch_blit:
 *  Opaque bitmap scaling function.
 */
void stretch_blit(BITMAP *src, BITMAP *dst, int sx, int sy, int sw, int sh,
		  int dx, int dy, int dw, int dh)
{
   #ifdef ALLEGRO_MPW
      if (is_system_bitmap(src) && is_system_bitmap(dst))
         system_stretch_blit(src, dst, sx, sy, sw, sh, dx, dy, dw, dh);
      else
   #endif
         _al_stretch_blit(src, dst, sx, sy, sw, sh, dx, dy, dw, dh, 0);
}



/* masked_stretch_blit:
 *  Masked bitmap scaling function.
 */
void masked_stretch_blit(BITMAP *src, BITMAP *dst, int sx, int sy, int sw, int sh,
                         int dx, int dy, int dw, int dh)
{
   _al_stretch_blit(src, dst, sx, sy, sw, sh, dx, dy, dw, dh, 1);
}



/* stretch_sprite:
 *  Masked version of stretch_blit().
 */
void stretch_sprite(BITMAP *dst, BITMAP *src, int x, int y, int w, int h)
{
   _al_stretch_blit(src, dst, 0, 0, src->w, src->h, x, y, w, h, 1);
}

