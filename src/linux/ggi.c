/*
 *	This file is not used as it doesn't work and is a mess.
 */


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
 *      Video driver for the Linux GGI system: sort of functional.
 *
 *      By Peter Wang, finishing off code by Carsten Schmidt.
 *
 *      See readme.txt for copyright information.
 */


/* TODOs:
 * - planar DB
 * - system driver?
 * - console switching (if not handled by GGI)
 * - fix up the "assembly" routines
 * - panning
 * - "fake" buffers are REALLY slow, why?
 * - they also give the wrong colours (with [pal|true]emu)
 */

/* Weirdnesses:
 * - with svgalib it (sometimes) chooses the next highest mode to the one we want
 */


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintunix.h"


#ifdef ALLEGRO_LINUX_GGI

#if !defined(_POSIX_MAPPED_FILES) || !defined(HAVE_MMAP)
#error "Sorry, mapped files are required for Linux console Allegro to work!"
#endif

#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <ggi/ggi.h>



static BITMAP *ggi_init(int w, int h, int v_w, int v_h, int color_depth);
static void ggi_exit(BITMAP * b);
static void ggi_set_palette_range(AL_CONST struct RGB *p, int from, int to, int vsync);

#ifndef ALLEGRO_NO_ASM
unsigned long ggi_read_bank_asm(BITMAP *bmp, int line);
unsigned long ggi_write_bank_asm(BITMAP *bmp, int line);
void ggi_unwrite_bank_asm(BITMAP *bmp);
unsigned long ggi_write_fake_bank_asm(BITMAP *bmp, int line);
void ggi_unfake_bank_asm(BITMAP *bmp);
#endif



GFX_DRIVER gfx_ggi =
{
   GFX_GGI,                     /* id */
   empty_string,                /* name */
   empty_string,                /* desc */
   "LibGGI",                    /* ascii_name */
   ggi_init,                    /* init */
   ggi_exit,                    /* exit */
   NULL,                        /* scroll */
   NULL,                        /* vsync */
   ggi_set_palette_range,       /* set_palette */
   NULL,                        /* request_scroll */
   NULL,                        /* poll_scroll */
   NULL,                        /* enable_triple_buffer */
   NULL,                        /* create_video_bitmap */
   NULL,                        /* destroy_video_bitmap */
   NULL,                        /* show_video_bitmap */
   NULL,                        /* request_video_bitmap */
   NULL,                        /* create_system_bitmap */
   NULL,                        /* destroy_system_bitmap */
   NULL,                        /* set_mouse_sprite */
   NULL,                        /* show_mouse */
   NULL,                        /* hide_mouse */
   NULL,                        /* move_mouse */
   NULL,                        /* drawing_mode */
   0,                           /* w */
   0,                           /* h */
   FALSE,                       /* linear */
   0,                           /* bank_size */
   0,                           /* bank_gran */
   0,                           /* vid_mem */
   0                            /* vid_phys_base */
};



static ggi_visual_t ggi_vis;
static int ggi_w, ggi_h, ggi_stride;
static ggi_color ggi_pal[PAL_SIZE];

static const ggi_directbuffer *ggi_db = NULL;
static void *ggi_fake_buf = NULL;      /* used if no db available */
static char *ggi_touched;

static int ggi_line;
static void *ggi_ptr;
static int ggi_access;                 /* 1-reading, 2-writing */

static int ggi_virgin = TRUE;



/* Returns pointer to start of line */
unsigned long ggi_read_bank(BITMAP *bmp, int line)
{
   ggi_line = line;

   if (ggi_access != 1) {
      ggiResourceAcquire(ggi_db->resource, GGI_ACTYPE_READ);
      ggi_access = 1;
   }

   ggi_ptr = ggi_db->write + ggi_stride * ggi_line;

   return (unsigned long) ggi_ptr;
}

unsigned long ggi_write_bank(BITMAP *bmp, int line)
{
   ggi_line = line;

   if (ggi_access != 2) {
      ggiResourceAcquire(ggi_db->resource, GGI_ACTYPE_WRITE);
      ggi_access = 2;
   }

   ggi_ptr = ggi_db->write + ggi_stride * ggi_line;

   return (unsigned long) ggi_ptr;
}

void ggi_unwrite_bank(BITMAP *bmp)
{
   if (ggi_access) {
      ggiResourceRelease(ggi_db->resource);
      ggiFlush(ggi_vis);
      ggi_access = 0;
   }
}

