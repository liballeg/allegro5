#ifndef __al_included_aintern_float_h
#define __al_included_aintern_float_h


/*
 * Fast float->int conversion.
 *
 * I found this version at
 * http://stackoverflow.com/questions/78619/what-is-the-fastest-way-to-convert-float-to-int-on-x86
 *
 * It says:
 * by Vlad Kaipetsky.
 * Portable assuming FP24 set to nearest rounding mode
 * Efficient on x86 platform
 *
 * Note that it does not truncate like (int)f.
 *
 * The SSE instruction CVTTSS2SI is faster, and the compiler should
 * automatically use it if you allow it to.  Hence this trick is only useful
 * when compatibility with Pentium 3's and below is required.
 */
#if defined(ALLEGRO_I386) && !defined(ALLEGRO_CFG_ALLOW_SSE)

   union _al_UFloatInt {
      int32_t i;
      float f;
   };

   AL_INLINE(int32_t, _al_fast_float_to_int, (float fval),
   {
      /* ASSERT(fabs(fval) <= 0x003fffff); */ /* only 23 bit values handled */
      union _al_UFloatInt *fi = (union _al_UFloatInt *)&fval;
      fi->f += (3 << 22);
      return ((fi->i) & 0x007fffff) - 0x00400000;
   })

#else

   #define _al_fast_float_to_int(f)  ((int)(f))

#endif


#endif   /* __al_included_aintern_float_h */

/* vim: set sts=3 sw=3 et: */
