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


PdDirectContext_t              *ph_direct_context = NULL;
PdOffscreenContext_t           *ph_screen_context = NULL;
PdOffscreenContext_t           *ph_window_context = NULL;
int                             ph_gfx_mode = PH_GFX_NONE;
char                           *ph_dirty_lines = NULL;
void                          (*ph_update_window)(PhRect_t* rect) = NULL;
int                             ph_window_w = 0, ph_window_h = 0;
PgColor_t                       ph_palette[256];


static char                     driver_desc[128];
static PgDisplaySettings_t      original_settings;
static COLORCONV_BLITTER_FUNC  *blitter = NULL;
static char                    *pseudo_screen_addr = NULL;
static int                      pseudo_screen_pitch;
static int                      pseudo_screen_depth;
static char                    *window_addr = NULL;
static int                      window_pitch;
static int                      desktop_depth;


static int find_video_mode(int w, int h, int depth);
static struct BITMAP *qnx_private_phd_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, int accel);

#ifdef ALLEGRO_NO_ASM
static inline void update_dirty_lines();
static unsigned long phd_write_line(BITMAP *bmp, int line);
static void phd_unwrite_line(BITMAP *bmp);
static void phd_acquire(BITMAP *bmp);
static void phd_release(BITMAP *bmp);
static unsigned long ph_write_line(BITMAP *bmp, int line);
static void ph_unwrite_line(BITMAP *bmp);
static void ph_acquire(BITMAP *bmp);
static void ph_release(BITMAP *bmp);
#else
unsigned long phd_write_line_asm(BITMAP *bmp, int line);
void phd_unwrite_line_asm(BITMAP *bmp);
void phd_acquire_asm(BITMAP *bmp);
void phd_release_asm(BITMAP *bmp);
unsigned long ph_write_line_asm(BITMAP *bmp, int line);
void ph_unwrite_line_asm(BITMAP *bmp);
void ph_acquire_asm(BITMAP *bmp);
void ph_release_asm(BITMAP *bmp);
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
   PhDim_t dim;
   PtArg_t arg;
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
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return NULL;
   }

   if ((w == 0) && (h == 0)) {
      w = 640;
      h = 480;
   }
   if (v_w < w) v_w = w;
   if (v_h < h) v_h = h;
   
   if ((v_w != w) || (v_h != h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   dim.w = ph_window_w = w;
   dim.h = ph_window_h = h;
   PtSetArg(&arg, Pt_ARG_DIM, &dim, 0);
   PtSetResources(ph_window, 1, &arg);
   
   PgGetVideoMode(&original_settings);
   
   mode_num = find_video_mode(w, h, color_depth);
   if (mode_num == -1) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }
   PgGetVideoModeInfo(mode_num, &mode_info);
   
   settings.mode = mode_num;
   settings.refresh = _refresh_rate_request;
   settings.flags = 0;
   
   if (PgSetVideoMode(&settings)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }
   if (!(ph_direct_context = PdCreateDirectContext())) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot create direct context"));
      PgSetVideoMode(&original_settings);
      return NULL;
   }
   if (!PdDirectStart(ph_direct_context)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot start direct context"));
      return NULL;
   }
   if (!(ph_screen_context = PdCreateOffscreenContext(0, w, h, 
       Pg_OSC_MAIN_DISPLAY | Pg_OSC_MEM_PAGE_ALIGN | Pg_OSC_CRTC_SAFE))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot access the framebuffer"));
      return NULL;
   }
   if (!(addr = PdGetOffscreenContextPtr(ph_screen_context))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot access the framebuffer"));
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
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }

#ifdef ALLEGRO_NO_ASM
   bmp->write_bank = phd_write_line;
   bmp->read_bank = phd_write_line;
   _screen_vtable.acquire = phd_acquire;
   _screen_vtable.release = phd_release;
#else
   bmp->write_bank = phd_write_line_asm;
   bmp->read_bank = phd_write_line_asm;
   _screen_vtable.acquire = phd_acquire_asm;
   _screen_vtable.release = phd_release_asm;
#endif

   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;
   desktop_depth = color_depth;

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
   PhDim_t dim;
   
   ph_gfx_mode = PH_GFX_NONE;
   if (ph_screen_context) {
      PhDCRelease(ph_screen_context);
   }
   ph_screen_context = NULL;
   if (ph_direct_context) {
      PdReleaseDirectContext(ph_direct_context);
      ph_direct_context = NULL;
      PgFlush();
      PgSetVideoMode(&original_settings);
   }
   dim.w = 1;
   dim.h = 1;
   PtSetResource(ph_window, Pt_ARG_DIM, &dim, 0);
}



/* qnx_phd_init:
 *  Initializes Photon gfx driver using the direct (fullscreen) mode.
 */
