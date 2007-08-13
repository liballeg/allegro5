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
 *      Improved screen display API, draft.
 *
 *	By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Display routines
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_vector.h"

#include ALLEGRO_INTERNAL_HEADER


static int gfx_virgin = TRUE;          /* is the graphics system active? */

typedef struct VRAM_BITMAP             /* list of video memory bitmaps */
{
   int x, y, w, h;
   BITMAP *bmp;
   struct VRAM_BITMAP *next_x, *next_y;
} VRAM_BITMAP;

static VRAM_BITMAP *vram_bitmap_list = NULL;

typedef struct SYSTEM_BITMAP
{
   BITMAP *bmp;
} SYSTEM_BITMAP;

static _AL_VECTOR sysbmp_list = _AL_VECTOR_INITIALIZER(SYSTEM_BITMAP);

#define BMP_MAX_SIZE  46340   /* sqrt(INT_MAX) */

/* the product of these must fit in an int */
static int failed_bitmap_w = BMP_MAX_SIZE;
static int failed_bitmap_h = BMP_MAX_SIZE;


static ALLEGRO_DISPLAY *new_display;


/* destroy_video_bitmaps:
 *  Destroys all video bitmaps
 */
static void destroy_video_bitmaps(void)
{
   while (vram_bitmap_list)
      destroy_bitmap(vram_bitmap_list->bmp);
}



/* add_vram_block:
 *  Creates a video memory bitmap out of the specified region
 *  of the larger screen surface, returning a pointer to it.
 */
static BITMAP *add_vram_block(int x, int y, int w, int h)
{
   VRAM_BITMAP *b, *new_b;
   VRAM_BITMAP **last_p;

   new_b = _AL_MALLOC(sizeof(VRAM_BITMAP));
   if (!new_b)
      return NULL;

   new_b->x = x;
   new_b->y = y;
   new_b->w = w;
   new_b->h = h;

   new_b->bmp = create_sub_bitmap(screen, x, y, w, h);
   new_b->bmp->id |= BMP_ID_VIDEO;

   if (!new_b->bmp) {
      _AL_FREE(new_b);
      return NULL;
   }

   /* find sorted y-position */
   last_p = &vram_bitmap_list;
   for (b = vram_bitmap_list; b && (b->y < new_b->y); b = b->next_y)
      last_p = &b->next_y;

   /* insert */
   *last_p = new_b;
   new_b->next_y = b;

   return new_b->bmp;
}



/* Function: create_video_bitmap
 *  Attempts to make a bitmap object for accessing offscreen video memory.
 *
 *  The algorithm is similar to algorithms for drawing polygons. Think of
 *  a horizontal stripe whose height is equal to that of the new bitmap.
 *  It is initially aligned to the top of video memory and then it moves
 *  downwards, stopping each time its top coincides with the bottom of a
 *  video bitmap. For each stop, create a list of video bitmaps intersecting
 *  the stripe, sorted by the left coordinate, from left to right, in the
 *  next_x linked list. We look through this list and stop as soon as the
 *  gap between the rightmost right edge seen so far and the current left
 *  edge is big enough for the new video bitmap. In that case, we are done.
 *  If we don't find such a gap, we move the stripe further down.
 *
 *  To make it efficient to find new bitmaps intersecting the stripe, the
 *  list of all bitmaps is always sorted by top, from top to bottom, in
 *  the next_y linked list. The list of bitmaps intersecting the stripe is
 *  merely updated, not recalculated from scratch, when we move the stripe.
 *  So every bitmap gets bubbled to its correct sorted x-position at most
 *  once. Bubbling a bitmap takes at most as many steps as the number of
 *  video bitmaps. So the algorithm behaves at most quadratically with
 *  regard to the number of video bitmaps. I think that a linear behaviour
 *  is more typical in practical applications (this happens, for instance,
 *  if you allocate many bitmaps of the same height).
 *
 *  There's one more optimization: if a call fails, we cache the size of the
 *  requested bitmap, so that next time we can detect very quickly that this
 *  size is too large (this needs to be maintained when destroying bitmaps).
 */
