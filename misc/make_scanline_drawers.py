#!/usr/bin/env python3
#
# Generate the routines to draw each scanline of a primitive.
# Run:
#  misc/make_scanline_drawers.py | indent -kr -i3 -l0 > src/scanline_drawers.inc

import sys, re

# http://code.activestate.com/recipes/502257/
def interp(string):
   locals  = sys._getframe(1).f_locals
   globals = sys._getframe(1).f_globals
   for item in re.findall(r'#\{([^}]*)\}', string):
      string = string.replace('#{%s}' % item, str(eval(item, globals, locals)))
   return string

def make_drawer(name):
   global texture, grad, solid, shade, opaque, white
   texture = "_texture_" in name
   grad = "_grad_" in name
   solid = "_solid_" in name
   shade = "_shade" in name
   opaque = "_opaque" in name
   white = "_white" in name
   repeat = "_repeat" in name

   if grad and solid:
      raise Exception("grad and solid")
   if grad and white:
      raise Exception("grad and white")
   if shade and opaque:
      raise Exception("shade and opaque")

   print(interp("static void #{name} (uintptr_t state, int x1, int y, int x2) {"))

   if not texture:
      if grad:
         print("""\
         state_grad_any_2d *gs = (state_grad_any_2d *)state;
         state_solid_any_2d *s = &gs->solid;
         A5O_COLOR cur_color = s->cur_color;
         """)
      else:
         print("""\
         state_solid_any_2d *s = (state_solid_any_2d *)state;
         A5O_COLOR cur_color = s->cur_color;
         """)
   else:
      if grad:
         print("""\
         state_texture_grad_any_2d *gs = (state_texture_grad_any_2d *)state;
         state_texture_solid_any_2d *s = &gs->solid;
         A5O_COLOR cur_color = s->cur_color;
         """)
      else:
         print("""\
         state_texture_solid_any_2d *s = (state_texture_solid_any_2d *)state;
         """)
      print("""\
         float u = s->u;
         float v = s->v;
         """)

   # XXX still don't understand why y-1 is required
   print("""\
      A5O_BITMAP *target = s->target;

      if (target->parent) {
         x1 += target->xofs;
         x2 += target->xofs;
         y += target->yofs;
         target = target->parent;
      }

      x1 -= target->lock_x;
      x2 -= target->lock_x;
      y -= target->lock_y;
      y--;

      if (y < 0 || y >= target->lock_h) {
         return;
      }

      if (x1 < 0) {
      """)
   if texture:
      print("""\
         u += s->du_dx * -x1;
         v += s->dv_dx * -x1;
         """)

   if grad:
      print("""\
         cur_color.r += gs->color_dx.r * -x1;
         cur_color.g += gs->color_dx.g * -x1;
         cur_color.b += gs->color_dx.b * -x1;
         cur_color.a += gs->color_dx.a * -x1;
         """)

   print("""\
         x1 = 0;
      }

      if (x2 > target->lock_w - 1) {
         x2 = target->lock_w - 1;
      }
      """)

   print("{")
   if shade:
      print("""\
      int op, src_mode, dst_mode;
      int op_alpha, src_alpha, dst_alpha;
      A5O_COLOR const_color;
      al_get_separate_bitmap_blender(&op, &src_mode, &dst_mode,
         &op_alpha, &src_alpha, &dst_alpha);
      const_color = al_get_blend_color();
      """)

   print("{")
   if texture:
      print("""\
      const int offset_x = s->texture->parent ? s->texture->xofs : 0;
      const int offset_y = s->texture->parent ? s->texture->yofs : 0;
      A5O_BITMAP* texture = s->texture->parent ? s->texture->parent : s->texture;
      const int src_format = texture->locked_region.format;
      const int src_size = texture->locked_region.pixel_size;
      A5O_BITMAP_WRAP wrap_u, wrap_v;
      _al_get_bitmap_wrap(texture, &wrap_u, &wrap_v);
      int tile_u = (int)(floorf(u / s->w));
      int tile_v = (int)(floorf(v / s->h));

      /* Ensure u in [0, s->w) and v in [0, s->h). */
      while (u < 0) u += s->w;
      while (v < 0) v += s->h;
      u = fmodf(u, s->w);
      v = fmodf(v, s->h);
      ASSERT(0 <= u); ASSERT(u < s->w);
      ASSERT(0 <= v); ASSERT(v < s->h);
      """)

   print("{")
   print("""\
      const int dst_format = target->locked_region.format;
      uint8_t *dst_data = (uint8_t *)target->lock_data
         + y * target->locked_region.pitch
         + x1 * target->locked_region.pixel_size;
      """)

   if shade:
      make_if_blender_loop(
            op='A5O_ADD',
            src_mode='A5O_ONE',
            src_alpha='A5O_ONE',
            op_alpha='A5O_ADD',
            dst_mode='A5O_INVERSE_ALPHA',
            dst_alpha='A5O_INVERSE_ALPHA',
            const_color='NULL',
            if_format='A5O_PIXEL_FORMAT_ARGB_8888',
            alpha_only=True,
            repeat=repeat,
            )
      print("else")
      make_if_blender_loop(
            op='A5O_ADD',
            src_mode='A5O_ALPHA',
            src_alpha='A5O_ALPHA',
            op_alpha='A5O_ADD',
            dst_mode='A5O_INVERSE_ALPHA',
            dst_alpha='A5O_INVERSE_ALPHA',
            const_color='NULL',
            if_format='A5O_PIXEL_FORMAT_ARGB_8888',
            alpha_only=True,
            repeat=repeat,
            )
      print("else")
      make_if_blender_loop(
            op='A5O_ADD',
            src_mode='A5O_ONE',
            src_alpha='A5O_ONE',
            op_alpha='A5O_ADD',
            dst_mode='A5O_ONE',
            dst_alpha='A5O_ONE',
            const_color='NULL',
            if_format='A5O_PIXEL_FORMAT_ARGB_8888',
            alpha_only=True,
            repeat=repeat,
            )
      print("else")

   if opaque and white:
      make_loop(copy_format=True, src_size='4')
      print("else")
      make_loop(copy_format=True, src_size='3')
      print("else")
      make_loop(copy_format=True, src_size='2')
      print("else")
   else:
      make_loop(
            if_format='A5O_PIXEL_FORMAT_ARGB_8888'
            )
      print("else")

   make_loop()

   print("""\
   }
   }
   }
   }
   """)

