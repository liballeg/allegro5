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
 *      The 3d polygon rasteriser.
 *
 *      By Shawn Hargreaves.
 *
 *      Hicolor support added by Przemek Podsiadly. Complete support for
 *      all drawing modes in every color depth and MMX optimisations
 *      added by Calin Andrian.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>
#include <float.h>

#include "allegro.h"
#include "allegro/aintern.h"

#ifdef ALLEGRO_MMX_HEADER
   #include ALLEGRO_MMX_HEADER
#endif

#ifdef ALLEGRO_USE_C
   #undef ALLEGRO_MMX
#endif


#ifdef ALLEGRO_MMX

/* for use by iscan.s */
unsigned long _mask_mmx_15[] = { 0x03E0001F, 0x007C };
unsigned long _mask_mmx_16[] = { 0x07E0001F, 0x00F8 };

#endif


SCANLINE_FILLER _optim_alternative_drawer;

void _poly_scanline_dummy(unsigned long addr, int w, POLYGON_SEGMENT *info) { }



/* _fill_3d_edge_structure:
 *  Polygon helper function: initialises an edge structure for the 3d 
 *  rasterising code, using fixed point vertex structures.
 */
void _fill_3d_edge_structure(POLYGON_EDGE *edge, V3D *v1, V3D *v2, int flags, BITMAP *bmp)
{
   int h, r1, r2, g1, g2, b1, b2;

   /* swap vertices if they are the wrong way up */
   if (v2->y < v1->y) {
      V3D *vt;

      vt = v1;
      v1 = v2;
      v2 = vt;
   }

   /* set up screen rasterising parameters */
   edge->top = fixtoi(v1->y);
   edge->bottom = fixtoi(v2->y) - 1;
   h = edge->bottom - edge->top + 1;
   edge->dx = ((v2->x - v1->x) << (POLYGON_FIX_SHIFT-16)) / h;
   edge->x = (v1->x << (POLYGON_FIX_SHIFT-16)) + (1<<(POLYGON_FIX_SHIFT-1)) - 1;
   edge->prev = NULL;
   edge->next = NULL;
   edge->w = 0;

   if (flags & INTERP_1COL) {
      /* single color shading interpolation */
      edge->dat.c = itofix(v1->c);
      edge->dat.dc = itofix(v2->c - v1->c) / h;
   }

   if (flags & INTERP_3COL) {
      /* RGB shading interpolation */
      if (flags & COLOR_TO_RGB) {
	 r1 = getr_depth(bitmap_color_depth(bmp), v1->c);
	 r2 = getr_depth(bitmap_color_depth(bmp), v2->c);
	 g1 = getg_depth(bitmap_color_depth(bmp), v1->c);
	 g2 = getg_depth(bitmap_color_depth(bmp), v2->c);
	 b1 = getb_depth(bitmap_color_depth(bmp), v1->c);
	 b2 = getb_depth(bitmap_color_depth(bmp), v2->c);
      } 
      else {
	 r1 = (v1->c >> 16) & 0xFF;
	 r2 = (v2->c >> 16) & 0xFF;
	 g1 = (v1->c >> 8) & 0xFF;
	 g2 = (v2->c >> 8) & 0xFF;
	 b1 = v1->c & 0xFF;
	 b2 = v2->c & 0xFF;
      }

      edge->dat.r = itofix(r1);
      edge->dat.g = itofix(g1);
      edge->dat.b = itofix(b1);
      edge->dat.dr = itofix(r2 - r1) / h;
      edge->dat.dg = itofix(g2 - g1) / h;
      edge->dat.db = itofix(b2 - b1) / h;
   }

   if (flags & INTERP_FIX_UV) {
      /* fixed point (affine) texture interpolation */
      edge->dat.u = v1->u;
      edge->dat.v = v1->v;
      edge->dat.du = (v2->u - v1->u) / h;
      edge->dat.dv = (v2->v - v1->v) / h;
   }

   if (flags & INTERP_Z) {
      /* Z (depth) interpolation */
      float z1 = 1.0 / fixtof(v1->z);
      float z2 = 1.0 / fixtof(v2->z);

      edge->dat.z = z1;
      edge->dat.dz = (z2 - z1) / h;

      if (flags & INTERP_FLOAT_UV) {
	 /* floating point (perspective correct) texture interpolation */
	 float fu1 = fixtof(v1->u + (1<<15)) * z1 * 65536;
	 float fv1 = fixtof(v1->v + (1<<15)) * z1 * 65536;
	 float fu2 = fixtof(v2->u + (1<<15)) * z2 * 65536;
	 float fv2 = fixtof(v2->v + (1<<15)) * z2 * 65536;

	 edge->dat.fu = fu1;
	 edge->dat.fv = fv1;
	 edge->dat.dfu = (fu2 - fu1) / h;
	 edge->dat.dfv = (fv2 - fv1) / h;
      }
   }
}



