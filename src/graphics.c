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
 *      Graphics mode set and bitmap creation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



static int gfx_virgin = TRUE;          /* is the graphics system active? */

int _sub_bitmap_id_count = 1;          /* hash value for sub-bitmaps */

int _gfx_mode_set_count = 0;           /* has the graphics mode changed? */

int _screen_split_position = 0;        /* has the screen been split? */

int _safe_gfx_mode_change = 0;         /* are we getting through GFX_SAFE? */

RGB_MAP *rgb_map = NULL;               /* RGB -> palette entry conversion */

COLOR_MAP *color_map = NULL;           /* translucency/lighting table */

int _color_depth = 8;                  /* how many bits per pixel? */

int _refresh_rate_request = 0;         /* requested refresh rate */
int _current_refresh_rate = 0;         /* refresh rate set by the driver */

int _color_conv = COLORCONV_TOTAL;     /* which formats to auto convert? */

static int color_conv_set = FALSE;     /* has the user set conversion mode? */

int _palette_color8[256];               /* palette -> pixel mapping */
int _palette_color15[256];
int _palette_color16[256];
int _palette_color24[256];
int _palette_color32[256];

int *palette_color = _palette_color8; 

BLENDER_FUNC _blender_func15 = NULL;   /* truecolor pixel blender routines */
BLENDER_FUNC _blender_func16 = NULL;
BLENDER_FUNC _blender_func24 = NULL;
BLENDER_FUNC _blender_func32 = NULL;

BLENDER_FUNC _blender_func15x = NULL;
BLENDER_FUNC _blender_func16x = NULL;
BLENDER_FUNC _blender_func24x = NULL;

int _blender_col_15 = 0;               /* for truecolor lit sprites */
int _blender_col_16 = 0;
int _blender_col_24 = 0;
int _blender_col_32 = 0;

int _blender_alpha = 0;                /* for truecolor translucent drawing */

int _rgb_r_shift_15 = 0;               /* truecolor pixel format */
int _rgb_g_shift_15 = 5;
int _rgb_b_shift_15 = 10;
int _rgb_r_shift_16 = 0;
int _rgb_g_shift_16 = 5;
int _rgb_b_shift_16 = 11;
int _rgb_r_shift_24 = 0;
int _rgb_g_shift_24 = 8;
int _rgb_b_shift_24 = 16;
int _rgb_r_shift_32 = 0;
int _rgb_g_shift_32 = 8;
int _rgb_b_shift_32 = 16;
int _rgb_a_shift_32 = 24;


/* lookup table for scaling 5 bit colors up to 8 bits */
int _rgb_scale_5[32] =
{
   0,   8,   16,  24,  32,  41,  49,  57,
   65,  74,  82,  90,  98,  106, 115, 123,
   131, 139, 148, 156, 164, 172, 180, 189,
   197, 205, 213, 222, 230, 238, 246, 255
};


/* lookup table for scaling 6 bit colors up to 8 bits */
int _rgb_scale_6[64] =
{
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  44,  48,  52,  56,  60,
   64,  68,  72,  76,  80,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   129, 133, 137, 141, 145, 149, 153, 157,
   161, 165, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 214, 218, 222,
   226, 230, 234, 238, 242, 246, 250, 255
};


GFX_VTABLE _screen_vtable;             /* accelerated drivers change this */


typedef struct VRAM_BITMAP             /* list of video memory bitmaps */
{
   int x, y, w, h;
   BITMAP *bmp;
   struct VRAM_BITMAP *next;
} VRAM_BITMAP;


static VRAM_BITMAP *vram_bitmap_list = NULL;



/* lock_bitmap:
 *  Locks all the memory used by a bitmap structure.
 */
void lock_bitmap(BITMAP *bmp)
{
   LOCK_DATA(bmp, sizeof(BITMAP) + sizeof(char *) * bmp->h);

   if (bmp->dat) {
      LOCK_DATA(bmp->dat, bmp->w * bmp->h * BYTES_PER_PIXEL(bitmap_color_depth(bmp)));
   }
}



/* request_refresh_rate:
 *  Requests that the next call to set_gfx_mode() use the specified refresh
 *  rate.
 */
void request_refresh_rate(int rate)
{
    _refresh_rate_request = rate;
}



/* get_refresh_rate:
 *  Returns the refresh rate set by the most recent call to set_gfx_mode().
 */