def make_if_blender_loop(
      op='op',
      src_mode='src_mode',
      dst_mode='dst_mode',
      op_alpha='op_alpha',
      src_alpha='src_alpha',
      dst_alpha='dst_alpha',
      src_format='src_format',
      dst_format='dst_format',
      const_color='NULL',
      if_format=None,
      alpha_only=False,
      repeat=False,
      ):
   print(interp("""\
      if (op == #{op} &&
            src_mode == #{src_mode} &&
            src_alpha == #{src_alpha} &&
            op_alpha == #{op_alpha} &&
            dst_mode == #{dst_mode} &&
            dst_alpha == #{dst_alpha}) {
      """))

   if texture and if_format:
      make_loop(
            op=op,
            src_mode=src_mode,
            src_alpha=src_alpha,
            op_alpha=op_alpha,
            dst_mode=dst_mode,
            dst_alpha=dst_alpha,
            const_color=const_color,
            if_format=if_format,
            alpha_only=alpha_only,
            repeat=repeat,
            )
      print("else")

   make_loop(
      op=op,
      src_mode=src_mode,
      src_alpha=src_alpha,
      op_alpha=op_alpha,
      dst_mode=dst_mode,
      dst_alpha=dst_alpha,
      const_color=const_color,
      alpha_only=alpha_only,
      repeat=repeat,
      )

   print("}")

