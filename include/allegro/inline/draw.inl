/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Draw inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DRAW_INL
#define ALLEGRO_DRAW_INL

#ifdef __cplusplus
   extern "C" {
#endif

#include "allegro/debug.h"
#include "gfx.inl"


AL_INLINE(int, getpixel, (BITMAP *bmp, int x, int y),
{
   ASSERT(bmp);

   return bmp->vtable->getpixel(bmp, x, y);
})


AL_INLINE(void, putpixel, (BITMAP *bmp, int x, int y, int color),
{
   ASSERT(bmp);

   bmp->vtable->putpixel(bmp, x, y, color);
})


AL_INLINE(void, vline, (BITMAP *bmp, int x, int y1, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->vline(bmp, x, y1, y2, color);
})


AL_INLINE(void, hline, (BITMAP *bmp, int x1, int y, int x2, int color),
{
   ASSERT(bmp);

   bmp->vtable->hline(bmp, x1, y, x2, color);
})


AL_INLINE(void, line, (BITMAP *bmp, int x1, int y1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->line(bmp, x1, y1, x2, y2, color);
})


AL_INLINE(void, rectfill, (BITMAP *bmp, int x1, int y1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->rectfill(bmp, x1, y1, x2, y2, color);
})


AL_INLINE(void, draw_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->vtable->color_depth == 8) {
      bmp->vtable->draw_256_sprite(bmp, sprite, x, y);
   }
   else {
      ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);
      bmp->vtable->draw_sprite(bmp, sprite, x, y);
   }
})


AL_INLINE(void, draw_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_v_flip(bmp, sprite, x, y);
})


AL_INLINE(void, draw_sprite_h_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_h_flip(bmp, sprite, x, y);
})


AL_INLINE(void, draw_sprite_vh_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_vh_flip(bmp, sprite, x, y);
})


AL_INLINE(void, draw_trans_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->vtable->color_depth == 32) {
      ASSERT(bmp->vtable->draw_trans_rgba_sprite);
      bmp->vtable->draw_trans_rgba_sprite(bmp, sprite, x, y);
   }
   else {
      ASSERT((bmp->vtable->color_depth == sprite->vtable->color_depth) ||
             ((bmp->vtable->color_depth == 32) &&
              (sprite->vtable->color_depth == 8)));
      bmp->vtable->draw_trans_sprite(bmp, sprite, x, y);
   }
})


AL_INLINE(void, draw_lit_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_lit_sprite(bmp, sprite, x, y, color);
})


AL_INLINE(void, draw_character, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(sprite->vtable->color_depth == 8);

   bmp->vtable->draw_character(bmp, sprite, x, y, color);
})


AL_INLINE(void, rotate_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
			     			      (y<<16) + (sprite->h * 0x10000) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, 0x10000, FALSE);
})


AL_INLINE(void, rotate_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
			     			      (y<<16) + (sprite->h * 0x10000) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, 0x10000, TRUE);
})


AL_INLINE(void, rotate_scaled_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
			     			      (y<<16) + (sprite->h * scale) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, scale, FALSE);
})


AL_INLINE(void, rotate_scaled_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
			     			      (y<<16) + (sprite->h * scale) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, scale, TRUE);
})


AL_INLINE(void, pivot_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, FALSE);
})


AL_INLINE(void, pivot_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, TRUE);
})


AL_INLINE(void, pivot_scaled_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, FALSE);
})


AL_INLINE(void, pivot_scaled_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, TRUE);
})


AL_INLINE(void, _putpixel, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write8(addr+x, color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read8(addr+x);
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel15, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write15(addr+x*sizeof(short), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel15, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read15(addr+x*sizeof(short));
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel16, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write16(addr+x*sizeof(short), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel16, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read16(addr+x*sizeof(short));
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel24, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write24(addr+x*3, color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel24, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read24(addr+x*3);
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel32, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write32(addr+x*sizeof(long), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel32, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read32(addr+x*sizeof(long));
   bmp_unwrite_line(bmp);

   return c;
})

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_DRAW_INL */