BITMAP *create_video_bitmap(int width, int height)
{
   VRAM_BITMAP *active_list, *b, *vram_bitmap;
   VRAM_BITMAP **last_p;
   BITMAP *bmp;
   int x = 0, y = 0;

   ASSERT(gfx_driver != NULL);
   ASSERT(width >= 0);
   ASSERT(height > 0);
   
   if (_dispsw_status)
      return NULL;

   /* let the driver handle the request if it can */
   if (gfx_driver->create_video_bitmap) {
      bmp = gfx_driver->create_video_bitmap(width, height);
      if (!bmp)
	 return NULL;

      b = _AL_MALLOC(sizeof(VRAM_BITMAP));
      b->x = -1;
      b->y = -1;
      b->w = 0;
      b->h = 0;
      b->bmp = bmp;
      b->next_y = vram_bitmap_list;
      vram_bitmap_list = b;

      return bmp;
   }

   /* check bad args */
   if ((width > VIRTUAL_W) || (height > VIRTUAL_H) ||
       (width < 0) || (height < 0))
      return NULL;

   /* check cached bitmap size */
   if ((width >= failed_bitmap_w) && (height >= failed_bitmap_h))
      return NULL;

   vram_bitmap = vram_bitmap_list;
   active_list = NULL;
   y = 0;

   while (TRUE) {
      /* Move the blocks from the VRAM_BITMAP_LIST that intersect the
       * stripe to the ACTIVE_LIST: look through the VRAM_BITMAP_LIST
       * following the next_y link, grow the ACTIVE_LIST following
       * the next_x link and sorting by x-position.
       * (this loop runs in amortized quadratic time)
       */
      while (vram_bitmap && (vram_bitmap->y < y+height)) {
	 /* find sorted x-position */
	 last_p = &active_list;
	 for (b = active_list; b && (vram_bitmap->x > b->x); b = b->next_x)
	    last_p = &b->next_x;

	 /* insert */
	 *last_p = vram_bitmap;
	 vram_bitmap->next_x = b;

	 /* next video bitmap */
	 vram_bitmap = vram_bitmap->next_y;
      }

      x = 0;

      /* Look for holes to put our bitmap in.
       * (this loop runs in quadratic time)
       */
      for (b = active_list; b; b = b->next_x) {
	 if (x+width <= b->x)  /* hole is big enough? */
	    return add_vram_block(x, y, width, height);

         /* Move search x-position to the right edge of the
          * skipped bitmap if we are not already past it.
          * And check there is enough room before continuing.
          */
	 if (x < b->x + b->w) {
	    x = (b->x + b->w + 15) & ~15;
	    if (x+width > VIRTUAL_W)
	       break;
	 }
      }

      /* If the whole ACTIVE_LIST was scanned, there is a big
       * enough hole to the right of the rightmost bitmap.
       */
      if (b == NULL)
	 return add_vram_block(x, y, width, height);

      /* Move search y-position to the topmost bottom edge
       * of the bitmaps intersecting the stripe.
       * (this loop runs in quadratic time)
       */
      y = active_list->y + active_list->h;
      for (b = active_list->next_x; b; b = b->next_x) {
	 if (y > b->y + b->h)
	    y = b->y + b->h;
      }

      if (y+height > VIRTUAL_H) {  /* too close to bottom? */
	 /* Before failing, cache this bitmap size so that later calls can
	  * learn from it. Use area as a measure to sort the bitmap sizes.
          */
	 if (width * height < failed_bitmap_w * failed_bitmap_h) {
	    failed_bitmap_w = width;
	    failed_bitmap_h = height;
	 }

	 return NULL;
      }

      /* Remove the blocks that don't intersect the new stripe from ACTIVE_LIST.
       * (this loop runs in quadratic time)
       */
      last_p = &active_list;
      for (b = active_list; b; b = b->next_x) {
	 if (y >= b->y + b->h)
	    *last_p = b->next_x;
	 else
	    last_p = &b->next_x;
      }
   }
}



/* Function: create_system_bitmap
 *  Attempts to make a system-specific (eg. DirectX surface) bitmap object.
 */
BITMAP *create_system_bitmap(int width, int height)
{
   SYSTEM_BITMAP *sysbmp;
   
   ASSERT(width >= 0);
   ASSERT(height > 0);
   ASSERT(gfx_driver != NULL);


   sysbmp = _al_vector_alloc_back(&sysbmp_list);
   if (gfx_driver->create_system_bitmap) {
      sysbmp->bmp = gfx_driver->create_system_bitmap(width, height);
   } else {
      sysbmp->bmp = create_bitmap(width, height);
      sysbmp->bmp->id |= BMP_ID_SYSTEM;
   }

   return sysbmp->bmp;
}



/* destroy_bitmap:
 *  Destroys a memory bitmap.
 */