/* _fill_3d_edge_structure_f:
 *  Polygon helper function: initialises an edge structure for the 3d 
 *  rasterising code, using floating point vertex structures.
 */
void _fill_3d_edge_structure_f(POLYGON_EDGE *edge, V3D_f *v1, V3D_f *v2, int flags, BITMAP *bmp)
{
   int h, r1, r2, g1, g2, b1, b2;

   /* swap vertices if they are the wrong way up */
   if (v2->y < v1->y) {
      V3D_f *vt;

      vt = v1;
      v1 = v2;
      v2 = vt;
   }

   /* set up screen rasterising parameters */
   edge->top = (int)(v1->y+0.5);
   edge->bottom = (int)(v2->y+0.5) - 1;
   h = edge->bottom - edge->top + 1;
   edge->dx = ((v2->x - v1->x) * (float)(1<<POLYGON_FIX_SHIFT)) / h;
   edge->x = (v1->x * (float)(1<<POLYGON_FIX_SHIFT)) + (1<<(POLYGON_FIX_SHIFT-1)) - 1;
   edge->prev = NULL;
   edge->next = NULL;
   edge->w = 0;

   if (flags & INTERP_1COL) {
      /* single color shading interpolation */
      edge->dat.c = itofix(v1->c);
      edge->dat.dc = itofix(v2->c - v1->c) / h;
   }

   if (flags & INTERP_3COL) {
      /* RGB shading interpolation */
      if (flags & COLOR_TO_RGB) {
	 r1 = getr_depth(bitmap_color_depth(bmp), v1->c);
	 r2 = getr_depth(bitmap_color_depth(bmp), v2->c);
	 g1 = getg_depth(bitmap_color_depth(bmp), v1->c);
	 g2 = getg_depth(bitmap_color_depth(bmp), v2->c);
	 b1 = getb_depth(bitmap_color_depth(bmp), v1->c);
	 b2 = getb_depth(bitmap_color_depth(bmp), v2->c);
      } 
      else {
	 r1 = (v1->c >> 16) & 0xFF;
	 r2 = (v2->c >> 16) & 0xFF;
	 g1 = (v1->c >> 8) & 0xFF;
	 g2 = (v2->c >> 8) & 0xFF;
	 b1 = v1->c & 0xFF;
	 b2 = v2->c & 0xFF;
      }

      edge->dat.r = itofix(r1);
      edge->dat.g = itofix(g1);
      edge->dat.b = itofix(b1);
      edge->dat.dr = itofix(r2 - r1) / h;
      edge->dat.dg = itofix(g2 - g1) / h;
      edge->dat.db = itofix(b2 - b1) / h;
   }

   if (flags & INTERP_FIX_UV) {
      /* fixed point (affine) texture interpolation */
      edge->dat.u = ftofix(v1->u);
      edge->dat.v = ftofix(v1->v);
      edge->dat.du = ftofix(v2->u - v1->u) / h;
      edge->dat.dv = ftofix(v2->v - v1->v) / h;
   }

   if (flags & INTERP_Z) {
      /* Z (depth) interpolation */
      float z1 = 1.0 / v1->z;
      float z2 = 1.0 / v2->z;

      edge->dat.z = z1;
      edge->dat.dz = (z2 - z1) / h;

      if (flags & INTERP_FLOAT_UV) {
	 /* floating point (perspective correct) texture interpolation */
	 float fu1 = (v1->u + 0.5) * z1 * 65536;
	 float fv1 = (v1->v + 0.5) * z1 * 65536;
	 float fu2 = (v2->u + 0.5) * z2 * 65536;
	 float fv2 = (v2->v + 0.5) * z2 * 65536;

	 edge->dat.fu = fu1;
	 edge->dat.fv = fv1;
	 edge->dat.dfu = (fu2 - fu1) / h;
	 edge->dat.dfv = (fv2 - fv1) / h;
      }
   }
}