unsigned long ggi_write_fake_bank(BITMAP *bmp, int line)
{
   ggi_line = line;

   ggi_ptr = ggi_fake_buf + ggi_stride * ggi_line;
   ggi_touched[ggi_line] = 1;

   return (unsigned long) ggi_ptr;
}

void ggi_unfake_bank(BITMAP *bmp)
{
   int i, j, f = 0;
   for (i=0; i<ggi_h; i++) {
      if (ggi_touched[i]) {
	 ggi_touched[i] = 0;
	 for (j=i+1; j<ggi_h && ggi_touched[j]; j++)
	    ggi_touched[j] = 0;

	 ggiPutBox(ggi_vis, 0, i, ggi_w, j-i, ggi_fake_buf + i * ggi_stride);
	 f = 1;
      }
   }

   if (f)
      ggiFlush(ggi_vis);
}



/* set a mode if possible, or suggest another one */
static int ggi_set(ggi_visual_t vis, int w, int h, int v_w, int v_h, ggi_graphtype gt, ggi_mode *sug_mode)
{
   int err;
   err = ggiCheckGraphMode(vis, w, h, v_w, v_h, gt, sug_mode);
   if (!err) 
      ggiSetGraphMode(vis, w, h, v_w, v_h, gt);
   return err;
}



#define LFB     1               /* linear frame buffer */
#define PFB     2               /* paged  frame buffer */



/* ggi_init:
 *  Initialise GGI and create a screen bitmap.
 */
