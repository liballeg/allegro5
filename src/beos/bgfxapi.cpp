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
 *      Stuff for BeOS.
 *
 *      By Jason Wilkins.
 *      
 *      Windowed mode by Peter Wang.
 *
 *      Various bug fixes, windowed driver optimizations and mouse wheel
 *      support by Angelo Mottola
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif


#define BE_DRAWING_THREAD_PERIOD    16000
#define BE_DRAWING_THREAD_NAME	    "drawing thread"
#define BE_DRAWING_THREAD_PRIORITY  B_REAL_TIME_DISPLAY_PRIORITY


#define SUSPEND_ALL_THREADS()		\
{									\
   be_key_suspend();				\
   be_sound_suspend();				\
   be_time_suspend();				\
   be_sys_suspend();				\
   be_main_suspend();				\
}

#define RESUME_ALL_THREADS()		\
{									\
   be_key_resume();					\
   be_sound_resume();				\
   be_time_resume();				\
   be_sys_resume();					\
   be_main_resume();				\
}



AL_CONST BE_MODE_TABLE _be_mode_table[] = {
   { 8,    640,  400, B_8_BIT_640x400,    B_CMAP8 },
   { 8,    640,  480, B_8_BIT_640x480,    B_CMAP8 },
   { 8,    800,  600, B_8_BIT_800x600,    B_CMAP8 },
   { 8,   1024,  768, B_8_BIT_1024x768,   B_CMAP8 },
   { 8,   1152,  900, B_8_BIT_1152x900,   B_CMAP8 },
   { 8,   1280, 1024, B_8_BIT_1280x1024,  B_CMAP8 },
   { 8,   1600, 1200, B_8_BIT_1600x1200,  B_CMAP8 },
   { 15,   640,  480, B_15_BIT_640x480,   B_RGB15 },
   { 15,   800,  600, B_15_BIT_800x600,   B_RGB15 },
   { 15,  1024,  768, B_15_BIT_1024x768,  B_RGB15 },
   { 15,  1152,  900, B_15_BIT_1152x900,  B_RGB15 },
   { 15,  1280, 1024, B_15_BIT_1280x1024, B_RGB15 },
   { 15,  1600, 1200, B_15_BIT_1600x1200, B_RGB15 },
   { 16,   640,  480, B_16_BIT_640x480,   B_RGB16 },
   { 16,   800,  600, B_16_BIT_800x600,   B_RGB16 },
   { 16,  1024,  768, B_16_BIT_1024x768,  B_RGB16 },
   { 16,  1152,  900, B_16_BIT_1152x900,  B_RGB16 },
   { 16,  1280, 1024, B_16_BIT_1280x1024, B_RGB16 },
   { 16,  1600, 1200, B_16_BIT_1600x1200, B_RGB16 },
   { 32,   640,  480, B_32_BIT_640x480,   B_RGB32 },
   { 32,   800,  600, B_32_BIT_800x600,   B_RGB32 },
   { 32,  1024,  768, B_32_BIT_1024x768,  B_RGB32 },
   { 32,  1152,  900, B_32_BIT_1152x900,  B_RGB32 },
   { 32,  1280, 1024, B_32_BIT_1280x1024, B_RGB32 },
   { 32,  1600, 1200, B_32_BIT_1600x1200, B_RGB32 },
   { -1 }
};

sem_id _be_fullscreen_lock = -1;
sem_id _be_window_lock = -1;

volatile int _be_lock_count  = 0;
int *_be_dirty_lines = NULL;
int _be_mouse_z = 0;

BeAllegroWindow	    *_be_allegro_window	    = NULL;
BeAllegroView	    *_be_allegro_view	    = NULL;
BeAllegroScreen	    *_be_allegro_screen	    = NULL; 
BWindow             *_be_window             = NULL;

void (*_be_window_close_hook)() = NULL;

static volatile bool be_gfx_initialized;

static char driver_desc[256] = EMPTY_STRING;
static int refresh_rate = 70;

static thread_id palette_thread_id = -1;
static sem_id palette_sem = -1;
static rgb_color palette_colors[256];
static unsigned char *cmap = NULL;



static void _be_gfx_set_truecolor_shifts()
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



/*----------------------------------------------------------------*/
/*    Fullscreen mode                                             */
/*----------------------------------------------------------------*/



static inline void change_focus(bool active)
{
   int i;
   
   if (_be_focus_count < 0)
      _be_focus_count = 0;
      
   if (active) {
      _be_focus_count++;
      if (be_gfx_initialized) {
         switch (_be_switch_mode) {
            case SWITCH_AMNESIA:
            case SWITCH_BACKAMNESIA:
               if ((_be_allegro_window) &&
                   (_be_allegro_window->drawing_thread_id > 0)) {
                  resume_thread(_be_allegro_window->drawing_thread_id);
               }
               if (_be_switch_mode == SWITCH_BACKAMNESIA)
                  break;
            case SWITCH_PAUSE:
               RESUME_ALL_THREADS();
               break;
         }
         be_display_switch_callback(SWITCH_IN);
      }
   }
   else {
      _be_focus_count--;
      if (be_gfx_initialized) {
         be_display_switch_callback(SWITCH_OUT);
         switch (_be_switch_mode) {
            case SWITCH_AMNESIA:
            case SWITCH_BACKAMNESIA:
               if ((_be_allegro_window) &&
                   (_be_allegro_window->drawing_thread_id > 0)) {
                  suspend_thread(_be_allegro_window->drawing_thread_id);
               }
               if (_be_switch_mode == SWITCH_BACKAMNESIA)
                  break;
            case SWITCH_PAUSE:
               if (_be_midisynth)
                  _be_midisynth->AllNotesOff(false);
               SUSPEND_ALL_THREADS();
               break;
         }
      }
   }
}



