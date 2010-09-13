#ifndef __al_included_allegro5_aintern_blend_h
#define __al_included_allegro5_aintern_blend_h


#ifdef __GNUC__
   #define _AL_ALWAYS_INLINE  __attribute__((always_inline))
#else
   #define _AL_ALWAYS_INLINE  inline
#endif


static _AL_ALWAYS_INLINE float
get_factor(enum ALLEGRO_BLEND_MODE operation, float alpha)
{
   switch (operation) {
       case ALLEGRO_ZERO: return 0;
       case ALLEGRO_ONE: return 1;
       case ALLEGRO_ALPHA: return alpha;
       case ALLEGRO_INVERSE_ALPHA: return 1 - alpha;
   }
   ASSERT(false);
   return 0; /* silence warning in release build */
}


static _AL_ALWAYS_INLINE
void _al_blend_inline(
   const ALLEGRO_COLOR *scol, const ALLEGRO_COLOR *dcol,
   int op, int src_, int dst_, int aop, int asrc_, int adst_,
   ALLEGRO_COLOR *result)
{
   float src, dst, asrc, adst;

   result->r = scol->r;
   result->g = scol->g;
   result->b = scol->b;
   result->a = scol->a;

   src = get_factor(src_, result->a);
   dst = get_factor(dst_, result->a);
   asrc = get_factor(asrc_, result->a);
   adst = get_factor(adst_, result->a);

   #define BLEND(c, src, dst) \
      result->c = OP(result->c * src, dcol->c * dst);
   switch (op) {
      case ALLEGRO_ADD:
         #define OP(x, y) _ALLEGRO_MIN(1, x + y)
         BLEND(r, src, dst)
         BLEND(g, src, dst)
         BLEND(b, src, dst)
         #undef OP
         break;
      case ALLEGRO_SRC_MINUS_DEST:
         #define OP(x, y) _ALLEGRO_MAX(0, x - y)
         BLEND(r, src, dst)
         BLEND(g, src, dst)
         BLEND(b, src, dst)
         #undef OP
         break;
      case ALLEGRO_DEST_MINUS_SRC:
         #define OP(x, y) _ALLEGRO_MAX(0, y - x)
         BLEND(r, src, dst)
         BLEND(g, src, dst)
         BLEND(b, src, dst)
         #undef OP
         break;
   }

   switch (aop) {
      case ALLEGRO_ADD:
         #define OP(x, y) _ALLEGRO_MIN(1, x + y)
         BLEND(a, asrc, adst)
         #undef OP
         break;
      case ALLEGRO_SRC_MINUS_DEST:
         #define OP(x, y) _ALLEGRO_MAX(0, x - y)
         BLEND(a, asrc, adst)
         #undef OP
         break;
      case ALLEGRO_DEST_MINUS_SRC:
         #define OP(x, y) _ALLEGRO_MAX(0, y - x)
         BLEND(a, asrc, adst)
         #undef OP
         break;
   }
}


static _AL_ALWAYS_INLINE void
_al_blend_inline_dest_zero_add(
   const ALLEGRO_COLOR *scol,
   int src_, int asrc_,
   ALLEGRO_COLOR *result)
{
   float src, asrc;

   result->r = scol->r;
   result->g = scol->g;
   result->b = scol->b;
   result->a = scol->a;

   src = get_factor(src_, result->a);
   asrc = get_factor(asrc_, result->a);

   result->r *= src;
   result->g *= src;
   result->b *= src;
   result->a *= asrc;
}


#endif

/* vim: set sts=3 sw=3 et: */
