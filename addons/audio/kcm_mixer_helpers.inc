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
