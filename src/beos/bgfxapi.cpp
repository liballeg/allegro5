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
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/aintern.h"
#include "allegro/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif

#define LINE8_HOOK_NUM    3
#define LINE16_HOOK_NUM  12
#define LINE32_HOOK_NUM   4
#define ARRAY8_HOOK_NUM   8
#define ARRAY32_HOOK_NUM  9
#define RECT8_HOOK_NUM    5
#define RECT16_HOOK_NUM  16
#define RECT32_HOOK_NUM   6
#define INVERT_HOOK_NUM  11
#define BLIT_HOOK_NUM     7
#define SYNC_HOOK_NUM    10



typedef int32 (*LINE8_HOOK)(int32 startX, int32 endX, int32 startY,
   int32 endY, uint8 colorIndex, bool clipToRect, int16 clipLeft,
   int16 clipTop, int16 clipRight, int16 clipBottom);

typedef int32 (*ARRAY8_HOOK)(indexed_color_line *array,
   int32 numItems, bool clipToRect, int16 top, int16 right,
   int16 bottom, int16 left);

typedef int32 (*LINE16_HOOK)(int32 startX, int32 endX, int32 startY,
   int32 endY, uint16 color, bool clipToRect, int16 clipLeft,
   int16 clipTop, int16 clipRight, int16 clipBottom);

typedef int32 (*LINE32_HOOK)(int32 startX, int32 endX, int32 startY,
   int32 endY, uint32 color, bool clipToRect, int16 clipLeft,
   int16 clipTop, int16 clipRight, int16 clipBottom);

typedef int32 (*ARRAY32_HOOK)(rgb_color_line *array,
   int32 numItems, bool clipToRect, int16 top, int16 right,
   int16 bottom, int16 left);

typedef int32 (*RECT8_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom, uint8 colorIndex);

typedef int32 (*RECT16_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom, uint16 color);

typedef int32 (*RECT32_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom, uint32 color);

typedef int32 (*INVERT_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom);

typedef int32 (*BLIT_HOOK)(int32 sourceX, int32 sourceY,
   int32 destinationX, int32 destinationY, int32 width,
   int32 height);

typedef int32 (*SYNC_HOOK)(void);



static struct {
   LINE8_HOOK   draw_line_with_8_bit_depth;
   LINE16_HOOK  draw_line_with_16_bit_depth;
   LINE32_HOOK  draw_line_with_32_bit_depth;
   ARRAY8_HOOK  draw_array_with_8_bit_depth;
   ARRAY32_HOOK draw_array_with_32_bit_depth;
   RECT8_HOOK   draw_rect_with_8_bit_depth;
   RECT16_HOOK  draw_rect_with_16_bit_depth;
   RECT32_HOOK  draw_rect_with_32_bit_depth;
   INVERT_HOOK  invert_rect;
   BLIT_HOOK    blit;
   SYNC_HOOK    sync;
} hooks;



static const struct {
   int    d, w, h;
   uint32 mode;
} mode_table[] = {
   { 8,    640,  480, B_8_BIT_640x480    },
   { 8,    800,  600, B_8_BIT_800x600    },
   { 8,   1024,  768, B_8_BIT_1024x768   },
   { 8,   1152,  900, B_8_BIT_1152x900   },
   { 8,   1280, 1024, B_8_BIT_1280x1024  },
   { 8,   1600, 1200, B_8_BIT_1600x1200  },
   { 15,   640,  480, B_15_BIT_640x480   },
   { 15,   800,  600, B_15_BIT_800x600   },
   { 15,  1024,  768, B_15_BIT_1024x768  },
   { 15,  1152,  900, B_15_BIT_1152x900  },
   { 15,  1280, 1024, B_15_BIT_1280x1024 },
   { 15,  1600, 1200, B_15_BIT_1600x1200 },
   { 16,   640,  480, B_16_BIT_640x480   },
   { 16,   800,  600, B_16_BIT_800x600   },
   { 16,  1024,  768, B_16_BIT_1024x768  },
   { 16,  1152,  900, B_16_BIT_1152x900  },
   { 16,  1280, 1024, B_16_BIT_1280x1024 },
   { 16,  1600, 1200, B_16_BIT_1600x1200 },
   { 32,   640,  480, B_32_BIT_640x480   },
   { 32,   800,  600, B_32_BIT_800x600   },
   { 32,  1024,  768, B_32_BIT_1024x768  },
   { 32,  1152,  900, B_32_BIT_1152x900  },
   { 32,  1280, 1024, B_32_BIT_1280x1024 },
   { 32,  1600, 1200, B_32_BIT_1600x1200 },
   { -1 }
};

static volatile bool be_gfx_initialized;

sem_id fullscreen_lock = -1;
int32  lock_count      = 0;

int refresh_rate = 70;

BeAllegroWindow *be_allegro_window = NULL;
BeAllegroView   *be_allegro_view   = NULL;
BeAllegroScreen *be_allegro_screen = NULL; 



