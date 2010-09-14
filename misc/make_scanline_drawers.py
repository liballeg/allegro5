#!/usr/bin/env python
#
# Generate the routines to draw each scanline of a primitive.
# Run:
#  python misc/make_scanline_drawers.py | indent -kr -i3 -l0 > addons/primitives/scanline_drawers.c

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
   texture = (name.find("_texture_") != -1)
   grad = (name.find("_grad_") != -1)
   solid = (name.find("_solid_") != -1)
   shade = (name.find("_shade") != -1)
   opaque = (name.find("_opaque") != -1)
   white = (name.find("_white") != -1)

   if grad and solid:
      raise Exception("grad and solid")
   if grad and white:
      raise Exception("grad and white")
   if shade and opaque:
      raise Exception("shade and opaque")

   print interp("static void #{name} (uintptr_t state, int x1, int y, int x2) {")

   if not texture:
      if grad:
         print """\
         state_grad_any_2d *gs = (state_grad_any_2d *)state;
         state_solid_any_2d *s = &gs->solid;
         ALLEGRO_COLOR cur_color = s->cur_color;
         """
      else:
         print """\
         state_solid_any_2d *s = (state_solid_any_2d *)state;
         ALLEGRO_COLOR cur_color = s->cur_color;
         """
   else:
      if grad:
         print """\
         state_texture_grad_any_2d *gs = (state_texture_grad_any_2d *)state;
         state_texture_solid_any_2d *s = &gs->solid;
         ALLEGRO_COLOR cur_color = s->cur_color;
         """
      else:
         print """\
         state_texture_solid_any_2d *s = (state_texture_solid_any_2d *)state;
         """
      print """\
         float u = s->u;
         float v = s->v;
         """

   # XXX keep target in the state
   # XXX still don't understand why y-1 is required
   print """\
      ALLEGRO_BITMAP *target = al_get_target_bitmap();
      x1 -= target->lock_x;
      x2 -= target->lock_x;
      y -= target->lock_y;
      y--;

      if (y < 0 || y >= target->lock_h) {
         return;
      }

      if (x1 < 0) {
      """
   if texture:
      print """\
         u += s->du_dx * -x1;
         v += s->dv_dx * -x1;
         """

   if grad:
      print """\
         cur_color.r += gs->color_dx.r * -x1;
         cur_color.g += gs->color_dx.g * -x1;
         cur_color.b += gs->color_dx.b * -x1;
         cur_color.a += gs->color_dx.a * -x1;
         """

   print """\
         x1 = 0;
      }

      if (x2 > target->lock_w - 1) {
         x2 = target->lock_w - 1;
      }
      """

   print "{"
   if shade:
      print """\
      int op, src_mode, dst_mode;
      int op_alpha, src_alpha, dst_alpha;
      al_get_separate_blender(&op, &src_mode, &dst_mode,
         &op_alpha, &src_alpha, &dst_alpha);
      """

   print "{"
   if texture:
      print """\
      const int offset_x = s->texture->parent ? s->texture->xofs : 0;
      const int offset_y = s->texture->parent ? s->texture->yofs : 0;
      ALLEGRO_BITMAP* texture = s->texture->parent ? s->texture->parent : s->texture;
      const int src_format = texture->locked_region.format;
      const int src_size = al_get_pixel_size(src_format);

      /* Ensure u in [0, s->w) and v in [0, s->h). */
      while (u < 0) u += s->w;
      while (v < 0) v += s->h;
      u = fmodf(u, s->w);
      v = fmodf(v, s->h);
      ASSERT(0 <= u); ASSERT(u < s->w);
      ASSERT(0 <= v); ASSERT(v < s->h);
      """

   print "{"
   print """\
      const int dst_format = target->locked_region.format;
      uint8_t *dst_data = (uint8_t *)target->locked_region.data
         + y * target->locked_region.pitch
         + x1 * al_get_pixel_size(dst_format);
      """

   if shade:
      make_if_blender_loop(
            op='ALLEGRO_ADD',
            src_mode='ALLEGRO_ALPHA',
            src_alpha='ALLEGRO_ALPHA',
            op_alpha='ALLEGRO_ADD',
            dst_mode='ALLEGRO_INVERSE_ALPHA',
            dst_alpha='ALLEGRO_INVERSE_ALPHA',
            if_format='ALLEGRO_PIXEL_FORMAT_ARGB_8888'
            )
      print "else"
      make_if_blender_loop(
            op='ALLEGRO_ADD',
            src_mode='ALLEGRO_ONE',
            src_alpha='ALLEGRO_ONE',
            op_alpha='ALLEGRO_ADD',
            dst_mode='ALLEGRO_ONE',
            dst_alpha='ALLEGRO_ONE',
            if_format='ALLEGRO_PIXEL_FORMAT_ARGB_8888'
            )
      print "else"

   make_loop(
         if_format='ALLEGRO_PIXEL_FORMAT_ARGB_8888'
         )
   print "else"

   make_loop()

   print """\
   }
   }
   }
   }
   """