/* _get_scanline_filler:
 *  Helper function for deciding which rasterisation function and 
 *  interpolation flags we should use for a specific polygon type.
 */
SCANLINE_FILLER _get_scanline_filler(int type, int *flags, POLYGON_SEGMENT *info, BITMAP *texture, BITMAP *bmp)
{
   typedef struct POLYTYPE_INFO 
   {
      SCANLINE_FILLER filler;
      SCANLINE_FILLER alternative;
   } POLYTYPE_INFO;

   int polytype_interp_pal[] = 
   {
      INTERP_FLAT,
      INTERP_1COL,
      INTERP_3COL,
      INTERP_FIX_UV,
      INTERP_Z | INTERP_FLOAT_UV | OPT_FLOAT_UV_TO_FIX,
      INTERP_FIX_UV,
      INTERP_Z | INTERP_FLOAT_UV | OPT_FLOAT_UV_TO_FIX,
      INTERP_FIX_UV | INTERP_1COL,
      INTERP_Z | INTERP_FLOAT_UV | INTERP_1COL | OPT_FLOAT_UV_TO_FIX,
      INTERP_FIX_UV | INTERP_1COL,
      INTERP_Z | INTERP_FLOAT_UV | INTERP_1COL | OPT_FLOAT_UV_TO_FIX
   };

   int polytype_interp_tc[] = 
   {
      INTERP_FLAT,
      INTERP_3COL | COLOR_TO_RGB,
      INTERP_3COL,
      INTERP_FIX_UV,
      INTERP_Z | INTERP_FLOAT_UV | OPT_FLOAT_UV_TO_FIX,
      INTERP_FIX_UV,
      INTERP_Z | INTERP_FLOAT_UV | OPT_FLOAT_UV_TO_FIX,
      INTERP_FIX_UV | INTERP_1COL,
      INTERP_Z | INTERP_FLOAT_UV | INTERP_1COL | OPT_FLOAT_UV_TO_FIX,
      INTERP_FIX_UV | INTERP_1COL,
      INTERP_Z | INTERP_FLOAT_UV | INTERP_1COL | OPT_FLOAT_UV_TO_FIX
   };

   #ifdef ALLEGRO_COLOR8
   static POLYTYPE_INFO polytype_info8[] =
   {
      {  _poly_scanline_dummy,          NULL },
      {  _poly_scanline_gcol8,          NULL },
      {  _poly_scanline_grgb8,          NULL },
      {  _poly_scanline_atex8,          NULL },
      {  _poly_scanline_ptex8,          _poly_scanline_atex8 },
      {  _poly_scanline_atex_mask8,     NULL },
      {  _poly_scanline_ptex_mask8,     _poly_scanline_atex_mask8 },
      {  _poly_scanline_atex_lit8,      NULL },
      {  _poly_scanline_ptex_lit8,      _poly_scanline_atex_lit8 },
      {  _poly_scanline_atex_mask_lit8, NULL },
      {  _poly_scanline_ptex_mask_lit8, _poly_scanline_atex_mask_lit8 }
   };

   #ifdef ALLEGRO_MMX
   static POLYTYPE_INFO polytype_info8x[] =
   {
      {  NULL,                    NULL },
      {  NULL,                    NULL },
      {  _poly_scanline_grgb8x,   NULL },
      {  NULL,                    NULL },
      {  NULL,                    NULL },
      {  NULL,                    NULL },
      {  NULL,                    NULL },
      {  NULL,                    NULL },
      {  NULL,                    NULL },
      {  NULL,                    NULL },
      {  NULL,                    NULL }
   };

   static POLYTYPE_INFO polytype_info8d[] =
   {
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL },
      {  NULL,                           NULL }
   };
   #endif
   #endif

   #ifdef ALLEGRO_COLOR16
   static POLYTYPE_INFO polytype_info15[] =
   {
      {  _poly_scanline_dummy,           NULL },
      {  _poly_scanline_grgb15,          NULL },
      {  _poly_scanline_grgb15,          NULL },
      {  _poly_scanline_atex16,          NULL },
      {  _poly_scanline_ptex16,          _poly_scanline_atex16 },
      {  _poly_scanline_atex_mask15,     NULL },
      {  _poly_scanline_ptex_mask15,     _poly_scanline_atex_mask15 },
      {  _poly_scanline_atex_lit15,      NULL },
      {  _poly_scanline_ptex_lit15,      _poly_scanline_atex_lit15 },
      {  _poly_scanline_atex_mask_lit15, NULL },
      {  _poly_scanline_ptex_mask_lit15, _poly_scanline_atex_mask_lit15 }
   };

   #ifdef ALLEGRO_MMX
   static POLYTYPE_INFO polytype_info15x[] =
   {
      {  NULL,                            NULL },
      {  _poly_scanline_grgb15x,          NULL },
      {  _poly_scanline_grgb15x,          NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_atex_lit15x,      NULL },
      {  _poly_scanline_ptex_lit15x,      _poly_scanline_atex_lit15x },
      {  _poly_scanline_atex_mask_lit15x, NULL },
      {  _poly_scanline_ptex_mask_lit15x, _poly_scanline_atex_mask_lit15x }
   };

   static POLYTYPE_INFO polytype_info15d[] =
   {
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_lit15d,      _poly_scanline_atex_lit15x },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_mask_lit15d, _poly_scanline_atex_mask_lit15x }
   };
   #endif

   static POLYTYPE_INFO polytype_info16[] =
   {
      {  _poly_scanline_dummy,           NULL },
      {  _poly_scanline_grgb16,          NULL },
      {  _poly_scanline_grgb16,          NULL },
      {  _poly_scanline_atex16,          NULL },
      {  _poly_scanline_ptex16,          _poly_scanline_atex16 },
      {  _poly_scanline_atex_mask16,     NULL },
      {  _poly_scanline_ptex_mask16,     _poly_scanline_atex_mask16 },
      {  _poly_scanline_atex_lit16,      NULL },
      {  _poly_scanline_ptex_lit16,      _poly_scanline_atex_lit16 },
      {  _poly_scanline_atex_mask_lit16, NULL },
      {  _poly_scanline_ptex_mask_lit16, _poly_scanline_atex_mask_lit16 }
   };

   #ifdef ALLEGRO_MMX
   static POLYTYPE_INFO polytype_info16x[] =
   {
      {  NULL,                            NULL },
      {  _poly_scanline_grgb16x,          NULL },
      {  _poly_scanline_grgb16x,          NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_atex_lit16x,      NULL },
      {  _poly_scanline_ptex_lit16x,      _poly_scanline_atex_lit16x },
      {  _poly_scanline_atex_mask_lit16x, NULL },
      {  _poly_scanline_ptex_mask_lit16x, _poly_scanline_atex_mask_lit16x }
   };

   static POLYTYPE_INFO polytype_info16d[] =
   {
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_lit16d,      _poly_scanline_atex_lit16x },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_mask_lit16d, _poly_scanline_atex_mask_lit16x }
   };
   #endif
   #endif

   #ifdef ALLEGRO_COLOR24
   static POLYTYPE_INFO polytype_info24[] =
   {
      {  _poly_scanline_dummy,           NULL },
      {  _poly_scanline_grgb24,          NULL },
      {  _poly_scanline_grgb24,          NULL },
      {  _poly_scanline_atex24,          NULL },
      {  _poly_scanline_ptex24,          _poly_scanline_atex24 },
      {  _poly_scanline_atex_mask24,     NULL },
      {  _poly_scanline_ptex_mask24,     _poly_scanline_atex_mask24 },
      {  _poly_scanline_atex_lit24,      NULL },
      {  _poly_scanline_ptex_lit24,      _poly_scanline_atex_lit24 },
      {  _poly_scanline_atex_mask_lit24, NULL },
      {  _poly_scanline_ptex_mask_lit24, _poly_scanline_atex_mask_lit24 }
   };

   #ifdef ALLEGRO_MMX
   static POLYTYPE_INFO polytype_info24x[] =
   {
      {  NULL,                            NULL },
      {  _poly_scanline_grgb24x,          NULL },
      {  _poly_scanline_grgb24x,          NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_atex_lit24x,      NULL },
      {  _poly_scanline_ptex_lit24x,      _poly_scanline_atex_lit24x },
      {  _poly_scanline_atex_mask_lit24x, NULL },
      {  _poly_scanline_ptex_mask_lit24x, _poly_scanline_atex_mask_lit24x }
   };

   static POLYTYPE_INFO polytype_info24d[] =
   {
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_lit24d,      _poly_scanline_atex_lit24x },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_mask_lit24d, _poly_scanline_atex_mask_lit24x }
   };
   #endif
   #endif

   #ifdef ALLEGRO_COLOR32
   static POLYTYPE_INFO polytype_info32[] =
   {
      {  _poly_scanline_dummy,           NULL },
      {  _poly_scanline_grgb32,          NULL },
      {  _poly_scanline_grgb32,          NULL },
      {  _poly_scanline_atex32,          NULL },
      {  _poly_scanline_ptex32,          _poly_scanline_atex32 },
      {  _poly_scanline_atex_mask32,     NULL },
      {  _poly_scanline_ptex_mask32,     _poly_scanline_atex_mask32 },
      {  _poly_scanline_atex_lit32,      NULL },
      {  _poly_scanline_ptex_lit32,      _poly_scanline_atex_lit32 },
      {  _poly_scanline_atex_mask_lit32, NULL },
      {  _poly_scanline_ptex_mask_lit32, _poly_scanline_atex_mask_lit32 }
   };

   #ifdef ALLEGRO_MMX
   static POLYTYPE_INFO polytype_info32x[] =
   {
      {  NULL,                            NULL },
      {  _poly_scanline_grgb32x,          NULL },
      {  _poly_scanline_grgb32x,          NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_atex_lit32x,      NULL },
      {  _poly_scanline_ptex_lit32x,      _poly_scanline_atex_lit32x },
      {  _poly_scanline_atex_mask_lit32x, NULL },
      {  _poly_scanline_ptex_mask_lit32x, _poly_scanline_atex_mask_lit32x }
   };

   static POLYTYPE_INFO polytype_info32d[] =
   {
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_lit32d,      _poly_scanline_atex_lit32x },
      {  NULL,                            NULL },
      {  _poly_scanline_ptex_mask_lit32d, _poly_scanline_atex_mask_lit32x }
   };
   #endif
   #endif

   int *interpinfo;
   POLYTYPE_INFO *typeinfo;

   #ifdef ALLEGRO_MMX
   POLYTYPE_INFO *typeinfo_mmx, *typeinfo_3d;
   #endif

   switch (bitmap_color_depth(bmp)) {

      #ifdef ALLEGRO_COLOR8

	 case 8:
	    interpinfo = polytype_interp_pal;
	    typeinfo = polytype_info8;
	 #ifdef ALLEGRO_MMX
	    typeinfo_mmx = polytype_info8x;
	    typeinfo_3d = polytype_info8d;
	 #endif
	    break;

      #endif

      #ifdef ALLEGRO_COLOR16

	 case 15:
	    interpinfo = polytype_interp_tc;
	    typeinfo = polytype_info15;
	 #ifdef ALLEGRO_MMX
	    typeinfo_mmx = polytype_info15x;
	    typeinfo_3d = polytype_info15d;
	 #endif
	    break;

	 case 16:
	    interpinfo = polytype_interp_tc;
	    typeinfo = polytype_info16;
	 #ifdef ALLEGRO_MMX
	    typeinfo_mmx = polytype_info16x;
	    typeinfo_3d = polytype_info16d;
	 #endif
	    break;

      #endif

      #ifdef ALLEGRO_COLOR24

	 case 24:
	    interpinfo = polytype_interp_tc;
	    typeinfo = polytype_info24;
	 #ifdef ALLEGRO_MMX
	    typeinfo_mmx = polytype_info24x;
	    typeinfo_3d = polytype_info24d;
	 #endif
	    break;

      #endif

      #ifdef ALLEGRO_COLOR32

	 case 32:
	    interpinfo = polytype_interp_tc;
	    typeinfo = polytype_info32;
	 #ifdef ALLEGRO_MMX
	    typeinfo_mmx = polytype_info32x;
	    typeinfo_3d = polytype_info32d;
	 #endif
	    break;

      #endif

      default:
	 return NULL;
   }

   type = MID(0, type, POLYTYPE_MAX-1);
   *flags = interpinfo[type];

   if (texture) {
      info->texture = texture->line[0];
      info->umask = texture->w - 1;
      info->vmask = texture->h - 1;
      info->vshift = 0;
      while ((1 << info->vshift) < texture->w)
	 info->vshift++;
   }
   else {
      info->texture = NULL;
      info->umask = info->vmask = info->vshift = 0;
   }

   info->seg = bmp->seg;
   bmp_select(bmp);

   #ifdef ALLEGRO_MMX
   if ((cpu_mmx) && (typeinfo_mmx[type].filler)) {
      if ((cpu_3dnow) && (typeinfo_3d[type].filler)) {
	 _optim_alternative_drawer = typeinfo_3d[type].alternative;
	 return typeinfo_3d[type].filler;
      }
      _optim_alternative_drawer = typeinfo_mmx[type].alternative;
      return typeinfo_mmx[type].filler;
   }
   #endif

   _optim_alternative_drawer = typeinfo[type].alternative;

   return typeinfo[type].filler;
}