static inline bool handle_window_close(const char *title)
{
   int result;
   char tmp1[512], tmp2[32], tmp3[32];
   
   if (_be_window_close_hook != NULL) {
      _be_window_close_hook();
   }
   else {
      BAlert *close_alert = new BAlert(title,
         uconvert_toascii(get_config_text(ALLEGRO_WINDOW_CLOSE_MESSAGE), tmp1),
         uconvert_toascii(get_config_text("Yes"), tmp2),
         uconvert_toascii(get_config_text("No"), tmp3), 
         NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
      close_alert->SetShortcut(1, B_ESCAPE);
      if (_be_midisynth)
         _be_midisynth->AllNotesOff(false);
      SUSPEND_ALL_THREADS();
      result = close_alert->Go();
      RESUME_ALL_THREADS();
      if (result == 0)
         be_terminate(0, false);
   }
   return false;
}



/* BeAllegroScreen::BeAllegroScreen:
 */
BeAllegroScreen::BeAllegroScreen(const char *title, uint32 space, status_t *error, bool debugging)
   : BWindowScreen(title, space, error, debugging)
{
}



/* BeAllegroScreen::ScreenConnected:
 */
void BeAllegroScreen::ScreenConnected(bool connected)
{
   if(connected) {
      change_focus(connected);      
      release_sem(_be_fullscreen_lock);
   }
   else {
      acquire_sem(_be_fullscreen_lock);
      change_focus(connected);
   }
}



/* BeAllegroScreen::QuitRequested:
 */
bool BeAllegroScreen::QuitRequested(void)
{
   return handle_window_close(_be_allegro_screen->Title());
}



/* BeAllegroScreen::MessageReceived:
 */
void BeAllegroScreen::MessageReceived(BMessage *message)
{
   switch (message->what) {
      case B_SIMPLE_DATA:
         break;
      
      case B_MOUSE_WHEEL_CHANGED:
         float dy; 
         message->FindFloat("be:wheel_delta_y", &dy);
         _be_mouse_z += ((int)dy > 0 ? -1 : 1);
         break;

      default:
         BWindowScreen::MessageReceived(message);
         break;
   }
}



static inline uint32 find_gfx_mode(int w, int h, int d)
{
   int index = 0;
   while (_be_mode_table[index].d > 0) {
      if ((_be_mode_table[index].w == w) &&
          (_be_mode_table[index].h == h) &&
          (_be_mode_table[index].d == d))
         return _be_mode_table[index].mode;
      index++;
   }

   return (uint32)-1;
}



static inline bool be_sort_out_virtual_resolution(int w, int h, int *v_w, int *v_h, int color_depth)
{
   int32 try_v_w;
   int32 try_v_h;
   int mb;

   if (*v_w == 0)
      try_v_w = w;
   else
      try_v_w = *v_w;
   try_v_h = *v_h;

   if (*v_h == 0) {
      for (mb=32; mb>1; mb--) {
	 try_v_h = (1024*1024*mb) / (try_v_w * BYTES_PER_PIXEL(color_depth));
	 /* Evil hack: under BeOS R5 SetFrameBuffer() should work with any
	  * int32 width and height parameters, but actually it crashes if
	  * one of them exceeds the boundaries of a signed short variable.
	  */
	 try_v_h = MIN(try_v_h, 32767);

	 if (_be_allegro_screen->SetFrameBuffer(try_v_w, try_v_h) == B_OK) {
	    *v_w = try_v_w;
	    *v_h = try_v_h;
	    return true;
         }
      }
      try_v_h = h;
   }

   if (_be_allegro_screen->SetFrameBuffer(try_v_w, try_v_h) == B_OK) {
      *v_w = try_v_w;
      *v_h = try_v_h;
      return true;
   }
   else {
      return false;
   }
}



/* palette_updater_thread:
 *  This small thread is used to update the palette in fullscreen mode. It may seem
 *  unnecessary as a direct call to SetColorList would do it, but calling directly
 *  this function from the main thread has proven to be a major bottleneck, so
 *  calling it from a separated thread is a good thing.
 */
static int32 palette_updater_thread(void *data) 
{
   BeAllegroScreen *s = (BeAllegroScreen *)data;
   
   while (be_gfx_initialized) {
      acquire_sem(palette_sem);
      s->SetColorList(palette_colors, 0, 255);
   }
   return B_OK;
}



/* be_gfx_fullscreen_fetch_mode_list:
 *  Builds the list of available video modes.
 */
extern "C" struct GFX_MODE_LIST *be_gfx_fullscreen_fetch_mode_list(void)
{
   int j, be_mode, num_modes = 0;
   uint32 i, count;
   display_mode *mode;
   GFX_MODE_LIST *mode_list;
   bool already_there;
   
   if (BScreen().GetModeList(&mode, &count) != B_OK)
      return NULL;
   
   mode_list = (GFX_MODE_LIST *)malloc(sizeof(GFX_MODE_LIST));
   if (!mode_list) {
      free(mode);
      return NULL;
   }
   mode_list->mode = NULL;
   
   for (i=0; i<count; i++) {
      be_mode = 0;
      while (_be_mode_table[be_mode].d > 0) {
         if ((mode[i].virtual_width == _be_mode_table[be_mode].w) &&
             (mode[i].virtual_height == _be_mode_table[be_mode].h) &&
             (mode[i].space == _be_mode_table[be_mode].space))
            break;
         be_mode++;
      }
      if (_be_mode_table[be_mode].d == -1)
         continue;

      already_there = false;
      for (j=0; j<num_modes; j++) {
         if ((mode_list->mode[j].width == _be_mode_table[be_mode].w) &&
             (mode_list->mode[j].height == _be_mode_table[be_mode].h) &&
             (mode_list->mode[j].bpp == _be_mode_table[be_mode].d)) {
            already_there = true;
            break;
         }
      }
      if (!already_there) {
         mode_list->mode = (GFX_MODE *)realloc(mode_list->mode, sizeof(GFX_MODE) * (num_modes + 1));
         if (!mode_list->mode) {
            free(mode);
            return NULL;
         }
         mode_list->mode[num_modes].width = _be_mode_table[be_mode].w;
         mode_list->mode[num_modes].height = _be_mode_table[be_mode].h;
         mode_list->mode[num_modes].bpp = _be_mode_table[be_mode].d;
         num_modes++;
      }
   }
   mode_list->mode = (GFX_MODE *)realloc(mode_list->mode, sizeof(GFX_MODE) * (num_modes + 1));
   if (!mode_list->mode) {
      free(mode);
      return NULL;
   }
   mode_list->mode[num_modes].width = 0;
   mode_list->mode[num_modes].height = 0;
   mode_list->mode[num_modes].bpp = 0;
   mode_list->num_modes = num_modes;
   
   free(mode);
   
   return mode_list;
}



static struct BITMAP *_be_gfx_fullscreen_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, bool accel)
{
   BITMAP             *bmp;
   status_t            error;
   uint32              mode;
   graphics_card_info *gfx_card;
   frame_buffer_info  *fbuffer;
   accelerant_device_info info;
   char  path[MAXPATHLEN];
   char tmp1[128], tmp2[128];
   char *exe;

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
   mode = find_gfx_mode(w, h, color_depth);

   if (mode == (uint32)-1) {
      goto cleanup;
   }

   _be_fullscreen_lock = create_sem(0, "screen lock");

   if (_be_fullscreen_lock < 0) {
      goto cleanup;
   }
   
   _be_lock_count = 0;

   _be_sys_get_executable_name(path, sizeof(path));
   path[sizeof(path)-1] = '\0';
   exe = get_filename(path);