static BITMAP *ggi_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;
   ggi_mode mode; 
   ggi_graphtype gt = 0;
   char tgt[1024], *e;
   int i, num_bufs, avail_buf, buf_num;
   void *p;

   if (ggi_virgin) {
      if (ggiInit() != 0) {
	 ustrcpy(allegro_error, get_config_text("Cannot initialise GGI!"));
	 return NULL;
      }
      ggi_virgin = FALSE;
   }

   ggi_vis = ggiOpen(NULL);
   if (!ggi_vis) {
      ggiExit();
      ustrcpy(allegro_error, get_config_text("Cannot open default visual!"));
      return NULL;
   }

   ggiSetFlags(ggi_vis, GGIFLAG_ASYNC);

   if (!v_w) v_w = GGI_AUTO;
   if (!v_h) v_h = GGI_AUTO;
   GT_SETDEPTH(gt, color_depth);

   /* try set the desired mode */
   /* FIXME - this is too complicated
    * actually, this whole function is too big
    */
   if (ggi_set(ggi_vis, w, h, v_w, v_h, gt, &mode) != 0) {

      /* try the suggested mode instead, but keeping same colour depth */
      if (ggi_set(ggi_vis, mode.visible.x, mode.visible.y, 
		  mode.virt.x, mode.virt.y, gt, &mode) != 0) {

	 /* we can't set this colour depth (eg. under X) */
	 ggiClose(ggi_vis);

	 if (color_depth == 8)
	    strcpy(tgt, "palemu");
	 else {
	    strcpy(tgt, "trueemu");
	    GT_SETDEPTH(gt, 16);
	 }

	 e = getenv("GGI_DISPLAY");
	 if (e) {
	    strcat(tgt, ":");
	    strcat(tgt, e);
	 }

	 ggi_vis = ggiOpen(tgt);
	 if (!ggi_vis) {
	    ggiExit();
	    ustrcpy(allegro_error, get_config_text("Cannot open palemu/trueemu visual!"));
	    return NULL;
	 }

	 ggiSetFlags(ggi_vis, GGIFLAG_ASYNC);

	 if (ggi_set(ggi_vis, w, h, v_w, v_h, gt, &mode) != 0) {
	    if (ggiSetMode(ggi_vis, &mode) != 0) {
	       ggiClose(ggi_vis);
	       ggiExit();
	       ustrcpy(allegro_error, get_config_text("Requested mode not available!"));
	       return NULL;
	    }
	 }
      }
   }

   ggiGetMode(ggi_vis, &mode);

   /* one last check */
   if ((mode.visible.x < w) || (mode.visible.y < h) || 
       (mode.virt.x < v_w) || (mode.virt.y < v_h)) {
      ggiClose(ggi_vis);
      ggiExit();
      ustrcpy(allegro_error, get_config_text("Requested mode not available!"));
      return NULL;
   }

   /* try to acquire direct buffer access */

   num_bufs = ggiDBGetNumBuffers(ggi_vis);
   avail_buf = buf_num = 0;

   for (i=0; i<num_bufs; i++) {
      ggi_db = ggiDBGetBuffer(ggi_vis, i);

      if (ggi_db->read != NULL && ggi_db->write != NULL && 
	  (ggi_db->noaccess | ggi_db->align) == 0) {

	 if (ggi_db->layout == blPixelLinearBuffer) {
	    avail_buf = LFB;
	    buf_num = i;
	    break;
	 }
	 else if (ggi_db->layout == blPixelPlanarBuffer && !avail_buf) {
	    avail_buf = PFB;
	    buf_num = i;
	 }
      }
   }

   ggi_stride = mode.virt.x * (int)((GT_DEPTH(mode.graphtype) + 7) / 8);

   if (!avail_buf || num_bufs <= 0) { 
      /* no (supported) db available */
      ggi_fake_buf = malloc(ggi_stride * mode.virt.y);
      ggi_touched = malloc(mode.virt.y);

      if (!ggi_fake_buf || !ggi_touched) {
	 if (ggi_fake_buf) free(ggi_fake_buf);
	 if (ggi_touched) free(ggi_touched);
	 ggiClose(ggi_vis);
	 ggiExit();
	 ustrcpy(allegro_error, get_config_text("Couldn't allocate screen bitmap!"));
	 return NULL;
      }
      memset(ggi_touched, 0, mode.virt.y);
      gfx_ggi.linear = TRUE;
   }
   else if (avail_buf == LFB) { 
      /* linear frame buffer is preferable */
      ggi_db = ggiDBGetBuffer(ggi_vis, buf_num);
      gfx_ggi.linear = TRUE;
   }
   else {
      /* otherwise use paged buffer */
#warning paged buffers are untested.
      ggi_db = ggiDBGetBuffer(ggi_vis, buf_num);
      gfx_ggi.linear = FALSE;
      gfx_ggi.bank_size = ggi_db->page_size;    /* is this right? FIXME */
      gfx_ggi.bank_gran = 0;                    /* what's this? FIXME */
   }

   gfx_ggi.w = ggi_w = w;
   gfx_ggi.h = ggi_h = h;
   gfx_ggi.vid_mem = ggi_stride * mode.virt.y;

   /* the b->lines should not be touched at all (without bmp_read_line, 
    * bmp_write_line) but just in case... */

   if (ggi_fake_buf) p = ggi_fake_buf;
   else p = ggi_db->write;

   b = _make_bitmap(mode.virt.x, mode.virt.y, (unsigned long)p, &gfx_ggi, 
		    GT_DEPTH(mode.graphtype), ggi_stride);
   if (!b) {
	 ggiClose(ggi_vis);
	 ggiExit();
	 ustrcpy(allegro_error, get_config_text("Couldn't allocate screen bitmap!"));
	 return NULL;
   }

   if (!ggi_fake_buf) {
#ifndef ALLEGRO_NO_ASM
      b->read_bank = ggi_read_bank_asm;
      b->write_bank = ggi_write_bank_asm;
      b->vtable->unwrite_bank = ggi_unwrite_bank_asm;
#else
      b->read_bank = ggi_read_bank;
      b->write_bank = ggi_write_bank;
      b->vtable->unwrite_bank = ggi_unwrite_bank;
#endif
   } 
   else {
#ifndef ALLEGRO_NO_ASM
      b->write_bank = ggi_write_fake_bank_asm;
      b->vtable->unwrite_bank = ggi_unfake_bank_asm;
#else
      b->write_bank = ggi_write_fake_bank;
      b->vtable->unwrite_bank = ggi_unfake_bank;
#endif
   }

   return b;
}



/* ggi_exit:
 *  Bye bye.
 */
void ggi_exit(BITMAP * b)
{
   if (ggi_fake_buf) {
      free(ggi_fake_buf);
      free(ggi_touched);
      ggi_fake_buf = NULL;
   }

   ggiClose(ggi_vis);
   ggiExit();
   ggi_virgin = TRUE;
}



/* ggi_set_palette_range:
 *  Convert Allegro's palette format (6 bit) to libggi's palette format (16 bit!)
 *  and set a range of values.
 */
void ggi_set_palette_range(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;
   for (i = from; i <= to; i++) {
      ggi_pal[i].r = p[i].r << 10;
      ggi_pal[i].g = p[i].g << 10;
      ggi_pal[i].b = p[i].b << 10;
      ggi_pal[i].a = 65535;            /* will Allegro have alpha some day? */
   }
   ggiSetPalette(ggi_vis, from, to - from + 1, ggi_pal+from);
}


#endif      /* ALLEGRO_LINUX_GGI */
