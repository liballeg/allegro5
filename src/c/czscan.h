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
 *      Z-buffered polygon filler helpers (gouraud shading, tmapping, etc).
 *
 *      Original routines by Michael Bukin.
 *      Modified to support z-buffered polygon drawing by Bertrand Coconnier
 *
 *      See readme.txt for copyright information.
 */

#define ZBUF_PTR	float*

#ifndef __bma_czscan_h
#define __bma_czscan_h

/* _poly_zbuf_flat:
 *  Fills a single-color polygon scanline.
 */
void FUNC_POLY_ZBUF_FLAT(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   float z = info->z;
   unsigned long c = info->c;
   PIXEL_PTR d = (PIXEL_PTR) addr;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PUT_PIXEL(d, c);
         *zb = z;
      }
      zb++;
      z += info->dz;
   }
}



#ifdef _bma_zbuf_gcol

/* _poly_zbuf_gcol:
 *  Fills a single-color gouraud shaded polygon scanline.
 */
void FUNC_POLY_ZBUF_GCOL(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   float z = info->z;
   fixed c = info->c;
   fixed dc = info->dc;
   PIXEL_PTR d = (PIXEL_PTR) addr;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PUT_PIXEL(d, (c >> 16));
         *zb = z;
      }
      c += dc;
      zb++;
      z += info->dz;
   }
}

#endif /* _bma_zbuf_gcol */



/* _poly_zbuf_grgb:
 *  Fills an gouraud shaded polygon scanline.
 */
void FUNC_POLY_ZBUF_GRGB(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   fixed r = info->r;
   fixed g = info->g;
   fixed b = info->b;
   fixed dr = info->dr;
   fixed dg = info->dg;
   fixed db = info->db;
   PIXEL_PTR d = (PIXEL_PTR) addr;
   float z = info->z;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PUT_RGB(d, (r >> 16), (g >> 16), (b >> 16));
         *zb = z;
      }
      r += dr;
      g += dg;
      b += db;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex:
 *  Fills an affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   float z = info->z;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         PUT_PIXEL(d, color);
         *zb = z;
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_mask:
 *  Fills a masked affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_MASK(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   float z = info->z;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    PUT_PIXEL(d, color);
            *zb = z;
         }
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_lit:
 *  Fills a lit affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   float z = info->z;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_BLEND(blender, (c >> 16), color);

         PUT_PIXEL(d, color);
         *zb = z;
      }
      u += du;
      v += dv;
      c += dc;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_mask_lit:
 *  Fills a masked lit affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_MASK_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   float z = info->z;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    color = PS_BLEND(blender, (c >> 16), color);
	    PUT_PIXEL(d, color);
            *zb = z;
         }
      }
      u += du;
      v += dv;
      c += dc;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_ptex:
 *  Fills a perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         PUT_PIXEL(d, color);
         *zb = (float) fz;
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}



/* _poly_zbuf_ptex_mask:
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_MASK(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    PUT_PIXEL(d, color);
            *zb = (float) fz;
         }
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}



/* _poly_zbuf_ptex_lit:
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_BLEND(blender, (c >> 16), color);

         PUT_PIXEL(d, color);
         *zb = (float) fz;
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      c += dc;
      zb++;
   }
}



/* _poly_zbuf_ptex_mask_lit:
 *  Fills a masked lit perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_MASK_LIT(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    color = PS_BLEND(blender, (c >> 16), color);
	    PUT_PIXEL(d, color);
            *zb = (float) fz;
         }
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      c += dc;
      zb++;
   }
}



/* _poly_zbuf_atex_trans:
 *  Fills a trans affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_TRANS(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed u = info->u;
   fixed v = info->v;
   fixed du = info->du;
   fixed dv = info->dv;
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;
   PIXEL_PTR r = (PIXEL_PTR) info->read_addr;
   float z = info->z;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));

         PUT_PIXEL(d, color);
         *zb = z;
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_mask_trans:
 *  Fills a trans masked affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_MASK_TRANS(unsigned long addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask = info->vmask << info->vshift;
   int vshift = 16 - info->vshift;
   int umask = info->umask;
   fixed u = info->u;
   fixed v = info->v;
   fixed du = info->du;
   fixed dv = info->dv;
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;
   PIXEL_PTR r = (PIXEL_PTR) info->read_addr;
   float z = info->z;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
	 if (!IS_MASK(color)) {
            color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));
            PUT_PIXEL(d, color);
            *zb = z;
	 }
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_ptex_trans:
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_TRANS(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;
   PIXEL_PTR r = (PIXEL_PTR) info->read_addr;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));

         PUT_PIXEL(d, color);
         *zb = (float) fz;
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}



/* _poly_zbuf_ptex_mask_trans:
 *  Fills a trans masked perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_MASK_TRANS(unsigned long addr, int w, POLYGON_SEGMENT *info)
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
   PS_BLENDER blender = MAKE_PS_BLENDER();
   PIXEL_PTR texture = (PIXEL_PTR) (info->texture);
   PIXEL_PTR d = (PIXEL_PTR) addr;
   PIXEL_PTR r = (PIXEL_PTR) info->read_addr;
   ZBUF_PTR zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
	 if (!IS_MASK(color)) {
            color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));
            PUT_PIXEL(d, color);
            *zb = (float) fz;
	 }
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}

#endif /* !__bma_czscan_h */