int get_refresh_rate(void)
{
    return _current_refresh_rate;
}



/* sort_gfx_mode_list:
 *  Callback for quick-sorting a mode-list.
 */
static int sort_gfx_mode_list(GFX_MODE *entry1, GFX_MODE *entry2)
{
   if (entry1->width > entry2->width) {
      return +1;
   }
   else if (entry1->width < entry2->width) {
      return -1;
   }
   else {
      if (entry1->height > entry2->height) {
         return +1;
      }
      else if (entry1->height < entry2->height) {
         return -1;
      }
      else {
         if (entry1->bpp > entry2->bpp) {
            return +1;
         }
         else if (entry1->bpp < entry2->bpp) {
            return -1;
         }
         else {
            return 0;
         }
      }
   }
}



/* get_gfx_mode_list:
 *  Attempts to create a list of all the supported video modes for a certain
 *  GFX driver.
 */
GFX_MODE_LIST *get_gfx_mode_list(int card)
{
   _DRIVER_INFO *list_entry;
   GFX_DRIVER *drv = NULL;
   GFX_MODE_LIST *gfx_mode_list = NULL;
   
   /* ask the system driver for a list of graphics hardware drivers */
   if (system_driver->gfx_drivers)
      list_entry = system_driver->gfx_drivers();
   else
      list_entry = _gfx_driver_list;

   /* find the graphics driver, and if it can fetch mode lists, do so */
   while (list_entry->driver) {
      if (list_entry->id == card) {
         drv = list_entry->driver;
         if (!drv->fetch_mode_list)
            return NULL;

         gfx_mode_list = drv->fetch_mode_list();
         if (!gfx_mode_list)
            return NULL;

         break;
      }

      list_entry++;
   }

   if (!drv)
      return NULL;

   /* sort the list and finish */
   qsort(gfx_mode_list->mode, gfx_mode_list->num_modes, sizeof(GFX_MODE),
         (int (*) (AL_CONST void *, AL_CONST void *))sort_gfx_mode_list);

   return gfx_mode_list;
}



/* destroy_gfx_mode_list:
 *  Removes the mode list created by get_gfx_mode_list() from memory.
 */
void destroy_gfx_mode_list(GFX_MODE_LIST *gfx_mode_list)
{
   if (gfx_mode_list) {
      if (gfx_mode_list->mode)
         free(gfx_mode_list->mode);

      free(gfx_mode_list);
   }
}



/* set_color_depth:
 *  Sets the pixel size (in bits) which will be used by subsequent calls to 
 *  set_gfx_mode() and create_bitmap(). Valid depths are 8, 15, 16, 24 and 32.
 */
void set_color_depth(int depth)
{
   _color_depth = depth;

   switch (depth) {
      case 8:  palette_color = _palette_color8;  break;
      case 15: palette_color = _palette_color15; break;
      case 16: palette_color = _palette_color16; break;
      case 24: palette_color = _palette_color24; break;
      case 32: palette_color = _palette_color32; break;
      default: ASSERT(FALSE);
   }
}



/* set_color_conversion:
 *  Sets a bit mask specifying which types of color format conversions are
 *  valid when loading data from disk.
 */
void set_color_conversion(int mode)
{
   _color_conv = mode;

   color_conv_set = TRUE;
}



/* _color_load_depth:
 *  Works out which color depth an image should be loaded as, given the
 *  current conversion mode.
 */
