#!/usr/bin/env python
#
# Run:
#  python misc/make_resamplers.py | indent -kr -i3 -l0

import sys, re

# http://code.activestate.com/recipes/502257/
def interp(string):
   locals  = sys._getframe(1).f_locals
   globals = sys._getframe(1).f_globals
   for item in re.findall(r'#\{([^}]*)\}', string):
      string = string.replace('#{%s}' % item, str(eval(item, globals, locals)))
   return string


class Depth:
   def index(self, fmt):
      if fmt == "f32":
         return self.index_f32
      if fmt == "s16":
         return self.index_s16

class Depth_f32(Depth):
   def constant(self):
      return "ALLEGRO_AUDIO_DEPTH_FLOAT32"
   def index_f32(self, buf, index):
      return interp("#{buf}.f32[ #{index} ]")
   def index_s16(self, buf, index):
      return interp("(int16_t) (#{buf}.f32[ #{index} ] * 0x7FFF)")

class Depth_int24(Depth):
   def constant(self):
      return "ALLEGRO_AUDIO_DEPTH_INT24"
   def index_f32(self, buf, index):
      return interp("(float) #{buf}.s24[ #{index} ] / ((float)0x7FFFFF + 0.5f)")
   def index_s16(self, buf, index):
      return interp("(int16_t) (#{buf}.s24[ #{index} ] >> 9)")

class Depth_uint24(Depth):
   def constant(self):
      return "ALLEGRO_AUDIO_DEPTH_UINT24"
   def index_f32(self, buf, index):
      return interp("(float) #{buf}.u24[ #{index} ] / ((float)0x7FFFFF + 0.5f) - 1.0f")
   def index_s16(self, buf, index):
      return interp("(int16_t) ((#{buf}.u24[ #{index} ] - 0x800000) >> 9)")

class Depth_int16(Depth):
   def constant(self):
      return "ALLEGRO_AUDIO_DEPTH_INT16"
   def index_f32(self, buf, index):
      return interp("(float) #{buf}.s16[ #{index} ] / ((float)0x7FFF + 0.5f)")
   def index_s16(self, buf, index):
      return interp("#{buf}.s16[ #{index} ]")

class Depth_uint16(Depth):
   def constant(self):
      return "ALLEGRO_AUDIO_DEPTH_UINT16"
   def index_f32(self, buf, index):
      return interp("(float) #{buf}.u16[ #{index} ] / ((float)0x7FFF + 0.5f) - 1.0f")
   def index_s16(self, buf, index):
      return interp("(int16_t) (#{buf}.u16[ #{index} ] - 0x8000)")

class Depth_int8(Depth):
   def constant(self):
      return "ALLEGRO_AUDIO_DEPTH_INT8"
   def index_f32(self, buf, index):
      return interp("(float) #{buf}.s8[ #{index} ] / ((float)0x7F + 0.5f)")
   def index_s16(self, buf, index):
      return interp("(int16_t) #{buf}.s8[ #{index} ] << 7")

class Depth_uint8(Depth):
   def constant(self):
      return "ALLEGRO_AUDIO_DEPTH_UINT8"
   def index_f32(self, buf, index):
      return interp("(float) #{buf}.u8[ #{index} ] / ((float)0x7F + 0.5f) - 1.0f")
   def index_s16(self, buf, index):
      return interp("(int16_t) (#{buf}.u8[ #{index} ] - 0x80) << 7")

depths = [
   Depth_f32(),
   Depth_int24(),
   Depth_uint24(),
   Depth_int16(),
   Depth_uint16(),
   Depth_int8(),
   Depth_uint8()
]

def make_point_interpolator(name, fmt):
   print interp("""\
   static INLINE const void *
      #{name}
      (SAMP_BUF *samp_buf,
       const ALLEGRO_SAMPLE_INSTANCE *spl,
       unsigned int maxc)
   {
      unsigned int i0 = spl->pos*maxc;
      unsigned int i;

      switch (spl->spl_data.depth) {
      """)

   for depth in depths:
      buf_index = depth.index(fmt)("spl->spl_data.buffer", "i0 + i")
      print interp("""\
         case #{depth.constant()}:
            for (i = 0; i < maxc; i++) {
               samp_buf-> #{fmt} [i] = #{buf_index};
            }
            break;
         """)

   print interp("""\
      }
      return samp_buf-> #{fmt} ;
   }""")

