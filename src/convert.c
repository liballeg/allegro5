// Warning: This file was created by make_converters.py - do not edit.
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_convert.h"
static void argb_8888_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ARGB_8888_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ARGB_8888_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_8888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_8888_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBA_8888_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBA_8888_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_8888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_8888_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ARGB_4444_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ARGB_4444_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_4444_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_4444_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_ARGB_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_RGBA_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_ARGB_4444(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_RGB_565(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_RGB_555(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_RGBA_5551(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_ARGB_1555(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_ABGR_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_XBGR_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx * 3;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         int dst_pixel = ALLEGRO_CONVERT_RGB_888_TO_BGR_888(src_pixel);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1 * 3;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_BGR_565(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_BGR_555(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_RGBX_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_XRGB_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_ABGR_F32(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_ABGR_8888_LE(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_RGB_888_TO_RGBA_4444(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGB_565_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGB_565_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_565_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_565_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGB_555_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGB_555_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgb_555_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGB_555_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBA_5551_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBA_5551_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_5551_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_5551_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ARGB_1555_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ARGB_1555_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void argb_1555_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ARGB_1555_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ABGR_8888_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ABGR_8888_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_XBGR_8888_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_XBGR_8888_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xbgr_8888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XBGR_8888_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_ARGB_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_RGBA_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_ARGB_4444(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx * 3;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         int dst_pixel = ALLEGRO_CONVERT_BGR_888_TO_RGB_888(src_pixel);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1 * 3;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_RGB_565(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_RGB_555(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_RGBA_5551(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_ARGB_1555(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_ABGR_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_XBGR_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_BGR_565(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_BGR_555(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_RGBX_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_XRGB_8888(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_ABGR_F32(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_ABGR_8888_LE(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint8_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 1 - width * 3;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx * 3;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         #ifdef ALLEGRO_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
         *dst_ptr = ALLEGRO_CONVERT_BGR_888_TO_RGBA_4444(src_pixel);
         src_ptr += 1 * 3;
         dst_ptr += 1;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_BGR_565_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_BGR_565_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_565_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_565_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_BGR_555_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_BGR_555_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void bgr_555_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_BGR_555_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBX_8888_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBX_8888_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgbx_8888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBX_8888_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_XRGB_8888_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_XRGB_8888_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void xrgb_8888_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_XRGB_8888_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ABGR_F32_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ABGR_F32_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_f32_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   ALLEGRO_COLOR *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 16 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_F32_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ABGR_8888_LE_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_ABGR_8888_LE_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void abgr_8888_le_to_rgba_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint32_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 4 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_ABGR_8888_LE_TO_RGBA_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_argb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_ARGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_rgba_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_RGBA_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_argb_4444(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_ARGB_4444(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_rgb_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBA_4444_TO_RGB_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_rgb_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_RGB_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_rgb_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_RGB_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_rgba_5551(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_RGBA_5551(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_argb_1555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_ARGB_1555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_abgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_ABGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_xbgr_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_XBGR_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_bgr_888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint8_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 1 - width * 3;
   src_ptr += sx;
   dst_ptr += dx * 3;
   for (y = 0; y < height; y++) {
      uint8_t *dst_end = dst_ptr + width * 3;
      while (dst_ptr < dst_end) {
         int dst_pixel = ALLEGRO_CONVERT_RGBA_4444_TO_BGR_888(*src_ptr);
         #ifdef ALLEGRO_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
         src_ptr += 1;
         dst_ptr += 1 * 3;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_bgr_565(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_BGR_565(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_bgr_555(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint16_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 2 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint16_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_BGR_555(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_rgbx_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_RGBX_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_xrgb_8888(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_XRGB_8888(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_abgr_f32(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   ALLEGRO_COLOR *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 16 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      ALLEGRO_COLOR *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_ABGR_F32(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
static void rgba_4444_to_abgr_8888_le(void *src, int src_pitch,
   void *dst, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   int y;
   uint16_t *src_ptr = (void *)((char *)src + sy * src_pitch);
   uint32_t *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / 2 - width;
   int dst_gap = dst_pitch / 4 - width;
   src_ptr += sx;
   dst_ptr += dx;
   for (y = 0; y < height; y++) {
      uint32_t *dst_end = dst_ptr + width;
      while (dst_ptr < dst_end) {
         *dst_ptr = ALLEGRO_CONVERT_RGBA_4444_TO_ABGR_8888_LE(*src_ptr);
         dst_ptr++;
         src_ptr++;
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
}
void (*_al_convert_funcs[ALLEGRO_NUM_PIXEL_FORMATS]
   [ALLEGRO_NUM_PIXEL_FORMATS])(void *, int, void *, int,
   int, int, int, int, int, int) = {
   {NULL},
   {NULL},
   {NULL},
   {NULL},
   {NULL},
   {NULL},
   {NULL},
   {NULL},
   {NULL},
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      argb_8888_to_rgba_8888,
      argb_8888_to_argb_4444,
      argb_8888_to_rgb_888,
      argb_8888_to_rgb_565,
      argb_8888_to_rgb_555,
      argb_8888_to_rgba_5551,
      argb_8888_to_argb_1555,
      argb_8888_to_abgr_8888,
      argb_8888_to_xbgr_8888,
      argb_8888_to_bgr_888,
      argb_8888_to_bgr_565,
      argb_8888_to_bgr_555,
      argb_8888_to_rgbx_8888,
      argb_8888_to_xrgb_8888,
      argb_8888_to_abgr_f32,
      argb_8888_to_abgr_8888_le,
      argb_8888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      rgba_8888_to_argb_8888,
      NULL,
      rgba_8888_to_argb_4444,
      rgba_8888_to_rgb_888,
      rgba_8888_to_rgb_565,
      rgba_8888_to_rgb_555,
      rgba_8888_to_rgba_5551,
      rgba_8888_to_argb_1555,
      rgba_8888_to_abgr_8888,
      rgba_8888_to_xbgr_8888,
      rgba_8888_to_bgr_888,
      rgba_8888_to_bgr_565,
      rgba_8888_to_bgr_555,
      rgba_8888_to_rgbx_8888,
      rgba_8888_to_xrgb_8888,
      rgba_8888_to_abgr_f32,
      rgba_8888_to_abgr_8888_le,
      rgba_8888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      argb_4444_to_argb_8888,
      argb_4444_to_rgba_8888,
      NULL,
      argb_4444_to_rgb_888,
      argb_4444_to_rgb_565,
      argb_4444_to_rgb_555,
      argb_4444_to_rgba_5551,
      argb_4444_to_argb_1555,
      argb_4444_to_abgr_8888,
      argb_4444_to_xbgr_8888,
      argb_4444_to_bgr_888,
      argb_4444_to_bgr_565,
      argb_4444_to_bgr_555,
      argb_4444_to_rgbx_8888,
      argb_4444_to_xrgb_8888,
      argb_4444_to_abgr_f32,
      argb_4444_to_abgr_8888_le,
      argb_4444_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      rgb_888_to_argb_8888,
      rgb_888_to_rgba_8888,
      rgb_888_to_argb_4444,
      NULL,
      rgb_888_to_rgb_565,
      rgb_888_to_rgb_555,
      rgb_888_to_rgba_5551,
      rgb_888_to_argb_1555,
      rgb_888_to_abgr_8888,
      rgb_888_to_xbgr_8888,
      rgb_888_to_bgr_888,
      rgb_888_to_bgr_565,
      rgb_888_to_bgr_555,
      rgb_888_to_rgbx_8888,
      rgb_888_to_xrgb_8888,
      rgb_888_to_abgr_f32,
      rgb_888_to_abgr_8888_le,
      rgb_888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      rgb_565_to_argb_8888,
      rgb_565_to_rgba_8888,
      rgb_565_to_argb_4444,
      rgb_565_to_rgb_888,
      NULL,
      rgb_565_to_rgb_555,
      rgb_565_to_rgba_5551,
      rgb_565_to_argb_1555,
      rgb_565_to_abgr_8888,
      rgb_565_to_xbgr_8888,
      rgb_565_to_bgr_888,
      rgb_565_to_bgr_565,
      rgb_565_to_bgr_555,
      rgb_565_to_rgbx_8888,
      rgb_565_to_xrgb_8888,
      rgb_565_to_abgr_f32,
      rgb_565_to_abgr_8888_le,
      rgb_565_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      rgb_555_to_argb_8888,
      rgb_555_to_rgba_8888,
      rgb_555_to_argb_4444,
      rgb_555_to_rgb_888,
      rgb_555_to_rgb_565,
      NULL,
      rgb_555_to_rgba_5551,
      rgb_555_to_argb_1555,
      rgb_555_to_abgr_8888,
      rgb_555_to_xbgr_8888,
      rgb_555_to_bgr_888,
      rgb_555_to_bgr_565,
      rgb_555_to_bgr_555,
      rgb_555_to_rgbx_8888,
      rgb_555_to_xrgb_8888,
      rgb_555_to_abgr_f32,
      rgb_555_to_abgr_8888_le,
      rgb_555_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      rgba_5551_to_argb_8888,
      rgba_5551_to_rgba_8888,
      rgba_5551_to_argb_4444,
      rgba_5551_to_rgb_888,
      rgba_5551_to_rgb_565,
      rgba_5551_to_rgb_555,
      NULL,
      rgba_5551_to_argb_1555,
      rgba_5551_to_abgr_8888,
      rgba_5551_to_xbgr_8888,
      rgba_5551_to_bgr_888,
      rgba_5551_to_bgr_565,
      rgba_5551_to_bgr_555,
      rgba_5551_to_rgbx_8888,
      rgba_5551_to_xrgb_8888,
      rgba_5551_to_abgr_f32,
      rgba_5551_to_abgr_8888_le,
      rgba_5551_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      argb_1555_to_argb_8888,
      argb_1555_to_rgba_8888,
      argb_1555_to_argb_4444,
      argb_1555_to_rgb_888,
      argb_1555_to_rgb_565,
      argb_1555_to_rgb_555,
      argb_1555_to_rgba_5551,
      NULL,
      argb_1555_to_abgr_8888,
      argb_1555_to_xbgr_8888,
      argb_1555_to_bgr_888,
      argb_1555_to_bgr_565,
      argb_1555_to_bgr_555,
      argb_1555_to_rgbx_8888,
      argb_1555_to_xrgb_8888,
      argb_1555_to_abgr_f32,
      argb_1555_to_abgr_8888_le,
      argb_1555_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      abgr_8888_to_argb_8888,
      abgr_8888_to_rgba_8888,
      abgr_8888_to_argb_4444,
      abgr_8888_to_rgb_888,
      abgr_8888_to_rgb_565,
      abgr_8888_to_rgb_555,
      abgr_8888_to_rgba_5551,
      abgr_8888_to_argb_1555,
      NULL,
      abgr_8888_to_xbgr_8888,
      abgr_8888_to_bgr_888,
      abgr_8888_to_bgr_565,
      abgr_8888_to_bgr_555,
      abgr_8888_to_rgbx_8888,
      abgr_8888_to_xrgb_8888,
      abgr_8888_to_abgr_f32,
      abgr_8888_to_abgr_8888_le,
      abgr_8888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      xbgr_8888_to_argb_8888,
      xbgr_8888_to_rgba_8888,
      xbgr_8888_to_argb_4444,
      xbgr_8888_to_rgb_888,
      xbgr_8888_to_rgb_565,
      xbgr_8888_to_rgb_555,
      xbgr_8888_to_rgba_5551,
      xbgr_8888_to_argb_1555,
      xbgr_8888_to_abgr_8888,
      NULL,
      xbgr_8888_to_bgr_888,
      xbgr_8888_to_bgr_565,
      xbgr_8888_to_bgr_555,
      xbgr_8888_to_rgbx_8888,
      xbgr_8888_to_xrgb_8888,
      xbgr_8888_to_abgr_f32,
      xbgr_8888_to_abgr_8888_le,
      xbgr_8888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      bgr_888_to_argb_8888,
      bgr_888_to_rgba_8888,
      bgr_888_to_argb_4444,
      bgr_888_to_rgb_888,
      bgr_888_to_rgb_565,
      bgr_888_to_rgb_555,
      bgr_888_to_rgba_5551,
      bgr_888_to_argb_1555,
      bgr_888_to_abgr_8888,
      bgr_888_to_xbgr_8888,
      NULL,
      bgr_888_to_bgr_565,
      bgr_888_to_bgr_555,
      bgr_888_to_rgbx_8888,
      bgr_888_to_xrgb_8888,
      bgr_888_to_abgr_f32,
      bgr_888_to_abgr_8888_le,
      bgr_888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      bgr_565_to_argb_8888,
      bgr_565_to_rgba_8888,
      bgr_565_to_argb_4444,
      bgr_565_to_rgb_888,
      bgr_565_to_rgb_565,
      bgr_565_to_rgb_555,
      bgr_565_to_rgba_5551,
      bgr_565_to_argb_1555,
      bgr_565_to_abgr_8888,
      bgr_565_to_xbgr_8888,
      bgr_565_to_bgr_888,
      NULL,
      bgr_565_to_bgr_555,
      bgr_565_to_rgbx_8888,
      bgr_565_to_xrgb_8888,
      bgr_565_to_abgr_f32,
      bgr_565_to_abgr_8888_le,
      bgr_565_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      bgr_555_to_argb_8888,
      bgr_555_to_rgba_8888,
      bgr_555_to_argb_4444,
      bgr_555_to_rgb_888,
      bgr_555_to_rgb_565,
      bgr_555_to_rgb_555,
      bgr_555_to_rgba_5551,
      bgr_555_to_argb_1555,
      bgr_555_to_abgr_8888,
      bgr_555_to_xbgr_8888,
      bgr_555_to_bgr_888,
      bgr_555_to_bgr_565,
      NULL,
      bgr_555_to_rgbx_8888,
      bgr_555_to_xrgb_8888,
      bgr_555_to_abgr_f32,
      bgr_555_to_abgr_8888_le,
      bgr_555_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      rgbx_8888_to_argb_8888,
      rgbx_8888_to_rgba_8888,
      rgbx_8888_to_argb_4444,
      rgbx_8888_to_rgb_888,
      rgbx_8888_to_rgb_565,
      rgbx_8888_to_rgb_555,
      rgbx_8888_to_rgba_5551,
      rgbx_8888_to_argb_1555,
      rgbx_8888_to_abgr_8888,
      rgbx_8888_to_xbgr_8888,
      rgbx_8888_to_bgr_888,
      rgbx_8888_to_bgr_565,
      rgbx_8888_to_bgr_555,
      NULL,
      rgbx_8888_to_xrgb_8888,
      rgbx_8888_to_abgr_f32,
      rgbx_8888_to_abgr_8888_le,
      rgbx_8888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      xrgb_8888_to_argb_8888,
      xrgb_8888_to_rgba_8888,
      xrgb_8888_to_argb_4444,
      xrgb_8888_to_rgb_888,
      xrgb_8888_to_rgb_565,
      xrgb_8888_to_rgb_555,
      xrgb_8888_to_rgba_5551,
      xrgb_8888_to_argb_1555,
      xrgb_8888_to_abgr_8888,
      xrgb_8888_to_xbgr_8888,
      xrgb_8888_to_bgr_888,
      xrgb_8888_to_bgr_565,
      xrgb_8888_to_bgr_555,
      xrgb_8888_to_rgbx_8888,
      NULL,
      xrgb_8888_to_abgr_f32,
      xrgb_8888_to_abgr_8888_le,
      xrgb_8888_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      abgr_f32_to_argb_8888,
      abgr_f32_to_rgba_8888,
      abgr_f32_to_argb_4444,
      abgr_f32_to_rgb_888,
      abgr_f32_to_rgb_565,
      abgr_f32_to_rgb_555,
      abgr_f32_to_rgba_5551,
      abgr_f32_to_argb_1555,
      abgr_f32_to_abgr_8888,
      abgr_f32_to_xbgr_8888,
      abgr_f32_to_bgr_888,
      abgr_f32_to_bgr_565,
      abgr_f32_to_bgr_555,
      abgr_f32_to_rgbx_8888,
      abgr_f32_to_xrgb_8888,
      NULL,
      abgr_f32_to_abgr_8888_le,
      abgr_f32_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      abgr_8888_le_to_argb_8888,
      abgr_8888_le_to_rgba_8888,
      abgr_8888_le_to_argb_4444,
      abgr_8888_le_to_rgb_888,
      abgr_8888_le_to_rgb_565,
      abgr_8888_le_to_rgb_555,
      abgr_8888_le_to_rgba_5551,
      abgr_8888_le_to_argb_1555,
      abgr_8888_le_to_abgr_8888,
      abgr_8888_le_to_xbgr_8888,
      abgr_8888_le_to_bgr_888,
      abgr_8888_le_to_bgr_565,
      abgr_8888_le_to_bgr_555,
      abgr_8888_le_to_rgbx_8888,
      abgr_8888_le_to_xrgb_8888,
      abgr_8888_le_to_abgr_f32,
      NULL,
      abgr_8888_le_to_rgba_4444,
   },
   {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      rgba_4444_to_argb_8888,
      rgba_4444_to_rgba_8888,
      rgba_4444_to_argb_4444,
      rgba_4444_to_rgb_888,
      rgba_4444_to_rgb_565,
      rgba_4444_to_rgb_555,
      rgba_4444_to_rgba_5551,
      rgba_4444_to_argb_1555,
      rgba_4444_to_abgr_8888,
      rgba_4444_to_xbgr_8888,
      rgba_4444_to_bgr_888,
      rgba_4444_to_bgr_565,
      rgba_4444_to_bgr_555,
      rgba_4444_to_rgbx_8888,
      rgba_4444_to_xrgb_8888,
      rgba_4444_to_abgr_f32,
      rgba_4444_to_abgr_8888_le,
      NULL,
   },
};

// Warning: This file was created by make_converters.py - do not edit.