/* _clip_polygon_segment:
 *  Updates interpolation state values when skipping several places, eg.
 *  clipping the first part of a scanline.
 */
void _clip_polygon_segment(POLYGON_SEGMENT *info, int gap, int flags)
{
   if (flags & INTERP_1COL)
      info->c += info->dc * gap;

   if (flags & INTERP_3COL) {
      info->r += info->dr * gap;
      info->g += info->dg * gap;
      info->b += info->db * gap;
   }

   if (flags & INTERP_FIX_UV) {
      info->u += info->du * gap;
      info->v += info->dv * gap;
   }

   if (flags & INTERP_Z) {
      info->z += info->dz * gap;

      if (flags & INTERP_FLOAT_UV) {
	 info->fu += info->dfu * gap;
	 info->fv += info->dfv * gap;
      }
   }
}



/* draw_polygon_segment: 
 *  Polygon helper function to fill a scanline. Calculates deltas for 
 *  whichever values need interpolating, clips the segment, and then calls
 *  the lowlevel scanline filler.
 */
static void draw_polygon_segment(BITMAP *bmp, int y, POLYGON_EDGE *e1, POLYGON_EDGE *e2, SCANLINE_FILLER drawer, int flags, int color, POLYGON_SEGMENT *info)
{
   int x = e1->x >> POLYGON_FIX_SHIFT;
   int w = (e2->x >> POLYGON_FIX_SHIFT) - x;
   int gap;

   if ((w <= 0) || (x+w <= bmp->cl) || (x >= bmp->cr))
      return;

   if (flags & INTERP_FLAT) {
      hline(bmp, x, y, x+w, color);
      return;
   } 
   else if (flags & INTERP_1COL) {
      info->c = e1->dat.c;
      info->dc = (e2->dat.c - e1->dat.c) / w;
   }

   if (flags & INTERP_3COL) {
      info->r = e1->dat.r;
      info->g = e1->dat.g;
      info->b = e1->dat.b;
      info->dr = (e2->dat.r - e1->dat.r) / w;
      info->dg = (e2->dat.g - e1->dat.g) / w;
      info->db = (e2->dat.b - e1->dat.b) / w;
   }

   if (flags & INTERP_FIX_UV) {
      info->u = e1->dat.u + (1<<15);
      info->v = e1->dat.v + (1<<15);
      info->du = (e2->dat.u - e1->dat.u) / w;
      info->dv = (e2->dat.v - e1->dat.v) / w;
   }

   if (flags & INTERP_Z) {
      info->z = e1->dat.z;
      info->dz = (e2->dat.z - e1->dat.z) / w;

      if (flags & INTERP_FLOAT_UV) {
	 info->fu = e1->dat.fu;
	 info->fv = e1->dat.fv;
	 info->dfu = (e2->dat.fu - e1->dat.fu) / w;
	 info->dfv = (e2->dat.fv - e1->dat.fv) / w;
      }
   }

   if (x < bmp->cl) {
      gap = bmp->cl - x;
      x = bmp->cl;
      w -= gap;
      _clip_polygon_segment(info, gap, flags);
   }

   if (x+w > bmp->cr)
      w = bmp->cr - x;

   if ((flags & OPT_FLOAT_UV_TO_FIX) && (info->dz == 0)) {
      info->u = info->fu/info->z;
      info->v = info->fv/info->z;
      info->du = info->dfu/info->z;
      info->dv = info->dfv/info->z;
      drawer = _optim_alternative_drawer;
   }

   drawer(bmp_write_line(bmp, y) + x * BYTES_PER_PIXEL(bitmap_color_depth(bmp)), w, info);
}