int _color_load_depth(int depth, int hasalpha)
{
   typedef struct CONVERSION_FLAGS
   {
      int flag;
      int in_depth;
      int out_depth;
      int hasalpha;
   } CONVERSION_FLAGS;

   static CONVERSION_FLAGS conversion_flags[] =
   {
      { COLORCONV_8_TO_15,   8,  15, FALSE },
      { COLORCONV_8_TO_16,   8,  16, FALSE },
      { COLORCONV_8_TO_24,   8,  24, FALSE },
      { COLORCONV_8_TO_32,   8,  32, FALSE },
      { COLORCONV_15_TO_8,   15, 8,  FALSE },
      { COLORCONV_15_TO_16,  15, 16, FALSE },
      { COLORCONV_15_TO_24,  15, 24, FALSE },
      { COLORCONV_15_TO_32,  15, 32, FALSE },
      { COLORCONV_16_TO_8,   16, 8,  FALSE },
      { COLORCONV_16_TO_15,  16, 15, FALSE },
      { COLORCONV_16_TO_24,  16, 24, FALSE },
      { COLORCONV_16_TO_32,  16, 32, FALSE },
      { COLORCONV_24_TO_8,   24, 8,  FALSE },
      { COLORCONV_24_TO_15,  24, 15, FALSE },
      { COLORCONV_24_TO_16,  24, 16, FALSE },
      { COLORCONV_24_TO_32,  24, 32, FALSE },
      { COLORCONV_32_TO_8,   32, 8,  FALSE },
      { COLORCONV_32_TO_15,  32, 15, FALSE },
      { COLORCONV_32_TO_16,  32, 16, FALSE },
      { COLORCONV_32_TO_24,  32, 24, FALSE },
      { COLORCONV_32A_TO_8,  32, 8 , TRUE  },
      { COLORCONV_32A_TO_15, 32, 15, TRUE  },
      { COLORCONV_32A_TO_16, 32, 16, TRUE  },
      { COLORCONV_32A_TO_24, 32, 24, TRUE  }
   };

   int i;

   ASSERT((_gfx_mode_set_count > 0) || (color_conv_set));

   if (depth == _color_depth)
      return depth;

   for (i=0; i < (int)(sizeof(conversion_flags)/sizeof(CONVERSION_FLAGS)); i++) {
      if ((conversion_flags[i].in_depth == depth) &&
	  (conversion_flags[i].out_depth == _color_depth) &&
	  ((conversion_flags[i].hasalpha != 0) == (hasalpha != 0))) {
	 if (_color_conv & conversion_flags[i].flag)
	    return _color_depth;
	 else
	    return depth;
      }
   }

   ASSERT(FALSE);
   return 0;
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
   if (gfx_driver)
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

   if (system_driver->restore_console_state)
      system_driver->restore_console_state();

   _remove_exit_func(shutdown_gfx);

   gfx_virgin = TRUE;
}



/* get_config_gfx_driver:
 *  Helper function for set_gfx_mode: it reads the gfx_card* config variables
 *  and sets the gfx_driver variable if the config variable was found
 */
