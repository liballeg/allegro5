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


PdOffscreenContext_t        *ph_screen_context = NULL;
PdOffscreenContext_t        *ph_window_context = NULL;
int                          ph_gfx_initialized = FALSE;

static PdDirectContext_t    *direct_context = NULL;
static PgColor_t             ph_palette[256];
static int                   ph_last_line = -1;
static char                  driver_desc[128];
static PgDisplaySettings_t   original_settings;


static int find_video_mode(int w, int h, int depth);
static struct BITMAP *qnx_private_phd_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, int accel);

unsigned long ph_write_line(BITMAP *bmp, int line);
#ifndef ALLEGRO_NO_ASM
unsigned long ph_write_line_asm(BITMAP *bmp, int line);
#endif



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
   BITMAP *bmp = NULL;
   PhRegion_t region;
   PgHWCaps_t caps;
   PgDisplaySettings_t settings;
   PgVideoModeInfo_t mode_info;
   int mode_num;
   char *addr;

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
      PgSetVideoMode(&original_settings);
      return NULL;
   }
   if (!PdDirectStart(direct_context)) {
      ustrcpy(allegro_error, get_config_text("Cannot start direct context"));
      return NULL;
   }
   if (!(ph_screen_context = PdCreateOffscreenContext(0, w, h, 
       Pg_OSC_MAIN_DISPLAY | Pg_OSC_MEM_PAGE_ALIGN | Pg_OSC_CRTC_SAFE))) {
      ustrcpy(allegro_error, get_config_text("Cannot access the framebuffer"));
      return NULL;
   }
   if (!(addr = PdGetOffscreenContextPtr(ph_screen_context))) {
      ustrcpy(allegro_error, get_config_text("Cannot access the framebuffer"));
      return NULL;
   }
   
   drv->linear = TRUE;
   if (PgGetGraphicsHWCaps(&caps)) {
      drv->vid_mem = w * h * BYTES_PER_PIXEL(color_depth);
   }
   else {
      drv->vid_mem = caps.total_video_ram;
   }

   bmp = _make_bitmap(w, h, (unsigned long)addr, drv,
                      color_depth, ph_screen_context->pitch);
   if(!bmp) {
      ustrcpy(allegro_error, get_config_text("Not enough memory"));
      return NULL;
   }

   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;

   setup_direct_shifts();
   sprintf(driver_desc, "Photon direct mode (%s)", caps.chip_name);
   drv->desc = driver_desc;

   _mouse_on = TRUE;
   
   PgFlush();
   PgWaitHWIdle();

   return bmp;
}



/* qnx_private_phd_exit:
 *  This is where the video driver is actually closed.
 */
static void qnx_private_phd_exit()
{
   ph_gfx_initialized = FALSE;
   if (ph_screen_context) {
      PhDCRelease(ph_screen_context);
   }
   ph_screen_context = NULL;
   if (direct_context) {
      PdDirectStop(direct_context);
      PdReleaseDirectContext(direct_context);
      direct_context = NULL;
      PgSetVideoMode(&original_settings);
   }
}



/* qnx_phd_init:
 *  Initializes Photon gfx driver using the direct (fullscreen) mode.
 */
struct BITMAP *qnx_phd_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_events_mutex);
   bmp = qnx_private_phd_init(&gfx_photon_direct, w, h, v_w, v_h, color_depth, TRUE);
   ph_gfx_initialized = TRUE;
   if (!bmp) {
      qnx_private_phd_exit(bmp);
   }
   pthread_mutex_unlock(&qnx_events_mutex);

   return bmp;
}



/* qnx_phd_exit:
 *  Shuts down photon direct driver.
 */
void qnx_phd_exit(struct BITMAP *bmp)
{
   pthread_mutex_lock(&qnx_events_mutex);
   qnx_private_phd_exit();
   pthread_mutex_unlock(&qnx_events_mutex);
}



/* qnx_ph_vsync:
 *  Waits for vertical retrace.
 */
void qnx_ph_vsync(void)
{
   PgWaitHWIdle();
   PgWaitVSync();
}



/* qnx_ph_set_palette:
 *  Sets hardware palette.
 */
void qnx_ph_set_palette(AL_CONST struct RGB *p, int from, int to, int retracesync)
{
   int i;
   
   for (i=from; i<=to; i++) {
      ph_palette[i] = (p[i].r << 18) | (p[i].g << 10) | (p[i].b << 2);
   }
   if (retracesync) {
      PgWaitVSync();
   }
   PgSetPalette(ph_palette, 0, from, to - from + 1, (ph_window_context ? Pg_PALSET_SOFT : Pg_PALSET_HARDLOCKED), 0);
   PgFlush();
}



