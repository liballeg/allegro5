// Warning: This file was created by make_scanline_drawers.py - do not edit.

static void shader_solid_any_draw_shade(uintptr_t state, int x1, int y, int x2)
{
   state_solid_any_2d *s = (state_solid_any_2d *) state;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

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

static void shader_solid_any_draw_opaque(uintptr_t state, int x1, int y, int x2)
{
   state_solid_any_2d *s = (state_solid_any_2d *) state;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

	 for (; x1 <= x2; x1++) {

	    ALLEGRO_COLOR src_color = cur_color;

	    _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

	 }
      }
   }
}

static void shader_grad_any_draw_shade(uintptr_t state, int x1, int y, int x2)
{
   state_grad_any_2d *gs = (state_grad_any_2d *) state;
   state_solid_any_2d *s = &gs->solid;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

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

static void shader_grad_any_draw_opaque(uintptr_t state, int x1, int y, int x2)
{
   state_grad_any_2d *gs = (state_grad_any_2d *) state;
   state_solid_any_2d *s = &gs->solid;
   ALLEGRO_COLOR cur_color = s->cur_color;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

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

static void shader_texture_solid_any_draw_shade(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int src_format = s->texture->locked_region.format;
	 const int src_size = al_get_pixel_size(src_format);
	 ALLEGRO_COLOR src_color = { 0, 0, 0, 0 };

	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

	 for (; x1 <= x2; x1++) {

	    const int src_x = fix_var(u, s->w);
	    const int src_y = fix_var(v, s->h);
	    uint8_t *src_data = (uint8_t *) s->texture->locked_region.data + (src_y - s->texture->lock_y) * s->texture->locked_region.pitch + (src_x - s->texture->lock_x) * src_size;
	    _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

	    SHADE_COLORS(src_color, s->cur_color);

	    {
	       ALLEGRO_COLOR dst_color;
	       ALLEGRO_COLOR result;
	       _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
	       _al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
	       _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
	    }

	    u += s->du_dx;
	    v += s->dv_dx;

	 }
      }
   }
}

static void shader_texture_solid_any_draw_shade_white(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int src_format = s->texture->locked_region.format;
	 const int src_size = al_get_pixel_size(src_format);
	 ALLEGRO_COLOR src_color = { 0, 0, 0, 0 };

	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

	 for (; x1 <= x2; x1++) {

	    const int src_x = fix_var(u, s->w);
	    const int src_y = fix_var(v, s->h);
	    uint8_t *src_data = (uint8_t *) s->texture->locked_region.data + (src_y - s->texture->lock_y) * s->texture->locked_region.pitch + (src_x - s->texture->lock_x) * src_size;
	    _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

	    {
	       ALLEGRO_COLOR dst_color;
	       ALLEGRO_COLOR result;
	       _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
	       _al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
	       _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
	    }

	    u += s->du_dx;
	    v += s->dv_dx;

	 }
      }
   }
}

static void shader_texture_solid_any_draw_opaque(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int src_format = s->texture->locked_region.format;
	 const int src_size = al_get_pixel_size(src_format);
	 ALLEGRO_COLOR src_color = { 0, 0, 0, 0 };

	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

	 for (; x1 <= x2; x1++) {

	    const int src_x = fix_var(u, s->w);
	    const int src_y = fix_var(v, s->h);
	    uint8_t *src_data = (uint8_t *) s->texture->locked_region.data + (src_y - s->texture->lock_y) * s->texture->locked_region.pitch + (src_x - s->texture->lock_x) * src_size;
	    _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

	    SHADE_COLORS(src_color, s->cur_color);

	    _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

	    u += s->du_dx;
	    v += s->dv_dx;

	 }
      }
   }
}

static void shader_texture_solid_any_draw_opaque_white(uintptr_t state, int x1, int y, int x2)
{
   state_texture_solid_any_2d *s = (state_texture_solid_any_2d *) state;

   float u = s->u;
   float v = s->v;

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int src_format = s->texture->locked_region.format;
	 const int src_size = al_get_pixel_size(src_format);
	 ALLEGRO_COLOR src_color = { 0, 0, 0, 0 };

	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

	 for (; x1 <= x2; x1++) {

	    const int src_x = fix_var(u, s->w);
	    const int src_y = fix_var(v, s->h);
	    uint8_t *src_data = (uint8_t *) s->texture->locked_region.data + (src_y - s->texture->lock_y) * s->texture->locked_region.pitch + (src_x - s->texture->lock_x) * src_size;
	    _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

	    _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

	    u += s->du_dx;
	    v += s->dv_dx;

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

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int src_format = s->texture->locked_region.format;
	 const int src_size = al_get_pixel_size(src_format);
	 ALLEGRO_COLOR src_color = { 0, 0, 0, 0 };

	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

	 for (; x1 <= x2; x1++) {

	    const int src_x = fix_var(u, s->w);
	    const int src_y = fix_var(v, s->h);
	    uint8_t *src_data = (uint8_t *) s->texture->locked_region.data + (src_y - s->texture->lock_y) * s->texture->locked_region.pitch + (src_x - s->texture->lock_x) * src_size;
	    _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

	    SHADE_COLORS(src_color, cur_color);

	    {
	       ALLEGRO_COLOR dst_color;
	       ALLEGRO_COLOR result;
	       _AL_INLINE_GET_PIXEL(dst_format, dst_data, dst_color, false);
	       _al_blend_inline(&src_color, &dst_color, op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
	       _AL_INLINE_PUT_PIXEL(dst_format, dst_data, result, true);
	    }

	    u += s->du_dx;
	    v += s->dv_dx;

	    cur_color.r += gs->color_dx.r;
	    cur_color.g += gs->color_dx.g;
	    cur_color.b += gs->color_dx.b;
	    cur_color.a += gs->color_dx.a;

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

   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
	 const int src_format = s->texture->locked_region.format;
	 const int src_size = al_get_pixel_size(src_format);
	 ALLEGRO_COLOR src_color = { 0, 0, 0, 0 };

	 const int dst_format = target->locked_region.format;
	 uint8_t *dst_data = (uint8_t *) target->locked_region.data + y * target->locked_region.pitch + x1 * al_get_pixel_size(dst_format);

	 for (; x1 <= x2; x1++) {

	    const int src_x = fix_var(u, s->w);
	    const int src_y = fix_var(v, s->h);
	    uint8_t *src_data = (uint8_t *) s->texture->locked_region.data + (src_y - s->texture->lock_y) * s->texture->locked_region.pitch + (src_x - s->texture->lock_x) * src_size;
	    _AL_INLINE_GET_PIXEL(src_format, src_data, src_color, false);

	    SHADE_COLORS(src_color, cur_color);

	    _AL_INLINE_PUT_PIXEL(dst_format, dst_data, src_color, true);

	    u += s->du_dx;
	    v += s->dv_dx;

	    cur_color.r += gs->color_dx.r;
	    cur_color.g += gs->color_dx.g;
	    cur_color.b += gs->color_dx.b;
	    cur_color.a += gs->color_dx.a;

	 }
      }
   }
}