void destroy_bitmap(BITMAP *bitmap)
{
   VRAM_BITMAP *prev, *pos;

   if (bitmap) {
      if (is_video_bitmap(bitmap)) {
	 /* special case for getting rid of video memory bitmaps */
	 ASSERT(!_dispsw_status);

	 prev = NULL;
	 pos = vram_bitmap_list;

	 while (pos) {
	    if (pos->bmp == bitmap) {
	       if (prev)
		  prev->next_y = pos->next_y;
	       else
		  vram_bitmap_list = pos->next_y;

	       if (pos->x < 0) {
		  /* the driver is in charge of this object */
		  ASSERT(gfx_driver);
		  ASSERT(gfx_driver->destroy_video_bitmap);
		  gfx_driver->destroy_video_bitmap(bitmap);
		  _AL_FREE(pos);
		  return;
	       } 

	       /* Update cached bitmap size using worst case scenario:
		* the bitmap lies between two holes whose size is the cached
		* size on each axis respectively.
		*/
	       failed_bitmap_w = failed_bitmap_w * 2 + ((bitmap->w + 15) & ~15);
	       if (failed_bitmap_w > BMP_MAX_SIZE)
		  failed_bitmap_w = BMP_MAX_SIZE;

	       failed_bitmap_h = failed_bitmap_h * 2 + bitmap->h;
	       if (failed_bitmap_h > BMP_MAX_SIZE)
		  failed_bitmap_h = BMP_MAX_SIZE;

	       _AL_FREE(pos);
	       break;
	    }

	    prev = pos;
	    pos = pos->next_y;
	 }

	 _unregister_switch_bitmap(bitmap);
      }
      else if (is_system_bitmap(bitmap)) {
	 /* special case for getting rid of system memory bitmaps */
         unsigned int c;
         for (c = 0; c < _al_vector_size(&sysbmp_list); c++) {
            SYSTEM_BITMAP *sysbmp = _al_vector_ref(&sysbmp_list, c);
            if (sysbmp->bmp == bitmap) {
	       ASSERT(gfx_driver != NULL);

	       if (gfx_driver->destroy_system_bitmap) {
	          gfx_driver->destroy_system_bitmap(bitmap);
                  _al_vector_delete_at(&sysbmp_list, c);
	          return;
	       }
               
               _al_vector_delete_at(&sysbmp_list, c);
               break;
            }
         }
      }
      else if (bitmap->needs_upload && bitmap == screen) {
        if (_al_current_display) {
   	   al_destroy_display(_al_current_display);
	   al_set_current_display(NULL);
	}
      }

      /* normal memory or sub-bitmap destruction */
      if (system_driver->destroy_bitmap) {
	 if (system_driver->destroy_bitmap(bitmap))
	    return;
      }

      if (bitmap->dat)
	 _AL_FREE(bitmap->dat);

      _AL_FREE(bitmap);
   }
}





/* _get_vtable:
 *  Returns a pointer to the linear vtable for the specified color depth.
 */
GFX_VTABLE *_get_vtable(int color_depth)
{
   GFX_VTABLE *vt;
   int i;

   ASSERT(system_driver);

   if (system_driver->get_vtable) {
      vt = system_driver->get_vtable(color_depth);

      if (vt) {
	 LOCK_DATA(vt, sizeof(GFX_VTABLE));
	 LOCK_CODE(vt->draw_sprite, (long)vt->draw_sprite_end - (long)vt->draw_sprite);
	 LOCK_CODE(vt->blit_from_memory, (long)vt->blit_end - (long)vt->blit_from_memory);
	 return vt;
      }
   }

   for (i=0; _vtable_list[i].vtable; i++) {
      if (_vtable_list[i].color_depth == color_depth) {
	 LOCK_DATA(_vtable_list[i].vtable, sizeof(GFX_VTABLE));
	 LOCK_CODE(_vtable_list[i].vtable->draw_sprite, (long)_vtable_list[i].vtable->draw_sprite_end - (long)_vtable_list[i].vtable->draw_sprite);
	 LOCK_CODE(_vtable_list[i].vtable->blit_from_memory, (long)_vtable_list[i].vtable->blit_end - (long)_vtable_list[i].vtable->blit_from_memory);
	 return _vtable_list[i].vtable;
      }
   }

   return NULL;
}



/* shutdown_gfx:
 *  Used by allegro_exit() to return the system to text mode.
 */