static inline void change_focus(bool active)
{
   if (focus_count < 0) {
      focus_count = 0;
   }

   if (active) {
      focus_count++;
   }
   else {
      focus_count--;
   }
}



/* BeAllegroWindow::BeAllegroWindow:
 */
BeAllegroWindow::BeAllegroWindow(BRect frame, const char *title,
   window_look look, window_feel feel, uint32 flags, uint32 workspaces)
   : BDirectWindow(frame, title, look, feel, flags, workspaces)
{
   BRect rect = Bounds();

   be_allegro_view = new BeAllegroView(rect, "Allegro", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
   rgb_color color = {0, 0, 0, 0};
   be_allegro_view->SetViewColor(color);
   AddChild(be_allegro_view);
}



/* BeAllegroWindow::~BeAllegroWindow:
 */
BeAllegroWindow::~BeAllegroWindow()
{
}



/* BeAllegroWindow::MessageReceived:
 */
void BeAllegroWindow::MessageReceived(BMessage *message)
{
   switch (message->what) {
      case B_SIMPLE_DATA:
         break;

      default:
         BDirectWindow::MessageReceived(message);
         break;
      }
}



/* BeAllegroWindow::DirectConnected:
 */
void BeAllegroWindow::DirectConnected(direct_buffer_info *)
{
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
      release_sem(fullscreen_lock);
   }
   else {
      acquire_sem(fullscreen_lock);
      change_focus(connected);
   }
}



/* BeAllegroScreen::QuitRequested:
 */
bool BeAllegroScreen::QuitRequested(void)
{
   return false;
}



static inline uint32 find_gfx_mode(int w, int h, int d)
{
   int index = 0;
   while (mode_table[index].d > 0) {
      if(mode_table[index].w == w) {
         if(mode_table[index].h == h) {
            if(mode_table[index].d == d) {
               return mode_table[index].mode;
            }
         }
      }

   index++;
   }

   return (uint32)-1;
}



static inline void _be_gfx_fullscreen_accelerate(void)
{
   hooks.draw_line_with_8_bit_depth   = (LINE8_HOOK)  be_allegro_screen->CardHookAt(LINE8_HOOK_NUM);
   hooks.draw_line_with_16_bit_depth  = (LINE16_HOOK) be_allegro_screen->CardHookAt(LINE16_HOOK_NUM);
   hooks.draw_line_with_32_bit_depth  = (LINE32_HOOK) be_allegro_screen->CardHookAt(LINE32_HOOK_NUM);
   hooks.draw_array_with_8_bit_depth  = (ARRAY8_HOOK) be_allegro_screen->CardHookAt(ARRAY8_HOOK_NUM);
   hooks.draw_array_with_32_bit_depth = (ARRAY32_HOOK)be_allegro_screen->CardHookAt(ARRAY32_HOOK_NUM);
   hooks.draw_rect_with_8_bit_depth   = (RECT8_HOOK)  be_allegro_screen->CardHookAt(RECT8_HOOK_NUM);
   hooks.draw_rect_with_16_bit_depth  = (RECT16_HOOK) be_allegro_screen->CardHookAt(RECT16_HOOK_NUM);
   hooks.draw_rect_with_32_bit_depth  = (RECT32_HOOK) be_allegro_screen->CardHookAt(RECT32_HOOK_NUM);
   hooks.invert_rect                  = (INVERT_HOOK) be_allegro_screen->CardHookAt(INVERT_HOOK_NUM);
   hooks.blit                         = (BLIT_HOOK)   be_allegro_screen->CardHookAt(BLIT_HOOK_NUM);
   hooks.sync                         = (SYNC_HOOK)   be_allegro_screen->CardHookAt(SYNC_HOOK_NUM);
}



static inline bool be_sort_out_virtual_resolution(int w, int h, int *v_w, int *v_h, int bpl, int bpp)
{
    int32 try_v_w;
    int32 try_v_h;

    if (*v_w == 0) {
       try_v_w = bpl / bpp;
    }
    else {
       try_v_w = *v_w;
    }

    try_v_w = MAX(w, try_v_w);

    if (*v_h == 0) {
        for (int mb = 32; mb >= 0; mb--) {
           try_v_h = (mb*1024*1024) / (try_v_w * bpp);

           if (be_allegro_screen->SetFrameBuffer(try_v_w, try_v_h) == B_OK) {
              break;
           }
        }
    }
    else {
        try_v_h = *v_h;
    }

    try_v_h = MAX(h, try_v_h);

    if (be_allegro_screen->SetFrameBuffer(try_v_w, try_v_h) == B_OK) {
       *v_w = try_v_w;
       *v_h = try_v_h;

       return true;
    }
    else {
       return false;
    }
}



static struct BITMAP *_be_gfx_fullscreen_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, bool accel)
{
   BITMAP             *bmp;
   status_t            error;
   uint32              mode;
   graphics_card_info *gfx_card;
   frame_buffer_info  *fbuffer;
   int bpp;
   int bpl;
   char  path[MAXPATHLEN];
   char *exe;