   set_display_switch_mode(SWITCH_AMNESIA);
   
   _be_allegro_screen = new BeAllegroScreen(exe, mode, &error, false);
   _be_window = _be_allegro_screen;

   if(error != B_OK) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      goto cleanup;
   }

   _be_mouse_view = new BView(_be_allegro_screen->Bounds(),
     "allegro mouse view", B_FOLLOW_ALL_SIDES, 0);
   _be_allegro_screen->Lock();
   _be_allegro_screen->AddChild(_be_mouse_view);
   _be_allegro_screen->Unlock();

   _be_mouse_window = _be_allegro_screen;
   _be_mouse_window_mode = false;
   _mouse_on = TRUE;

   release_sem(_be_mouse_view_attached);

   palette_sem = create_sem(0, "palette sem");
   if (palette_sem < 0) {
      goto cleanup;
   }
      
   be_gfx_initialized = false;
   _be_allegro_screen->Show();
   acquire_sem(_be_fullscreen_lock);

   gfx_card = _be_allegro_screen->CardInfo();

   if (_be_allegro_screen->CanControlFrameBuffer()) {
      if (!be_sort_out_virtual_resolution(w, h, &v_w, &v_h, color_depth)) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         goto cleanup;
      }
   }
   else {
      if (((v_w != 0) && (v_w != w)) || ((v_h != 0) && (v_h != h)))
	 goto cleanup;
   }

   fbuffer = _be_allegro_screen->FrameBufferInfo();

   drv->w       = w;
   drv->h       = h;
   drv->linear  = TRUE;
   drv->vid_mem = v_w * v_h * BYTES_PER_PIXEL(color_depth);
   
   bmp = _make_bitmap(fbuffer->width, fbuffer->height,
            (unsigned long)gfx_card->frame_buffer, drv,
            color_depth, fbuffer->bytes_per_row);

   if (!bmp) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      goto cleanup;
   }

   _be_sync_func = NULL;
   if (accel) {
      be_gfx_fullscreen_accelerate(color_depth);
   }

#ifdef ALLEGRO_NO_ASM
   if (gfx_capabilities) {
      bmp->write_bank = be_gfx_fullscreen_read_write_bank;
      bmp->read_bank  = be_gfx_fullscreen_read_write_bank;
      _screen_vtable.unwrite_bank = be_gfx_fullscreen_unwrite_bank;
   }
#else
   if (gfx_capabilities) {
      bmp->write_bank = _be_gfx_fullscreen_read_write_bank_asm;
      bmp->read_bank  = _be_gfx_fullscreen_read_write_bank_asm;
      _screen_vtable.unwrite_bank = _be_gfx_fullscreen_unwrite_bank_asm;
   }
#endif
   gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;

   _screen_vtable.acquire      = be_gfx_fullscreen_acquire;
   _screen_vtable.release      = be_gfx_fullscreen_release;

   _be_gfx_set_truecolor_shifts();
   if (BScreen().GetDeviceInfo(&info) == B_OK)
      uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("BWindowScreen object (%s)", tmp1),
                uconvert_ascii(info.name, tmp2));
   else
      ustrzcpy(driver_desc, sizeof(driver_desc), uconvert_ascii("BWindowScreen object", tmp1));
   drv->desc = driver_desc;

   be_gfx_initialized = true;
   
   palette_thread_id = spawn_thread(palette_updater_thread, "palette updater", 
				    B_DISPLAY_PRIORITY, (void *)_be_allegro_screen);
   resume_thread(palette_thread_id);

   be_app->HideCursor();
   
   release_sem(_be_fullscreen_lock);
   while (!_be_focus_count);

   return bmp;

   cleanup: {
      be_gfx_fullscreen_exit(NULL);
      return NULL;
   }
}



extern "C" struct BITMAP *be_gfx_fullscreen_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return _be_gfx_fullscreen_init(&gfx_beos_fullscreen, w, h, v_w, v_h, color_depth, true);
}



extern "C" struct BITMAP *be_gfx_fullscreen_safe_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return _be_gfx_fullscreen_init(&gfx_beos_fullscreen_safe, w, h, v_w, v_h, color_depth, false);
}



extern "C" void be_gfx_fullscreen_exit(struct BITMAP *bmp)
{
   status_t ret_value;
   (void)bmp;

   if (_be_fullscreen_lock > 0) {
      delete_sem(_be_fullscreen_lock);
      _be_fullscreen_lock = -1;
   }
   be_gfx_initialized = false;
   
   if (palette_thread_id >= 0) {
      release_sem(palette_sem);
      wait_for_thread(palette_thread_id, &ret_value);
      palette_thread_id = -1;
   }
   if (palette_sem >= 0) {
      delete_sem(palette_sem);
      palette_sem = -1;
   }
   
   if (_be_allegro_screen != NULL) {
      if (_be_mouse_view_attached < 1) {
	 acquire_sem(_be_mouse_view_attached);
      }

      _be_allegro_screen->Lock();
      _be_allegro_screen->Quit();

      _be_allegro_screen = NULL;
      _be_window = NULL;
   }
   
   be_app->ShowCursor();

   _be_mouse_window   = NULL;
   _be_mouse_view     = NULL;

   _be_focus_count = 0;
   _be_lock_count = 0;
}



#ifdef ALLEGRO_NO_ASM

extern "C" unsigned long be_gfx_fullscreen_read_write_bank(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      _be_sync_func();
      bmp->id |= (BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
   }
   return (unsigned long)(bmp->line[line]);
}



extern "C" void be_gfx_fullscreen_unwrite_bank(BITMAP *bmp)
{
   if (bmp->id & BMP_AUTOLOCK) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
   }
}

#endif



extern "C" void be_gfx_fullscreen_acquire(struct BITMAP *bmp)
{
   if (_be_lock_count == 0) {
      acquire_sem(_be_fullscreen_lock);
      bmp->id |= BMP_ID_LOCKED;
   }
   if (_be_sync_func)
      _be_sync_func();
   _be_lock_count++;
}



extern "C" void be_gfx_fullscreen_release(struct BITMAP *bmp)
{
   _be_lock_count--;

   if (_be_lock_count == 0) {
      bmp->id &= ~BMP_ID_LOCKED;
      release_sem(_be_fullscreen_lock);
   }
}



extern "C" void be_gfx_fullscreen_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   if (vsync)
      be_gfx_vsync();

   for(int index = from; index <= to; index++) {
      palette_colors[index].red   = _rgb_scale_6[p[index].r];
      palette_colors[index].green = _rgb_scale_6[p[index].g];
      palette_colors[index].blue  = _rgb_scale_6[p[index].b];
      palette_colors[index].alpha = 255;
   }
   /* Update palette! */
   release_sem(palette_sem);
}



extern "C" int be_gfx_fullscreen_scroll(int x, int y)
{
   int rv;

   acquire_screen();
   
   if (_be_allegro_screen->MoveDisplayArea(x, y) != B_ERROR) {
      rv = 0;
   }
   else {
      rv = 1;
   }

   release_screen();

   be_gfx_vsync();

   return rv;
}



/* be_gfx_fullscreen_poll_scroll:
 *  Returns true if there are pending scrolling requests left.
 */
extern "C" int be_gfx_fullscreen_poll_scroll(void)
{
   return (BScreen(_be_allegro_screen).WaitForRetrace(0) == B_ERROR ? TRUE : FALSE);
}



/* be_gfx_fullscreen_request_scroll:
 *  Starts a screen scroll but doesn't wait for the retrace.
 */
extern "C" int be_gfx_fullscreen_request_scroll(int x, int y)
{
   acquire_screen();
   _be_allegro_screen->MoveDisplayArea(x, y);
   release_screen();
   
   return 0;
}



/* be_gfx_fullscreen_request_video_bitmap:
 *  Page flips to display specified bitmap, but doesn't wait for retrace.
 */
extern "C" int be_gfx_fullscreen_request_video_bitmap(struct BITMAP *bmp)
{
   int rv;
   
   acquire_screen();
   rv = _be_allegro_screen->MoveDisplayArea(bmp->x_ofs, bmp->y_ofs) == B_ERROR ? 1 : 0;
   release_screen();

   return rv;
}



extern "C" void be_gfx_vsync(void)
{
   if(BScreen(_be_window).WaitForRetrace() != B_OK) {
      if (_timer_installed) {
         int start_count;

         start_count = retrace_count;

         while (start_count == retrace_count) {
         }
      }
      else {
         snooze (500000 / refresh_rate);
      }
   }
}



/*----------------------------------------------------------------*/
/*    Windowed mode				                  */
/*----------------------------------------------------------------*/


