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
 *      Video driver using SVGAlib.
 *
 *      By Stefan T. Boettner.
 * 
 *      Banked mode and VT switching by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintunix.h"
#include "linalleg.h"


#ifdef ALLEGRO_LINUX_SVGALIB

#include <signal.h>
#include <termios.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <vga.h>



static BITMAP *svga_init(int w, int h, int v_w, int v_h, int color_depth);
static void svga_exit(BITMAP *b);
static int  svga_scroll(int x, int y);
static void svga_vsync(void);
static void svga_set_palette(RGB *p, int from, int to, int vsync);
static void svga_flip(void);

#ifndef ALLEGRO_NO_ASM
unsigned long _svgalib_read_line_asm(BITMAP *bmp, int line);
unsigned long _svgalib_write_line_asm(BITMAP *bmp, int line);
void _svgalib_unwrite_line_asm(BITMAP *bmp);
#endif



GFX_DRIVER gfx_svgalib = 
{
   GFX_SVGALIB,
   empty_string,
   empty_string,
   "SVGAlib", 
   svga_init,
   svga_exit,
   svga_scroll,
   svga_vsync,
   svga_set_palette,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   svga_flip,
   svga_flip,
   0, 0,
   TRUE,
   0, 0, 0, 0
};



static char svga_desc[256] = EMPTY_STRING;

static unsigned int display_start_mask, scanline_width, bytesperpixel;

static unsigned char *screen_buffer;
static int last_line;



/* _svgalib_read_line:
 *  Return linear offset for reading line.
 */
unsigned long _svgalib_read_line(BITMAP *bmp, int line)
{
   return (unsigned long) (bmp->line[line]);
}


/* _svgalib_write_line:
 *  Update last selected line and select new line.
 */
unsigned long _svgalib_write_line(BITMAP *bmp, int line)
{
   int new_line = line + bmp->y_ofs;
   if ((new_line != last_line) && (last_line >= 0))
      vga_drawscanline(last_line, screen_buffer + last_line * scanline_width);
   last_line = new_line;
   return (unsigned long) (bmp->line[line]);
}


/* _svgalib_unwrite_line:
 *  Update last selected line.
 */
void _svgalib_unwrite_line(BITMAP *bmp)
{
   if (last_line >= 0) {
      vga_drawscanline(last_line, screen_buffer + last_line * scanline_width);
      last_line = -1;
   }
}



static struct sigaction old_sigusr1, old_sigusr2;

static void svga_save_signals()
{
   sigaction(SIGUSR1, NULL, &old_sigusr1);
   sigaction(SIGUSR2, NULL, &old_sigusr2);
}

static void svga_restore_signals()
{
   sigaction(SIGUSR1, &old_sigusr1, NULL);
   sigaction(SIGUSR2, &old_sigusr2, NULL);
}



static void *svga_create_screen_buffer(int size)
{
   screen_buffer = malloc(size);
   return screen_buffer;
}

static void svga_free_screen_buffer()
{
   if (screen_buffer) {
      free(screen_buffer);
      screen_buffer = NULL;
   }
}



/*
 * Wrapper for vga_init().
 */
static int __vga_init(void)
{
   int ret;
   
   __al_linux_async_exit();

   seteuid(0);
   vga_disabledriverreport();
   ret = vga_init();
   seteuid(getuid());

   __al_linux_async_init();

   return ret;
}


/*
 * SVGAlib changes the termios, which mucks up our keyboard driver.
 * We also want to do the console switching ourselves, so wrap the 
 * vga_setmode() function call.
 */
static int __vga_setmode(int num) 
{
   struct termios termio;
   struct vt_mode vtm;
   int ret;

   ioctl(__al_linux_console_fd, VT_GETMODE, &vtm);
   tcgetattr(__al_linux_console_fd, &termio);

   ret = vga_setmode(num);

   tcsetattr(__al_linux_console_fd, TCSANOW, &termio);
   ioctl(__al_linux_console_fd, VT_SETMODE, &vtm);
   svga_restore_signals();
   
   return ret;
}



