// Warning: This file was created by make_resamplers.py - do not edit.
// vim: set ft=c:
static INLINE const void *point_spl32(SAMP_BUF * samp_buf, const ALLEGRO_SAMPLE_INSTANCE * spl, unsigned int maxc) {
   unsigned int i0 = spl->pos * maxc;
   unsigned int i;

   switch (spl->spl_data.depth) {

   case ALLEGRO_AUDIO_DEPTH_FLOAT32:
      for (i = 0; i < maxc; i++) {
	 samp_buf->f32[i] = spl->spl_data.buffer.f32[i0 + i];
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT24:
      for (i = 0; i < maxc; i++) {
	 samp_buf->f32[i] = (float) spl->spl_data.buffer.s24[i0 + i] / ((float) 0x7FFFFF + 0.5f);
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT24:
      for (i = 0; i < maxc; i++) {
	 samp_buf->f32[i] = (float) spl->spl_data.buffer.u24[i0 + i] / ((float) 0x7FFFFF + 0.5f) - 1.0f;
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT16:
      for (i = 0; i < maxc; i++) {
	 samp_buf->f32[i] = (float) spl->spl_data.buffer.s16[i0 + i] / ((float) 0x7FFF + 0.5f);
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT16:
      for (i = 0; i < maxc; i++) {
	 samp_buf->f32[i] = (float) spl->spl_data.buffer.u16[i0 + i] / ((float) 0x7FFF + 0.5f) - 1.0f;
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT8:
      for (i = 0; i < maxc; i++) {
	 samp_buf->f32[i] = (float) spl->spl_data.buffer.s8[i0 + i] / ((float) 0x7F + 0.5f);
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT8:
      for (i = 0; i < maxc; i++) {
	 samp_buf->f32[i] = (float) spl->spl_data.buffer.u8[i0 + i] / ((float) 0x7F + 0.5f) - 1.0f;
      }
      break;

   }
   return samp_buf->f32;
}

static INLINE const void *point_spl16(SAMP_BUF * samp_buf, const ALLEGRO_SAMPLE_INSTANCE * spl, unsigned int maxc) {
   unsigned int i0 = spl->pos * maxc;
   unsigned int i;

   switch (spl->spl_data.depth) {

   case ALLEGRO_AUDIO_DEPTH_FLOAT32:
      for (i = 0; i < maxc; i++) {
	 samp_buf->s16[i] = (int16_t) (spl->spl_data.buffer.f32[i0 + i] * 0x7FFF);
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT24:
      for (i = 0; i < maxc; i++) {
	 samp_buf->s16[i] = (int16_t) (spl->spl_data.buffer.s24[i0 + i] >> 9);
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT24:
      for (i = 0; i < maxc; i++) {
	 samp_buf->s16[i] = (int16_t) ((spl->spl_data.buffer.u24[i0 + i] - 0x800000) >> 9);
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT16:
      for (i = 0; i < maxc; i++) {
	 samp_buf->s16[i] = spl->spl_data.buffer.s16[i0 + i];
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT16:
      for (i = 0; i < maxc; i++) {
	 samp_buf->s16[i] = (int16_t) (spl->spl_data.buffer.u16[i0 + i] - 0x8000);
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT8:
      for (i = 0; i < maxc; i++) {
	 samp_buf->s16[i] = (int16_t) spl->spl_data.buffer.s8[i0 + i] << 7;
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT8:
      for (i = 0; i < maxc; i++) {
	 samp_buf->s16[i] = (int16_t) (spl->spl_data.buffer.u8[i0 + i] - 0x80) << 7;
      }
      break;

   }
   return samp_buf->s16;
}

static INLINE const void *linear_spl32(SAMP_BUF * samp_buf, const ALLEGRO_SAMPLE_INSTANCE * spl, unsigned int maxc) {
   int p0 = spl->pos;
   int p1 = spl->pos + 1;

   switch (spl->loop) {
   case ALLEGRO_PLAYMODE_ONCE:
      if (p1 >= spl->spl_data.len)
	 p1 = p0;
      break;
   case ALLEGRO_PLAYMODE_LOOP_ONCE:
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
   case _ALLEGRO_PLAYMODE_STREAM_LOOP_ONCE:
   case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:
      p0--;
      p1--;
      break;
   }

   p0 *= maxc;
   p1 *= maxc;

   switch (spl->spl_data.depth) {

   case ALLEGRO_AUDIO_DEPTH_FLOAT32:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const float x0 = spl->spl_data.buffer.f32[p0 + i];
	    const float x1 = spl->spl_data.buffer.f32[p1 + i];
	    const float s = (x0 * (1.0f - t)) + (x1 * t);
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT24:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const float x0 = (float) spl->spl_data.buffer.s24[p0 + i] / ((float) 0x7FFFFF + 0.5f);
	    const float x1 = (float) spl->spl_data.buffer.s24[p1 + i] / ((float) 0x7FFFFF + 0.5f);
	    const float s = (x0 * (1.0f - t)) + (x1 * t);
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT24:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const float x0 = (float) spl->spl_data.buffer.u24[p0 + i] / ((float) 0x7FFFFF + 0.5f) - 1.0f;
	    const float x1 = (float) spl->spl_data.buffer.u24[p1 + i] / ((float) 0x7FFFFF + 0.5f) - 1.0f;
	    const float s = (x0 * (1.0f - t)) + (x1 * t);
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT16:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const float x0 = (float) spl->spl_data.buffer.s16[p0 + i] / ((float) 0x7FFF + 0.5f);
	    const float x1 = (float) spl->spl_data.buffer.s16[p1 + i] / ((float) 0x7FFF + 0.5f);
	    const float s = (x0 * (1.0f - t)) + (x1 * t);
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT16:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const float x0 = (float) spl->spl_data.buffer.u16[p0 + i] / ((float) 0x7FFF + 0.5f) - 1.0f;
	    const float x1 = (float) spl->spl_data.buffer.u16[p1 + i] / ((float) 0x7FFF + 0.5f) - 1.0f;
	    const float s = (x0 * (1.0f - t)) + (x1 * t);
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT8:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const float x0 = (float) spl->spl_data.buffer.s8[p0 + i] / ((float) 0x7F + 0.5f);
	    const float x1 = (float) spl->spl_data.buffer.s8[p1 + i] / ((float) 0x7F + 0.5f);
	    const float s = (x0 * (1.0f - t)) + (x1 * t);
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT8:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const float x0 = (float) spl->spl_data.buffer.u8[p0 + i] / ((float) 0x7F + 0.5f) - 1.0f;
	    const float x1 = (float) spl->spl_data.buffer.u8[p1 + i] / ((float) 0x7F + 0.5f) - 1.0f;
	    const float s = (x0 * (1.0f - t)) + (x1 * t);
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   }
   return samp_buf->f32;
}

static INLINE const void *linear_spl16(SAMP_BUF * samp_buf, const ALLEGRO_SAMPLE_INSTANCE * spl, unsigned int maxc) {
   int p0 = spl->pos;
   int p1 = spl->pos + 1;

   switch (spl->loop) {
   case ALLEGRO_PLAYMODE_ONCE:
      if (p1 >= spl->spl_data.len)
	 p1 = p0;
      break;
   case ALLEGRO_PLAYMODE_LOOP_ONCE:
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
   case _ALLEGRO_PLAYMODE_STREAM_LOOP_ONCE:
   case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:
      p0--;
      p1--;
      break;
   }

   p0 *= maxc;
   p1 *= maxc;

   switch (spl->spl_data.depth) {

   case ALLEGRO_AUDIO_DEPTH_FLOAT32:
      {
	 const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const int32_t x0 = (int16_t) (spl->spl_data.buffer.f32[p0 + i] * 0x7FFF);
	    const int32_t x1 = (int16_t) (spl->spl_data.buffer.f32[p1 + i] * 0x7FFF);
	    const int32_t s = ((x0 * (256 - t)) >> 8) + ((x1 * t) >> 8);
	    samp_buf->s16[i] = (int16_t) s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT24:
      {
	 const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const int32_t x0 = (int16_t) (spl->spl_data.buffer.s24[p0 + i] >> 9);
	    const int32_t x1 = (int16_t) (spl->spl_data.buffer.s24[p1 + i] >> 9);
	    const int32_t s = ((x0 * (256 - t)) >> 8) + ((x1 * t) >> 8);
	    samp_buf->s16[i] = (int16_t) s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT24:
      {
	 const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const int32_t x0 = (int16_t) ((spl->spl_data.buffer.u24[p0 + i] - 0x800000) >> 9);
	    const int32_t x1 = (int16_t) ((spl->spl_data.buffer.u24[p1 + i] - 0x800000) >> 9);
	    const int32_t s = ((x0 * (256 - t)) >> 8) + ((x1 * t) >> 8);
	    samp_buf->s16[i] = (int16_t) s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT16:
      {
	 const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const int32_t x0 = spl->spl_data.buffer.s16[p0 + i];
	    const int32_t x1 = spl->spl_data.buffer.s16[p1 + i];
	    const int32_t s = ((x0 * (256 - t)) >> 8) + ((x1 * t) >> 8);
	    samp_buf->s16[i] = (int16_t) s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT16:
      {
	 const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const int32_t x0 = (int16_t) (spl->spl_data.buffer.u16[p0 + i] - 0x8000);
	    const int32_t x1 = (int16_t) (spl->spl_data.buffer.u16[p1 + i] - 0x8000);
	    const int32_t s = ((x0 * (256 - t)) >> 8) + ((x1 * t) >> 8);
	    samp_buf->s16[i] = (int16_t) s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT8:
      {
	 const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const int32_t x0 = (int16_t) spl->spl_data.buffer.s8[p0 + i] << 7;
	    const int32_t x1 = (int16_t) spl->spl_data.buffer.s8[p1 + i] << 7;
	    const int32_t s = ((x0 * (256 - t)) >> 8) + ((x1 * t) >> 8);
	    samp_buf->s16[i] = (int16_t) s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT8:
      {
	 const int32_t t = 256 * spl->pos_bresenham_error / spl->step_denom;
	 int i;
	 for (i = 0; i < (int) maxc; i++) {
	    const int32_t x0 = (int16_t) (spl->spl_data.buffer.u8[p0 + i] - 0x80) << 7;
	    const int32_t x1 = (int16_t) (spl->spl_data.buffer.u8[p1 + i] - 0x80) << 7;
	    const int32_t s = ((x0 * (256 - t)) >> 8) + ((x1 * t) >> 8);
	    samp_buf->s16[i] = (int16_t) s;
	 }
      }
      break;

   }
   return samp_buf->s16;
}

static INLINE const void *cubic_spl32(SAMP_BUF * samp_buf, const ALLEGRO_SAMPLE_INSTANCE * spl, unsigned int maxc) {
   int p0 = spl->pos - 1;
   int p1 = spl->pos;
   int p2 = spl->pos + 1;
   int p3 = spl->pos + 2;

   switch (spl->loop) {
   case ALLEGRO_PLAYMODE_ONCE:
      if (p0 < 0)
	 p0 = 0;
      if (p2 >= spl->spl_data.len)
	 p2 = spl->spl_data.len - 1;
      if (p3 >= spl->spl_data.len)
	 p3 = spl->spl_data.len - 1;
      break;
   case ALLEGRO_PLAYMODE_LOOP_ONCE:
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
   case _ALLEGRO_PLAYMODE_STREAM_LOOP_ONCE:
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

   case ALLEGRO_AUDIO_DEPTH_FLOAT32:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 signed int i;
	 for (i = 0; i < (signed int) maxc; i++) {
	    float x0 = spl->spl_data.buffer.f32[p0 + i];
	    float x1 = spl->spl_data.buffer.f32[p1 + i];
	    float x2 = spl->spl_data.buffer.f32[p2 + i];
	    float x3 = spl->spl_data.buffer.f32[p3 + i];
	    float c0 = x1;
	    float c1 = 0.5f * (x2 - x0);
	    float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
	    float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
	    float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT24:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 signed int i;
	 for (i = 0; i < (signed int) maxc; i++) {
	    float x0 = (float) spl->spl_data.buffer.s24[p0 + i] / ((float) 0x7FFFFF + 0.5f);
	    float x1 = (float) spl->spl_data.buffer.s24[p1 + i] / ((float) 0x7FFFFF + 0.5f);
	    float x2 = (float) spl->spl_data.buffer.s24[p2 + i] / ((float) 0x7FFFFF + 0.5f);
	    float x3 = (float) spl->spl_data.buffer.s24[p3 + i] / ((float) 0x7FFFFF + 0.5f);
	    float c0 = x1;
	    float c1 = 0.5f * (x2 - x0);
	    float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
	    float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
	    float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT24:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 signed int i;
	 for (i = 0; i < (signed int) maxc; i++) {
	    float x0 = (float) spl->spl_data.buffer.u24[p0 + i] / ((float) 0x7FFFFF + 0.5f) - 1.0f;
	    float x1 = (float) spl->spl_data.buffer.u24[p1 + i] / ((float) 0x7FFFFF + 0.5f) - 1.0f;
	    float x2 = (float) spl->spl_data.buffer.u24[p2 + i] / ((float) 0x7FFFFF + 0.5f) - 1.0f;
	    float x3 = (float) spl->spl_data.buffer.u24[p3 + i] / ((float) 0x7FFFFF + 0.5f) - 1.0f;
	    float c0 = x1;
	    float c1 = 0.5f * (x2 - x0);
	    float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
	    float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
	    float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT16:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 signed int i;
	 for (i = 0; i < (signed int) maxc; i++) {
	    float x0 = (float) spl->spl_data.buffer.s16[p0 + i] / ((float) 0x7FFF + 0.5f);
	    float x1 = (float) spl->spl_data.buffer.s16[p1 + i] / ((float) 0x7FFF + 0.5f);
	    float x2 = (float) spl->spl_data.buffer.s16[p2 + i] / ((float) 0x7FFF + 0.5f);
	    float x3 = (float) spl->spl_data.buffer.s16[p3 + i] / ((float) 0x7FFF + 0.5f);
	    float c0 = x1;
	    float c1 = 0.5f * (x2 - x0);
	    float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
	    float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
	    float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT16:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 signed int i;
	 for (i = 0; i < (signed int) maxc; i++) {
	    float x0 = (float) spl->spl_data.buffer.u16[p0 + i] / ((float) 0x7FFF + 0.5f) - 1.0f;
	    float x1 = (float) spl->spl_data.buffer.u16[p1 + i] / ((float) 0x7FFF + 0.5f) - 1.0f;
	    float x2 = (float) spl->spl_data.buffer.u16[p2 + i] / ((float) 0x7FFF + 0.5f) - 1.0f;
	    float x3 = (float) spl->spl_data.buffer.u16[p3 + i] / ((float) 0x7FFF + 0.5f) - 1.0f;
	    float c0 = x1;
	    float c1 = 0.5f * (x2 - x0);
	    float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
	    float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
	    float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_INT8:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 signed int i;
	 for (i = 0; i < (signed int) maxc; i++) {
	    float x0 = (float) spl->spl_data.buffer.s8[p0 + i] / ((float) 0x7F + 0.5f);
	    float x1 = (float) spl->spl_data.buffer.s8[p1 + i] / ((float) 0x7F + 0.5f);
	    float x2 = (float) spl->spl_data.buffer.s8[p2 + i] / ((float) 0x7F + 0.5f);
	    float x3 = (float) spl->spl_data.buffer.s8[p3 + i] / ((float) 0x7F + 0.5f);
	    float c0 = x1;
	    float c1 = 0.5f * (x2 - x0);
	    float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
	    float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
	    float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   case ALLEGRO_AUDIO_DEPTH_UINT8:
      {
	 const float t = (float) spl->pos_bresenham_error / spl->step_denom;
	 signed int i;
	 for (i = 0; i < (signed int) maxc; i++) {
	    float x0 = (float) spl->spl_data.buffer.u8[p0 + i] / ((float) 0x7F + 0.5f) - 1.0f;
	    float x1 = (float) spl->spl_data.buffer.u8[p1 + i] / ((float) 0x7F + 0.5f) - 1.0f;
	    float x2 = (float) spl->spl_data.buffer.u8[p2 + i] / ((float) 0x7F + 0.5f) - 1.0f;
	    float x3 = (float) spl->spl_data.buffer.u8[p3 + i] / ((float) 0x7F + 0.5f) - 1.0f;
	    float c0 = x1;
	    float c1 = 0.5f * (x2 - x0);
	    float c2 = x0 - (2.5f * x1) + (2.0f * x2) - (0.5f * x3);
	    float c3 = (0.5f * (x3 - x0)) + (1.5f * (x1 - x2));
	    float s = (((((c3 * t) + c2) * t) + c1) * t) + c0;
	    samp_buf->f32[i] = s;
	 }
      }
      break;

   }
   return samp_buf->f32;
}