static inline void ph_update_screen(int line)
{
   PhRect_t rect;

   rect.ul.x = 0;
   rect.ul.y = line;
   rect.lr.x = ph_window_context->dim.w;
   rect.lr.y = line + 1;
   PgContextBlit(ph_window_context, &rect, NULL, &rect);
}



unsigned long ph_write_line(struct BITMAP *bmp, int line)
{
   int new_line = line + bmp->y_ofs;
   
   if ((new_line != ph_last_line) && (ph_last_line >= 0))
      ph_update_screen(ph_last_line);
   ph_last_line = new_line;
   return (unsigned long)(bmp->line[line]);
}



/* ph_unwrite_line:
 *  Update last selected line.
 */
static void ph_unwrite_line(struct BITMAP *bmp)
{
   if (ph_last_line >= 0)
      ph_update_screen(ph_last_line);
   ph_last_line = -1;
}



static void ph_release(struct BITMAP *bmp)
{
   PgFlush();
}



/* qnx_private_ph_init:
 *  Real screen initialization runs here.
 */
static struct BITMAP *qnx_private_ph_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp = NULL;
   PtArg_t arg;
   PhDim_t dim;
   int type = 0;
   char *addr;

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
      w = 320;
      h = 200;
   }
   if (v_w < w) v_w = w;
   if (v_h < h) v_h = h;
   
   if ((v_w != w) || (v_h != h)) {
      ustrcpy(allegro_error, get_config_text("Resolution not supported"));
      return NULL;
   }

   dim.w = w;
   dim.h = h;
   PtSetArg(&arg, Pt_ARG_DIM, &dim, 0);
   PtSetResources(ph_window, 1, &arg);

   ph_window_context = PdCreateOffscreenContext(0, w, h, Pg_OSC_MEM_PAGE_ALIGN);
   if (!ph_window_context) {
      ustrcpy(allegro_error, get_config_text("Cannot create offscreen context"));
      return NULL;
   }
   addr = PdGetOffscreenContextPtr(ph_window_context);
/*
   switch (color_depth) {
      case 8: type = Pg_IMAGE_PALETTE_BYTE; break;
      case 15: type = Pg_IMAGE_DIRECT_555; break;
      case 16: type = Pg_IMAGE_DIRECT_565; break;
      case 24: type = Pg_IMAGE_DIRECT_888; break;
      case 32: type = Pg_IMAGE_DIRECT_8888; break;
   }
   if (ph_window_context->format != type) {
      ustrcpy(allegro_error, get_config_text("bad color depth matching"));
      return NULL;
   }
*/

   drv->linear = TRUE;
   drv->vid_mem = ph_window_context->shared_size;

   bmp = _make_bitmap(w, h, (unsigned long)addr, drv, 
                      color_depth, ph_window_context->pitch);
   if(!bmp) {
      ustrcpy(allegro_error, get_config_text("Not enough memory"));
      return NULL;
   }

   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;

#ifdef ALLEGRO_NO_ASM
   bmp->write_bank = ph_write_line;
   bmp->read_bank = ph_write_line;
#else
   bmp->write_bank = ph_write_line_asm;
   bmp->read_bank = ph_write_line_asm;
#endif

   _screen_vtable.release = ph_release;
   _screen_vtable.unwrite_bank = ph_unwrite_line;
   ph_last_line = -1;

   setup_direct_shifts();
   sprintf(driver_desc, "Photon windowed");
   drv->desc = driver_desc;

   PgFlush();
   PgWaitHWIdle();

   return bmp;
}



/* qnx_private_ph_exit:
 *  This is where the video driver is actually closed.
 */
static void qnx_private_ph_exit()
{
   ph_gfx_initialized = FALSE;
   if (ph_window_context)
      PhDCRelease(ph_window_context);
   ph_window_context = NULL;
}



/* qnx_ph_init:
 *  Initializes normal windowed Photon gfx driver.
 */
struct BITMAP *qnx_ph_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_events_mutex);
   bmp = qnx_private_ph_init(&gfx_photon, w, h, v_w, v_h, color_depth);
   ph_gfx_initialized = TRUE;
   if (!bmp) {
      qnx_private_ph_exit();
   }
   pthread_mutex_unlock(&qnx_events_mutex);

   return bmp;
}



/* qnx_ph_exit:
 *  Shuts down photon windowed gfx driver.
 */
void qnx_ph_exit(struct BITMAP *b)
{
   pthread_mutex_lock(&qnx_events_mutex);
   qnx_private_ph_exit();
   pthread_mutex_unlock(&qnx_events_mutex);
}
