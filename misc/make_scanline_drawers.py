#!/usr/bin/env python
#
# Generate the routines to draw each scanline of a primitive.
# Run:
#  python misc/make_scanline_drawers.py | indent -kr -i3 -l0 > addons/primitives/scanline_drawers.c

def make_drawer(name):
   texture = (name.find("_texture_") != -1)
   grad = (name.find("_grad_") != -1)
   solid = (name.find("_solid_") != -1)
   shade = (name.find("_shade") != -1)
   opaque = (name.find("_opaque") != -1)
   white = (name.find("_white") != -1)

   if grad and solid:
      print "#error grad and solid"
   if grad and white:
      print "#error grad and white"
   if shade and opaque:
      print "#error shade and opaque"

   print "static void", name, "(uintptr_t state, int x1, int y, int x2) {"

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
      const int src_format = s->texture->locked_region.format;
      const int src_size = al_get_pixel_size(src_format);
      ALLEGRO_COLOR src_color = {0, 0, 0, 0};
      """

   print """\
      const int dst_format = target->locked_region.format;
      uint8_t *dst_data = (uint8_t *)target->locked_region.data
         + y * target->locked_region.pitch
         + x1 * al_get_pixel_size(dst_format);
      """

   print """\
      for (; x1 <= x2; x1++) {
      """

   if not texture:
      print """\
         ALLEGRO_COLOR src_color = cur_color;
         """
   else:
      print """\
         const int src_x = fix_var(u, s->w);
         const int src_y = fix_var(v, s->h);
         uint8_t *src_data = (uint8_t *)s->texture->locked_region.data
            + (src_y - s->texture->lock_y) * s->texture->locked_region.pitch
            + (src_x - s->texture->lock_x) * src_size;
         _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);
         """
      if grad:
         print """\
         SHADE_COLORS(src_color, cur_color);
         """
      elif not white:
         print """\
         SHADE_COLORS(src_color, s->cur_color);
         """

   if shade:
      print """\
         {
            ALLEGRO_COLOR dst_color;
            ALLEGRO_COLOR result;
            _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
            _al_blend_inline(&src_color, &dst_color,
               op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
            _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
         }
         """
   else:
      print """\
         _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);
         """

   if texture:
      print """\
         u += s->du_dx;
         v += s->dv_dx;
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