   mode = find_gfx_mode(w, h, color_depth);

   if (mode == (uint32)-1) {
      goto cleanup;
   }

   fullscreen_lock = create_sem(0, "screen lock");

   if (fullscreen_lock < 0) {
      goto cleanup;
   }

   lock_count = 0;

   _be_sys_get_executable_name(path, sizeof(path));
   path[sizeof(path)-1] = '\0';
   exe = get_filename(path);
   be_allegro_screen = new BeAllegroScreen(exe, mode, &error, false);

   if(error != B_OK) {
      goto cleanup;
   }

   be_mouse_view = new BView(be_allegro_screen->Bounds(),
     "allegro mouse view", B_FOLLOW_ALL_SIDES, 0);
   be_allegro_screen->Lock();
   be_allegro_screen->AddChild(be_mouse_view);
   be_allegro_screen->Unlock();

   be_mouse_window = be_allegro_screen;

   release_sem(be_mouse_view_attached);

   be_gfx_initialized = false;
   be_allegro_screen->Show();
   acquire_sem(fullscreen_lock);

   if (be_allegro_screen->SetSpace(mode) != B_OK) {
      goto cleanup;
   }

   gfx_card = be_allegro_screen->CardInfo();

   bpp = ((color_depth + 7) / 8);
   bpl = gfx_card->bytes_per_row;

   if (be_allegro_screen->CanControlFrameBuffer()) {
      if (!be_sort_out_virtual_resolution(w, h, &v_w, &v_h, bpl, bpp)) {
         goto cleanup;
      }
   }
   else if (((w != 0) && (v_w != w)) || ((h != 0) && (v_h != h))) {
      goto cleanup;
   }

   fbuffer  = be_allegro_screen->FrameBufferInfo();

   drv->w       = w;
   drv->h       = h;
   drv->linear  = TRUE;
   drv->vid_mem = bpp * v_w * v_h;

   bmp = _make_bitmap(fbuffer->width, fbuffer->height,
            (unsigned long)gfx_card->frame_buffer, drv,
            color_depth, fbuffer->bytes_per_row);

   if(bmp == NULL) {
      goto cleanup;
   }

   bmp->write_bank = _be_gfx_fullscreen_read_write_bank;
   bmp->read_bank  = _be_gfx_fullscreen_read_write_bank;

   _screen_vtable.unwrite_bank = _be_gfx_fullscreen_unwrite_bank;
   _screen_vtable.acquire      = be_gfx_fullscreen_acquire;
   _screen_vtable.release      = be_gfx_fullscreen_release;

   if (accel) {
      _be_gfx_fullscreen_accelerate();
   }

   be_gfx_initialized = true;

   release_sem(fullscreen_lock);

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
   (void)bmp;

   if (fullscreen_lock > 0) {
      release_sem(fullscreen_lock);
   }


   be_mouse_view = NULL;
   be_mouse_window = NULL;

   if (be_allegro_screen != NULL) {
      acquire_sem(be_mouse_view_attached);

      be_allegro_screen->Lock();
      be_allegro_screen->Quit();

      be_allegro_screen = NULL;
      be_mouse_window   = NULL;
      be_mouse_view     = NULL;
   }

   if (fullscreen_lock > 0) {
      delete_sem(fullscreen_lock);
      fullscreen_lock = -1;
   }

   be_gfx_initialized = false;

   lock_count = 0;
}



extern "C" void be_gfx_fullscreen_acquire(struct BITMAP *bmp)
{
   if (lock_count == 0) {
      acquire_sem(fullscreen_lock);
      bmp->id |= BMP_ID_LOCKED;
   }

   lock_count++;
}



extern "C" void be_gfx_fullscreen_release(struct BITMAP *bmp)
{
   lock_count--;

   if (lock_count == 0) {
      bmp->id &= ~BMP_ID_LOCKED;
      release_sem(fullscreen_lock);
   }
}



extern "C" void be_gfx_fullscreen_set_palette(struct RGB *p, int from, int to, int vsync)
{
   rgb_color colors[256];

   (void)vsync;

   for(int index = from; index <= to; index++) {
      colors[index].red   = _rgb_scale_6[p[index].r];
      colors[index].green = _rgb_scale_6[p[index].g];
      colors[index].blue  = _rgb_scale_6[p[index].b];
      colors[index].alpha = 255;
   }

   acquire_screen();

   be_allegro_screen->SetColorList(colors, from, to);

   release_screen();
}



extern "C" int be_gfx_fullscreen_scroll(int x, int y)
{
   int rv;

   acquire_screen();
   
   if (be_allegro_screen->MoveDisplayArea(x, y) != B_ERROR) {
      rv = 0;
   }
   else {
      rv = 1;
   }

   release_screen();

   be_gfx_fullscreen_vsync();

   return rv;
}



extern "C" void be_gfx_fullscreen_vsync(void)
{
   if(BScreen(be_allegro_screen).WaitForRetrace() != B_OK) {
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