static void shutdown_gfx(void)
{
   if (system_driver->restore_console_state)
      system_driver->restore_console_state();

   _remove_exit_func(shutdown_gfx);

   gfx_virgin = TRUE;
}


#define GFX_DRIVER_FULLSCREEN_FLAG    0x01
#define GFX_DRIVER_WINDOWED_FLAG      0x02


/* gfx_driver_is_valid:
 *  Checks that the graphics driver 'drv' fulfills the condition
 *  expressed by the bitmask 'flags'.
 */
static int gfx_driver_is_valid(GFX_DRIVER *drv, int flags)
{
   if ((flags & GFX_DRIVER_FULLSCREEN_FLAG) && drv->windowed)
      return FALSE;

   if ((flags & GFX_DRIVER_WINDOWED_FLAG) && !drv->windowed)
      return FALSE;

   return TRUE;
}



/* get_gfx_driver_from_id:
 *  Retrieves a pointer to the graphics driver identified by 'card' from
 *  the list 'driver_list' or NULL if it doesn't exist.
 */
static GFX_DRIVER *get_gfx_driver_from_id(int card, _DRIVER_INFO *driver_list)
{
   int c;

   for (c=0; driver_list[c].driver; c++) {
      if (driver_list[c].id == card)
	 return driver_list[c].driver;
   }

   return NULL;
}



/* init_gfx_driver:
 *  Helper function for initializing a graphics driver.
 */
static BITMAP *init_gfx_driver(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int depth)
{
   drv->name = drv->desc = get_config_text(drv->ascii_name);

   /* set gfx_driver so that it is visible when initializing the driver */
   gfx_driver = drv;

   return drv->init(w, h, v_w, v_h, depth);
}



/* get_config_gfx_driver:
 *  Helper function for set_gfx_mode: it reads the gfx_card* config variables and
 *  tries to set the graphics mode if a matching driver is found. Returns TRUE if
 *  at least one matching driver was found, FALSE otherwise.
 */
static int get_config_gfx_driver(char *gfx_card, int w, int h, int v_w, int v_h, int depth, int flags, int drv_flags, _DRIVER_INFO *driver_list)
{
   char buf[512], tmp[64];
   GFX_DRIVER *drv;
   int found = FALSE;
   int card, n;

   /* try the drivers that are listed in the config file */
   for (n=-2; n<255; n++) {
      switch (n) {

	 case -2:
	    /* example: gfx_card_640x480x16 = */
	    uszprintf(buf, sizeof(buf), uconvert_ascii("%s_%dx%dx%d", tmp), gfx_card, w, h, depth);
	    break;

	 case -1:
	    /* example: gfx_card_24bpp = */
	    uszprintf(buf, sizeof(buf), uconvert_ascii("%s_%dbpp", tmp), gfx_card, depth);
	    break;

	 case 0:
	    /* example: gfx_card = */
	    uszprintf(buf, sizeof(buf), uconvert_ascii("%s", tmp), gfx_card);
	    break;

	 default:
	    /* example: gfx_card1 = */
	    uszprintf(buf, sizeof(buf), uconvert_ascii("%s%d", tmp), gfx_card, n);
	    break;
      }

      card = get_config_id(uconvert_ascii("graphics", tmp), buf, 0);

      if (card) {
	 drv = get_gfx_driver_from_id(card, driver_list);

	 if (drv && gfx_driver_is_valid(drv, drv_flags)) {
	    found = TRUE;
	    screen = init_gfx_driver(drv, w, h, v_w, v_h, depth);

	    if (screen)
	       break;
	 }
      }
      else {
	 /* Stop searching the gfx_card#n (n>0) family at the first missing member,
	  * except gfx_card1 which could have been identified with gfx_card.
	  */
	 if (n > 1)
	    break;
      }
   }

   return found;
}



/* set_gfx_mode:
 *  Sets the graphics mode. The card should be one of the GFX_* constants
 *  from allegro.h, or GFX_AUTODETECT to accept any graphics driver. Pass
 *  GFX_TEXT to return to text mode (although allegro_exit() will usually 
 *  do this for you). The w and h parameters specify the screen resolution 
 *  you want, and v_w and v_h specify the minumum virtual screen size. 
 *  The graphics drivers may actually create a much larger virtual screen, 
 *  so you should check the values of VIRTUAL_W and VIRTUAL_H after you
 *  set the mode. If unable to select an appropriate mode, this function 
 *  returns -1.
 */
int set_gfx_mode(int card, int w, int h, int v_w, int v_h)
{
   static int allow_config = TRUE;
   extern void blit_end(void);
   _DRIVER_INFO *driver_list;
   GFX_DRIVER *drv;
   struct GFX_MODE mode;
   char buf[ALLEGRO_ERROR_SIZE], tmp1[64], tmp2[64];
   AL_CONST char *dv;
   int drv_flags = 0;
   int c, driver, ret;
   ASSERT(system_driver);
   int depth = get_color_depth();

   _gfx_mode_set_count++;

   /* FIXME: */
   #ifdef ALLEGRO_UNIX
   card = GFX_XGLX_WINDOWED;
   #else
   #if defined ALLEGRO_D3D
   if (card == GFX_AUTODETECT || card == GFX_AUTODETECT_FULLSCREEN) {
   	card = GFX_DIRECT3D_FULLSCREEN;
   }
   else if (card == GFX_AUTODETECT_WINDOWED) {
   	card = GFX_DIRECT3D_WINDOWED;
   }
   #endif
   #endif

   /* special bodge for the GFX_SAFE driver */
   if (card == GFX_SAFE) {
      if (system_driver->get_gfx_safe_mode) {
	 ustrzcpy(buf, sizeof(buf), allegro_error);

	 /* retrieve the safe graphics mode */
	 system_driver->get_gfx_safe_mode(&driver, &mode);

	 /* try using the specified depth and resolution */
	 if (set_gfx_mode(driver, w, h, v_w, v_h) == 0)
	    return 0;

	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, buf);

	 /* finally use the safe settings */
	 set_color_depth(mode.bpp);
	 if (set_gfx_mode(driver, mode.width, mode.height, v_w, v_h) == 0)
	    return 0;

	 ASSERT(FALSE);  /* the safe graphics mode must always work */
      }
      else {
	 /* no safe graphics mode, try hard-coded autodetected modes with custom settings */
	 allow_config = FALSE;
	 _safe_gfx_mode_change = 1;

	 ret = set_gfx_mode(GFX_AUTODETECT, w, h, v_w, v_h);

	 _safe_gfx_mode_change = 0;
	 allow_config = TRUE;

	 if (ret == 0)
	    return 0;
      }

      /* failing to set GFX_SAFE is a fatal error */
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message(uconvert_ascii("%s\n", tmp1), get_config_text("Fatal error: unable to set GFX_SAFE"));
      return -1;
   }

   /* remember the current console state */
   if (gfx_virgin) {
      LOCK_FUNCTION(_stub_bank_switch);
      LOCK_FUNCTION(blit);

      if (system_driver->save_console_state)
	 system_driver->save_console_state();

      _add_exit_func(shutdown_gfx, "shutdown_gfx");

      gfx_virgin = FALSE;
   }

   /* lock the application in the foreground */
   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(TRUE, TRUE);

   timer_simulate_retrace(FALSE);
   _screen_split_position = 0;

   /* close down any existing graphics driver */
   if (gfx_driver) {
      if (_al_linker_mouse)
         _al_linker_mouse->show_mouse(NULL);

      destroy_video_bitmaps();

      bmp_read_line(screen, 0);
      bmp_write_line(screen, 0);
      bmp_unwrite_line(screen);

      if (gfx_driver->scroll)
	 gfx_driver->scroll(0, 0);

      if (gfx_driver->exit)
	 gfx_driver->exit(screen);

      destroy_bitmap(screen);

      gfx_driver = NULL;
      screen = NULL;
      gfx_capabilities = 0;
   }

   _set_current_refresh_rate(0);

   /* return to text mode? */
   if (card == GFX_TEXT) {
      if (system_driver->restore_console_state)
	 system_driver->restore_console_state();

      if (_gfx_bank) {
	 _AL_FREE(_gfx_bank);
	 _gfx_bank = NULL;
      }

      if (system_driver->display_switch_lock)
	 system_driver->display_switch_lock(FALSE, FALSE);

      return 0;
   }

   /* now to the interesting part: let's try to find a graphics driver */
   usetc(allegro_error, 0);

   /* ask the system driver for a list of graphics hardware drivers */
   if (system_driver->gfx_drivers)
      driver_list = system_driver->gfx_drivers();
   else
      driver_list = _gfx_driver_list;

   /* filter specific fullscreen/windowed driver requests */
   if (card == GFX_AUTODETECT_FULLSCREEN) {
      drv_flags |= GFX_DRIVER_FULLSCREEN_FLAG;
      card = GFX_AUTODETECT;
   }
   else if (card == GFX_AUTODETECT_WINDOWED) {
      drv_flags |= GFX_DRIVER_WINDOWED_FLAG;
      card = GFX_AUTODETECT;
   }

   #ifdef ALLEGRO_UNIX
   if (card == GFX_XGLX_FULLSCREEN || card == GFX_XGLX_WINDOWED) {
      int windowed_flag = (card == GFX_XGLX_WINDOWED) ? ALLEGRO_WINDOWED : 0;
      al_init();
      screen = create_bitmap(w, h);
      al_set_new_display_flags(ALLEGRO_SINGLEBUFFER | windowed_flag);
      screen->display = al_create_display(w, h);
      if (!screen->display) {
         destroy_bitmap(screen);
	 return 1;
      }
      screen->needs_upload = true;
      screen->display->vt->upload_compat_screen(screen, 0, 0, w, h);
      gfx_driver = &_al_xglx_gfx_driver;
      gfx_driver->w = w;
      gfx_driver->h = h;
      gfx_driver->windowed = true;
      return 0;
   }
   else
   #else
   #if defined ALLEGRO_D3D
   if (card == GFX_DIRECT3D || card == GFX_DIRECT3D_FULLSCREEN) {
   	int windowed_flag = (card == GFX_DIRECT3D_WINDOWED) ? ALLEGRO_WINDOWED : 0;
	al_init();
	if (v_w != 0 || v_h != 0) {
		return -1;
	}
	screen = create_bitmap(w, h);
	if (!screen) {
		return 1;
	}
	al_set_new_display_format(ALLEGRO_PIXEL_FORMAT_RGB_565);
	al_set_new_display_flags(ALLEGRO_SINGLEBUFFER|windowed_flag);
	screen->display = al_create_display(w, h);
	if (!screen->display) {
		destroy_bitmap(screen);
		return 1;
	}
	al_set_current_display(screen->display);
	al_set_target_bitmap(al_get_backbuffer());
	screen->needs_upload = true;
	screen->display->vt->upload_compat_screen(screen, 0, 0, w, h);

	gfx_driver = &_al_d3d_dummy_gfx_driver;
	gfx_driver->w = w;
	gfx_driver->h = h;
	gfx_driver->windowed = true;

	for (c=0; c<256; c++)
	  _palette_color8[c] = c;
        set_palette(default_palette);
	if (_al_linker_mouse)
		_al_linker_mouse->set_mouse_etc();
	return 0;
   }
   else
   #endif
   #endif
   if (card == GFX_AUTODETECT) {
      /* autodetect the driver */
      int found = FALSE;

      /* first try the config variables */
      if (allow_config) {
	 /* try the gfx_card variable if GFX_AUTODETECT or GFX_AUTODETECT_FULLSCREEN was selected */
	 if (!(drv_flags & GFX_DRIVER_WINDOWED_FLAG))
	    found = get_config_gfx_driver(uconvert_ascii("gfx_card", tmp1), w, h, v_w, v_h, depth, 0, drv_flags, driver_list);

	 /* try the gfx_cardw variable if GFX_AUTODETECT or GFX_AUTODETECT_WINDOWED was selected */
	 if (!(drv_flags & GFX_DRIVER_FULLSCREEN_FLAG) && !found)
	    found = get_config_gfx_driver(uconvert_ascii("gfx_cardw", tmp1), w, h, v_w, v_h, depth, 0, drv_flags, driver_list);
      }

      /* go through the list of autodetected drivers if none was previously found */
      if (!found) {
         /* Adjust virtual width and virtual height if needed and allowed */
      
	 for (c=0; driver_list[c].driver; c++) {
	    if (driver_list[c].autodetect) {
	       drv = driver_list[c].driver;

	       if (gfx_driver_is_valid(drv, drv_flags)) {
		  screen = init_gfx_driver(drv, w, h, v_w, v_h, depth);

		  if (screen)
		     break;
	       }
	    }
	 }
      }
   }
   else {
      /* search the list for the requested driver */
      drv = get_gfx_driver_from_id(card, driver_list);

      if (drv)
	 screen = init_gfx_driver(drv, w, h, v_w, v_h, depth);
   }

   /* gracefully handle failure */
   if (!screen) {
      gfx_driver = NULL;  /* set by init_gfx_driver() */

      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to find a suitable graphics driver"));

      if (system_driver->display_switch_lock)
	 system_driver->display_switch_lock(FALSE, FALSE);

      return -1;
   }
   
   /* set the basic capabilities of the driver */

   if ((VIRTUAL_W > SCREEN_W) || (VIRTUAL_H > SCREEN_H)) {
      if (gfx_driver->scroll)
	 gfx_capabilities |= GFX_CAN_SCROLL;

      if ((gfx_driver->request_scroll) || (gfx_driver->request_video_bitmap))
	 gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }
   
   /* check whether we are instructed to disable vsync */
   dv = get_config_string(uconvert_ascii("graphics", tmp1),
                          uconvert_ascii("disable_vsync", tmp2),
                          NULL);

   if ((dv) && ((c = ugetc(dv)) != 0) && ((c == 'y') || (c == 'Y') || (c == '1')))
      _wait_for_vsync = FALSE;
   else
      _wait_for_vsync = TRUE;

   clear_bitmap(screen);

   /* set up the default colors */
   for (c=0; c<256; c++)
      _palette_color8[c] = c;

   set_palette(default_palette);

   if (_color_depth == 8) {
      gui_fg_color = 255;
      gui_mg_color = 8;
      gui_bg_color = 0;
   }
   else {
      gui_fg_color = makecol(0, 0, 0);
      gui_mg_color = makecol(128, 128, 128);
      gui_bg_color = makecol(255, 255, 255);
   }

   if (_al_linker_mouse)
      _al_linker_mouse->set_mouse_etc();

   LOCK_DATA(gfx_driver, sizeof(GFX_DRIVER));

   _register_switch_bitmap(screen, NULL);

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(FALSE, FALSE);

   return 0;
}



