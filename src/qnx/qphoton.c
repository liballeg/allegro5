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
 *      QNX Photon gfx drivers.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "qnxalleg.h"
#include "allegro/aintern.h"
#include "allegro/aintqnx.h"

#ifndef ALLEGRO_QNX
#error Something is wrong with the makefile
#endif


static PgDisplaySettings_t   original_settings;
static PdDirectContext_t    *direct_context = NULL;
static PgColor_t             ph_palette[256];


static int find_video_mode(int w, int h, int depth);
static struct BITMAP *qnx_private_phd_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, int accel);



/* setup_direct_shifts
 *  Setups direct color hue shifts.
 */
static inline void setup_direct_shifts(void)
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



/* find_video_mode:
 *  Scans the list of available video modes and returns the index of a
 *  suitable mode, -1 on failure.
 */
static int find_video_mode(int w, int h, int depth)
{
   PgVideoModes_t mode_list;
   PgVideoModeInfo_t mode_info;
   int i;
   
   if (PgGetVideoModeList(&mode_list))
      return -1;
         
   for (i=0; i<mode_list.num_modes; i++) {
      if ((!PgGetVideoModeInfo(mode_list.modes[i], &mode_info)) &&
          (mode_info.mode_capabilities1 & PgVM_MODE_CAP1_LINEAR) &&
          (mode_info.width == w) &&
          (mode_info.height == h) &&
          (mode_info.bits_per_pixel == depth)) {
         return mode_list.modes[i];
      }
   }
   return -1;
}



/* qnx_private_phd_init:
 *  Real screen initialization runs here.
 */
static struct BITMAP *qnx_private_phd_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, int accel)
{
   BITMAP *bmp;
   PgHWCaps_t caps;
   PdOffscreenContext_t *screen_context;
   PgDisplaySettings_t settings;
   PgVideoModeInfo_t mode_info;
   int mode_num;

   if (1
#ifdef ALLEGRO_COLOR8
       && (color_depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (color_depth != 15)
       && (color_depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (color_depth != 24)
#endif
#ifdef ALLEGRO_COLOR32
       && (color_depth != 32)
#endif
       ) {
      ustrcpy(allegro_error, get_config_text("Unsupported color depth"));
      return NULL;
   }

   if ((w == 0) && (h == 0)) {
      w = 640;
      h = 480;
   }
   if (v_w < w) v_w = w;
   if (v_h < h) v_h = h;
   
   if ((v_w != w) || (v_h != h)) {
      ustrcpy(allegro_error, get_config_text("Resolution not supported"));
      return NULL;
   }
   
   PgGetVideoMode(&original_settings);

   mode_num = find_video_mode(w, h, color_depth);
   if (mode_num == -1) {
      ustrcpy(allegro_error, get_config_text("Resolution not supported"));
      return NULL;
   }
   PgGetVideoModeInfo(mode_num, &mode_info);
   
   settings.mode = mode_num;
   settings.refresh = _refresh_rate_request;
   settings.flags = 0;
   
   if (PgSetVideoMode(&settings)) {
      ustrcpy(allegro_error, get_config_text("Resolution not supported"));
      return NULL;
   }
   if (!(direct_context = PdCreateDirectContext())) {
      ustrcpy(allegro_error, get_config_text("Cannot create direct context"));
      return NULL;
   }
   if (!PdDirectStart(direct_context)) {
      ustrcpy(allegro_error, get_config_text("Cannot start direct context"));
      return NULL;
   }
   if (!(screen_context = PdCreateOffscreenContext(0, w, h, 
       Pg_OSC_MAIN_DISPLAY | Pg_OSC_MEM_PAGE_ALIGN))) {
      ustrcpy(allegro_error, get_config_text("Cannot access the framebuffer"));
      return NULL;
   }
   if (!PdGetOffscreenContextPtr(screen_context)) {
      ustrcpy(allegro_error, get_config_text("Cannot access the framebuffer"));
      return NULL;
   }
   PgWaitHWIdle();
      
   drv->w = w;
   drv->h = h;
   drv->linear = TRUE;
   if (PgGetGraphicsHWCaps(&caps)) {
      drv->vid_mem = w * h * BYTES_PER_PIXEL(color_depth);
   }
   else {
      drv->vid_mem = caps.total_video_ram;
   }

   bmp = _make_bitmap(w, h, (unsigned long)screen_context->shared_ptr, drv,
                      color_depth, screen_context->pitch);
                      
   if(!bmp) {
      ustrcpy(allegro_error, get_config_text("Not enough memory"));
      return NULL;
   }

   setup_direct_shifts();

   return bmp;
}



/* qnx_phd_init:
 *  Initializes Photon gfx driver using the direct (fullscreen) mode.
 */
struct BITMAP *qnx_phd_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   DISABLE();
   bmp = qnx_private_phd_init(&gfx_photon_direct, w, h, v_w, v_h, color_depth, TRUE);
   ENABLE();
   if (!bmp)
      qnx_phd_exit(bmp);
   return bmp;
}



/* qnx_phd_exit:
 *  Shuts down photon direct driver.
 */
void qnx_phd_exit(struct BITMAP *b)
{
   DISABLE();
   if (direct_context) {
      PdDirectStop(direct_context);
      PdReleaseDirectContext(direct_context);
      direct_context = NULL;
   }
   PgSetVideoMode(&original_settings);
   PgFlush();
   PgWaitHWIdle();
   ENABLE();
}



/* qnx_phd_vsync:
 *  Waits for vertical retrace.
 */
void qnx_phd_vsync(void)
{
   DISABLE();
   PgWaitHWIdle();
   PgWaitVSync();
   ENABLE();
}



/* qnx_phd_set_palette:
 *  Sets hardware palette.
 */
void qnx_phd_set_palette(AL_CONST struct RGB *p, int from, int to, int retracesync)
{
   int i;
   
   for (i=from; i<=to; i++) {
      ph_palette[i] = (_rgb_scale_6[p[i].r] << 16) |
                      (_rgb_scale_6[p[i].g] << 8) |
                      _rgb_scale_6[p[i].b];
   }
   DISABLE();
   if (retracesync) {
      PgWaitVSync();
   }
   PgSetPalette(ph_palette, 0, from, to - from + 1, Pg_PALSET_HARD, 0);
   PgFlush();
   ENABLE();
}

