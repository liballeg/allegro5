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

#include "allegro/display.h"

#include ALLEGRO_INTERNAL_HEADER



/* Variable: al_main_display
 *  This variable points to the first display created with al_create_display().
 *  This is a convenience for simple programs.  Programs using multiple Allegro
 *  windows should use their own, more descriptive, variables to avoid
 *  confusion.
 */
//AL_DISPLAY *al_main_display = NULL;



static _AL_VECTOR display_list = _AL_VECTOR_INITIALIZER(AL_DISPLAY *);

static int gfx_virgin = TRUE;          /* is the graphics system active? */

typedef struct VRAM_BITMAP             /* list of video memory bitmaps */
{
   int x, y, w, h;
   BITMAP *bmp;
   struct VRAM_BITMAP *next_x, *next_y;
   //AL_DISPLAY *root_display;
} VRAM_BITMAP;

static VRAM_BITMAP *vram_bitmap_list = NULL;

typedef struct SYSTEM_BITMAP
{
   BITMAP *bmp;
   //AL_DISPLAY *root_display;
} SYSTEM_BITMAP;

static _AL_VECTOR sysbmp_list = _AL_VECTOR_INITIALIZER(SYSTEM_BITMAP);

#define BMP_MAX_SIZE  46340   /* sqrt(INT_MAX) */

/* the product of these must fit in an int */
static int failed_bitmap_w = BMP_MAX_SIZE;
static int failed_bitmap_h = BMP_MAX_SIZE;


static AL_DISPLAY *new_display;
static AL_BITMAP *new_screen;


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
//static BITMAP *add_vram_block(AL_DISPLAY *display, int x, int y, int w, int h)
static BITMAP *add_vram_block(int x, int y, int w, int h)
{
   VRAM_BITMAP *b, *new_b;
   VRAM_BITMAP **last_p;

   //ASSERT(display);

   new_b = _AL_MALLOC(sizeof(VRAM_BITMAP));
   if (!new_b)
      return NULL;

   new_b->x = x;
   new_b->y = y;
   new_b->w = w;
   new_b->h = h;
   //new_b->root_display = display;

   //new_b->bmp = create_sub_bitmap(display->screen, x, y, w, h);
   new_b->bmp = create_sub_bitmap(screen, x, y, w, h);

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



/* Function: al_create_video_bitmap
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
//BITMAP *al_create_video_bitmap(AL_DISPLAY *display, int width, int height)
BITMAP *al_create_video_bitmap(int width, int height)
{
   VRAM_BITMAP *active_list, *b, *vram_bitmap;
   VRAM_BITMAP **last_p;
   BITMAP *bmp;
   int x = 0, y = 0;

   //ASSERT(display != NULL);
   //ASSERT(display->gfx_driver != NULL);
   ASSERT(gfx_driver != NULL);
   ASSERT(width >= 0);
   ASSERT(height > 0);
   
   if (_dispsw_status)
      return NULL;

   /* let the driver handle the request if it can */
   //if (display->gfx_driver->create_video_bitmap) {
   if (gfx_driver->create_video_bitmap) {
      //bmp = display->gfx_driver->create_video_bitmap(width, height);
      bmp = gfx_driver->create_video_bitmap(width, height);
      if (!bmp)
	 return NULL;

      b = _AL_MALLOC(sizeof(VRAM_BITMAP));
      b->x = -1;
      b->y = -1;
      b->w = 0;
      b->h = 0;
      b->bmp = bmp;
      //b->root_display = display;
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
	    //return add_vram_block(display, x, y, width, height);
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
	 //return add_vram_block(display, x, y, width, height);
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



/* Function: al_create_system_bitmap
 *  Attempts to make a system-specific (eg. DirectX surface) bitmap object.
 */
//BITMAP *al_create_system_bitmap(AL_DISPLAY *display, int width, int height)
BITMAP *al_create_system_bitmap(int width, int height)
{
   SYSTEM_BITMAP *sysbmp;
   
   ASSERT(width >= 0);
   ASSERT(height > 0);
   //ASSERT(display);
   //ASSERT(display->gfx_driver != NULL);
   ASSERT(gfx_driver != NULL);


   sysbmp = _al_vector_alloc_back(&sysbmp_list);
   //if (display->gfx_driver->create_system_bitmap) {
   if (gfx_driver->create_system_bitmap) {
      //sysbmp->bmp = display->gfx_driver->create_system_bitmap(width, height);
      sysbmp->bmp = gfx_driver->create_system_bitmap(width, height);
   } else {
      sysbmp->bmp = create_bitmap(width, height);
      sysbmp->bmp->id |= BMP_ID_SYSTEM;
   }

   //sysbmp->root_display = display;

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
		  //ASSERT(pos->root_display);
		  //ASSERT(pos->root_display->gfx_driver);
		  ASSERT(gfx_driver);
		  //ASSERT(pos->root_display->gfx_driver->destroy_video_bitmap);
		  ASSERT(gfx_driver->destroy_video_bitmap);
		  //pos->root_display->gfx_driver->destroy_video_bitmap(bitmap);
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
	       //ASSERT(sysbmp->root_display != NULL);
	       //ASSERT(sysbmp->root_display->gfx_driver != NULL);
	       ASSERT(gfx_driver != NULL);

	       //if (sysbmp->root_display->gfx_driver->destroy_system_bitmap) {
	       if (gfx_driver->destroy_system_bitmap) {
	          //sysbmp->root_display->gfx_driver->destroy_system_bitmap(bitmap);
	          gfx_driver->destroy_system_bitmap(bitmap);
                  _al_vector_delete_at(&sysbmp_list, c);
	          return;
	       }
               
               _al_vector_delete_at(&sysbmp_list, c);
               break;
            }
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
   /* Destroy all remaining displays */
   //while (!_al_vector_is_empty(&display_list)) {
     // AL_DISPLAY **display = _al_vector_ref_back(&display_list);
      
     // al_destroy_display(*display);
  // }
  // _al_vector_free(&display_list);
   
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
//static BITMAP *init_gfx_driver(AL_DISPLAY *display, GFX_DRIVER *drv, int w, int h, int depth, int flags)
static BITMAP *init_gfx_driver(GFX_DRIVER *drv, int w, int h, int depth, int flags)
{
   int v_w = 0; 
   int v_h = 0;
   drv->name = drv->desc = get_config_text(drv->ascii_name);

   /* set gfx_driver so that it is visible when initializing the driver */
   //display->gfx_driver = drv;
   gfx_driver = drv;

   #ifdef ALLEGRO_VRAM_SINGLE_SURFACE
   if (flags&(AL_UPDATE_TRIPLE_BUFFER|AL_UPDATE_PAGE_FLIP)) {
      v_w = w;
      v_h = h * (flags&AL_UPDATE_TRIPLE_BUFFER?3:2);
   }
   #endif

   return drv->init(w, h, v_w, v_h, depth);
}



/* get_config_gfx_driver:
 *  Helper function for set_gfx_mode: it reads the gfx_card* config variables and
 *  tries to set the graphics mode if a matching driver is found. Returns TRUE if
 *  at least one matching driver was found, FALSE otherwise.
 */
//static int get_config_gfx_driver(AL_DISPLAY *display, char *gfx_card, int w, int h, int v_w, int v_h, int depth, int flags, int drv_flags, _DRIVER_INFO *driver_list)
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
	    //display->screen = init_gfx_driver(display, drv, w, h, depth, flags);
	    screen = init_gfx_driver(drv, w, h, depth, flags);

	    //if (display->screen)
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
//static int do_set_gfx_mode(AL_DISPLAY *display, int card, int w, int h, int depth, int flags)
int do_set_gfx_mode(int card, int w, int h, int depth, int flags)
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
   int v_w = 0;
   int v_h = 0;
   ASSERT(system_driver);

   _gfx_mode_set_count++;

   if (card == GFX_DIRECT3D) {
   	al_init();
	new_display = al_create_display(w, h, AL_UPDATE_IMMEDIATE);
	al_make_display_current(new_display);
	screen = create_bitmap(w, h);
	screen->al_bitmap = _al_current_display->vt->create_bitmap(_al_current_display, w, h, 0);
	screen->needs_upload = true;
	screen->al_bitmap->vt->make_compat_screen(screen->al_bitmap);
	screen->al_bitmap->vt->upload_compat_bitmap(screen, 0, 0, w, h);
	return 0;
   }

   /* special bodge for the GFX_SAFE driver */
   if (card == GFX_SAFE) {
      if (system_driver->get_gfx_safe_mode) {
	 ustrzcpy(buf, sizeof(buf), allegro_error);

	 /* retrieve the safe graphics mode */
	 system_driver->get_gfx_safe_mode(&driver, &mode);

	 /* try using the specified depth and resolution */
	 //if (do_set_gfx_mode(display, driver, w, h, depth, flags) == 0)
	 if (do_set_gfx_mode(driver, w, h, depth, flags) == 0)
	    return 0;

	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, buf);

	 /* finally use the safe settings */
	 set_color_depth(mode.bpp);
	 //if (do_set_gfx_mode(display, driver, mode.width, mode.height, depth, flags) == 0)
	 if (do_set_gfx_mode(driver, mode.width, mode.height, depth, flags) == 0)
	    return 0;

	 ASSERT(FALSE);  /* the safe graphics mode must always work */
      }
      else {
	 /* no safe graphics mode, try hard-coded autodetected modes with custom settings */
	 allow_config = FALSE;
	 _safe_gfx_mode_change = 1;

	 //ret = do_set_gfx_mode(display, GFX_AUTODETECT, w, h, depth, flags);
	 ret = do_set_gfx_mode(GFX_AUTODETECT, w, h, depth, flags);

	 _safe_gfx_mode_change = 0;
	 allow_config = TRUE;

	 if (ret == 0)
	    return 0;
      }

      /* failing to set GFX_SAFE is a fatal error */
      //do_set_gfx_mode(display, GFX_TEXT, 0, 0, 0, 0);
      do_set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
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
   //if (display->gfx_driver) {
   if (gfx_driver) {
      if (_al_linker_mouse)
         _al_linker_mouse->show_mouse(NULL);

      destroy_video_bitmaps();
      
      //bmp_read_line(display->screen, 0);
      //bmp_write_line(display->screen, 0);
      //bmp_unwrite_line(display->screen);
      bmp_read_line(screen, 0);
      bmp_write_line(screen, 0);
      bmp_unwrite_line(screen);

      //if (display->gfx_driver->scroll)
      if (gfx_driver->scroll)
	 //display->gfx_driver->scroll(0, 0);
	 gfx_driver->scroll(0, 0);

      //if (display->gfx_driver->exit)
      if (gfx_driver->exit)
	 //display->gfx_driver->exit(display->screen);
	 gfx_driver->exit(screen);

      //destroy_bitmap(display->screen);
      destroy_bitmap(screen);

      //display->gfx_driver = NULL;
      //display->screen = NULL;
      //display->gfx_capabilities = 0;
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

   if (card == GFX_AUTODETECT) {
      /* autodetect the driver */
      int found = FALSE;

      /* first try the config variables */
      if (allow_config) {
	 /* try the gfx_card variable if GFX_AUTODETECT or GFX_AUTODETECT_FULLSCREEN was selected */
	 if (!(drv_flags & GFX_DRIVER_WINDOWED_FLAG))
	    //found = get_config_gfx_driver(display, uconvert_ascii("gfx_card", tmp1), w, h, v_w, v_h, depth, flags, drv_flags, driver_list);
	    found = get_config_gfx_driver(uconvert_ascii("gfx_card", tmp1), w, h, v_w, v_h, depth, flags, drv_flags, driver_list);

	 /* try the gfx_cardw variable if GFX_AUTODETECT or GFX_AUTODETECT_WINDOWED was selected */
	 if (!(drv_flags & GFX_DRIVER_FULLSCREEN_FLAG) && !found)
	    //found = get_config_gfx_driver(display, uconvert_ascii("gfx_cardw", tmp1), w, h, v_w, v_h, depth, flags, drv_flags, driver_list);
	    found = get_config_gfx_driver(uconvert_ascii("gfx_cardw", tmp1), w, h, v_w, v_h, depth, flags, drv_flags, driver_list);
      }

      /* go through the list of autodetected drivers if none was previously found */
      if (!found) {
         /* Adjust virtual width and virtual height if needed and allowed */
      
	 for (c=0; driver_list[c].driver; c++) {
	    if (driver_list[c].autodetect) {
	       drv = driver_list[c].driver;

	       if (gfx_driver_is_valid(drv, drv_flags)) {
		  //display->screen = init_gfx_driver(display, drv, w, h, depth, flags);
		  screen = init_gfx_driver(drv, w, h, depth, flags);

		  //if (display->screen)
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
	 //display->screen = init_gfx_driver(display, drv, w, h, depth, flags);
	 screen = init_gfx_driver(drv, w, h, depth, flags);
   }

   /* gracefully handle failure */
   //if (!display->screen) {
   if (!screen) {
      //display->gfx_driver = NULL;  /* set by init_gfx_driver() */
      gfx_driver = NULL;  /* set by init_gfx_driver() */

      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to find a suitable graphics driver"));

      if (system_driver->display_switch_lock)
	 system_driver->display_switch_lock(FALSE, FALSE);

      return -1;
   }
   
   /* Set global variables */
   //screen = display->screen;
   //gfx_driver = display->gfx_driver;

   /* set the basic capabilities of the driver */
   //display->gfx_capabilities = gfx_capabilities;

   if ((VIRTUAL_W > SCREEN_W) || (VIRTUAL_H > SCREEN_H)) {
      //if (display->gfx_driver->scroll)
      if (gfx_driver->scroll)
	 //display->gfx_capabilities |= GFX_CAN_SCROLL;
	 gfx_capabilities |= GFX_CAN_SCROLL;

      //if ((display->gfx_driver->request_scroll) || (display->gfx_driver->request_video_bitmap))
      if ((gfx_driver->request_scroll) || (gfx_driver->request_video_bitmap))
	 //display->gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
	 gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }
   
   //gfx_capabilities = display->gfx_capabilities;

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

   //LOCK_DATA(display->gfx_driver, sizeof(GFX_DRIVER));
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



/* Function: al_scroll_screen
 *  Attempts to scroll the hardware screen, returning 0 on success. 
 *  Check the VIRTUAL_W and VIRTUAL_H values to see how far the screen
 *  can be scrolled. Note that a lot of VESA drivers can only handle
 *  horizontal scrolling in four pixel increments.
 */
//int al_scroll_display(AL_DISPLAY *display, int x, int y)
int al_scroll_display(int x, int y)
{
   int ret = 0;
   int h;
   
   //ASSERT(display);

   /* can driver handle hardware scrolling? */
   //if ((!display->gfx_driver->scroll) || (_dispsw_status))
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
   //if (display->gfx_driver->scroll(x, y) != 0)
   if (gfx_driver->scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* Function: al_request_scroll
 *  Attempts to initiate a triple buffered hardware scroll, which will
 *  take place during the next retrace. Returns 0 on success.
 */
//int al_request_scroll(AL_DISPLAY *display, int x, int y)
int al_request_scroll(int x, int y)
{
   int ret = 0;
   int h;

   //ASSERT(display);

   /* can driver handle triple buffering? */
   //if ((!display->gfx_driver->request_scroll) || (_dispsw_status)) {
   if ((!gfx_driver->request_scroll) || (_dispsw_status)) {
      //al_scroll_display(display, x, y);
      al_scroll_display(x, y);
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
   //if (display->gfx_driver->request_scroll(x, y) != 0)
   if (gfx_driver->request_scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* Function: al_poll_scroll
 *  Checks whether a requested triple buffer flip has actually taken place.
 */
//int al_poll_scroll(AL_DISPLAY *display)
int al_poll_scroll()
{
   //ASSERT(display);

   //if ((!display->gfx_driver->poll_scroll) || (_dispsw_status))
   if ((!gfx_driver->poll_scroll) || (_dispsw_status))
      return FALSE;

   //return display->gfx_driver->poll_scroll();
   return gfx_driver->poll_scroll();
}



/* Function: al_show_video_bitmap
 *  Page flipping function: swaps to display the specified video memory 
 *  bitmap object (this must be the same size as the physical screen).
 */
//int al_show_video_bitmap(AL_DISPLAY *display, BITMAP *bitmap)
int al_show_video_bitmap(BITMAP *bitmap)
{
   //ASSERT(display);
   ASSERT(bitmap);

   if ((!is_video_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   //if (display->gfx_driver->show_video_bitmap)
   if (gfx_driver->show_video_bitmap)
      //return display->gfx_driver->show_video_bitmap(bitmap);
      return gfx_driver->show_video_bitmap(bitmap);

   //return al_scroll_display(display, bitmap->x_ofs, bitmap->y_ofs);
   return al_scroll_display(bitmap->x_ofs, bitmap->y_ofs);
}



/* Function: al_request_video_bitmap
 *  Triple buffering function: triggers a swap to display the specified 
 *  video memory bitmap object, which will take place on the next retrace.
 */
//int al_request_video_bitmap(AL_DISPLAY *display, BITMAP *bitmap)
int al_request_video_bitmap(BITMAP *bitmap)
{
   //ASSERT(display);
   ASSERT(bitmap);

   if ((!is_video_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   //if (display->gfx_driver->request_video_bitmap)
   if (gfx_driver->request_video_bitmap)
      //return display->gfx_driver->request_video_bitmap(bitmap);
      return gfx_driver->request_video_bitmap(bitmap);

   //return al_request_scroll(display, bitmap->x_ofs, bitmap->y_ofs);
   return al_request_scroll(bitmap->x_ofs, bitmap->y_ofs);
}



/* Function: al_enable_triple_buffer
 *  Asks a driver to turn on triple buffering mode, if it is capable
 *  of that.
 */
//int al_enable_triple_buffer(AL_DISPLAY *display)
int al_enable_triple_buffer()
{
   //ASSERT(display);

   //if (display->gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)
   if (gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)
      return 0;

   if (_dispsw_status)
      return -1;

   //if (display->gfx_driver->enable_triple_buffer) {
   if (gfx_driver->enable_triple_buffer) {
      //display->gfx_driver->enable_triple_buffer();
      gfx_driver->enable_triple_buffer();

      //if ((display->gfx_driver->request_scroll) || (display->gfx_driver->request_video_bitmap)) {
      if ((gfx_driver->request_scroll) || (gfx_driver->request_video_bitmap)) {
	 //display->gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
	 gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
	 return 0;
      }
   }

   return -1;
}


#if 0
/* Function: al_create_display
 *  Create a new Allegro display object, return NULL on failure
 *  The first display object created becomes the al_main_display.
 */
AL_DISPLAY *al_create_display(int driver, int flags, int depth, int w, int h)
{
   AL_DISPLAY *new_display;
   AL_DISPLAY **new_display_ptr;
   int c;

   ASSERT(system_driver);
   
   /* It's an error to select more than one update method */
   if ( (flags & AL_UPDATE_ALL) & ( (flags & AL_UPDATE_ALL) - 1) )
      return NULL;

   /* Create a new display object */
   new_display = _AL_MALLOC(sizeof *new_display);
   if (!new_display)
      return NULL;
   memset(new_display, 0, sizeof *new_display);
      
   new_display_ptr = _al_vector_alloc_back(&display_list);
   if (!new_display_ptr) {
      _AL_FREE (new_display);
      return NULL;
   }
   (*new_display_ptr) = new_display;

   /* Set graphics mode */
   gfx_capabilities = 0;
   if (depth>0)
      set_color_depth(depth);
   if (do_set_gfx_mode(new_display, driver, w, h, depth, flags) == -1) {
      goto Error;
   }
   
   new_display->page = NULL;
   new_display->num_pages = 0;
   new_display->active_page = 0;
   new_display->flags = flags;
   new_display->depth = get_color_depth();
   new_display->gfx_capabilities |= gfx_capabilities;
   
   /* Select update method */
   switch(new_display->flags & AL_UPDATE_ALL) {
      case AL_UPDATE_TRIPLE_BUFFER:
         /* Try to enable triple buffering */
         if (!(new_display->gfx_capabilities & GFX_CAN_TRIPLE_BUFFER))
            al_enable_triple_buffer(new_display);

         if (new_display->gfx_capabilities & GFX_CAN_TRIPLE_BUFFER) {
            new_display->num_pages = 3;
            new_display->page = _AL_MALLOC(new_display->num_pages * sizeof *(new_display->page));

            for (c=0; c<new_display->num_pages; c++)
               new_display->page[c] = al_create_video_bitmap(new_display, w, h);
            
            /* Check for succes */
            if (new_display->page[0] && new_display->page[1] && new_display->page[2]) {
               for (c=0; c<new_display->num_pages; c++)
                  clear_bitmap(new_display->page[c]);
               al_show_video_bitmap(new_display, new_display->page[2]);
            }
            else {
               for (c=0; c<new_display->num_pages; c++)
                  destroy_bitmap(new_display->page[c]);
               _AL_FREE(new_display->page);
               do_set_gfx_mode(new_display, GFX_TEXT, 0, 0, 0, 0);
	       goto Error;
            }
         }
         else {
            do_set_gfx_mode(new_display, GFX_TEXT, 0, 0, 0, 0);
	    goto Error;
         }
         break;

      case AL_UPDATE_PAGE_FLIP:
         new_display->num_pages = 2;
         new_display->page = _AL_MALLOC(new_display->num_pages * sizeof *(new_display->page));

         for (c=0; c<new_display->num_pages; c++)
            new_display->page[c] = al_create_video_bitmap(new_display, w, h);
         
         /* Check for succes */
         if (new_display->page[0] && new_display->page[1]) {
            for (c=0; c<new_display->num_pages; c++)
               clear_bitmap(new_display->page[c]);
            al_show_video_bitmap(new_display, new_display->page[1]);
         }
         else {
            for (c=0; c<new_display->num_pages; c++)
               destroy_bitmap(new_display->page[c]);
            _AL_FREE(new_display->page);
            do_set_gfx_mode(new_display, GFX_TEXT, 0, 0, 0, 0);
	    goto Error;
         }
         break;

      case AL_UPDATE_SYSTEM_BUFFER:
      case AL_UPDATE_DOUBLE_BUFFER:
         new_display->num_pages = 1;
         new_display->page = _AL_MALLOC(new_display->num_pages * sizeof *(new_display->page));

         if (new_display->flags & AL_UPDATE_SYSTEM_BUFFER)
            new_display->page[0] = al_create_system_bitmap(new_display, w, h);
         else
            new_display->page[0] = create_bitmap(w, h);
         
         /* Check for succes */
         if (new_display->page[0]) {
            clear_bitmap(new_display->page[0]);
            al_show_video_bitmap(new_display, new_display->page[0]);
         }
         else {
            _AL_FREE(new_display->page);
            do_set_gfx_mode(new_display, GFX_TEXT, 0, 0, 0, 0);
	    goto Error;
         }
         break;
   }
   
   gfx_capabilities |= new_display->gfx_capabilities;
   
   if (!al_main_display)
      al_main_display = new_display;
      
   return new_display;

  Error:

   ASSERT(new_display);
   _al_vector_find_and_delete(&display_list, &new_display);
   _AL_FREE(new_display);
   return NULL;
}



/* Function: al_set_update_method
 *  Change the update method for the selected display. If the method change 
 *  fails the current method is either retained or reset to AL_UPDATE_NONE.
 *  Returns the method being used after this call.
 */
int al_set_update_method(AL_DISPLAY *display, int method)
{
   int c, w, h;
   ASSERT(display);
   ASSERT(display->gfx_driver);

   /* Requesting multiple modes is not valid */
   if ( (method & (method - 1)) ||  (method & ~AL_UPDATE_ALL) )
      return display->flags & AL_UPDATE_ALL;

   /* Do nothing if the methods are the same */      
   if (method == (display->flags & AL_UPDATE_ALL))
      return display->flags & AL_UPDATE_ALL;

   /* Retain current method if triple buffering was requested, but can't work */
   if ( (method & AL_UPDATE_TRIPLE_BUFFER) && 
        (!(display->gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)))
      return display->flags & AL_UPDATE_ALL;
      
   /* Destroy old setup */
   display->flags &= ~AL_UPDATE_ALL;
   for(c=0; c<display->num_pages; c++)
      destroy_bitmap(display->page[c]);
   _AL_FREE(display->page);
   display->page = NULL;
   display->num_pages = 0;
   display->active_page = 0;

   w = display->gfx_driver->w;
   h = display->gfx_driver->h;

   /* Set new method */
   switch(method) {
      case AL_UPDATE_TRIPLE_BUFFER:
         display->num_pages = 3;
         display->page = _AL_MALLOC(display->num_pages * sizeof *(display->page));

         for (c=0; c<display->num_pages; c++)
            display->page[c] = al_create_video_bitmap(display, w, h);
            
         /* Check for succes */
         if (display->page[0] && display->page[1] && display->page[2]) {
            for (c=0; c<display->num_pages; c++)
               clear_bitmap(display->page[c]);
            al_show_video_bitmap(display, display->page[2]);
            display->flags |= method;
         }
         else {
            for (c=0; c<display->num_pages; c++)
               destroy_bitmap(display->page[c]);
            display->page = NULL;
            display->num_pages = 0;
         }
         break;

      case AL_UPDATE_PAGE_FLIP:
         display->num_pages = 2;
         display->page = _AL_MALLOC(display->num_pages * sizeof *(display->page));

         for (c=0; c<display->num_pages; c++)
            display->page[c] = al_create_video_bitmap(display, w, h);
         
         /* Check for succes */
         if (display->page[0] && display->page[1]) {
            for (c=0; c<display->num_pages; c++)
               clear_bitmap(display->page[c]);
            al_show_video_bitmap(display, display->page[1]);
            display->flags |= method;
         }
         else {
            for (c=0; c<display->num_pages; c++)
               destroy_bitmap(display->page[c]);
            display->page = NULL;
            display->num_pages = 0;
         }
         break;

      case AL_UPDATE_SYSTEM_BUFFER:
      case AL_UPDATE_DOUBLE_BUFFER:
         display->num_pages = 1;
         display->page = _AL_MALLOC(display->num_pages * sizeof *(display->page));

         for (c=0; c<display->num_pages; c++)
            if (display->flags & AL_UPDATE_SYSTEM_BUFFER)
               display->page[c] = al_create_system_bitmap(display, w, h);
            else
               display->page[c] = create_bitmap(w, h);
         
         /* Check for succes */
         if (display->page[0]) {
            for (c=0; c<display->num_pages; c++)
               clear_bitmap(display->page[c]);
            al_show_video_bitmap(display, display->page[1]);
            display->flags |= method;
         }
         else {
            for (c=0; c<display->num_pages; c++)
               destroy_bitmap(display->page[c]);
            display->page = NULL;
            display->num_pages = 0;
         }
         break;
   }

   return display->flags & AL_UPDATE_ALL;
}



/* Function: al_destroy_display
 *  Destroys an Allegro display. On some platforms this will return to text
 *  mode.
 */
void al_destroy_display(AL_DISPLAY *display)
{
   int n;
   
   ASSERT(system_driver);
   
   if (!display)
      return;

   for(n=0; n<display->num_pages; n++)
      destroy_bitmap(display->page[n]);
   _AL_FREE(display->page);
   
   do_set_gfx_mode(display, GFX_TEXT, 0, 0, 0, 0);
   
   if (display == al_main_display)
      al_main_display = NULL;

   /* Remove the display from the list */
   _al_vector_find_and_delete(&display_list, &display);

   _AL_FREE(display);
}



/* Function: al_flip_display
 *  Flip the front and back buffers.
 */
void al_flip_display(AL_DISPLAY *display)
{
   ASSERT(display);

   switch(display->flags & AL_UPDATE_ALL) {
      case AL_UPDATE_TRIPLE_BUFFER:
         while (al_poll_scroll(display));
         al_request_video_bitmap(display, display->page[display->active_page]);
         
         display->active_page = (display->active_page+1)%display->num_pages;
         return;

      case AL_UPDATE_PAGE_FLIP:
         al_show_video_bitmap(display, display->page[display->active_page]);

         display->active_page = (display->active_page+1)%display->num_pages;
         return;

      case AL_UPDATE_SYSTEM_BUFFER:
      case AL_UPDATE_DOUBLE_BUFFER:
         /* Wait for vsync */
         if (!(display->flags&AL_DISABLE_VSYNC))
            vsync();
         blit(display->page[0], display->screen, 0, 0, 0, 0, display->page[0]->w, display->page[0]->h);
         return;
   }
}



/* Function: al_get_buffer
 *  Get a pointer to the backbuffer.
 */
BITMAP *al_get_buffer(const AL_DISPLAY *display)
{
   ASSERT(display);
   
   if (display->flags & AL_UPDATE_ALL)
      return display->page[display->active_page];
      
   return display->screen;
}



/* Function: al_get_update_method
 *  Get the update method being used to update the display.
 */
int al_get_update_method(const AL_DISPLAY *display)
{
   ASSERT(system_driver);

   return display->flags & AL_UPDATE_ALL;
}



/* VSync settings */

/* Function: al_enable_vsync
 *  Enable vsync for the display.
 */
void al_enable_vsync(AL_DISPLAY *display)
{
   ASSERT(display);
   
   display->flags &= ~AL_DISABLE_VSYNC;
}



/* Function: al_disable_vsync
 *  Disable vsync for the display.
 */
void al_disable_vsync(AL_DISPLAY *display)
{
   ASSERT(display);
   
   display->flags |= AL_DISABLE_VSYNC;
}



/* Function: al_toggle_vsync
 *  Toggle vsync for the display.
 */
void al_toggle_vsync(AL_DISPLAY *display)
{
   ASSERT(display);
   
   display->flags ^= AL_DISABLE_VSYNC;
}



/* Function: al_vsync_is_enabled
 *  Returns true if vsync is enabled on the display.
 */
int al_vsync_is_enabled(const AL_DISPLAY *display)
{
   ASSERT(display);
   
   return (display->flags&AL_DISABLE_VSYNC) == 0;
}
#endif