def make_if_blender_loop(
      op='op',
      src_mode='src_mode',
      dst_mode='dst_mode',
      op_alpha='op_alpha',
      src_alpha='src_alpha',
      dst_alpha='dst_alpha',
      src_format='src_format',
      dst_format='dst_format',
      if_format=None
      ):
   print interp("""\
      if (op == #{op} &&
            src_mode == #{src_mode} &&
            src_alpha == #{src_alpha} &&
            op_alpha == #{op_alpha} &&
            dst_mode == #{dst_mode} &&
            dst_alpha == #{dst_alpha}) {
      """)

   if texture and if_format:
      make_loop(
            op=op,
            src_mode=src_mode,
            src_alpha=src_alpha,
            op_alpha=op_alpha,
            dst_mode=dst_mode,
            dst_alpha=dst_alpha,
            if_format=if_format
            )
      print "else"

   make_loop(
      op=op,
      src_mode=src_mode,
      src_alpha=src_alpha,
      op_alpha=op_alpha,
      dst_mode=dst_mode,
      dst_alpha=dst_alpha)

   print "}"

def make_loop(
      op='op',
      src_mode='src_mode',
      dst_mode='dst_mode',
      op_alpha='op_alpha',
      src_alpha='src_alpha',
      dst_alpha='dst_alpha',
      src_format='src_format',
      dst_format='dst_format',
      if_format=None
      ):

   if if_format:
      src_format = if_format
      dst_format = if_format
      print interp("if (dst_format == #{dst_format}")
      if texture:
         print interp("&& src_format == #{src_format}")
      print ")"

   print "{"

   if texture:
      print """\
         uint8_t *lock_data = texture->locked_region.data;
         const int src_pitch = texture->locked_region.pitch;
         const int lock_y = texture->lock_y;
         const int lock_x = texture->lock_x;
         const float du_dx = s->du_dx;
         const float dv_dx = s->dv_dx;
         const int w = s->w;
         const int h = s->h;
         """

   print """\
      for (; x1 <= x2; x1++) {
      """

   if not texture:
      print """\
         ALLEGRO_COLOR src_color = cur_color;
         """
   else:
      print interp("""\
         ALLEGRO_COLOR src_color;
         const int src_x = _al_fast_float_to_int(u) + offset_x;
         const int src_y = _al_fast_float_to_int(v) + offset_y;
         uint8_t *src_data = lock_data
            + (src_y - lock_y) * src_pitch
            + (src_x - lock_x) * src_size;
         _AL_INLINE_GET_PIXEL(#{src_format}, src_data, src_color, false);
         """)
      if grad:
         print """\
         SHADE_COLORS(src_color, cur_color);
         """
      elif not white:
         print """\
         SHADE_COLORS(src_color, s->cur_color);
         """

   if shade:
      print interp("""\
         {
            ALLEGRO_COLOR dst_color;
            ALLEGRO_COLOR result;
            _AL_INLINE_GET_PIXEL(#{dst_format}, dst_data, dst_color, false);
            _al_blend_inline(&src_color, &dst_color,
               #{op}, #{src_mode}, #{dst_mode},
               #{op_alpha}, #{src_alpha}, #{dst_alpha},
               &result);
            _AL_INLINE_PUT_PIXEL(#{dst_format}, dst_data, result, true);
         }
         """)
   else:
      print interp("""\
         _AL_INLINE_PUT_PIXEL(#{dst_format}, dst_data, src_color, true);
         """)

   if texture:
      print """\
         u += du_dx;
         v += dv_dx;

         if (u < 0)
            u += w;
         else if (u >= w)
            u -= w;

         if (v < 0)
            v += h;
         else if (v >= h)
            v -= h;
         """

   if grad:
      print """\
         cur_color.r += gs->color_dx.r;
         cur_color.g += gs->color_dx.g;
         cur_color.b += gs->color_dx.b;
         cur_color.a += gs->color_dx.a;
         """

   print """\
      }
   }
   """

if __name__ == "__main__":
   print """\
// Warning: This file was created by make_scanline_drawers.py - do not edit.
"""

   make_drawer("shader_solid_any_draw_shade")
   make_drawer("shader_solid_any_draw_opaque")

   make_drawer("shader_grad_any_draw_shade")
   make_drawer("shader_grad_any_draw_opaque")

   make_drawer("shader_texture_solid_any_draw_shade")
   make_drawer("shader_texture_solid_any_draw_shade_white")
   make_drawer("shader_texture_solid_any_draw_opaque")
   make_drawer("shader_texture_solid_any_draw_opaque_white")

   make_drawer("shader_texture_grad_any_draw_shade")
   make_drawer("shader_texture_grad_any_draw_opaque")

# vim: set sts=3 sw=3 et:
