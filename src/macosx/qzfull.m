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
 *      MacOS X quartz fullscreen gfx driver
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif


static BITMAP *osx_qz_full_init(int, int, int, int, int);
static void osx_qz_full_exit(BITMAP *);
static void osx_qz_full_vsync(void);
static void osx_qz_full_set_palette(AL_CONST struct RGB *, int, int, int);
static int osx_qz_show_video_bitmap(BITMAP *);


static int lock_nesting = 0;
static char driver_desc[256];
static CFDictionaryRef old_mode = NULL;
static CGDirectPaletteRef palette = NULL;


GFX_DRIVER gfx_quartz_full =
{
   GFX_QUARTZ_FULLSCREEN,
   empty_string, 
   empty_string,
   "Quartz fullscreen", 
   osx_qz_full_init,
   osx_qz_full_exit,
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   osx_qz_full_vsync,
   osx_qz_full_set_palette,
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   osx_qz_create_video_bitmap,   /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   osx_qz_destroy_video_bitmap,  /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   osx_qz_show_video_bitmap,     /* AL_METHOD(int, show_video_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   NULL,                         /* AL_METHOD(int, fetch_mode_list, (void)); */
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE
};



static BITMAP *private_osx_qz_full_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   CFDictionaryRef mode = NULL;
   boolean_t match = FALSE;
   int bpp, refresh_rate;
   char tmp1[128], tmp2[128];
   
   if (1
#ifdef ALLEGRO_COLOR8
       && (color_depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (color_depth != 15)
#endif
#ifdef ALLEGRO_COLOR32
       && (color_depth != 32)
#endif
       ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return NULL;
   }

   if ((w == 0) && (h == 0)) {
      w = 320;
      h = 200;
   }
   
   if (v_w < w) v_w = w;
   if (v_h < h) v_h = h;
   
   if ((v_w != w) || (v_h != h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }
   
   bpp = color_depth == 15 ? 16 : color_depth;
   if (_refresh_rate_request > 0)
      mode = CGDisplayBestModeForParametersAndRefreshRate(kCGDirectMainDisplay, bpp, w, h,
	 (double)_refresh_rate_request, &match);
   if (!match)
      mode = CGDisplayBestModeForParameters(kCGDirectMainDisplay, bpp, w, h, &match);
   if (!match) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }
   
   old_mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   
   CGDisplayHideCursor(kCGDirectMainDisplay);
   CGDisplayCapture(kCGDirectMainDisplay);
   CGDisplaySwitchToMode(kCGDirectMainDisplay, mode);
   HideMenuBar();
   
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberSInt32Type, &refresh_rate);
   _set_current_refresh_rate(refresh_rate);
   
   if (CGDisplayCanSetPalette(kCGDirectMainDisplay))
      palette = CGPaletteCreateDefaultColorPalette();
   
   bmp = _make_bitmap(w, h, (unsigned long)CGDisplayBaseAddress(kCGDirectMainDisplay),
                      &gfx_quartz_full, color_depth, CGDisplayBytesPerRow(kCGDirectMainDisplay));
   if (!bmp) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }
   
   bmp->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
   BMP_EXTRA(bmp)->port = CreateNewPortForCGDisplayID((UInt32)kCGDirectMainDisplay);
   if (!BMP_EXTRA(bmp)->port) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot create DirectDisplay port"));
      return NULL;
   }
   
   setup_direct_shifts();
   
   gfx_quartz_full.w = w;
   gfx_quartz_full.h = h;
   gfx_quartz_full.vid_mem = w * h * BYTES_PER_PIXEL(color_depth);
   
   _screen_vtable.created_sub_bitmap = osx_qz_created_sub_bitmap;
/*
   _screen_vtable.acquire = osx_qz_acquire;
   _screen_vtable.release = osx_qz_release;
   _screen_vtable.unwrite_bank = osx_qz_unwrite_line;
   bmp->read_bank = osx_qz_write_line;
   bmp->write_bank = osx_qz_write_line;
*/
   
   uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("Core Graphics DirectDisplay access, %d bpp", tmp1),
             color_depth);
   gfx_quartz_full.desc = driver_desc;
   
   osx_gfx_mode = OSX_GFX_FULL;
   
   return bmp;
}

static BITMAP *osx_qz_full_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   _unix_bg_man->disable_interrupts();
   pthread_mutex_lock(&osx_event_mutex);
   bmp = private_osx_qz_full_init(w, h, v_w, v_h, color_depth);
   pthread_mutex_unlock(&osx_event_mutex);
   _unix_bg_man->enable_interrupts();
   if (!bmp)
      osx_qz_full_exit(bmp);
   return bmp;
}


static void osx_qz_full_exit(BITMAP *bmp)
{
   _unix_bg_man->disable_interrupts();
   pthread_mutex_lock(&osx_event_mutex);
   
   if ((bmp) && (bmp->extra)) {
      if (BMP_EXTRA(bmp)->port)
         DisposeGWorld(BMP_EXTRA(bmp)->port);
      free(bmp->extra);
   }
   
   if (palette) {
      CGPaletteRelease(palette);
      palette = NULL;
   }
   
   if (old_mode) {
      ShowMenuBar();
      CGDisplaySwitchToMode(kCGDirectMainDisplay, old_mode);
      CGDisplayRelease(kCGDirectMainDisplay);
      CGDisplayShowCursor(kCGDirectMainDisplay);
      old_mode = NULL;
   }
   
   osx_gfx_mode = OSX_GFX_NONE;
   
   pthread_mutex_unlock(&osx_event_mutex);
   _unix_bg_man->enable_interrupts();
}



/* osx_qz_full_vsync:
 *  Quartz video vertical synchronization routine for fullscreen mode.
 */
static void osx_qz_full_vsync(void)
{
   CGDisplayWaitForBeamPositionOutsideLines(kCGDirectMainDisplay, 0, gfx_quartz_full.h / 2);
   CGDisplayWaitForBeamPositionOutsideLines(kCGDirectMainDisplay, gfx_quartz_full.h / 2, gfx_quartz_full.h - 1);
}



/* osx_qz_full_set_palette:
 *  Sets palette for quartz fullscreen.
 */
static void osx_qz_full_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;
   CGDeviceColor color;
   
   if (!CGDisplayCanSetPalette(kCGDirectMainDisplay))
      return;
   
   if (vsync)
      osx_qz_full_vsync();
   
   for (i = from; i <= to; i++) {
      color.red = ((float)p[i].r / 63.0);
      color.green = ((float)p[i].g / 63.0);
      color.blue = ((float)p[i].b / 63.0);
      CGPaletteSetColorAtIndex(palette, color, i);
   }
   CGDisplaySetPalette(kCGDirectMainDisplay, palette);
}



/* osx_qz_show_video_bitmap:
 *  Displays a video bitmap on the screen by copying it using CopyBits
 *  (hw accelerated copy).
 */
static int osx_qz_show_video_bitmap(BITMAP *bmp)
{
   Rect rect;
   
   if ((bmp->w != gfx_quartz_full.w) || (bmp->h != gfx_quartz_full.h))
      return -1;
   
   SetRect(&rect, 0, 0, bmp->w - 1, bmp->h - 1);
   while (!QDDone(BMP_EXTRA(screen)->port));
   while (!QDDone(BMP_EXTRA(bmp)->port));
   LockPortBits(BMP_EXTRA(screen)->port);
   LockPortBits(BMP_EXTRA(bmp)->port);
   osx_qz_full_vsync();
   CopyBits(GetPortBitMapForCopyBits(BMP_EXTRA(bmp)->port),
            GetPortBitMapForCopyBits(BMP_EXTRA(screen)->port),
	    &rect, &rect, srcCopy, NULL);
   UnlockPortBits(BMP_EXTRA(screen)->port);
   UnlockPortBits(BMP_EXTRA(bmp)->port);
   
   return 0;
}