/* _sort_out_virtual_width:
 *  Decides how wide the virtual screen really needs to be. That is more 
 *  complicated than it sounds, because the Allegro graphics primitives 
 *  require that each scanline be contained within a single bank. That 
 *  causes problems on cards that don't have overlapping banks, unless the 
 *  bank size is a multiple of the virtual width. So we may need to adjust 
 *  the width just to keep things running smoothly...
 */
void _sort_out_virtual_width(int *width, GFX_DRIVER *driver)
{
   int w = *width;

   /* hah! I love VBE 2.0... */
   if (driver->linear)
      return;

   /* if banks can overlap, we are ok... */ 
   if (driver->bank_size > driver->bank_gran)
      return;

   /* damn, have to increase the virtual width */
   while (((driver->bank_size / w) * w) != driver->bank_size) {
      w++;
      if (w > driver->bank_size)
	 break; /* oh shit */
   }

   *width = w;
}



/* _make_bitmap:
 *  Helper function for creating the screen bitmap. Sets up a bitmap 
 *  structure for addressing video memory at addr, and fills the bank 
 *  switching table using bank size/granularity information from the 
 *  specified graphics driver.
 */
BITMAP *_make_bitmap(int w, int h, uintptr_t addr, GFX_DRIVER *driver, int color_depth, int bpl)
{
   GFX_VTABLE *vtable = _get_vtable(color_depth);
   int i, bank, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *)_AL_MALLOC(size);
   if (!b)
      return NULL;

   _gfx_bank = _AL_REALLOC(_gfx_bank, h * sizeof(int));
   if (!_gfx_bank) {
      _AL_FREE(b);
      return NULL;
   }

   LOCK_DATA(b, size);
   LOCK_DATA(_gfx_bank, h * sizeof(int));

   b->w = b->cr = w;
   b->h = b->cb = h;
   b->clip = TRUE;
   b->cl = b->ct = 0;
   b->vtable = &_screen_vtable;
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->id = BMP_ID_VIDEO;
   b->extra = NULL;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _video_ds();
   b->needs_upload = false;
   b->dirty_x1 = b->dirty_x2 = b->dirty_y1 = b->dirty_y2 = -1;

   memcpy(&_screen_vtable, vtable, sizeof(GFX_VTABLE));
   LOCK_DATA(&_screen_vtable, sizeof(GFX_VTABLE));

   _last_bank_1 = _last_bank_2 = -1;

   driver->vid_phys_base = addr;

   b->line[0] = (unsigned char *)addr;
   _gfx_bank[0] = 0;

   if (driver->linear) {
      for (i=1; i<h; i++) {
	 b->line[i] = b->line[i-1] + bpl;
	 _gfx_bank[i] = 0;
      }
   }
   else {
      bank = 0;

      for (i=1; i<h; i++) {
	 b->line[i] = b->line[i-1] + bpl;
	 if (b->line[i]+bpl-1 >= (unsigned char *)addr + driver->bank_size) {
	    while (b->line[i] >= (unsigned char *)addr + driver->bank_gran) {
	       b->line[i] -= driver->bank_gran;
	       bank++;
	    }
	 }
	 _gfx_bank[i] = bank;
      }
   }

   return b;
}