static int32 _be_gfx_window_drawing_thread(void *data) 
{
   BeAllegroWindow *w = (BeAllegroWindow *)data;
   GRAPHICS_RECT src_gfx_rect, dest_gfx_rect;

   src_gfx_rect.pitch = w->screen_pitch;
   src_gfx_rect.height = 1;

   while (!w->dying) {
      if (w->connected && w->blitter) {
         clipping_rect *rect;
         uint32 i, j;

         acquire_sem(_be_window_lock);
         w->locker->Lock();
         for (i=0; i<w->screen_height; i++) {
            if (_be_dirty_lines[i]) {
               rect = w->rects;
               for (j=0; j<w->num_rects; j++, rect++)
                  if (((int32)i >= rect->top - w->window.top) &&
                      ((int32)i <= rect->bottom - w->window.top)) {
                     src_gfx_rect.width = rect->right - rect->left + 1;
                     src_gfx_rect.data = (void *)((unsigned long)w->screen_data + 
                        (i * w->screen_pitch) +
                        ((rect->left - w->window.left) * BYTES_PER_PIXEL(w->screen_depth)));
                     dest_gfx_rect.data = (void *)((unsigned long)w->display_data +
                        (i * w->display_pitch) +
                        (rect->left * BYTES_PER_PIXEL(w->display_depth)));
                     dest_gfx_rect.pitch = w->display_pitch;
                     w->blitter(&src_gfx_rect, &dest_gfx_rect);
                  }
               _be_dirty_lines[i] = 0;
            }
         }
         w->locker->Unlock();
      }      

      snooze(BE_DRAWING_THREAD_PERIOD);
   }  

   return B_OK;
}



#ifdef ALLEGRO_NO_ASM

extern "C" void be_gfx_windowed_unwrite_bank(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
      release_sem(_be_window_lock);
   }
}



extern "C" unsigned long be_gfx_windowed_read_write_bank(BITMAP *bmp, int line)
{
   if (!bmp->id & BMP_ID_LOCKED) {
      bmp->id |= (BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
   }
   _be_dirty_lines[bmp->y_ofs + line] = 1;
   return (unsigned long)(bmp->line[line]);
}

#endif



/* BeAllegroWindow::BeAllegroWindow:
 */
BeAllegroWindow::BeAllegroWindow(BRect frame, const char *title,
   window_look look, window_feel feel, uint32 flags, uint32 workspaces,
   uint32 v_w, uint32 v_h, uint32 color_depth)
   : BDirectWindow(frame, title, look, feel, flags, workspaces)
{
   BRect rect = Bounds();
   uint32 i;

   _be_allegro_view = new BeAllegroView(rect, "Allegro", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
   rgb_color color = {0, 0, 0, 0};
   _be_allegro_view->SetViewColor(color);
   AddChild(_be_allegro_view);
   
   screen_pitch = v_w * BYTES_PER_PIXEL(color_depth);
   screen_data = malloc(v_h * screen_pitch);
   screen_depth = color_depth;
   display_depth = 0;

   num_rects = 0;
   rects     = NULL;
   
   connected = false;
   dying     = false;
   locker    = new BLocker();
   blitter   = NULL;
   
   _be_dirty_lines = (int *)malloc(v_h * sizeof(int));
   
   _be_window_lock = create_sem(0, "window lock");
      
   drawing_thread_id = spawn_thread(_be_gfx_window_drawing_thread, BE_DRAWING_THREAD_NAME, 
				    BE_DRAWING_THREAD_PRIORITY, (void *)this);

   resume_thread(drawing_thread_id);
}



/* BeAllegroWindow::~BeAllegroWindow:
 */
BeAllegroWindow::~BeAllegroWindow()
{
   int32 result;

   dying = true;
   release_sem(_be_window_lock);
   locker->Unlock();
   Sync();
   wait_for_thread(drawing_thread_id, &result);
   drawing_thread_id = -1;
   Hide();

   delete locker;
   delete_sem(_be_window_lock);
   _release_colorconv_blitter(blitter);
   blitter = NULL;
   connected = false;
   _be_focus_count = 0;

   if (rects) {
      free(rects);
      rects = NULL;
   }
   if (_be_dirty_lines) {
      free(_be_dirty_lines);
      _be_dirty_lines = NULL;
   }
   if (screen_data) {
      free(screen_data);
      screen_data = NULL;
   }
}



/* BeAllegroWindow::MessageReceived:
 */
void BeAllegroWindow::MessageReceived(BMessage *message)
{
   switch (message->what) {
      case B_SIMPLE_DATA:
         break;
      
      case B_MOUSE_WHEEL_CHANGED:
         float dy; 
         message->FindFloat("be:wheel_delta_y", &dy);
         _be_mouse_z += ((int)dy > 0 ? -1 : 1);
         break;

      default:
         BDirectWindow::MessageReceived(message);
         break;
   }
}



/* BeAllegroWindow::DirectConnected:
 */
void BeAllegroWindow::DirectConnected(direct_buffer_info *info)
{
   size_t size;
   char tmp[128];
   
   if (dying) {
      return;
   }

   locker->Lock();
   connected = false;
   
   switch (info->buffer_state & B_DIRECT_MODE_MASK) {
      case B_DIRECT_START:
	    
	 /* fallthrough */
	 
      case B_DIRECT_MODIFY: {
	 uint8 old_display_depth = display_depth;
	 uint32 i;

	 switch (info->pixel_format) {
	    case B_CMAP8:  
	       display_depth = 8;  
	       break;
	    case B_RGB15:  
	    case B_RGBA15: 
	       display_depth = 15; 
	       break;
	    case B_RGB16:  
	       display_depth = 16; 
	       break;
	    case B_RGB32:  
	    case B_RGBA32: 
	       display_depth = 32; 
	       break;
	    default:	      
	       display_depth = 0;  
	       break;
	 }
	 
	 if (!display_depth) {
	    break;  
	 }

	 if (old_display_depth != display_depth) {
	    int i;
	    
	    _release_colorconv_blitter(blitter);
	    blitter = _get_colorconv_blitter(screen_depth, display_depth);
	    if (display_depth == 8) {
	       cmap = _get_colorconv_map();
	       if (screen_depth == 8) {
	          for (i=0; i<256; i++)
	             cmap[i] = BScreen().IndexForColor(palette_colors[i]);
	       }
	       else {
	          for (i=0; i<4096; i++)
	             cmap[i] = BScreen().IndexForColor(((i >> 4) & 0xF0) | (i >> 8),  (i & 0xF0) | ((i >> 4) & 0xF),  (i & 0xF) | ((i & 0xF) << 4));
	       }
	    }
	    AL_TRACE("Color conversion mode set: %d->%d\n", (int)screen_depth, (int)display_depth);
	 }
	 			       
	 if (rects) {
	    free(rects);
	 }

	 num_rects = info->clip_list_count;
	 size = num_rects * (sizeof *rects);
	 rects = (clipping_rect *)malloc(num_rects * size);
	 
	 if (rects) {
	    memcpy(rects, info->clip_list, size);
	 }

	 window = info->window_bounds;
	 screen_height = window.bottom - window.top + 1;
	 display_pitch = info->bytes_per_row;
	 display_data = (void *)((unsigned long)info->bits + 
	    (window.top * display_pitch));
     
	 for (i=0; i<screen_height; i++) {
	    _be_dirty_lines[i] = 1;
	 }

	 connected = true;
	 be_gfx_initialized = true;
	 break;
      }

      case B_DIRECT_STOP:
	 break;
   }
   
   if (screen_depth == 8)
      be_gfx_windowed_set_palette(_current_palette, 0, 255, 0);

   uszprintf(driver_desc, sizeof(driver_desc), get_config_text("BDirectWindow, %d bit in %s"),
             screen_depth, uconvert_ascii(screen_depth == display_depth ? "matching" : "fast emulation", tmp));

   release_sem(_be_window_lock);
   locker->Unlock();
}



/* BeAllegroWindow::WindowActivated:
 */
void BeAllegroWindow::WindowActivated(bool active)
{
   change_focus(active);
   BDirectWindow::WindowActivated(active);
}



/* BeAllegroWindow::QuitRequested:
 */
bool BeAllegroWindow::QuitRequested(void)
{
    return handle_window_close(_be_allegro_window->Title());
}



static struct BITMAP *_be_gfx_windowed_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, int direct)
{
   BITMAP *bmp;
   int bpp;
   char path[MAXPATHLEN];
   char tmp[128];
   char *exe;

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

   if ((!v_w) && (!v_h)) {
      v_w = w;
      v_h = h;
   }

   if ((w != v_w) || (h != v_h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported virtual resolution"));
      return NULL;
   }
   
   _be_sys_get_executable_name(path, sizeof(path));
   path[sizeof(path)-1] = '\0';
   exe = get_filename(path);

   set_display_switch_mode(SWITCH_PAUSE);

   _be_allegro_window = new BeAllegroWindow(BRect(0, 0, w-1, h-1), exe,
			      B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			      B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
			      B_CURRENT_WORKSPACE, v_w, v_h, color_depth);
   _be_window = _be_allegro_window;

   if (!_be_allegro_window->SupportsWindowMode()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Windowed mode not supported"));
      goto cleanup;
   }

   _be_mouse_view = new BView(_be_allegro_window->Bounds(),
			     "allegro mouse view", B_FOLLOW_ALL_SIDES, 0);
   _be_allegro_window->Lock();
   _be_allegro_window->AddChild(_be_mouse_view);
   _be_allegro_window->Unlock();

   _be_mouse_window = _be_allegro_window;
   _be_mouse_window_mode = true;

   release_sem(_be_mouse_view_attached);

   be_gfx_initialized = false;
   
   _be_allegro_window->MoveTo(6, 25);
   _be_allegro_window->Show();

   bpp = (color_depth + 7) / 8;

   drv->w       = w;
   drv->h       = h;
   drv->linear	= TRUE;
   drv->vid_mem	= bpp * v_w * v_h;
   
   bmp = _make_bitmap(v_w, v_h, (unsigned long)_be_allegro_window->screen_data, drv, 
		      color_depth, v_w * bpp);
  
   if (!bmp) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      goto cleanup;
   }

#ifdef ALLEGRO_NO_ASM
   bmp->read_bank = be_gfx_windowed_read_write_bank;
   bmp->write_bank = be_gfx_windowed_read_write_bank;
   _screen_vtable.unwrite_bank = _be_gfx_windowed_unwrite_bank;
#else
   bmp->read_bank = _be_gfx_windowed_read_write_bank_asm;
   bmp->write_bank = _be_gfx_windowed_read_write_bank_asm;
   _screen_vtable.unwrite_bank = _be_gfx_windowed_unwrite_bank_asm;
#endif

   _screen_vtable.acquire = be_gfx_windowed_acquire;
   _screen_vtable.release = be_gfx_windowed_release;

   _be_gfx_set_truecolor_shifts();

   /* Give time to connect the screen and do the first update */
   snooze(50000);
   while (!be_gfx_initialized);

   uszprintf(driver_desc, sizeof(driver_desc), get_config_text("BDirectWindow object, %d bit in %s"),
             _be_allegro_window->screen_depth,
             uconvert_ascii(_be_allegro_window->screen_depth == _be_allegro_window->display_depth ?
                            "matching" : "fast emulation", tmp));
   drv->desc = driver_desc;
   
   return bmp;

   cleanup: {
      be_gfx_windowed_exit(NULL);
      return NULL;
   }
}



extern "C" struct BITMAP *be_gfx_windowed_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return _be_gfx_windowed_init(&gfx_beos_windowed, w, h, v_w, v_h, color_depth, true);
}