static int get_config_gfx_driver(char *gfx_card, int check_mode, int require_window, _DRIVER_INFO *driver_list, int card, int w, int h, int v_w, int v_h)
{
   int tried = FALSE;
   char buf[512], tmp[64];
   int c, n;

   /* try the drivers that are listed in the config file */
   for (n=-2; n<255; n++) {
      switch (n) {

	 case -2:
	    /* example: gfx_card_640x480x16 = */
	    uszprintf(buf, sizeof(buf), uconvert_ascii("%s_%dx%dx%d", tmp), gfx_card, w, h, _color_depth);
	    break;

	 case -1:
	    /* example: gfx_card_24bpp = */
	    uszprintf(buf, sizeof(buf), uconvert_ascii("%s_%dbpp", tmp), gfx_card, _color_depth);
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
      card = get_config_id(uconvert_ascii("graphics", tmp), buf, GFX_AUTODETECT);

      if (card != GFX_AUTODETECT) {
	 for (c=0; driver_list[c].driver; c++) {
	    if (driver_list[c].id == card) {
	       gfx_driver = driver_list[c].driver;
	       if (check_mode) {
		  if (((require_window) && (!gfx_driver->windowed)) ||
		      ((!require_window) && (gfx_driver->windowed))) {
		     gfx_driver = NULL;
		     continue;
		  }
	       }
	       break;
	    }
	 }
	 if (gfx_driver) {
	    tried = TRUE;
	    gfx_driver->name = gfx_driver->desc = get_config_text(gfx_driver->ascii_name);
	    screen = gfx_driver->init(w, h, v_w, v_h, _color_depth);
	    if (screen)
	       break;
	    else
	       gfx_driver = NULL;
	 }
      }
      else {
	 if (n > 1)
	    break;
      }
   }

   return tried;
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
   extern void blit_end();
   _DRIVER_INFO *driver_list;
   int tried = FALSE;
   char buf[ALLEGRO_ERROR_SIZE], tmp[64];
   int c;
   int check_mode = FALSE, require_window = FALSE;


   _gfx_mode_set_count++;

   /* check specific fullscreen/windowed mode detection */
   if ((card == GFX_AUTODETECT_FULLSCREEN) ||
       (card == GFX_AUTODETECT_WINDOWED)) {
      check_mode = TRUE;
      require_window = (card == GFX_AUTODETECT_WINDOWED ? TRUE : FALSE);
      card = GFX_AUTODETECT;
   }

   /* special bodge for the GFX_SAFE driver */
   if (card == GFX_SAFE) {
      _safe_gfx_mode_change = 1;
      ustrzcpy(buf, sizeof(buf), allegro_error);
      if (set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0) != 0) {
	 set_color_depth(8);
	 allow_config = FALSE;
	 _safe_gfx_mode_change = 0;
      #ifdef GFX_SAFE_ID
	 if (set_gfx_mode(GFX_SAFE_ID, GFX_SAFE_W, GFX_SAFE_H, 0, 0) != 0) {
      #else
	 if (set_gfx_mode(GFX_AUTODETECT, 0, 0, 0, 0) != 0) {
      #endif
	    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	    allegro_message(uconvert_ascii("%s\n", tmp), get_config_text("Fatal error: unable to set GFX_SAFE"));
	    return -1;
	 }
	 allow_config = TRUE;
      }

      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, buf);
      _safe_gfx_mode_change = 0;
      return 0;
   }

   /* remember the current console state */
   if (gfx_virgin) {
      LOCK_FUNCTION(_stub_bank_switch);
      LOCK_FUNCTION(blit);

      if (system_driver->save_console_state)
	 system_driver->save_console_state();

      _add_exit_func(shutdown_gfx);

      gfx_virgin = FALSE;
   }

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(TRUE, TRUE);

   timer_simulate_retrace(FALSE);
   _screen_split_position = 0;

   /* close down any existing graphics driver */
   if (gfx_driver) {
      if (_al_linker_mouse)
         _al_linker_mouse->show_mouse(NULL);

      while (vram_bitmap_list)
	 destroy_bitmap(vram_bitmap_list->bmp);

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

   /* restore default truecolor pixel format */
   _rgb_r_shift_15 = 0; 
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 10;
   _rgb_r_shift_16 = 0;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 11;
   _rgb_r_shift_24 = 0;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 16;
   _rgb_r_shift_32 = 0;
   _rgb_g_shift_32 = 8;
   _rgb_b_shift_32 = 16;
   _rgb_a_shift_32 = 24;

   gfx_capabilities = 0;

   _current_refresh_rate = 0;

   /* return to text mode? */
   if (card == GFX_TEXT) {
      if (system_driver->restore_console_state)
	 system_driver->restore_console_state();

      if (_gfx_bank) {
	 free(_gfx_bank);
	 _gfx_bank = NULL;
      }

      if (system_driver->display_switch_lock)
	 system_driver->display_switch_lock(FALSE, FALSE);

      return 0;
   }

   /* ask the system driver for a list of graphics hardware drivers */
   if (system_driver->gfx_drivers)
      driver_list = system_driver->gfx_drivers();
   else
      driver_list = _gfx_driver_list;

   usetc(allegro_error, 0);

   /* try windowed mode drivers first if GFX_AUTODETECT_WINDOWED was selected */
   if ((card == GFX_AUTODETECT) && (allow_config) && (require_window))
      tried = get_config_gfx_driver(uconvert_ascii("gfx_cardw", tmp), check_mode, require_window, driver_list, card, w, h, v_w, v_h);

   /* check the gfx_card config variable if gfx_cardw wasn't used */
   if ((card == GFX_AUTODETECT) && (allow_config) && (!gfx_driver))
      tried = get_config_gfx_driver(uconvert_ascii("gfx_card", tmp), check_mode, require_window, driver_list, card, w, h, v_w, v_h);

   if (!tried) {
      /* search table for the requested driver */
      for (c=0; driver_list[c].driver; c++) {
	 if (driver_list[c].id == card) {
	    gfx_driver = driver_list[c].driver;
	    break;
	 }
      }

      if (gfx_driver) {                            /* specific driver? */
	 gfx_driver->name = gfx_driver->desc = get_config_text(gfx_driver->ascii_name);
	 screen = gfx_driver->init(w, h, v_w, v_h, _color_depth);
      }
      else {                                      /* otherwise autodetect */
	 for (c=0; driver_list[c].driver; c++) {
	    if (driver_list[c].autodetect) {
	       gfx_driver = driver_list[c].driver;
	       if (check_mode) {
		  if (((require_window) && (!gfx_driver->windowed)) ||
		      ((!require_window) && (gfx_driver->windowed)))
		  continue;
	       }
	       gfx_driver->name = gfx_driver->desc = get_config_text(gfx_driver->ascii_name);
	       screen = gfx_driver->init(w, h, v_w, v_h, _color_depth);
	       if (screen)
		  break;
	    }
	 }
      }
   }


   if (!screen) {
      gfx_driver = NULL;
      screen = NULL;

      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to find a suitable graphics driver"));

      if (system_driver->display_switch_lock)
	 system_driver->display_switch_lock(FALSE, FALSE);

      return -1;
   }

   if ((VIRTUAL_W > SCREEN_W) || (VIRTUAL_H > SCREEN_H)) {
      if (gfx_driver->scroll)
	 gfx_capabilities |= GFX_CAN_SCROLL;

      if ((gfx_driver->request_scroll) || (gfx_driver->request_video_bitmap))
	 gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }

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

   clear_bitmap(screen);

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
BITMAP *_make_bitmap(int w, int h, unsigned long addr, GFX_DRIVER *driver, int color_depth, int bpl)
{
   GFX_VTABLE *vtable = _get_vtable(color_depth);
   int i, bank, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *)malloc(size);
   if (!b)
      return NULL;

   _gfx_bank = realloc(_gfx_bank, h * sizeof(int));
   if (!_gfx_bank) {
      free(b);
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



/* create_bitmap_ex
 *  Creates a new memory bitmap in the specified color_depth
 */
BITMAP *create_bitmap_ex(int color_depth, int width, int height)
{
   GFX_VTABLE *vtable;
   BITMAP *bitmap;
   int i;

   if (system_driver->create_bitmap)
      return system_driver->create_bitmap(color_depth, width, height);

   vtable = _get_vtable(color_depth);
   if (!vtable)
      return NULL;

   bitmap = malloc(sizeof(BITMAP) + (sizeof(char *) * height));
   if (!bitmap)
      return NULL;

   bitmap->dat = malloc(width * height * BYTES_PER_PIXEL(color_depth));
   if (!bitmap->dat) {
      free(bitmap);
      return NULL;
   }

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = vtable;
   bitmap->write_bank = bitmap->read_bank = _stub_bank_switch;
   bitmap->id = 0;
   bitmap->extra = NULL;
   bitmap->x_ofs = 0;
   bitmap->y_ofs = 0;
   bitmap->seg = _default_ds();

   bitmap->line[0] = bitmap->dat;
   for (i=1; i<height; i++)
      bitmap->line[i] = bitmap->line[i-1] + width * BYTES_PER_PIXEL(color_depth);

   if (system_driver->created_bitmap)
      system_driver->created_bitmap(bitmap);

   return bitmap;
}



/* create_bitmap:
 *  Creates a new memory bitmap.
 */
BITMAP *create_bitmap(int width, int height)
{
   return create_bitmap_ex(_color_depth, width, height);
}



/* create_sub_bitmap:
 *  Creates a sub bitmap, ie. a bitmap sharing drawing memory with a
 *  pre-existing bitmap, but possibly with different clipping settings.
 *  Usually will be smaller, and positioned at some arbitrary point.
 *
 *  Mark Wodrich is the owner of the brain responsible this hugely useful 
 *  and beautiful function.
 */
BITMAP *create_sub_bitmap(BITMAP *parent, int x, int y, int width, int height)
{
   BITMAP *bitmap;
   int i;

   if (!parent) 
      return NULL;

   if (x < 0) 
      x = 0; 

   if (y < 0) 
      y = 0;

   if (x+width > parent->w) 
      width = parent->w-x;

   if (y+height > parent->h) 
      height = parent->h-y;

   if (parent->vtable->create_sub_bitmap)
      return parent->vtable->create_sub_bitmap(parent, x, y, width, height);

   if (system_driver->create_sub_bitmap)
      return system_driver->create_sub_bitmap(parent, x, y, width, height);

   /* get memory for structure and line pointers */
   bitmap = malloc(sizeof(BITMAP) + (sizeof(char *) * height));
   if (!bitmap)
      return NULL;

   acquire_bitmap(parent);

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = parent->vtable;
   bitmap->write_bank = parent->write_bank;
   bitmap->read_bank = parent->read_bank;
   bitmap->dat = NULL;
   bitmap->extra = NULL;
   bitmap->x_ofs = x + parent->x_ofs;
   bitmap->y_ofs = y + parent->y_ofs;
   bitmap->seg = parent->seg;

   /* All bitmaps are created with zero ID's. When a sub-bitmap is created,
    * a unique ID is needed to identify the relationship when blitting from
    * one to the other. This is obtained from the global variable
    * _sub_bitmap_id_count, which provides a sequence of integers (yes I
    * know it will wrap eventually, but not for a long time :-) If the
    * parent already has an ID the sub-bitmap adopts it, otherwise a new
    * ID is given to both the parent and the child.
    */
   if (!(parent->id & BMP_ID_MASK)) {
      parent->id |= _sub_bitmap_id_count;
      _sub_bitmap_id_count = (_sub_bitmap_id_count+1) & BMP_ID_MASK;
   }

   bitmap->id = parent->id | BMP_ID_SUB;
   bitmap->id &= ~BMP_ID_LOCKED;

   if (is_planar_bitmap(bitmap))
      x /= 4;

   x *= BYTES_PER_PIXEL(bitmap_color_depth(bitmap));

   /* setup line pointers: each line points to a line in the parent bitmap */
   for (i=0; i<height; i++)
      bitmap->line[i] = parent->line[y+i] + x;

   if (bitmap->vtable->set_clip)
      bitmap->vtable->set_clip(bitmap);

   if (parent->vtable->created_sub_bitmap)
      parent->vtable->created_sub_bitmap(bitmap, parent);

   if (system_driver->created_sub_bitmap)
      system_driver->created_sub_bitmap(bitmap, parent);

   if (parent->id & BMP_ID_VIDEO)
      _register_switch_bitmap(bitmap, parent);

   release_bitmap(parent);

   return bitmap;
}



/* try_vram_location:
 *  Attempts to create a video memory bitmap out of the specified region
 *  of the larger screen surface, returning a pointer to it if that area
 *  is free.
 */
static BITMAP *try_vram_location(int x, int y, int w, int h)
{
   VRAM_BITMAP *block = vram_bitmap_list;

   if ((x<0) || (y<0) || (x+w > VIRTUAL_W) || (y+h > VIRTUAL_H))
      return NULL;

   while (block) {
      if (((x < block->x+block->w) && (x+w > block->x)) &&
	  ((y < block->y+block->h) && (y+h > block->y)))
	 return NULL;

      block = block->next;
   }

   block = malloc(sizeof(VRAM_BITMAP));
   if (!block)
      return NULL;

   block->x = x;
   block->y = y;
   block->w = w;
   block->h = h;

   block->bmp = create_sub_bitmap(screen, x, y, w, h);
   if (!(block->bmp)) {
      free(block);
      return NULL;
   }

   block->next = vram_bitmap_list;
   vram_bitmap_list = block;

   return block->bmp;
}



/* create_video_bitmap:
 *  Attempts to make a bitmap object for accessing offscreen video memory.
 */
BITMAP *create_video_bitmap(int width, int height)
{
   VRAM_BITMAP *blockx;
   VRAM_BITMAP *blocky;
   VRAM_BITMAP *block;
   BITMAP *bmp;

   if (_dispsw_status)
      return NULL;

   /* let the driver handle the request if it can */
   if (gfx_driver->create_video_bitmap) {
      bmp = gfx_driver->create_video_bitmap(width, height);
      if (!bmp)
	 return NULL;

      block = malloc(sizeof(VRAM_BITMAP));
      block->x = -1;
      block->y = -1;
      block->w = 0;
      block->h = 0;
      block->bmp = bmp;
      block->next = vram_bitmap_list;
      vram_bitmap_list = block;

      return bmp;
   }

   /* otherwise fall back on subdividing the normal screen surface */
   if ((bmp = try_vram_location(0, 0, width, height)) != NULL)
      return bmp;

   for (blocky = vram_bitmap_list; blocky; blocky = blocky->next) {
      for (blockx = vram_bitmap_list; blockx; blockx = blockx->next) {
	 if ((bmp = try_vram_location((blockx->x+blockx->w+15)&~15, blocky->y, width, height)) != NULL)
	    return bmp;
	 if ((bmp = try_vram_location((blockx->x-width)&~15, blocky->y, width, height)) != NULL)
	    return bmp;
	 if ((bmp = try_vram_location(blockx->x, blocky->y+blocky->h, width, height)) != NULL)
	    return bmp;
	 if ((bmp = try_vram_location(blockx->x, blocky->y-height, width, height)) != NULL)
	    return bmp;
      }
   }

   return NULL;
}



/* create_system_bitmap:
 *  Attempts to make a system-specific (eg. DirectX surface) bitmap object.
 */
BITMAP *create_system_bitmap(int width, int height)
{
   BITMAP *bmp;

   ASSERT(gfx_driver != NULL);

   if (gfx_driver->create_system_bitmap)
      return gfx_driver->create_system_bitmap(width, height);

   bmp = create_bitmap(width, height);
   bmp->id |= BMP_ID_SYSTEM;

   return bmp;
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
		  prev->next = pos->next;
	       else
		  vram_bitmap_list = pos->next;

	       if (pos->x < 0) {
		  /* the driver is in charge of this object */
		  gfx_driver->destroy_video_bitmap(bitmap);
		  free(pos);
		  return;
	       } 

	       free(pos);
	       break;
	    }

	    prev = pos;
	    pos = pos->next;
	 }

	 _unregister_switch_bitmap(bitmap);
      }
      else if (is_system_bitmap(bitmap)) {
	 /* special case for getting rid of system memory bitmaps */
	 ASSERT(gfx_driver != NULL);

	 if (gfx_driver->destroy_system_bitmap) {
	    gfx_driver->destroy_system_bitmap(bitmap);
	    return;
	 }
      }

      /* normal memory or sub-bitmap destruction */
      if (system_driver->destroy_bitmap) {
	 if (system_driver->destroy_bitmap(bitmap))
	    return;
      }

      if (bitmap->dat)
	 free(bitmap->dat);

      free(bitmap);
   }
}