/* do_polygon3d:
 *  Helper function for rendering 3d polygon, used by both the fixed point
 *  and floating point drawing functions.
 */
static void do_polygon3d(BITMAP *bmp, int top, int bottom, POLYGON_EDGE *inactive_edges, SCANLINE_FILLER drawer, int flags, int color, POLYGON_SEGMENT *info)
{
   int y;
 #ifdef ALLEGRO_DOS
   int old87 = 0;
 #endif
   POLYGON_EDGE *edge, *next_edge;
   POLYGON_EDGE *active_edges = NULL;

   if (bottom >= bmp->cb)
      bottom = bmp->cb-1;

   /* set fpu to single-precision, truncate mode */
 #ifdef ALLEGRO_DOS
   if (flags & (INTERP_Z | INTERP_FLOAT_UV))
      old87 = _control87(PC_24 | RC_CHOP, MCW_PC | MCW_RC);
 #endif

   acquire_bitmap(bmp);

   /* for each scanline in the polygon... */
   for (y=top; y<=bottom; y++) {

      /* check for newly active edges */
      edge = inactive_edges;
      while ((edge) && (edge->top == y)) {
	 next_edge = edge->next;
	 inactive_edges = _remove_edge(inactive_edges, edge);
	 active_edges = _add_edge(active_edges, edge, TRUE);
	 edge = next_edge;
      }

      /* fill the scanline */
      draw_polygon_segment(bmp, y, active_edges, active_edges->next, drawer, flags, color, info);

      /* update edges, removing dead ones */
      edge = active_edges;
      while (edge) {
	 next_edge = edge->next;
	 if (y >= edge->bottom) {
	    active_edges = _remove_edge(active_edges, edge);
	 }
	 else {
	    edge->x += edge->dx;

	    if (flags & INTERP_1COL)
	       edge->dat.c += edge->dat.dc;

	    if (flags & INTERP_3COL) {
	       edge->dat.r += edge->dat.dr;
	       edge->dat.g += edge->dat.dg;
	       edge->dat.b += edge->dat.db;
	    }

	    if (flags & INTERP_FIX_UV) {
	       edge->dat.u += edge->dat.du;
	       edge->dat.v += edge->dat.dv;
	    }

	    if (flags & INTERP_Z) {
	       edge->dat.z += edge->dat.dz;

	       if (flags & INTERP_FLOAT_UV) {
		  edge->dat.fu += edge->dat.dfu;
		  edge->dat.fv += edge->dat.dfv;
	       }
	    }
	 }

	 edge = next_edge;
      }
   }

   bmp_unwrite_line(bmp);
   release_bitmap(bmp);

   /* reset fpu mode */
 #ifdef ALLEGRO_DOS
   if (flags & (INTERP_Z | INTERP_FLOAT_UV))
      _control87(old87, MCW_PC | MCW_RC);
 #endif
}