extern "C" void be_gfx_windowed_exit(struct BITMAP *bmp)
{
   (void)bmp;

   be_gfx_initialized = false;

   if (_be_allegro_window) {
      if (_be_mouse_view_attached < 1) {
	 acquire_sem(_be_mouse_view_attached);
      }

      _be_allegro_window->Lock();
      _be_allegro_window->Quit();

      _be_allegro_window = NULL;
      _be_window = NULL;
   }
   _be_mouse_window   = NULL;	    
   _be_mouse_view     = NULL;
}



extern "C" void be_gfx_windowed_acquire(struct BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      _be_allegro_window->locker->Lock();
      bmp->id |= BMP_ID_LOCKED;
   }
}



extern "C" void be_gfx_windowed_release(struct BITMAP *bmp)
{
   if (bmp->id & BMP_ID_LOCKED) {
      bmp->id &= ~BMP_ID_LOCKED;
      _be_allegro_window->locker->Unlock();
      release_sem(_be_window_lock);
   }
}



extern "C" void be_gfx_windowed_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;

   if (!_be_allegro_window->blitter)
      return;
   
   if (vsync) {
      be_gfx_vsync();
   }
   
   _be_allegro_window->locker->Lock();
   _set_colorconv_palette(p, from, to);
   if ((_be_allegro_window->display_depth == 8) &&
       (_be_allegro_window->screen_depth == 8)) {
      for (i=from; i<=to; i++) {
         palette_colors[i].red   = _rgb_scale_6[p[i].r];
         palette_colors[i].green = _rgb_scale_6[p[i].g];
         palette_colors[i].blue  = _rgb_scale_6[p[i].b];
      }
      for (i=0; i<256; i++)
         cmap[i] = BScreen().IndexForColor(palette_colors[i]);
   }
   for (i=0; i<(int)_be_allegro_window->screen_height; i++)
      _be_dirty_lines[i] = 1;
   release_sem(_be_window_lock);

   _be_allegro_window->locker->Unlock();
}