static int svga_mode_ok(vga_modeinfo *info, int w, int h, int v_w, int v_h, int color_depth)
{
   if ((color_depth == 8  && info->colors == 256)  	||
       (color_depth == 15 && info->colors == 32768) 	|| 
       (color_depth == 16 && info->colors == 65536) 	||
       (color_depth == 24 && info->bytesperpixel == 3)  ||
       (color_depth == 32 && info->bytesperpixel == 4))
      return (((info->width == w && info->height == h) || (!w && !h))
	      && (info->maxlogicalwidth >= v_w * info->bytesperpixel));
   else
      return 0;
}


static vga_modeinfo *svga_set_mode(int w, int h, int v_w, int v_h, int color_depth, int flags)
{
   vga_modeinfo *info;
   int i;
    
   for (i = 0; i <= vga_lastmodenumber(); i++) {
      if (!vga_hasmode(i)) 
	 continue;
      
      info = vga_getmodeinfo(i);
      if ((info->flags & IS_MODEX) 
	  || (!svga_mode_ok(info, w, h, v_w, v_h, color_depth))
	  || (flags && !(info->flags & flags)))
	 continue;
      
      if (__vga_setmode(i) == 0) {
	 gfx_svgalib.w = vga_getxdim();
	 gfx_svgalib.h = vga_getydim();
	 __al_linux_console_graphics();
	 return info;
      }
   }

   return NULL;
}


static void svga_setup_colour(int color_depth)
{
   switch (color_depth) {
      case 15:
	 _rgb_r_shift_15 = 10;
	 _rgb_g_shift_15 = 5;
	 _rgb_b_shift_15 = 0;
	 break;

      case 16:
	 _rgb_r_shift_16 = 11;
	 _rgb_g_shift_16 = 5;
	 _rgb_b_shift_16 = 0;
	 break;

      case 24:
	 _rgb_r_shift_24 = 16;
	 _rgb_g_shift_24 = 8;
	 _rgb_b_shift_24 = 0;
      	 break;

      case 32:
      	 _rgb_a_shift_32 = 24;
   	 _rgb_r_shift_32 = 16;
         _rgb_g_shift_32 = 8;
   	 _rgb_b_shift_32 = 0;
	 break;
   }
}



/* svga_init:
 *  Sets a graphics mode.
 */