/* Function: scroll_screen
 *  Attempts to scroll the hardware screen, returning 0 on success. 
 *  Check the VIRTUAL_W and VIRTUAL_H values to see how far the screen
 *  can be scrolled. Note that a lot of VESA drivers can only handle
 *  horizontal scrolling in four pixel increments.
 */
int scroll_screen(int x, int y)
{
   int ret = 0;
   int h;

   /* can driver handle hardware scrolling? */
   if ((!gfx_driver->scroll) || (_dispsw_status))
      return -1;

   /* clip x */
   if (x < 0) {
      x = 0;
      ret = -1;
   }
   else if (x > (VIRTUAL_W - SCREEN_W)) {
      x = VIRTUAL_W - SCREEN_W;
      ret = -1;
   }

   /* clip y */
   if (y < 0) {
      y = 0;
      ret = -1;
   }
   else {
      h = (_screen_split_position > 0) ? _screen_split_position : SCREEN_H;
      if (y > (VIRTUAL_H - h)) {
	 y = VIRTUAL_H - h;
	 ret = -1;
      }
   }

   /* scroll! */
   if (gfx_driver->scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* Function: request_scroll
 *  Attempts to initiate a triple buffered hardware scroll, which will
 *  take place during the next retrace. Returns 0 on success.
 */
int request_scroll(int x, int y)
{
   int ret = 0;
   int h;

   /* can driver handle triple buffering? */
   if ((!gfx_driver->request_scroll) || (_dispsw_status)) {
      scroll_screen(x, y);
      return -1;
   }

   /* clip x */
   if (x < 0) {
      x = 0;
      ret = -1;
   }
   else if (x > (VIRTUAL_W - SCREEN_W)) {
      x = VIRTUAL_W - SCREEN_W;
      ret = -1;
   }

   /* clip y */
   if (y < 0) {
      y = 0;
      ret = -1;
   }
   else {
      h = (_screen_split_position > 0) ? _screen_split_position : SCREEN_H;
      if (y > (VIRTUAL_H - h)) {
	 y = VIRTUAL_H - h;
	 ret = -1;
      }
   }

   /* scroll! */
   if (gfx_driver->request_scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* Function: poll_scroll
 *  Checks whether a requested triple buffer flip has actually taken place.
 */
int poll_scroll()
{
   if ((!gfx_driver->poll_scroll) || (_dispsw_status))
      return FALSE;

   return gfx_driver->poll_scroll();
}



/* Function: show_video_bitmap
 *  Page flipping function: swaps to display the specified video memory 
 *  bitmap object (this must be the same size as the physical screen).
 */
int show_video_bitmap(BITMAP *bitmap)
{
   ASSERT(bitmap);

   if ((!is_video_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   if (gfx_driver->show_video_bitmap)
      return gfx_driver->show_video_bitmap(bitmap);

   return scroll_screen(bitmap->x_ofs, bitmap->y_ofs);
}



/* Function: request_video_bitmap
 *  Triple buffering function: triggers a swap to display the specified 
 *  video memory bitmap object, which will take place on the next retrace.
 */
int request_video_bitmap(BITMAP *bitmap)
{
   ASSERT(bitmap);

   if ((!is_video_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   if (gfx_driver->request_video_bitmap)
      return gfx_driver->request_video_bitmap(bitmap);

   return request_scroll(bitmap->x_ofs, bitmap->y_ofs);
}



/* Function: enable_triple_buffer
 *  Asks a driver to turn on triple buffering mode, if it is capable
 *  of that.
 */
int enable_triple_buffer()
{
   if (gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)
      return 0;

   if (_dispsw_status)
      return -1;

   if (gfx_driver->enable_triple_buffer) {
      gfx_driver->enable_triple_buffer();

      if ((gfx_driver->request_scroll) || (gfx_driver->request_video_bitmap)) {
	 gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
	 return 0;
      }
   }

   return -1;
}