def make_loop(
      op='op',
      src_mode='src_mode',
      dst_mode='dst_mode',
      op_alpha='op_alpha',
      src_alpha='src_alpha',
      dst_alpha='dst_alpha',
      src_format='src_format',
      dst_format='dst_format',
      src_size='src_size',
      const_color='&const_color',
      if_format=None,
      copy_format=False,
      alpha_only=False,
      repeat=False,
      ):

   if if_format:
      src_format = if_format
      dst_format = if_format
      print(interp("if (dst_format == #{dst_format}"))
      if texture:
         print(interp("&& src_format == #{src_format}"))
      print(")")
   elif copy_format:
      assert opaque and white
      print(interp("if (dst_format == src_format && src_size == #{src_size})"))

   print("{")

   if texture:
      print("""\
         uint8_t *lock_data = texture->locked_region.data;
         const int src_pitch = texture->locked_region.pitch;
         const al_fixed du_dx = al_ftofix(s->du_dx);
         const al_fixed dv_dx = al_ftofix(s->dv_dx);
         """)

      if opaque:
         # If texture coordinates never wrap around then we can simplify the
         # innermost loop. It doesn't seem to have so great an impact when the
         # loop is complicated by blending.
         print("""\
            const float steps = x2 - x1 + 1;
            const float end_u = u + steps * s->du_dx;
            const float end_v = v + steps * s->dv_dx;
            if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {
            """)
         make_innermost_loop(
            op=op,
            src_mode=src_mode,
            dst_mode=dst_mode,
            op_alpha=op_alpha,
            src_alpha=src_alpha,
            dst_alpha=dst_alpha,
            src_format=src_format,
            dst_format=dst_format,
            const_color=const_color,
            src_size=src_size,
            copy_format=copy_format,
            tiling=False,
            alpha_only=alpha_only,
            repeat=repeat,
            )
         print("} else")

   make_innermost_loop(
      op=op,
      src_mode=src_mode,
      dst_mode=dst_mode,
      op_alpha=op_alpha,
      src_alpha=src_alpha,
      dst_alpha=dst_alpha,
      src_format=src_format,
      dst_format=dst_format,
      const_color=const_color,
      src_size=src_size,
      copy_format=copy_format,
      alpha_only=alpha_only,
      repeat=repeat,
      )

   print("}")