/* polygon3d:
 *  Draws a 3d polygon in the specified mode. The vertices parameter should
 *  be followed by that many pointers to V3D structures, which describe each
 *  vertex of the polygon.
 */
void polygon3d(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D *vtx[])
{
   int c;
   int flags;
   int top = INT_MAX;
   int bottom = INT_MIN;
   int v1y, v2y;
   V3D *v1, *v2;
   POLYGON_EDGE *edge;
   POLYGON_EDGE *inactive_edges = NULL;
   POLYGON_SEGMENT info;
   SCANLINE_FILLER drawer;

   /* set up the drawing mode */
   drawer = _get_scanline_filler(type, &flags, &info, texture, bmp);
   if (!drawer)
      return;

   /* allocate some space for the active edge table */
   _grow_scratch_mem(sizeof(POLYGON_EDGE) * vc);
   edge = (POLYGON_EDGE *)_scratch_mem;

   v2 = vtx[vc-1];

   /* fill the edge table */
   for (c=0; c<vc; c++) {
      v1 = v2;
      v2 = vtx[c];
      v1y = fixtoi(v1->y);
      v2y = fixtoi(v2->y);

      if ((v1y != v2y) && 
	  ((v1y >= bmp->ct) || (v2y >= bmp->ct)) &&
	  ((v1y < bmp->cb) || (v2y < bmp->cb))) {

	 _fill_3d_edge_structure(edge, v1, v2, flags, bmp);

	 if (edge->top < bmp->ct) {
	    int gap = bmp->ct - edge->top;
	    edge->top = bmp->ct;
	    edge->x += edge->dx * gap;
	    _clip_polygon_segment(&edge->dat, gap, flags);
	 }

	 if (edge->bottom >= edge->top) {

	    if (edge->top < top)
	       top = edge->top;

	    if (edge->bottom > bottom)
	       bottom = edge->bottom;

	    inactive_edges = _add_edge(inactive_edges, edge, FALSE);
	    edge++;
	 }
      }
   }

   /* render the polygon */
   do_polygon3d(bmp, top, bottom, inactive_edges, drawer, flags, vtx[0]->c, &info);
}



/* polygon3d_f:
 *  Floating point version of polygon3d().
 */
void polygon3d_f(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D_f *vtx[])
{
   int c;
   int flags;
   int top = INT_MAX;
   int bottom = INT_MIN;
   int v1y, v2y;
   V3D_f *v1, *v2;
   POLYGON_EDGE *edge;
   POLYGON_EDGE *inactive_edges = NULL;
   POLYGON_SEGMENT info;
   SCANLINE_FILLER drawer;

   /* set up the drawing mode */
   drawer = _get_scanline_filler(type, &flags, &info, texture, bmp);
   if (!drawer)
      return;

   /* allocate some space for the active edge table */
   _grow_scratch_mem(sizeof(POLYGON_EDGE) * vc);
   edge = (POLYGON_EDGE *)_scratch_mem;

   v2 = vtx[vc-1];

   /* fill the edge table */
   for (c=0; c<vc; c++) {
      v1 = v2;
      v2 = vtx[c];
      v1y = (int)(v1->y+0.5);
      v2y = (int)(v2->y+0.5);

      if ((v1y != v2y) && 
	  ((v1y >= bmp->ct) || (v2y >= bmp->ct)) &&
	  ((v1y < bmp->cb) || (v2y < bmp->cb))) {

	 _fill_3d_edge_structure_f(edge, v1, v2, flags, bmp);

	 if (edge->top < bmp->ct) {
	    int gap = bmp->ct - edge->top;
	    edge->top = bmp->ct;
	    edge->x += edge->dx * gap;
	    _clip_polygon_segment(&edge->dat, gap, flags);
	 }

	 if (edge->bottom >= edge->top) {

	    if (edge->top < top)
	       top = edge->top;

	    if (edge->bottom > bottom)
	       bottom = edge->bottom;

	    inactive_edges = _add_edge(inactive_edges, edge, FALSE);
	    edge++;
	 }
      }
   }

   /* render the polygon */
   do_polygon3d(bmp, top, bottom, inactive_edges, drawer, flags, vtx[0]->c, &info);
}



/* triangle3d:
 *  Draws a 3d triangle.
 */
void triangle3d(BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3)
{
   #if (defined ALLEGRO_GCC) && (defined ALLEGRO_I386)

      /* dodgy assumption alert! See comments for triangle() */
      polygon3d(bmp, type, texture, 3, &v1);

   #else

      V3D *vertex[3];

      vertex[0] = v1;
      vertex[1] = v2;
      vertex[2] = v3;
      polygon3d(bmp, type, texture, 3, vertex);

   #endif
}



/* triangle3d_f:
 *  Draws a 3d triangle.
 */
void triangle3d_f(BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3)
{
   #if (defined ALLEGRO_GCC) && (defined ALLEGRO_I386)

      /* dodgy assumption alert! See comments for triangle() */
      polygon3d_f(bmp, type, texture, 3, &v1);

   #else

      V3D_f* vertex[3];

      vertex[0] = v1;
      vertex[1] = v2;
      vertex[2] = v3;
      polygon3d_f(bmp, type, texture, 3, vertex);

   #endif
}



/* quad3d:
 *  Draws a 3d quad.
 */
void quad3d(BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4)
{
   #if (defined ALLEGRO_GCC) && (defined ALLEGRO_I386)

      /* dodgy assumption alert! See comments for triangle() */
      polygon3d(bmp, type, texture, 4, &v1);

   #else

      V3D *vertex[4];

      vertex[0] = v1;
      vertex[1] = v2;
      vertex[2] = v3;
      vertex[3] = v4;
      polygon3d(bmp, type, texture, 4, vertex);

   #endif
}



/* quad3d_f:
 *  Draws a 3d quad.
 */
void quad3d_f(BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4)
{
   #if (defined ALLEGRO_GCC) && (defined ALLEGRO_I386)

      /* dodgy assumption alert! See comments for triangle() */
      polygon3d_f(bmp, type, texture, 4, &v1);

   #else

      V3D_f *vertex[4];

      vertex[0] = v1;
      vertex[1] = v2;
      vertex[2] = v3;
      vertex[3] = v4;
      polygon3d_f(bmp, type, texture, 4, vertex);

   #endif
}