static BITMAP *svga_private_init(int w, int h, int v_w, int v_h, int color_depth)
{
   int vidmem, width, height;
   vga_modeinfo *info;
   BITMAP *bmp;

   /* makes the mode switch more stable */
   __vga_setmode(TEXT);


   /* Try get a linear frame buffer */

   info = svga_set_mode(w, h, v_w, v_h, color_depth, CAPABLE_LINEAR);
   if (info) { 
      vidmem = vga_setlinearaddressing();
      if (vidmem < 0) {
	 ustrcpy(allegro_error, get_config_text("Cannot enable linear addressing"));
	 return NULL;
      }

      if (v_w) {
	 width = v_w;
	 scanline_width = width * info->bytesperpixel;
	 vga_setlogicalwidth(scanline_width);
      }
      else {
	 width = info->linewidth / info->bytesperpixel;
	 scanline_width = info->linewidth;
      }

      /* set entries in gfx_svgalib */
      gfx_svgalib.vid_mem = vidmem;
      gfx_svgalib.vid_phys_base = (unsigned long)vga_getgraphmem();
      gfx_svgalib.scroll = svga_scroll;

      ustrcpy(svga_desc, uconvert_ascii("SVGAlib (linear)", NULL));
      gfx_svgalib.desc = svga_desc;

      /* for hardware scrolling */
      display_start_mask = info->startaddressrange;
      bytesperpixel = info->bytesperpixel;

      /* set truecolour format */
      svga_setup_colour(color_depth);

      /* make the screen bitmap and run */
      bmp = _make_bitmap(width, info->maxpixels / width,
			 (unsigned long)vga_getgraphmem(),
			 &gfx_svgalib, color_depth, scanline_width);
      return bmp;
   }


   /* Try get a banked frame buffer */
   
   if ((v_w > w) || (v_h > h)) {
      ustrcpy(allegro_error, get_config_text("Resolution not supported"));
      return NULL;
   }

   info = svga_set_mode(w, h, v_w, v_h, color_depth, 0);
   if (info) {
      width = gfx_svgalib.w;
      height = gfx_svgalib.h;
      scanline_width = width * info->bytesperpixel;

      /* allocate memory buffer for screen */
      if (!svga_create_screen_buffer(scanline_width * height))
	 return NULL;
      last_line = -1;

      /* set entries in gfx_svgalib */
      gfx_svgalib.vid_mem = scanline_width * height;
      gfx_svgalib.vid_phys_base = (unsigned long)screen_buffer;
      gfx_svgalib.scroll = NULL;

      ustrcpy(svga_desc, uconvert_ascii("SVGAlib (banked)", NULL));
      gfx_svgalib.desc = svga_desc;

      /* set truecolour format */
      svga_setup_colour(color_depth);

      /* make the screen bitmap and run */
      bmp = _make_bitmap(width, height, (unsigned long)screen_buffer,
			 &gfx_svgalib, color_depth, scanline_width);
      if (bmp) {
	 /* set bank switching routines */
#ifndef ALLEGRO_NO_ASM
	 bmp->read_bank = _svgalib_read_line_asm;
	 bmp->write_bank = _svgalib_write_line_asm;
	 bmp->vtable->unwrite_bank = _svgalib_unwrite_line_asm;
#else
	 bmp->read_bank = _svgalib_read_line;
	 bmp->write_bank = _svgalib_write_line;
	 bmp->vtable->unwrite_bank = _svgalib_unwrite_line;
#endif
      }

      return bmp;
   }

   /* all is lost */
   ustrcpy(allegro_error, get_config_text("Resolution not supported"));
   return NULL;
}

static BITMAP *svga_init(int w, int h, int v_w, int v_h, int color_depth)
{
   static int virgin = 1;
   BITMAP *bmp;

   if (!__al_linux_have_ioperms) {
      ustrcpy(allegro_error, get_config_text("This driver needs root privileges"));
      return NULL;
   }

   if (virgin) {
      svga_save_signals();
      if (__vga_init() != 0) 
	 return NULL;
      virgin = 0;
   }
   
   DISABLE();
   bmp = svga_private_init(w, h, v_w, v_h, color_depth);
   ENABLE();
   return bmp;
}



/* svga_exit:
 *  Unsets the video mode.
 */
static void svga_exit(BITMAP *b)
{
   DISABLE();

   svga_free_screen_buffer();
   
   __vga_setmode(TEXT);
   __vga_setmode(TEXT);

   ENABLE();
   
   /* Make sure the screen is not filled with crap when we exit.
    * Not sure why this works; damned if I'm going to find out! :) */
   ioctl(__al_linux_console_fd, KDSETMODE, KD_GRAPHICS);
   __al_linux_console_text();
}



/* svga_scroll:
 *  Hardware scrolling routine.
 */
static int svga_scroll(int x, int y)
{
   vga_setdisplaystart((x * bytesperpixel + y * scanline_width) & display_start_mask);
   return 0;
}



/* svga_vsync:
 *  Waits for a retrace.
 */
static void svga_vsync()
{
   vga_waitretrace();
}



/* svga_set_palette:
 *  Sets the palette.
 */
static void svga_set_palette(RGB *p, int from, int to, int vsync)
{
   int i;

   if (vsync)
      vga_waitretrace();

   for (i = from; i <= to; i++)
      vga_setpalette(i, p[i].r, p[i].g, p[i].b);
}



/* svga_flip:
 *  Switch between text and graphics modes.
 */
static void svga_flip()
{
   DISABLE();
   vga_flip();
   ENABLE();
}



#endif      /* ifdef ALLEGRO_LINUX_SVGALIB */
