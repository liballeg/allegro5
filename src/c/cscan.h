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
 *      Polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __bma_cscan_h
#define __bma_cscan_h

/* _poly_scanline_flat:
 *  Fills a single-color polygon scanline.
 */
void FUNC_POLY_SCANLINE_FLAT(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   unsigned long c = info->c;
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      PUT_PIXEL(d, c);
   }
}



/* _poly_scanline_gcol:
 *  Fills a single-color gouraud shaded polygon scanline.
 */
void FUNC_POLY_SCANLINE_GCOL(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   fixed c = info->c;
   fixed dc = info->dc;
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      PUT_PIXEL(d, (c >> 16));
      c += dc;
   }
}



/* _poly_scanline_grgb:
 *  Fills an gouraud shaded polygon scanline.
 */
void FUNC_POLY_SCANLINE_GRGB(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   fixed r = info->r;
   fixed g = info->g;
   fixed b = info->b;
   fixed dr = info->dr;
   fixed dg = info->dg;
   fixed db = info->db;
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      PUT_RGB(d, (r >> 16), (g >> 16), (b >> 16));
      r += dr;
      g += dg;
      b += db;
   }
}



/* _poly_scanline_atex:
 *  Fills an affine texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_ATEX(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed u = info->u;
   fixed v = info->v;
   fixed du = info->du;
   fixed dv = info->dv;
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);

      PUT_PIXEL(d, color);
      u += du;
      v += dv;
   }
}



/* _poly_scanline_atex_mask:
 *  Fills a masked affine texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_ATEX_MASK(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed u = info->u;
   fixed v = info->v;
   fixed du = info->du;
   fixed dv = info->dv;
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);

      if (!IS_MASK(color)) {
	 PUT_PIXEL(d, color);
      }
      u += du;
      v += dv;
   }
}



/* _poly_scanline_atex_lit:
 *  Fills a lit affine texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_ATEX_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed u = info->u;
   fixed v = info->v;
   fixed c = info->c;
   fixed du = info->du;
   fixed dv = info->dv;
   fixed dc = info->dc;
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);
      color = PS_BLEND(blender, (c >> 16), color);

      PUT_PIXEL(d, color);
      u += du;
      v += dv;
      c += dc;
   }
}



/* _poly_scanline_atex_mask_lit:
 *  Fills a masked lit affine texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_ATEX_MASK_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed u = info->u;
   fixed v = info->v;
   fixed c = info->c;
   fixed du = info->du;
   fixed dv = info->dv;
   fixed dc = info->dc;
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);

      if (!IS_MASK(color)) {
	 color = PS_BLEND(blender, (c >> 16), color);
	 PUT_PIXEL(d, color);
      }
      u += du;
      v += dv;
      c += dc;
   }
}



/* _poly_scanline_ptex:
 *  Fills a perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_PTEX(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   double fu = info->fu;
   double fv = info->fv;
   double fz = info->z;
   double dfu = info->dfu;
   double dfv = info->dfv;
   double dfz = info->dz;
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      long u = fu / fz;
      long v = fv / fz;
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);

      PUT_PIXEL(d, color);
      fu += dfu;
      fv += dfv;
      fz += dfz;
   }
}



/* _poly_scanline_ptex_mask:
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_PTEX_MASK(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   double fu = info->fu;
   double fv = info->fv;
   double fz = info->z;
   double dfu = info->dfu;
   double dfv = info->dfv;
   double dfz = info->dz;
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      long u = fu / fz;
      long v = fv / fz;
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);

      if (!IS_MASK(color)) {
	 PUT_PIXEL(d, color);
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
   }
}



/* _poly_scanline_ptex_lit:
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_PTEX_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed c = info->c;
   fixed dc = info->dc;
   double fu = info->fu;
   double fv = info->fv;
   double fz = info->z;
   double dfu = info->dfu;
   double dfv = info->dfv;
   double dfz = info->dz;
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      long u = fu / fz;
      long v = fv / fz;
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);
      color = PS_BLEND(blender, (c >> 16), color);

      PUT_PIXEL(d, color);
      fu += dfu;
      fv += dfv;
      fz += dfz;
      c += dc;
   }
}



/* _poly_scanline_ptex_mask_lit:
 *  Fills a masked lit perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_SCANLINE_PTEX_MASK_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed c = info->c;
   fixed dc = info->dc;
   double fu = info->fu;
   double fv = info->fv;
   double fz = info->z;
   double dfu = info->dfu;
   double dfv = info->dfv;
   double dfz = info->dz;
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      long u = fu / fz;
      long v = fv / fz;
      PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
      unsigned long color = GET_MEMORY_PIXEL(s);

      if (!IS_MASK(color)) {
	 color = PS_BLEND(blender, (c >> 16), color);
	 PUT_PIXEL(d, color);
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      c += dc;
   }
}

#endif /* !__bma_cscan_h */