struct BITMAP *qnx_phd_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_events_mutex);
   bmp = qnx_private_phd_init(&gfx_photon_direct, w, h, v_w, v_h, color_depth, TRUE);
   ph_gfx_mode = PH_GFX_DIRECT;
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



#ifdef ALLEGRO_NO_ASM


/* phd_write_line:
 *  Line switcher for Photon direct mode.
 */
static unsigned long phd_write_line(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
      PgWaitHWIdle();
   }
   return (unsigned long)(bmp->line[line]);
}



/* phd_acquire:
 *  Bitmap locking for direct mode.
 */
static void phd_acquire(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
      PgWaitHWIdle();
   }
}



/* phd_release:
 *  Bitmap unlocking for direct mode.
 */
static void phd_release(BITMAP *bmp)
{
   bmp->id &= ~BMP_ID_LOCKED;
}


#endif



/* qnx_ph_vsync:
 *  Waits for vertical retrace.
 */
void qnx_ph_vsync(void)
{
   PgWaitVSync();
}



/* qnx_ph_set_palette:
 *  Sets hardware palette.
 */
void qnx_ph_set_palette(AL_CONST struct RGB *p, int from, int to, int retracesync)
{
   int i;
   int mode = Pg_PALSET_HARDLOCKED;
   
   for (i=from; i<=to; i++) {
      ph_palette[i] = (p[i].r << 18) | (p[i].g << 10) | (p[i].b << 2);
   }
   if (retracesync) {
      PgWaitVSync();
   }
   if (ph_gfx_mode == PH_GFX_WINDOW) {
      if (desktop_depth != 8) {
         _set_colorconv_palette(p, from, to);
         ph_update_window(NULL);
      }
      mode |= Pg_PALSET_FORCE_EXPOSE;
   }
   if (desktop_depth == 8) {
      PgSetPalette(ph_palette, 0, from, to - from + 1, mode, 0);
   }
   PgFlush();
}



/* update_window_hw:
 *  Window updater for matching color depths.
 */
static void update_window_hw(PhRect_t *rect)
{
   PhRect_t temp;
   
   if (!rect) {
      temp.ul.x = 0;
      temp.ul.y = 0;
      temp.lr.x = ph_window_w;
      temp.lr.y = ph_window_h;
      rect = &temp;
   }
   pthread_mutex_lock(qnx_gfx_mutex);
   PgContextBlit(ph_window_context, rect, NULL, rect);
   pthread_mutex_unlock(qnx_gfx_mutex);
}



/* update_window:
 *  Window updater for non matching color depths, performing color conversion.
 */
static void update_window(PhRect_t *rect)
{
   struct GRAPHICS_RECT src_gfx_rect, dest_gfx_rect;
   PhRect_t temp;
   
   if (!rect) {
      temp.ul.x = 0;
      temp.ul.y = 0;
      temp.lr.x = ph_window_w;
      temp.lr.y = ph_window_h;
      rect = &temp;
   }

   /* fill in source graphics rectangle description */
   src_gfx_rect.width  = rect->lr.x - rect->ul.x;
   src_gfx_rect.height = rect->lr.y - rect->ul.y;
   src_gfx_rect.pitch  = pseudo_screen_pitch;
   src_gfx_rect.data   = pseudo_screen_addr +
                         (rect->ul.y * pseudo_screen_pitch) +
                         (rect->ul.x * BYTES_PER_PIXEL(pseudo_screen_depth));

   /* fill in destination graphics rectangle description */
   dest_gfx_rect.pitch = window_pitch;
   dest_gfx_rect.data  = window_addr +
                         (rect->ul.y * window_pitch) +
                         (rect->ul.x * BYTES_PER_PIXEL(desktop_depth));
   
   /* function doing the hard work */
   blitter(&src_gfx_rect, &dest_gfx_rect);

   pthread_mutex_lock(qnx_gfx_mutex);
   PgContextBlit(ph_window_context, rect, NULL, rect);
   pthread_mutex_unlock(qnx_gfx_mutex);
}



/* qnx_private_ph_init:
 *  Real screen initialization runs here.
 */
static struct BITMAP *qnx_private_ph_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp = NULL;
   PgHWCaps_t caps;
   PgDisplaySettings_t settings;
   PgVideoModeInfo_t mode_info;
   PtArg_t arg;
   PhDim_t dim;
   char *addr;
   int pitch;

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
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return NULL;
   }

   if ((w == 0) && (h == 0)) {
      w = 320;
      h = 200;
   }
   if (v_w < w) v_w = w;
   if (v_h < h) v_h = h;
   
   if ((v_w != w) || (v_h != h) || (w % 4)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   PgGetVideoMode(&settings);
   PgGetVideoModeInfo(settings.mode, &mode_info);

   dim.w = ph_window_w = w;
   dim.h = ph_window_h = h;
   PtSetArg(&arg, Pt_ARG_DIM, &dim, 0);
   PtSetResources(ph_window, 1, &arg);

   ph_window_context = PdCreateOffscreenContext(0, w, h, Pg_OSC_MEM_PAGE_ALIGN);
   if (!ph_window_context) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot create offscreen context"));
      return NULL;
   }
   addr = PdGetOffscreenContextPtr(ph_window_context);

   pseudo_screen_depth = color_depth;
   desktop_depth = mode_info.bits_per_pixel;

   if (color_depth == desktop_depth) {
      /* the color depths match */
      ph_update_window = update_window_hw;
      pseudo_screen_addr = NULL;
      pitch = ph_window_context->pitch;
   }
   else {
      /* the color depths don't match, need color conversion */
      blitter = _get_colorconv_blitter(color_depth, desktop_depth);
      if (!blitter) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         return NULL;
      }
      ph_update_window = update_window;
      pseudo_screen_addr = (char *)malloc(w * h * BYTES_PER_PIXEL(color_depth));
      if (!pseudo_screen_addr) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
         return NULL;
      }
      pseudo_screen_pitch = pitch = w * BYTES_PER_PIXEL(color_depth);
      window_pitch = ph_window_context->pitch;
      window_addr = addr;
      addr = pseudo_screen_addr;
   }

   drv->linear = TRUE;
   if (PgGetGraphicsHWCaps(&caps)) {
      drv->vid_mem = w * h * BYTES_PER_PIXEL(color_depth);
   }
   else {
      drv->vid_mem = caps.total_video_ram;
   }

   bmp = _make_bitmap(w, h, (unsigned long)addr, drv, color_depth, pitch);
   ph_dirty_lines = (char *)calloc(h + 1, sizeof(char));
   
   if ((!bmp) || (!ph_dirty_lines)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }

   ph_dirty_lines[h] = 0;
   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;

#ifdef ALLEGRO_NO_ASM
   bmp->write_bank = ph_write_line;
   _screen_vtable.unwrite_bank = ph_unwrite_line;
   _screen_vtable.acquire = ph_acquire;
   _screen_vtable.release = ph_release;
#else
   bmp->write_bank = ph_write_line_asm;
   _screen_vtable.unwrite_bank = ph_unwrite_line_asm;
   _screen_vtable.acquire = ph_acquire_asm;
   _screen_vtable.release = ph_release_asm;
#endif

   setup_direct_shifts();
   sprintf(driver_desc, "Photon windowed, %d bpp %s", color_depth,
      (color_depth == desktop_depth) ? "in matching" : "in fast emulation");
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
   PhDim_t dim;
   
   PgSetPalette(ph_palette, 0, 0, -1, 0, 0);
   PgFlush();
   ph_gfx_mode = PH_GFX_NONE;
   if (ph_window_context)
      PhDCRelease(ph_window_context);
   ph_window_context = NULL;
   if (pseudo_screen_addr)
      free(pseudo_screen_addr);
   pseudo_screen_addr = NULL;
   if (ph_dirty_lines)
      free(ph_dirty_lines);
   ph_dirty_lines = NULL;
   blitter = NULL;
   dim.w = 1;
   dim.h = 1;
   PtSetResource(ph_window, Pt_ARG_DIM, &dim, 0);
}



/* qnx_ph_init:
 *  Initializes normal windowed Photon gfx driver.
 */
struct BITMAP *qnx_ph_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_events_mutex);
   bmp = qnx_private_ph_init(&gfx_photon, w, h, v_w, v_h, color_depth);
   ph_gfx_mode = PH_GFX_WINDOW;
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



#ifdef ALLEGRO_NO_ASM


/* update_dirty_lines:
 *  Dirty line updater routine.
 */
static inline void update_dirty_lines()
{
   PhRect_t rect;
   int i = 0;

   rect.ul.x = 0;
   rect.lr.x = ph_window_w;
   do {
      if (ph_dirty_lines[i]) {
         rect.ul.y = i;
         while (ph_dirty_lines[i]) {
            ph_dirty_lines[i] = 0;
            i++;
         }
         rect.lr.y = i;
         ph_update_window(&rect);
      }      
      i++;
   } while (i < ph_window_h);
   
   PgFlush();
}



/* ph_write_line:
 *  Line switcher for Photon windowed mode.
 */
static unsigned long ph_write_line(BITMAP *bmp, int line)
{
   ph_dirty_lines[line + bmp->y_ofs] = 1;
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= (BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
      PgWaitHWIdle();
   }
   return (unsigned long)(bmp->line[line]);
}



/* ph_unwrite_line:
 *  Line updater for Photon windowed mode.
 */
static void ph_unwrite_line(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
      update_dirty_lines();
   }
}



/* ph_acquire:
 *  Bitmap locking for windowed mode.
 */
static void ph_acquire(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
      PgWaitHWIdle();
   }
}



/* ph_release:
 *  Bitmap unlocking for windowed mode.
 */
static void ph_release(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_LOCKED) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
      update_dirty_lines();
   }
}


#endif