/* set_clip:
 *  Sets the two opposite corners of the clipping rectangle to be used when
 *  drawing to the bitmap. Nothing will be drawn to positions outside of this 
 *  rectangle. When a new bitmap is created the clipping rectangle will be 
 *  set to the full area of the bitmap. If x1, y1, x2 and y2 are all zero 
 *  clipping will be turned off, which will slightly speed up drawing 
 *  operations but will allow memory to be corrupted if you attempt to draw 
 *  off the edge of the bitmap.
 */
void set_clip(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
   int t;

   ASSERT(bitmap);

   if ((!x1) && (!y1) && (!x2) && (!y2)) {
      bitmap->clip = FALSE;
      bitmap->cl = bitmap->ct = 0;
      bitmap->cr = bitmap->w;
      bitmap->cb = bitmap->h;

      if (bitmap->vtable->set_clip)
	 bitmap->vtable->set_clip(bitmap);

      return;
   }

   if (x2 < x1) {
      t = x1;
      x1 = x2;
      x2 = t;
   }

   if (y2 < y1) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   x2++;
   y2++;

   bitmap->clip = TRUE;
   bitmap->cl = MID(0, x1, bitmap->w-1);
   bitmap->ct = MID(0, y1, bitmap->h-1);
   bitmap->cr = MID(0, x2, bitmap->w);
   bitmap->cb = MID(0, y2, bitmap->h);

   if (bitmap->vtable->set_clip)
      bitmap->vtable->set_clip(bitmap);
}



/* scroll_screen:
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



/* request_scroll:
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



/* poll_scroll:
 *  Checks whether a requested triple buffer flip has actually taken place.
 */
int poll_scroll()
{
   if ((!gfx_driver->poll_scroll) || (_dispsw_status))
      return FALSE;

   return gfx_driver->poll_scroll();
}



/* show_video_bitmap:
 *  Page flipping function: swaps to display the specified video memory 
 *  bitmap object (this must be the same size as the physical screen).
 */
int show_video_bitmap(BITMAP *bitmap)
{
   if ((!is_video_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   if (gfx_driver->show_video_bitmap)
      return gfx_driver->show_video_bitmap(bitmap);

   return scroll_screen(bitmap->x_ofs, bitmap->y_ofs);
}



/* request_video_bitmap:
 *  Triple buffering function: triggers a swap to display the specified 
 *  video memory bitmap object, which will take place on the next retrace.
 */
int request_video_bitmap(BITMAP *bitmap)
{
   if ((!is_video_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   if (gfx_driver->request_video_bitmap)
      return gfx_driver->request_video_bitmap(bitmap);

   return request_scroll(bitmap->x_ofs, bitmap->y_ofs);
}



/* enable_triple_buffer:
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


