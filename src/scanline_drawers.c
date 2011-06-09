// Warning: This file was created by make_scanline_drawers.py - do not edit.

#if __GNUC__
#define _AL_EXPECT_FAIL(expr) __builtin_expect((expr), 0)
#else
#define _AL_EXPECT_FAIL(expr) (expr)
#endif

static void shader_solid_any_draw_shade(uintptr_t state, int x1, int y, int x2)
{
   state_solid_any_2d *s = (state_solid_any_2d *) state;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = s->target;

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

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      int op, src_mode, dst_mode;
      int op_alpha, src_alpha, dst_alpha;
      al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

      {
	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       {
		  {
		     for (; x1 <= x2; x1++) {
			ALLEGRO_COLOR src_color = cur_color;

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ALPHA && src_alpha == ALLEGRO_ALPHA && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       {
		  {
		     for (; x1 <= x2; x1++) {
			ALLEGRO_COLOR src_color = cur_color;

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_ONE && dst_alpha == ALLEGRO_ONE) {

	       {
		  {
		     for (; x1 <= x2; x1++) {
			ALLEGRO_COLOR src_color = cur_color;

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

		     }
		  }
	       }
	    } else if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
		     }

		  }
	       }
	    } else {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
		     }

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_solid_any_draw_opaque(uintptr_t state, int x1, int y, int x2)
{
   state_solid_any_2d *s = (state_solid_any_2d *) state;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = s->target;

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

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      {
	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, src_color, true);

		  }
	       }
	    } else {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_grad_any_draw_shade(uintptr_t state, int x1, int y, int x2)
{
   state_grad_any_2d *gs = (state_grad_any_2d *) state;
   state_solid_any_2d *s = &gs->solid;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = s->target;

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

      cur_color.r += gs->color_dx.r * -x1;
      cur_color.g += gs->color_dx.g * -x1;
      cur_color.b += gs->color_dx.b * -x1;
      cur_color.a += gs->color_dx.a * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      int op, src_mode, dst_mode;
      int op_alpha, src_alpha, dst_alpha;
      al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

      {
	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       {
		  {
		     for (; x1 <= x2; x1++) {
			ALLEGRO_COLOR src_color = cur_color;

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ALPHA && src_alpha == ALLEGRO_ALPHA && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       {
		  {
		     for (; x1 <= x2; x1++) {
			ALLEGRO_COLOR src_color = cur_color;

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_ONE && dst_alpha == ALLEGRO_ONE) {

	       {
		  {
		     for (; x1 <= x2; x1++) {
			ALLEGRO_COLOR src_color = cur_color;

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       }
	    } else if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
		     }

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    } else {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
		     }

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_grad_any_draw_opaque(uintptr_t state, int x1, int y, int x2)
{
   state_grad_any_2d *gs = (state_grad_any_2d *) state;
   state_solid_any_2d *s = &gs->solid;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = s->target;

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

      cur_color.r += gs->color_dx.r * -x1;
      cur_color.g += gs->color_dx.g * -x1;
      cur_color.b += gs->color_dx.b * -x1;
      cur_color.a += gs->color_dx.a * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      {
	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, src_color, true);

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    } else {
	       {
		  for (; x1 <= x2; x1++) {
		     ALLEGRO_COLOR src_color = cur_color;

		     _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_texture_solid_any_draw_shade(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = s->target;

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

      u += s->du_dx * -x1;
      v += s->dv_dx * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      int op, src_mode, dst_mode;
      int op_alpha, src_alpha, dst_alpha;
      al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

      {
	 const int offset_x = s->texture->parent ? s->texture->xofs : 0;
	 const int offset_y = s->texture->parent ? s->texture->yofs : 0;
	 ALLEGRO_BITMAP *texture = s->texture->parent ? s->texture->parent : s->texture;
	 const int src_format = texture->locked_region.format;
	 const int src_size = texture->locked_region.pixel_size;

	 /* Ensure u in [0, s->w) and v in [0, s->h). */
	 while (u < 0)
	    u += s->w;
	 while (v < 0)
	    v += s->h;
	 u = fmodf(u, s->w);
	 v = fmodf(v, s->h);
	 ASSERT(0 <= u);
	 ASSERT(u < s->w);
	 ASSERT(0 <= v);
	 ASSERT(v < s->h);

	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ALPHA && src_alpha == ALLEGRO_ALPHA && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_ONE && dst_alpha == ALLEGRO_ONE) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       }
	    } else if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

		     SHADE_COLORS(src_color, s->cur_color);

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
		     }

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    } else {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

		     SHADE_COLORS(src_color, s->cur_color);

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
		     }

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_texture_solid_any_draw_shade_white(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = s->target;

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

      u += s->du_dx * -x1;
      v += s->dv_dx * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      int op, src_mode, dst_mode;
      int op_alpha, src_alpha, dst_alpha;
      al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

      {
	 const int offset_x = s->texture->parent ? s->texture->xofs : 0;
	 const int offset_y = s->texture->parent ? s->texture->yofs : 0;
	 ALLEGRO_BITMAP *texture = s->texture->parent ? s->texture->parent : s->texture;
	 const int src_format = texture->locked_region.format;
	 const int src_size = texture->locked_region.pixel_size;

	 /* Ensure u in [0, s->w) and v in [0, s->h). */
	 while (u < 0)
	    u += s->w;
	 while (v < 0)
	    v += s->h;
	 u = fmodf(u, s->w);
	 v = fmodf(v, s->h);
	 ASSERT(0 <= u);
	 ASSERT(u < s->w);
	 ASSERT(0 <= v);
	 ASSERT(v < s->h);

	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ALPHA && src_alpha == ALLEGRO_ALPHA && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_ONE && dst_alpha == ALLEGRO_ONE) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

		     }
		  }
	       }
	    } else if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
		     }

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    } else {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
		     }

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_texture_solid_any_draw_opaque(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = s->target;

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

      u += s->du_dx * -x1;
      v += s->dv_dx * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      {
	 const int offset_x = s->texture->parent ? s->texture->xofs : 0;
	 const int offset_y = s->texture->parent ? s->texture->yofs : 0;
	 ALLEGRO_BITMAP *texture = s->texture->parent ? s->texture->parent : s->texture;
	 const int src_format = texture->locked_region.format;
	 const int src_size = texture->locked_region.pixel_size;

	 /* Ensure u in [0, s->w) and v in [0, s->h). */
	 while (u < 0)
	    u += s->w;
	 while (v < 0)
	    v += s->h;
	 u = fmodf(u, s->w);
	 v = fmodf(v, s->h);
	 ASSERT(0 <= u);
	 ASSERT(u < s->w);
	 ASSERT(0 <= v);
	 ASSERT(v < s->h);

	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			_AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, src_color, true);

			uu += du_dx;
			vv += dv_dx;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

		     SHADE_COLORS(src_color, s->cur_color);

		     _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, src_color, true);

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    } else {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, s->cur_color);

			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

			uu += du_dx;
			vv += dv_dx;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

		     SHADE_COLORS(src_color, s->cur_color);

		     _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_texture_solid_any_draw_opaque_white(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = s->target;

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

      u += s->du_dx * -x1;
      v += s->dv_dx * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      {
	 const int offset_x = s->texture->parent ? s->texture->xofs : 0;
	 const int offset_y = s->texture->parent ? s->texture->yofs : 0;
	 ALLEGRO_BITMAP *texture = s->texture->parent ? s->texture->parent : s->texture;
	 const int src_format = texture->locked_region.format;
	 const int src_size = texture->locked_region.pixel_size;

	 /* Ensure u in [0, s->w) and v in [0, s->h). */
	 while (u < 0)
	    u += s->w;
	 while (v < 0)
	    v += s->h;
	 u = fmodf(u, s->w);
	 v = fmodf(v, s->h);
	 ASSERT(0 <= u);
	 ASSERT(u < s->w);
	 ASSERT(0 <= v);
	 ASSERT(v < s->h);

	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (dst_format == src_format && src_size == 4) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * 4;

			switch (4) {
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

			uu += du_dx;
			vv += dv_dx;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * 4;

		     switch (4) {
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

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    } else if (dst_format == src_format && src_size == 3) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * 3;

			switch (3) {
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

			uu += du_dx;
			vv += dv_dx;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * 3;

		     switch (3) {
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

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    } else if (dst_format == src_format && src_size == 2) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * 2;

			switch (2) {
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

			uu += du_dx;
			vv += dv_dx;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * 2;

		     switch (2) {
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

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    } else {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

			uu += du_dx;
			vv += dv_dx;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

		     _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_texture_grad_any_draw_shade(uintptr_t state, int x1, int y, int x2)
{
   state_texture_grad_any_2d *gs = (state_texture_grad_any_2d *) state;
   state_texture_solid_any_2d *s = &gs->solid;
   ALLEGRO_COLOR cur_color = s->cur_color;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = s->target;

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

      u += s->du_dx * -x1;
      v += s->dv_dx * -x1;

      cur_color.r += gs->color_dx.r * -x1;
      cur_color.g += gs->color_dx.g * -x1;
      cur_color.b += gs->color_dx.b * -x1;
      cur_color.a += gs->color_dx.a * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      int op, src_mode, dst_mode;
      int op_alpha, src_alpha, dst_alpha;
      al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

      {
	 const int offset_x = s->texture->parent ? s->texture->xofs : 0;
	 const int offset_y = s->texture->parent ? s->texture->yofs : 0;
	 ALLEGRO_BITMAP *texture = s->texture->parent ? s->texture->parent : s->texture;
	 const int src_format = texture->locked_region.format;
	 const int src_size = texture->locked_region.pixel_size;

	 /* Ensure u in [0, s->w) and v in [0, s->h). */
	 while (u < 0)
	    u += s->w;
	 while (v < 0)
	    v += s->h;
	 u = fmodf(u, s->w);
	 v = fmodf(v, s->h);
	 ASSERT(0 <= u);
	 ASSERT(u < s->w);
	 ASSERT(0 <= v);
	 ASSERT(v < s->h);

	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ALPHA && src_alpha == ALLEGRO_ALPHA && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_INVERSE_ALPHA && dst_alpha == ALLEGRO_INVERSE_ALPHA) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       }
	    } else if (op == ALLEGRO_ADD && src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && op_alpha == ALLEGRO_ADD && dst_mode == ALLEGRO_ONE && dst_alpha == ALLEGRO_ONE) {

	       if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       } else {
		  uint8_t *lock_data = texture->locked_region.data;
		  const int src_pitch = texture->locked_region.pitch;
		  const al_fixed du_dx = al_ftofix(s->du_dx);
		  const al_fixed dv_dx = al_ftofix(s->dv_dx);

		  {
		     al_fixed uu = al_ftofix(u);
		     al_fixed vv = al_ftofix(v);
		     const int uu_ofs = offset_x - texture->lock_x;
		     const int vv_ofs = offset_y - texture->lock_y;
		     const al_fixed w = al_ftofix(s->w);
		     const al_fixed h = al_ftofix(s->h);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + uu_ofs;
			const int src_y = (vv >> 16) + vv_ofs;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			{
			   ALLEGRO_COLOR dst_color;
			   ALLEGRO_COLOR result;
			   _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			   _al_blend_alpha_inline(&src_color, &dst_color, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE, &result);
			   _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
			}

			uu += du_dx;
			vv += dv_dx;

			if (_AL_EXPECT_FAIL(uu < 0))
			   uu += w;
			else if (_AL_EXPECT_FAIL(uu >= w))
			   uu -= w;

			if (_AL_EXPECT_FAIL(vv < 0))
			   vv += h;
			else if (_AL_EXPECT_FAIL(vv >= h))
			   vv -= h;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       }
	    } else if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

		     SHADE_COLORS(src_color, cur_color);

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, result, true);
		     }

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    } else {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

		     SHADE_COLORS(src_color, cur_color);

		     {
			ALLEGRO_COLOR dst_color;
			ALLEGRO_COLOR result;
			_AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
			_al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
		     }

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    }
	 }
      }
   }
}

static void shader_texture_grad_any_draw_opaque(uintptr_t state, int x1, int y, int x2)
{
   state_texture_grad_any_2d *gs = (state_texture_grad_any_2d *) state;
   state_texture_solid_any_2d *s = &gs->solid;
   ALLEGRO_COLOR cur_color = s->cur_color;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = s->target;

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

      u += s->du_dx * -x1;
      v += s->dv_dx * -x1;

      cur_color.r += gs->color_dx.r * -x1;
      cur_color.g += gs->color_dx.g * -x1;
      cur_color.b += gs->color_dx.b * -x1;
      cur_color.a += gs->color_dx.a * -x1;

      x1 = 0;
   }

   if (x2 > target->lock_w - 1) {
      x2 = target->lock_w - 1;
   }

   {
      {
	 const int offset_x = s->texture->parent ? s->texture->xofs : 0;
	 const int offset_y = s->texture->parent ? s->texture->yofs : 0;
	 ALLEGRO_BITMAP *texture = s->texture->parent ? s->texture->parent : s->texture;
	 const int src_format = texture->locked_region.format;
	 const int src_size = texture->locked_region.pixel_size;

	 /* Ensure u in [0, s->w) and v in [0, s->h). */
	 while (u < 0)
	    u += s->w;
	 while (v < 0)
	    v += s->h;
	 u = fmodf(u, s->w);
	 v = fmodf(v, s->h);
	 ASSERT(0 <= u);
	 ASSERT(u < s->w);
	 ASSERT(0 <= v);
	 ASSERT(v < s->h);

	 {
	    const int dst_format = target->locked_region.format;
	    uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * target->locked_region.pixel_size;

	    if (dst_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888 && src_format == ALLEGRO_PIXEL_FORMAT_ARGB_8888) {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			_AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, src_color, true);

			uu += du_dx;
			vv += dv_dx;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, src_data, src_color, false);

		     SHADE_COLORS(src_color, cur_color);

		     _AL_INLINE_PUT_PIXEL(ALLEGRO_PIXEL_FORMAT_ARGB_8888, dst_data, src_color, true);

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    } else {
	       uint8_t *lock_data = texture->locked_region.data;
	       const int src_pitch = texture->locked_region.pitch;
	       const al_fixed du_dx = al_ftofix(s->du_dx);
	       const al_fixed dv_dx = al_ftofix(s->dv_dx);

	       const float steps = x2 - x1 + 1;
	       const float end_u = u + steps * s->du_dx;
	       const float end_v = v + steps * s->dv_dx;
	       if (end_u >= 0 && end_u < s->w && end_v >= 0 && end_v < s->h) {

		  {
		     al_fixed uu = al_ftofix(u) + ((offset_x - texture->lock_x) << 16);
		     al_fixed vv = al_ftofix(v) + ((offset_y - texture->lock_y) << 16);

		     for (; x1 <= x2; x1++) {
			const int src_x = (uu >> 16) + 0;
			const int src_y = (vv >> 16) + 0;
			uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

			ALLEGRO_COLOR src_color;
			_AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

			SHADE_COLORS(src_color, cur_color);

			_AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

			uu += du_dx;
			vv += dv_dx;

			cur_color.r += gs->color_dx.r;
			cur_color.g += gs->color_dx.g;
			cur_color.b += gs->color_dx.b;
			cur_color.a += gs->color_dx.a;

		     }
		  }
	       } else {
		  al_fixed uu = al_ftofix(u);
		  al_fixed vv = al_ftofix(v);
		  const int uu_ofs = offset_x - texture->lock_x;
		  const int vv_ofs = offset_y - texture->lock_y;
		  const al_fixed w = al_ftofix(s->w);
		  const al_fixed h = al_ftofix(s->h);

		  for (; x1 <= x2; x1++) {
		     const int src_x = (uu >> 16) + uu_ofs;
		     const int src_y = (vv >> 16) + vv_ofs;
		     uint8_t *src_data = lock_data + src_y * src_pitch + src_x * src_size;

		     ALLEGRO_COLOR src_color;
		     _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

		     SHADE_COLORS(src_color, cur_color);

		     _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

		     uu += du_dx;
		     vv += dv_dx;

		     if (_AL_EXPECT_FAIL(uu < 0))
			uu += w;
		     else if (_AL_EXPECT_FAIL(uu >= w))
			uu -= w;

		     if (_AL_EXPECT_FAIL(vv < 0))
			vv += h;
		     else if (_AL_EXPECT_FAIL(vv >= h))
			vv -= h;

		     cur_color.r += gs->color_dx.r;
		     cur_color.g += gs->color_dx.g;
		     cur_color.b += gs->color_dx.b;
		     cur_color.a += gs->color_dx.a;

		  }
	       }
	    }
	 }
      }
   }
}