def make_innermost_loop(
      op='op',
      src_mode='src_mode',
      dst_mode='dst_mode',
      op_alpha='op_alpha',
      src_alpha='src_alpha',
      dst_alpha='dst_alpha',
      src_format='src_format',
      dst_format='dst_format',
      const_color='&const_color',
      src_size='src_size',
      copy_format=False,
      tiling=True,
      alpha_only=True,
      repeat=False,
      ):

   print("{")

   if texture:
      # In non-tiling mode we can hoist offsets out of the loop.
      if tiling:
         print("""\
            al_fixed uu = al_ftofix(u);
            al_fixed vv = al_ftofix(v);
            const int uu_ofs = offset_x - texture->lock_x;
            const int vv_ofs = offset_y - texture->lock_y;
            const al_fixed w = al_ftofix(s->w);
            const al_fixed h = al_ftofix(s->h);
            """)
         uu_ofs = "uu_ofs"
         vv_ofs = "vv_ofs"
      else:
         print("""\
            al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
            al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);
            """)
         uu_ofs = vv_ofs = "0"

   print("for (; x1 <= x2; x1++) {")

   if not texture:
      print("""\
         A5O_COLOR src_color = cur_color;
         """)
   else:
      print(interp("""\
         int src_x = (uu >> 16) + #{uu_ofs};
         int src_y = (vv >> 16) + #{vv_ofs};
         """));

      if not repeat:
         print("""\
         switch (wrap_u) {
            case A5O_BITMAP_WRAP_CLAMP:
               if (tile_u < 0)
                  src_x = 0;
               if (tile_u > 0)
                  src_x = s->w - 1;
               break;
            case A5O_BITMAP_WRAP_MIRROR:
               if (tile_u % 2)
                  src_x = s->w - 1 - src_x;
            // REPEAT and DEFAULT.
            default:
               break;
         }

         switch (wrap_v) {
            case A5O_BITMAP_WRAP_CLAMP:
               if (tile_v < 0)
                  src_y = 0;
               if (tile_v > 0)
                  src_y = s->h - 1;
               break;
            case A5O_BITMAP_WRAP_MIRROR:
               if (tile_v % 2)
                  src_y = s->h - 1 - src_y;
            // REPEAT and DEFAULT.
            default:
               break;
         }
         """)

      print(interp("""\
         uint8_t *src_data = lock_data
               + src_y * src_pitch
               + src_x * #{src_size};
         """))

      if copy_format:
         pass
      else:
         print(interp("""\
            A5O_COLOR src_color;
            _AL_INLINE_GET_PIXEL(#{src_format}, src_data, src_color, false);
            """))
         if grad:
            print("""\
            SHADE_COLORS(src_color, cur_color);
            """)
         elif not white:
            print("""\
            SHADE_COLORS(src_color, s->cur_color);
            """)

   if copy_format:
      print(interp("""\
         switch (#{src_size}) {
            case 4:
               memcpy(dst_data, src_data, 4);
               dst_data += 4;
               break;
            case 3:
               memcpy(dst_data, src_data, 3);
               dst_data += 3;
               break;
            case 2:
               *dst_data++ = *src_data++;
               *dst_data++ = *src_data;
               break;
            case 1:
               *dst_data++ = *src_data;
               break;
         }
         """))
   elif shade:
      blend = "_al_blend_inline"
      if alpha_only:
         blend = "_al_blend_alpha_inline"
      print(interp("""\
         {
            A5O_COLOR dst_color;
            A5O_COLOR result;
            _AL_INLINE_GET_PIXEL(#{dst_format}, dst_data, dst_color, false);
            #{blend}(&src_color, &dst_color,
               #{op}, #{src_mode}, #{dst_mode},
               #{op_alpha}, #{src_alpha}, #{dst_alpha},
               #{const_color}, &result);
            _AL_INLINE_PUT_PIXEL(#{dst_format}, dst_data, result, true);
         }
         """))
   else:
      print(interp("""\
         _AL_INLINE_PUT_PIXEL(#{dst_format}, dst_data, src_color, true);
         """))

   if texture:
      print("""\
         uu += du_dx;
         vv += dv_dx;
         """)

      if tiling:
         print("""\
         if (_AL_EXPECT_FAIL(uu < 0)) {
            uu += w;
            tile_u--;
         }
         else if (_AL_EXPECT_FAIL(uu >= w)) {
            uu -= w;
            tile_u++;
         }

         if (_AL_EXPECT_FAIL(vv < 0)) {
            vv += h;
            tile_v--;
         }
         else if (_AL_EXPECT_FAIL(vv >= h)) {
            vv -= h;
            tile_v++;
         }
         """)

   if grad:
      print("""\
         cur_color.r += gs->color_dx.r;
         cur_color.g += gs->color_dx.g;
         cur_color.b += gs->color_dx.b;
         cur_color.a += gs->color_dx.a;
         """)

   print("""\
      }
   }""")

if __name__ == "__main__":
   print("""\
// Warning: This file was created by make_scanline_drawers.py - do not edit.
#if __GNUC__
#define _AL_EXPECT_FAIL(expr) __builtin_expect((expr), 0)
#else
#define _AL_EXPECT_FAIL(expr) (expr)
#endif
""")

   make_drawer("shader_solid_any_draw_shade")
   make_drawer("shader_solid_any_draw_opaque")

   make_drawer("shader_grad_any_draw_shade")
   make_drawer("shader_grad_any_draw_opaque")

   make_drawer("shader_texture_solid_any_draw_shade")
   make_drawer("shader_texture_solid_any_draw_shade_repeat")
   make_drawer("shader_texture_solid_any_draw_shade_white")
   make_drawer("shader_texture_solid_any_draw_shade_white_repeat")
   make_drawer("shader_texture_solid_any_draw_opaque")
   make_drawer("shader_texture_solid_any_draw_opaque_white")

   make_drawer("shader_texture_grad_any_draw_shade")
   make_drawer("shader_texture_grad_any_draw_opaque")

# vim: set sts=3 sw=3 et:
