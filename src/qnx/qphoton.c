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
 *      Dirty rectangles mechanism by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
#error Something is wrong with the makefile
#endif

#include <pthread.h>


static struct BITMAP *qnx_ph_init(int, int, int, int, int);
static void qnx_ph_exit(struct BITMAP *);
static struct BITMAP *qnx_phd_init(int, int, int, int, int);
static void qnx_phd_exit(struct BITMAP *);
static void qnx_ph_vsync(void);
static void qnx_ph_set_palette(AL_CONST struct RGB *, int, int, int);
static GFX_MODE_LIST *qnx_fetch_mode_list(void);


GFX_DRIVER gfx_photon_direct =
{
   GFX_PHOTON_DIRECT,
   empty_string, 
   empty_string,
   "Photon direct",
   qnx_phd_init,                 /* AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth)); */
   qnx_phd_exit,                 /* AL_METHOD(void, exit, (struct BITMAP *b)); */
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync,                 /* AL_METHOD(void, vsync, (void)); */
   qnx_ph_set_palette,           /* AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   qnx_fetch_mode_list,
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE
};


GFX_DRIVER gfx_photon =
{
   GFX_PHOTON,
   empty_string, 
   empty_string,
   "Photon", 
   qnx_ph_init,                  /* AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth)); */
   qnx_ph_exit,                  /* AL_METHOD(void, exit, (struct BITMAP *b)); */
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync,                 /* AL_METHOD(void, vsync, (void)); */
   qnx_ph_set_palette,           /* AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y)); */
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
   TRUE
};


/* global variables */
int ph_gfx_mode = PH_GFX_NONE;
void (*ph_update_window)(PhRect_t* rect) = NULL;
PdOffscreenContext_t *ph_window_context = NULL;
PgColor_t ph_palette[256];

/* exported only for qswitch.s */
char *ph_dirty_lines = NULL;

static PdDirectContext_t *ph_direct_context = NULL;
static PdOffscreenContext_t *ph_screen_context = NULL;

static pthread_mutex_t ph_screen_lock;
static int lock_nesting = 0;

static char driver_desc[256];
static PgDisplaySettings_t original_settings;
static COLORCONV_BLITTER_FUNC *blitter = NULL;
static char *pseudo_screen_addr = NULL;
static int pseudo_screen_pitch;
static int pseudo_screen_depth;
static char *window_addr = NULL;
static int window_pitch;
static int desktop_depth;

#define RENDER_DELAY (1000/70)  /* 70 Hz */
static void update_dirty_lines(void);

static void phd_acquire(struct BITMAP *bmp);
static void phd_release(struct BITMAP *bmp);
static void ph_acquire(struct BITMAP *bmp);
static void ph_release(struct BITMAP *bmp);


#ifdef ALLEGRO_NO_ASM

static unsigned long phd_write_line(struct BITMAP *bmp, int line);
static void phd_unwrite_line(struct BITMAP *bmp);
static unsigned long ph_write_line(struct BITMAP *bmp, int line);
static void ph_unwrite_line(struct BITMAP *bmp);

#else

void (*ptr_ph_acquire)(struct BITMAP *) = &ph_acquire;
void (*ptr_ph_release)(struct BITMAP *) = &ph_release;
unsigned long phd_write_line_asm(struct BITMAP *bmp, int line);
void phd_unwrite_line_asm(struct BITMAP *bmp);
unsigned long ph_write_line_asm(struct BITMAP *bmp, int line);
void ph_unwrite_line_asm(struct BITMAP *bmp);

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



/* qnx_fetch_mode_list:
 *  Generates a list of valid video modes.
 *  Returns the mode list on success or NULL on failure.
 */
static GFX_MODE_LIST *qnx_fetch_mode_list(void)
{
   GFX_MODE_LIST *mode_list;
   PgVideoModes_t ph_mode_list;
   PgVideoModeInfo_t ph_mode_info;
   int i, count = 0;

   if (PgGetVideoModeList(&ph_mode_list))
      return NULL;

   mode_list = malloc(sizeof(GFX_MODE_LIST));
   if (!mode_list)
      return NULL;

   mode_list->mode = malloc(sizeof(GFX_MODE) * (ph_mode_list.num_modes + 1));
   if (!mode_list->mode) {
       free(mode_list);
       return NULL;
   }
   
   for (i=0; i<ph_mode_list.num_modes; i++) {
      if ((!PgGetVideoModeInfo(ph_mode_list.modes[i], &ph_mode_info)) &&
          (ph_mode_info.mode_capabilities1 & PgVM_MODE_CAP1_LINEAR)) {
         mode_list->mode[count].width = ph_mode_info.width;
         mode_list->mode[count].height = ph_mode_info.height;
         mode_list->mode[count].bpp = ph_mode_info.bits_per_pixel;
         count++;
      }
   }
   
   mode_list->mode[count].width = 0;
   mode_list->mode[count].height = 0;
   mode_list->mode[count].bpp = 0;

   mode_list->num_modes = count;

   return mode_list;
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
   char tmp1[128], tmp2[128];

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

   dim.w = gfx_photon_direct.w = w;
   dim.h = gfx_photon_direct.h = h;
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
   
   PgGetVideoMode(&settings);
   _current_refresh_rate = settings.refresh;
   
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
#else
   bmp->write_bank = phd_write_line_asm;
   bmp->read_bank = phd_write_line_asm;
#endif

   _screen_vtable.acquire = phd_acquire;
   _screen_vtable.release = phd_release; 

   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;
   desktop_depth = color_depth;

   setup_direct_shifts();
   uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("Photon direct mode (%s)", tmp1),
             uconvert(caps.chip_name, U_UTF8, tmp2, U_CURRENT, sizeof(tmp2)));
   drv->desc = driver_desc;

   _mouse_on = TRUE;

   PgFlush();
   PgWaitHWIdle();

   return bmp;
}



/* qnx_private_phd_exit:
 *  This is where the video driver is actually closed.
 */
static void qnx_private_phd_exit(void)
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
static struct BITMAP *qnx_phd_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_events_mutex);
   bmp = qnx_private_phd_init(&gfx_photon_direct, w, h, v_w, v_h, color_depth, TRUE);
   ph_gfx_mode = PH_GFX_DIRECT;
   if (!bmp)
      qnx_private_phd_exit();

   pthread_mutex_unlock(&qnx_events_mutex);

   return bmp;
}



/* qnx_phd_exit:
 *  Shuts down photon direct driver.
 */
static void qnx_phd_exit(struct BITMAP *bmp)
{
   pthread_mutex_lock(&qnx_events_mutex);
   qnx_private_phd_exit();
   pthread_mutex_unlock(&qnx_events_mutex);
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


#endif



/* qnx_ph_vsync:
 *  Waits for vertical retrace.
 */
static void qnx_ph_vsync(void)
{
   PgWaitVSync();
}



/* qnx_ph_set_palette:
 *  Sets hardware palette.
 */
static void qnx_ph_set_palette(AL_CONST struct RGB *p, int from, int to, int retracesync)
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
      temp.lr.x = gfx_photon.w;
      temp.lr.y = gfx_photon.h;
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
      temp.lr.x = gfx_photon.w;
      temp.lr.y = gfx_photon.h;
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
   char tmp1[128], tmp2[128];

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
   _current_refresh_rate = settings.refresh;
      
   PgGetVideoModeInfo(settings.mode, &mode_info);

   dim.w = gfx_photon.w = w;
   dim.h = gfx_photon.h = h;
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
   ph_dirty_lines = (char *)calloc(h, sizeof(char));
   
   if ((!bmp) || (!ph_dirty_lines)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }

   pthread_mutex_init(&ph_screen_lock, NULL);

   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;

#ifdef ALLEGRO_NO_ASM
   bmp->write_bank = ph_write_line;
   _screen_vtable.unwrite_bank = ph_unwrite_line;
#else
   bmp->write_bank = ph_write_line_asm;
   _screen_vtable.unwrite_bank = ph_unwrite_line_asm;
#endif

   _screen_vtable.acquire = ph_acquire;
   _screen_vtable.release = ph_release;

   setup_direct_shifts();
   uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("Photon windowed, %d bpp %s", tmp1), color_depth,
             uconvert_ascii(color_depth == desktop_depth ? "in matching" : "in fast emulation", tmp2));
   drv->desc = driver_desc;

   PgFlush();
   PgWaitHWIdle();

   install_int(update_dirty_lines, RENDER_DELAY);

   return bmp;
}