def make_linear_interpolator(name, fmt):
   assert fmt == "f32" or fmt == "s16"

   print interp("""\
   static INLINE const void *
      #{name}
      (SAMP_BUF *samp_buf,
       const ALLEGRO_SAMPLE_INSTANCE *spl,
       unsigned int maxc)
   {
      int p0 = spl->pos;
      int p1 = spl->pos+1;

      switch (spl->loop) {
         case ALLEGRO_PLAYMODE_ONCE:
            if (p1 >= spl->spl_data.len)
               p1 = p0;
            break;
         case ALLEGRO_PLAYMODE_LOOP:
            if (p1 >= spl->loop_end)
               p1 = spl->loop_start;
            break;
         case ALLEGRO_PLAYMODE_BIDIR:
            if (p1 >= spl->loop_end) {
               p1 = spl->loop_end - 1;
               if (p1 < spl->loop_start)
                  p1 = spl->loop_start;
            }
            break;
         case _ALLEGRO_PLAYMODE_STREAM_ONCE:
         case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:""" +
            # For audio streams, sample i+1 may be in the next buffer fragment,
            # which may not even be generated yet.  So we lag by one sample and
            # interpolate between sample i-1 and sample i.
            #
            # We arrange the buffers in memory such that indexing i-1 is always
            # valid, even after wrapping around from the last buffer fragment to
            # the first buffer fragment.  See _al_kcm_refill_stream.
            """
            p0--;
            p1--;
            break;
      }

      p0 *= maxc;
      p1 *= maxc;

      switch (spl->spl_data.depth) {
      """)

   for depth in depths:
      x0 = depth.index(fmt)("spl->spl_data.buffer", "p0 + i")
      x1 = depth.index(fmt)("spl->spl_data.buffer", "p1 + i")
      print interp("""\
         case #{depth.constant()}:
         {""")

      if fmt == "f32":
         print interp("""\
            const float t = (float)spl->pos_bresenham_error / spl->step_denom;
            int i;
            for (i = 0; i < (int)maxc; i++) {
               const float x0 = #{x0};
               const float x1 = #{x1};
               const float s = (x0 * (1.0f - t)) + (x1 * t);
               samp_buf->f32[i] = s;
            }""")
      elif fmt == "s16":
         print interp("""\
            const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
            int i;
            for (i = 0; i < (int)maxc; i++) {
               const int32_t x0 = #{x0};
               const int32_t x1 = #{x1};
               const int32_t s = ((x0 * (256 - t))>>8) + ((x1 * t)>>8);
               samp_buf->s16[i] = (int16_t)s;
            }""")

      print interp("""\
         }
         break;
         """)

   print interp("""\
      }
      return samp_buf-> #{fmt};
   }""")

def make_cubic_interpolator(name, fmt):
   assert fmt == "f32"

   print interp("""\
   static INLINE const void *
      #{name}
      (SAMP_BUF *samp_buf,
       const ALLEGRO_SAMPLE_INSTANCE *spl,
       unsigned int maxc)
   {
      int p0 = spl->pos-1;
      int p1 = spl->pos;
      int p2 = spl->pos+1;
      int p3 = spl->pos+2;

      switch (spl->loop) {
         case ALLEGRO_PLAYMODE_ONCE:
            if (p0 < 0)
               p0 = 0;
            if (p2 >= spl->spl_data.len)
               p2 = spl->spl_data.len - 1;
            if (p3 >= spl->spl_data.len)
               p3 = spl->spl_data.len - 1;
            break;
         case ALLEGRO_PLAYMODE_LOOP:
         case ALLEGRO_PLAYMODE_BIDIR:
            /* These positions should really wrap/bounce instead of clamping
             * but it's probably unnoticeable.
             */
            if (p0 < spl->loop_start)
               p0 = spl->loop_end - 1;
            if (p2 >= spl->loop_end)
               p2 = spl->loop_start;
            if (p3 >= spl->loop_end)
               p3 = spl->loop_start;
            break;
         case _ALLEGRO_PLAYMODE_STREAM_ONCE:
         case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:
            /* Lag by three samples in total. */
            p0 -= 2;
            p1 -= 2;
            p2 -= 2;
            p3 -= 2;
            break;
      }

      p0 *= maxc;
      p1 *= maxc;
      p2 *= maxc;
      p3 *= maxc;

      switch (spl->spl_data.depth) {
      """)

   for depth in depths:
      value0 = depth.index(fmt)("spl->spl_data.buffer", "p0 + i")
      value1 = depth.index(fmt)("spl->spl_data.buffer", "p1 + i")
      value2 = depth.index(fmt)("spl->spl_data.buffer", "p2 + i")
      value3 = depth.index(fmt)("spl->spl_data.buffer", "p3 + i")
      # 4-point, cubic Hermite interpolation
      # Code transcribed from "Polynomial Interpolators for High-Quality
      # Resampling of Oversampled Audio" by Olli Niemitalo
      # http://yehar.com/blog/?p=197
      print interp("""\
         case #{depth.constant()}:
         {
            const float t = (float)spl->pos_bresenham_error / spl->step_denom;
            signed int i;
            for (i = 0; i < (signed int)maxc; i++) {
               float x0 = #{value0};
               float x1 = #{value1};
               float x2 = #{value2};
               float x3 = #{value3};
               float c0 = x1;
               float c1 = 0.5f * (x2 - x0);
               float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
               float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
               float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
               samp_buf->f32[i] = s;
            }
         }
         break;
         """)

   print interp("""\
      }
      return samp_buf-> #{fmt} ;
   }""")

if __name__ == "__main__":
   print "// Warning: This file was created by make_resamplers.py - do not edit."
   print "// vim: set ft=c:"

   make_point_interpolator("point_spl32", "f32")
   make_point_interpolator("point_spl16", "s16")
   make_linear_interpolator("linear_spl32", "f32")
   make_linear_interpolator("linear_spl16", "s16")
   make_cubic_interpolator("cubic_spl32", "f32")

# vim: set sts=3 sw=3 et:
