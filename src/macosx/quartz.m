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
 *      MacOS X common quartz (quickdraw) gfx driver functions.
 *
 *      By Angelo Mottola, based on similar QNX code by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif


/* setup_direct_shifts
 *  Setups direct color shifts.
 */
void setup_direct_shifts(void)
{
   _rgb_r_shift_15 = 10;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;

   _rgb_r_shift_16 = 11;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;

   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;

   _rgb_a_shift_32 = 24;
   _rgb_r_shift_32 = 16; 
   _rgb_g_shift_32 = 8; 
   _rgb_b_shift_32 = 0;
}



/* osx_qz_write_line:
 *  Line switcher for video bitmaps.
 */
unsigned long osx_qz_write_line(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
      if (bmp->extra) {
         while (!QDDone(BMP_EXTRA(bmp)->port));
         LockPortBits(BMP_EXTRA(bmp)->port);
      }
   }
   
   return (unsigned long)(bmp->line[line]);
}



/* osx_qz_unwrite_line:
 *  Line updater for video bitmaps.
 */
void osx_qz_unwrite_line(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
      if (bmp->extra)
         UnlockPortBits(BMP_EXTRA(bmp)->port);
   }
}



/* osx_qz_acquire:
 *  Bitmap locking for video bitmaps.
 */
void osx_qz_acquire(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
      if (bmp->extra) {
         while (!QDDone(BMP_EXTRA(bmp)->port));
         while (LockPortBits(BMP_EXTRA(bmp)->port));
      }
   }
}



/* osx_qz_release:
 *  Bitmap unlocking for video bitmaps.
 */
void osx_qz_release(BITMAP *bmp)
{
   bmp->id &= ~BMP_ID_LOCKED;
   if (bmp->extra)
      UnlockPortBits(BMP_EXTRA(bmp)->port);
}



/* _make_video_bitmap:
 *  Helper function for wrapping up video memory in a video bitmap.
 */
static BITMAP *_make_video_bitmap(int w, int h, unsigned long addr, struct GFX_VTABLE *vtable, int bpl)
{
   int i, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *) malloc(size);
   if (!b)
      return NULL;

   b->w = b->cr = w;
   b->h = b->cb = h;
   b->clip = TRUE;
   b->cl = b->ct = 0;
   b->vtable = vtable;
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->id = BMP_ID_VIDEO;
   b->extra = NULL;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _video_ds();

   b->line[0] = (char *)addr;

   for (i = 1; i < h; i++)
      b->line[i] = b->line[i - 1] + bpl;

   return b;
}



/* osx_qz_created_sub_bitmap:
 */
void osx_qz_created_sub_bitmap(BITMAP *bmp, BITMAP *parent)
{
   bmp->extra = parent->extra;
}



/* osx_qz_create_video_bitmap:
 */
BITMAP *osx_qz_create_video_bitmap(int width, int height)
{
   struct BITMAP *bmp;
   GWorldPtr gworld;
   Rect rect;
   char *addr;
   int pitch;
   
   SetRect(&rect, 0, 0, width, height);
   
   /* Try video RAM first, then local with AGP */
   if (NewGWorld(&gworld, screen->vtable->color_depth, &rect, NULL, NULL, useDistantHdwrMem) ||
       NewGWorld(&gworld, screen->vtable->color_depth, &rect, NULL, NULL, useLocalHdwrMem))
      return NULL;
   
   LockPortBits(gworld);
   addr = GetPixBaseAddr(GetPortPixMap(gworld));
   pitch = GetPixRowBytes(GetPortPixMap(gworld));
   UnlockPortBits(gworld);
   if (!addr) {
      return NULL;
   }

   /* create Allegro bitmap */
   bmp = _make_video_bitmap(width, height, (unsigned long)addr, &_screen_vtable, pitch);
   if (!bmp)
      return NULL;

   bmp->write_bank = osx_qz_write_line;
   bmp->read_bank = osx_qz_write_line;

   /* setup surface info structure to store additional information */
   bmp->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
   BMP_EXTRA(bmp)->port = gworld;

   return bmp;
}



/* osx_qz_destroy_video_bitmap:
 */
void osx_qz_destroy_video_bitmap(BITMAP *bmp)
{
   if (bmp) {
      if (bmp->extra) {
         if (BMP_EXTRA(bmp)->port)
            DisposeGWorld(BMP_EXTRA(bmp)->port);
         free(bmp->extra);
      }
      free(bmp);
   }
}