/* qnx_private_ph_exit:
 *  This is where the video driver is actually closed.
 */
static void qnx_private_ph_exit(void)
{
   PhDim_t dim;
   
   remove_int(update_dirty_lines);
   
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
   pthread_mutex_destroy(&ph_screen_lock);
   dim.w = 1;
   dim.h = 1;
   PtSetResource(ph_window, Pt_ARG_DIM, &dim, 0);
}



/* qnx_ph_init:
 *  Initializes normal windowed Photon gfx driver.
 */
static struct BITMAP *qnx_ph_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_events_mutex);
   bmp = qnx_private_ph_init(&gfx_photon, w, h, v_w, v_h, color_depth);
   ph_gfx_mode = PH_GFX_WINDOW;
   if (!bmp)
      qnx_private_ph_exit();
      
   pthread_mutex_unlock(&qnx_events_mutex);

   return bmp;
}



/* qnx_ph_exit:
 *  Shuts down photon windowed gfx driver.
 */
static void qnx_ph_exit(struct BITMAP *b)
{
   pthread_mutex_lock(&qnx_events_mutex);
   qnx_private_ph_exit();
   pthread_mutex_unlock(&qnx_events_mutex);
}



/* update_dirty_lines:
 *  Dirty line updater routine.
 */
static void update_dirty_lines(void)
{
   PhRect_t rect;
   
   /* to prevent the drawing threads and the rendering proc
      from concurrently accessing the dirty lines array */
   pthread_mutex_lock(&ph_screen_lock);
   
   /* pseudo dirty rectangles mechanism:
    *  at most only one PgFlush() call is performed for each frame.
    */

   rect.ul.x = 0;
   rect.lr.x = gfx_photon.w;

   /* find the first dirty line */
   rect.ul.y = 0;

   while (!ph_dirty_lines[rect.ul.y])
      rect.ul.y++;

   if (rect.ul.y < gfx_photon.h) {
      /* find the last dirty line */
      rect.lr.y = gfx_photon.h;

      while (!ph_dirty_lines[rect.lr.y-1])
         rect.lr.y--;

      ph_update_window(&rect);
   
      PgFlush();
      
      /* clean up the dirty lines */
      while (rect.ul.y < rect.lr.y)
         ph_dirty_lines[rect.ul.y++] = 0;
   }
   
   pthread_mutex_unlock(&ph_screen_lock);
}



/* ph_acquire:
 *  Bitmap locking for windowed mode.
 */
static void ph_acquire(struct BITMAP *bmp)
{
   PgWaitHWIdle();
   
   /* to prevent the drawing threads and the rendering proc
      from concurrently accessing the dirty lines array */
   pthread_mutex_lock(&ph_screen_lock);

   lock_nesting++;
   bmp->id |= BMP_ID_LOCKED;
}

 

/* ph_release:
 *  Bitmap unlocking for windowed mode.
 */
static void ph_release(struct BITMAP *bmp)
{
   if (lock_nesting > 0) {
      lock_nesting--;

      if (!lock_nesting)
         bmp->id &= ~BMP_ID_LOCKED;

      pthread_mutex_unlock(&ph_screen_lock);
   }
}



#ifdef ALLEGRO_NO_ASM


/* ph_write_line:
 *  Line switcher for Photon windowed mode.
 */
static unsigned long ph_write_line(BITMAP *bmp, int line)
{
   ph_dirty_lines[line + bmp->y_ofs] = 1;
   
   if (!(bmp->id & BMP_ID_LOCKED)) {
      ph_acquire(bmp);   
      bmp->id |= BMP_ID_AUTOLOCK;
   }

   return (unsigned long)(bmp->line[line]);
}



/* ph_unwrite_line:
 *  Line updater for Photon windowed mode.
 */
static void ph_unwrite_line(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      ph_release(bmp);
      bmp->id &= ~BMP_ID_AUTOLOCK;
   }
}


#endif
