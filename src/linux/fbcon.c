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
 *      Video driver for the Linux framebuffer device.
 *
 *      By Shawn Hargreaves.
 *
 *      Proper mode setting support added by George Foot.
 *
 *      Modified by Grzegorz Adam Hankiewicz.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"


#if (defined ALLEGRO_LINUX_FBCON) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#if !defined(_POSIX_MAPPED_FILES) || !defined(HAVE_MMAP)
#error "Sorry, mapped files are required for Linux console Allegro to work!"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

/* 2.4 kernel doesn't seem to have these, so assume that they don't
 * "have sticky," heheh. */

#ifndef FB_VBLANK_HAVE_STICKY
#define FB_VBLANK_HAVE_STICKY 0
#endif

#ifndef FB_VBLANK_STICKY
#define FB_VBLANK_STICKY 0
#endif

static BITMAP *fb_init(int w, int h, int v_w, int v_h, int color_depth);
static int fb_open_device(void);
static void fb_exit(BITMAP *b);
static void fb_save(void);
static void fb_restore(void);
static int  fb_scroll(int x, int y);
static void fb_vsync(void);
static void fb_set_palette(AL_CONST RGB *p, int from, int to, int vsync);
static void fb_save_cmap(void);
static void fb_restore_cmap(void);



GFX_DRIVER gfx_fbcon = 
{
   GFX_FBCON,
   empty_string,
   empty_string,
   "fbcon", 
   fb_init,
   fb_exit,
   fb_scroll,
   fb_vsync,
   fb_set_palette,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   fb_save,
   fb_restore,
   NULL,                         /* no fetch mode hook */
   0, 0,
   TRUE,
   0, 0, 0, 0, FALSE
};


static char fb_desc[256] = EMPTY_STRING;     /* description string */

static struct fb_fix_screeninfo fix_info;    /* read-only video mode info */
static struct fb_var_screeninfo orig_mode;   /* original video mode info */
static struct fb_var_screeninfo my_mode;     /* my video mode info */

static void *fbaddr;                         /* frame buffer address */

static int fbfd;                             /* file descriptor */

static int fb_approx;                        /* emulate small resolution */

#ifdef FBIOGET_VBLANK
   static int vblank_flags;                  /* supports retrace detection? */
#endif



static int update_timings(struct fb_var_screeninfo *mode);



/* fb_init:
 *  Sets a graphics mode.
 */
static BITMAP *fb_init(int w, int h, int v_w, int v_h, int color_depth)
{
   AL_CONST char *p;
   int stride, tries, original_color_depth = _color_depth;
   BITMAP *b;
   char tmp[16];

   /* open framebuffer and store info in global variables */
   if (fb_open_device() != 0)
      return NULL;

   /* look for a nice graphics mode in several passes */
   fb_approx = FALSE;

   ASSERT (w >= 0);
   ASSERT (h >= 0);

   /* preset a resolution if the user didn't ask for one */
   if (((!w) && (!h)) || _safe_gfx_mode_change) {
      w = orig_mode.xres;
      h = orig_mode.yres;
   }

   if (_safe_gfx_mode_change) tries = -1;
   else tries = 0;
   
   for (; tries<3; tries++) {
      my_mode = orig_mode;

      switch (tries) {

	 case -1:
	    /* let's see if we can get the actual screen mode */
	    /* shouldn't we be keeping the previous color depth setting? */
	    switch (orig_mode.bits_per_pixel) {
	       case 8:
	       case 16:
	       case 24:
	       case 32:
		  color_depth = orig_mode.bits_per_pixel;
		  set_color_depth(color_depth);
	       default:
	    }
	    break;
	 case 0:
	    /* try for largest possible virtual screen size */
	    my_mode.xres = w;
	    my_mode.yres = h;
	    my_mode.xres_virtual = MAX(w, v_w);
	    my_mode.yres_virtual = MAX(MAX(h, v_h), fix_info.smem_len / (my_mode.xres_virtual * BYTES_PER_PIXEL(color_depth)));
	    break;

	 case 1:
	    /* try setting the exact size that was requested */
	    my_mode.xres = w;
	    my_mode.yres = h;
	    my_mode.xres_virtual = MAX(w, v_w);
	    my_mode.yres_virtual = MAX(h, v_h);
	    break;

	 case 2:
	    /* see if we can fake a smaller mode (better than nothing) */
	    if ((my_mode.xres < w) || (my_mode.yres < h) || (v_w > w) || (v_h > h))
	       continue;
	    fb_approx = TRUE;
	    break;
      }

      my_mode.bits_per_pixel = color_depth;
      my_mode.grayscale = 0;
      my_mode.xoffset = 0;
      my_mode.yoffset = 0;

      switch (color_depth) {

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	       my_mode.red.offset = 10;
	       my_mode.red.length = 5;
	       my_mode.green.offset = 5;
	       my_mode.green.length = 5;
	       my_mode.blue.offset = 0;
	       my_mode.blue.length = 5;
	       break;

	    case 16:
	       my_mode.red.offset = 11;
	       my_mode.red.length = 5;
	       my_mode.green.offset = 5;
	       my_mode.green.length = 6;
	       my_mode.blue.offset = 0;
	       my_mode.blue.length = 5;
	       break;

	 #endif

	 #if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

	    case 24:
	    case 32:
	       my_mode.red.offset = 16;
	       my_mode.red.length = 8;
	       my_mode.green.offset = 8;
	       my_mode.green.length = 8;
	       my_mode.blue.offset = 0;
	       my_mode.blue.length = 8;
	       break;

	 #endif

	 case 8:
	 default:
	    my_mode.red.offset = 0;
	    my_mode.red.length = 0;
	    my_mode.green.offset = 0;
	    my_mode.green.length = 0;
	    my_mode.blue.offset = 0;
	    my_mode.blue.length = 0;
	    break;
      }

      my_mode.red.msb_right = 0;
      my_mode.green.msb_right = 0;
      my_mode.blue.msb_right = 0;

      /* fill in the timings */
      if (update_timings(&my_mode) != 0)
	 continue;

      /* try to set the mode */
      if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &my_mode) == 0)
	 goto got_a_nice_mode;
   }

   /* oops! */
   if (_safe_gfx_mode_change)
      set_color_depth(original_color_depth);
      
   ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_mode);
   close(fbfd);
   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Framebuffer resolution not available"));
   return NULL;

   got_a_nice_mode:

   /* map the framebuffer */
   fbaddr = mmap(NULL, fix_info.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
   if (fbaddr == MAP_FAILED) {
      ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_mode);
      close(fbfd);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can't map framebuffer"));
      return NULL;
   }

   /* set up the screen bitmap */
   gfx_fbcon.w = w;
   gfx_fbcon.h = h;

   gfx_fbcon.vid_mem = fix_info.smem_len;

   stride = my_mode.xres_virtual * BYTES_PER_PIXEL(color_depth);
   v_w = my_mode.xres_virtual;
   v_h = my_mode.yres_virtual;
   p = fbaddr;

   if (fb_approx) {
      v_w = w;
      v_h = h;

      p += (my_mode.xres-w)/2 * BYTES_PER_PIXEL(color_depth) + 
	   (my_mode.yres-h)/2 * stride;
   }

   b = _make_bitmap(v_w, v_h, (unsigned long)p, &gfx_fbcon, color_depth, stride);
   if (!b) {
      ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_mode);
      munmap(fbaddr, fix_info.smem_len);
      close(fbfd);
      return NULL;
   }

   b->vtable->acquire = __al_linux_acquire_bitmap;
   b->vtable->release = __al_linux_release_bitmap;

   do_uconvert(fix_info.id, U_ASCII, fb_desc, U_CURRENT, sizeof(fb_desc));

   if (fb_approx) {
      ustrzcat(fb_desc, sizeof(fb_desc), uconvert_ascii(", ", tmp));
      ustrzcat(fb_desc, sizeof(fb_desc), get_config_text("approx."));
   }

   gfx_fbcon.desc = fb_desc;

   /* set up the truecolor pixel format */
   switch (color_depth) {

      #ifdef ALLEGRO_COLOR16

	 case 15:
	    _rgb_r_shift_15 = my_mode.red.offset; 
	    _rgb_g_shift_15 = my_mode.green.offset;
	    _rgb_b_shift_15 = my_mode.blue.offset;
	    break;

	 case 16:
	    _rgb_r_shift_16 = my_mode.red.offset; 
	    _rgb_g_shift_16 = my_mode.green.offset;
	    _rgb_b_shift_16 = my_mode.blue.offset;
	    break;

      #endif

      #ifdef ALLEGRO_COLOR24

	 case 24:
	    _rgb_r_shift_24 = my_mode.red.offset; 
	    _rgb_g_shift_24 = my_mode.green.offset;
	    _rgb_b_shift_24 = my_mode.blue.offset;
	    break;

      #endif

      #ifdef ALLEGRO_COLOR32

	 case 32:
	    _rgb_r_shift_32 = my_mode.red.offset; 
	    _rgb_g_shift_32 = my_mode.green.offset;
	    _rgb_b_shift_32 = my_mode.blue.offset;
	    break;

      #endif
   }

   /* is retrace detection available? */
 #ifdef FBIOGET_VBLANK
   {
      struct fb_vblank vblank;

      if (ioctl(fbfd, FBIOGET_VBLANK, &vblank) == 0)
	 vblank_flags = vblank.flags;
      else
	 vblank_flags = 0;
   }

   if (!(vblank_flags & (FB_VBLANK_HAVE_VBLANK | FB_VBLANK_HAVE_STICKY | FB_VBLANK_HAVE_VCOUNT)))
 #endif
   {
      ustrzcat(fb_desc, sizeof(fb_desc), uconvert_ascii(", ", tmp));
      ustrzcat(fb_desc, sizeof(fb_desc), get_config_text("no vsync"));
   }

   /* is scrolling available? */
   if ((my_mode.xres_virtual > my_mode.xres) ||
       (my_mode.yres_virtual > my_mode.yres)) {

      fb_scroll(0, 0);

      if ((fb_approx) ||
	  ((my_mode.xres_virtual > my_mode.xres) && (!fix_info.xpanstep)) ||
	  ((my_mode.yres_virtual > my_mode.yres) && (!fix_info.ypanstep)))
	 gfx_fbcon.scroll = NULL;
      else
	 gfx_fbcon.scroll = fb_scroll;
   }
   else
      gfx_fbcon.scroll = NULL;

   __al_linux_console_graphics();

   if (fb_approx)
      memset(fbaddr, 0, gfx_fbcon.vid_mem);

   fb_save_cmap();    /* Maybe we should fill in our default palette too... */
   return b;
}



/* fb_open_device:
 *  Opens the framebuffer device, first checking config values or
 *  environment variables. Returns 0 on success.
 */
static int fb_open_device(void)
{
   char fname[1024], tmp1[256], tmp2[256];
   AL_CONST char *p;

   /* find the device filename */
   p = get_config_string(uconvert_ascii("graphics", tmp1), uconvert_ascii("framebuffer", tmp2), NULL);

   if (p && ugetc(p))
      do_uconvert(p, U_CURRENT, fname, U_ASCII, sizeof(fname));
   else {
      p = getenv("FRAMEBUFFER");

      if ((p) && (p[0])) {
	 strncpy(fname, p, sizeof(fname)-1);
	 fname[sizeof(fname)-1] = 0;
      }
      else
	 strcpy(fname, "/dev/fb0");
   }

   /* open the framebuffer device */
   if ((fbfd = open(fname, O_RDWR)) < 0) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can't open framebuffer %s"), uconvert_ascii(fname, tmp1));
      return 1;
   }

   /* read video mode information */
   if ((ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) != 0) ||
       (ioctl(fbfd, FBIOGET_VSCREENINFO, &orig_mode) != 0)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Framebuffer ioctl() failed"));
      return 2;
   }

   return 0;
}



/* fb_exit:
 *  Unsets the video mode.
 */
static void fb_exit(BITMAP *b)
{
   ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_mode);
   fb_restore_cmap();

   munmap(fbaddr, fix_info.smem_len);
   close(fbfd);

   __al_linux_console_text();
}



/* fb_save:
 *  Saves the graphics state.
 */
static void fb_save()
{
   ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_mode);
}



/* fb_restore:
 *  Restores the graphics state.
 */
static void fb_restore()
{
   ioctl(fbfd, FBIOPUT_VSCREENINFO, &my_mode);

   if (fb_approx)
      memset(fbaddr, 0, gfx_fbcon.vid_mem);
}



/* fb_scroll:
 *  Hardware scrolling routine.
 */
static int fb_scroll(int x, int y)
{
   int ret;

   my_mode.xoffset = x;
   my_mode.yoffset = y;

   ret = ioctl(fbfd, FBIOPAN_DISPLAY, &my_mode);

   fb_vsync();

 #ifdef FBIOGET_VBLANK
   if (!vblank_flags)
 #endif
      fb_vsync();

   return (ret) ? -1 : 0;
}



/* fb_vsync:
 *  Waits for a retrace.
 */
static void fb_vsync()
{
   int prev;

 #ifdef FBIOGET_VBLANK

   struct fb_vblank vblank;

   if (vblank_flags & FB_VBLANK_HAVE_STICKY) {
      /* it's really good if sticky bits are available */
      if (ioctl(fbfd, FBIOGET_VBLANK, &vblank) != 0)
	 return;

      do { 
	 if (ioctl(fbfd, FBIOGET_VBLANK, &vblank) != 0)
	    break;
      } while (!(vblank.flags & FB_VBLANK_STICKY));
   }
   else if (vblank_flags & FB_VBLANK_HAVE_VCOUNT) {
      /* we can read the exact scanline position, which avoids skipping */
      if (ioctl(fbfd, FBIOGET_VBLANK, &vblank) != 0)
	 return;

      do {
	 prev = vblank.vcount;
	 if (ioctl(fbfd, FBIOGET_VBLANK, &vblank) != 0)
	    break;
      } while (vblank.vcount >= prev);
   }
   else if (vblank_flags & FB_VBLANK_HAVE_VBLANK) {
      /* boring, normal style poll operation */
      do {
	 if (ioctl(fbfd, FBIOGET_VBLANK, &vblank) != 0)
	    break;
      } while (vblank.flags & FB_VBLANK_VBLANKING);

      do {
	 if (ioctl(fbfd, FBIOGET_VBLANK, &vblank) != 0)
	    break;
      } while (!(vblank.flags & FB_VBLANK_VBLANKING));
   }
   else

 #endif

   /* bodged implementation for when the framebuffer doesn't support it */
   if (_timer_installed) {
      prev = retrace_count;

      do {
      } while (retrace_count == prev);
   }
}



/* fb_set_palette:
 *  Sets the palette.
 */
static void fb_set_palette(AL_CONST RGB *p, int from, int to, int vsync)
{
   unsigned short r[256], g[256], b[256];
   struct fb_cmap cmap;
   int i;

   cmap.start = from;
   cmap.len = to-from+1;
   cmap.red = r;
   cmap.green = g;
   cmap.blue = b;
   cmap.transp = NULL;

   for (i=0; i<cmap.len; i++) {
      r[i] = p[from+i].r << 10;
      g[i] = p[from+i].g << 10;
      b[i] = p[from+i].b << 10;
   }

   fb_vsync();

   ioctl(fbfd, FBIOPUTCMAP, &cmap);
}



static unsigned short *orig_cmap_data;      /* original palette data */

/* fb_do_cmap:
 *  Helper for fb_{save|restore}_cmap.
 */
static void fb_do_cmap (int ioctlno)
{
	struct fb_cmap cmap;
	cmap.start = 0;
	cmap.len = 256;
	cmap.red = orig_cmap_data;
	cmap.green = orig_cmap_data+256;
	cmap.blue = orig_cmap_data+512;
	cmap.transp = NULL;
	ioctl(fbfd, ioctlno, &cmap);
}



/* fb_{save|restore}_cmap:
 *  Routines to save and restore the whole palette.
 */
static void fb_save_cmap (void)
{
	if (orig_cmap_data) free (orig_cmap_data);   /* can't happen */
	orig_cmap_data = malloc (sizeof *orig_cmap_data* 768);
	if (orig_cmap_data)
		fb_do_cmap (FBIOGETCMAP);
}



static void fb_restore_cmap (void)
{
	if (orig_cmap_data) {
		fb_do_cmap (FBIOPUTCMAP);
		free (orig_cmap_data);
		orig_cmap_data = NULL;
	}
}



static struct timings {
   char config_item[1024];
   int pixclock;
   int left_margin;
   int right_margin;
   int upper_margin;
   int lower_margin;
   int hsync_len;
   int vsync_len;
   int vmode;
   int sync;
   int xres;
   int yres;
} _fb_current_timings;

static struct timings temp_timings;


static int read_config_file (int w, int h);
static int read_fbmodes_file (int w, int h);
static int tweak_timings (int w, int h);


/* _fb_get_timings:
 *  Returns a pointer to a struct as above containing timings for the given
 *  resolution, or NULL on error.
 */
static struct timings *_fb_get_timings (int w, int h)
{
   /* First try the config file */
   if (read_config_file (w, h)) return &temp_timings;

   /* Failing that, try fb.modes */
   if (read_fbmodes_file (w, h)) return &temp_timings;

   /* Still no luck, so tweak the current mode instead */
   if (tweak_timings (w, h)) return &temp_timings;
   return NULL;
}


/* read_config_file:
 *  Assigns timing settings from the config file or returns 0.
 */
static int read_config_file (int w, int h)
{
   char tmp[128];
   char **argv;
   int argc;

   /* Let the setup program know what config string we read for this mode */
   uszprintf(temp_timings.config_item, sizeof(temp_timings.config_item), uconvert_ascii("fb_mode_%dx%d", tmp), w, h);

   /* First try the config file */
   argv = get_config_argv (NULL, temp_timings.config_item, &argc);
   if (argv) {
      #define get_info(info) if (*argv) temp_timings.info = ustrtol (*argv++, NULL, 10)
      get_info(pixclock);
      get_info(left_margin);
      get_info(right_margin);
      get_info(upper_margin);
      get_info(lower_margin);
      get_info(hsync_len);
      get_info(vsync_len);

      if (*argv) {
	 if (!ustrcmp (*argv, uconvert_ascii("none", tmp)))
	    temp_timings.vmode = FB_VMODE_NONINTERLACED;
	 else if (!ustrcmp (*argv, uconvert_ascii("interlaced", tmp)))
	    temp_timings.vmode = FB_VMODE_INTERLACED;
	 else if (!ustrcmp (*argv, uconvert_ascii("doublescan", tmp)))
	    temp_timings.vmode = FB_VMODE_DOUBLE;
	 argv++;
      } else
	 temp_timings.vmode = FB_VMODE_NONINTERLACED;

      get_info(sync);
      #undef get_info

      temp_timings.xres = w;
      temp_timings.yres = h;

      return 1;
   }
   return 0;
}


/* helper to read the relevant parts of a line from fb.modes */
static char *get_line (FILE *file)
{
   static char buffer[1024];
   char *ch;

   ASSERT (file);
   if (!fgets (buffer, sizeof (buffer), file))
      return 0;

   /* if there's no eol, get one before continuing */
   if (!strchr (buffer, '\n') && strlen (buffer) == 1 + sizeof (buffer)) {
      char waste[128], *ret;

      do {
	 ret = fgets (waste, sizeof (waste), file);
      } while (ret && !strchr (waste, '\n'));
      /* this doesn't actually exit because we still have buffer */
   }
   
   if ((ch = strpbrk (buffer, "#\n")))
      *ch = 0;
      
   ch = buffer;
   while (uisspace(*ch)) ch++;
   return ch;
}

/* read_fbmodes_file:
 *  Assigns timing settings from the fbmodes file or returns 0.
 */
static int read_fbmodes_file (int w, int h)
{
   char *mode_id = NULL;
   char *geometry = NULL;
   char *timings = NULL;
   int sync = 0, vmode = 0;
   char *s, *t;
   FILE *fbmodes;
   int ret = 0;

   fbmodes = fopen ("/etc/fb.modes", "r");
   if (!fbmodes) return 0;

   do {
      s = get_line (fbmodes);
      if (!s) break;
      t = strchr (s, ' ');
      if (t) {
	 *t++ = '\0';
	 while (uisspace(*t)) t++;
      } else {
	 t = strchr (s, '\0');
      }

      if (!strcmp (s, "mode")) {
	 free (mode_id);
	 free (geometry);
	 free (timings);
	 mode_id = strdup (t);
	 geometry = timings = NULL;
	 sync = 0;
	 vmode = FB_VMODE_NONINTERLACED;
      } else if (!strcmp (s, "endmode")) {
	 if (geometry && timings) {
	    int mw, mh;
	    sscanf (geometry, "%d %d", &mw, &mh);
	    if ((mw == w) && (mh == h)) {
	       sscanf (timings, "%d %d %d %d %d %d %d",
		  &temp_timings.pixclock,
		  &temp_timings.left_margin,
		  &temp_timings.right_margin,
		  &temp_timings.upper_margin,
		  &temp_timings.lower_margin,
		  &temp_timings.hsync_len,
		  &temp_timings.vsync_len
	       );
	       temp_timings.sync = sync;
	       temp_timings.vmode = vmode;
	       temp_timings.xres = w;
	       temp_timings.yres = h;
	       ret = 1;
	       s = NULL;
	    }
	 }
	 free (mode_id);
	 free (geometry);
	 free (timings);
	 mode_id = geometry = timings = NULL;
      } else if (!strcmp (s, "geometry")) {
	 free (geometry);
	 geometry = strdup (t);
      } else if (!strcmp (s, "timings")) {
	 free (timings);
	 timings = strdup (t);
      } else if (!strcmp (s, "hsync")) {
	 #define set_bit(var,bit,on) var = ((var) &~ (bit)) | ((on) ? bit : 0)
	 set_bit (sync, FB_SYNC_HOR_HIGH_ACT, t[0] == 'h');
      } else if (!strcmp (s, "vsync")) {
	 set_bit (sync, FB_SYNC_VERT_HIGH_ACT, t[0] == 'h');
      } else if (!strcmp (s, "csync")) {
	 set_bit (sync, FB_SYNC_COMP_HIGH_ACT, t[0] == 'h');
      } else if (!strcmp (s, "gsync")) {
	 set_bit (sync, FB_SYNC_ON_GREEN, t[0] == 'h');
      } else if (!strcmp (s, "extsync")) {
	 set_bit (sync, FB_SYNC_EXT, t[0] == 't');
      } else if (!strcmp (s, "bcast")) {
	 set_bit (sync, FB_SYNC_BROADCAST, t[0] == 't');
      } else if (!strcmp (s, "laced")) {
	 if (t[0] == 't') vmode = FB_VMODE_INTERLACED;
      } else if (!strcmp (s, "double")) {
	 if (t[0] == 't') vmode = FB_VMODE_DOUBLE;
      }
   } while (s);

   free (mode_id);
   free (geometry);
   free (timings);

   fclose (fbmodes);
   return ret;
}


/* tweak_timings:
 *  Tweak the timings to match the mode we want to set.  Only works if
 *  the parent is a higher resolution.
 */
static int tweak_timings (int w, int h)
{
   if ((w <= temp_timings.xres) && (h <= temp_timings.yres)) {
      int diff = temp_timings.xres - w;
      temp_timings.left_margin += diff/2;
      temp_timings.right_margin += diff/2 + diff%2;
      temp_timings.xres = w;

      diff = temp_timings.yres - h;
      temp_timings.upper_margin += diff/2;
      temp_timings.lower_margin += diff/2 + diff%2;
      temp_timings.yres = h;

      return 1;
   }
   return 0;
}


static void set_default_timings (void)
{
   char tmp[128];

   #define cp(x) temp_timings.x = orig_mode.x
   cp(pixclock);
   cp(left_margin);
   cp(right_margin);
   cp(upper_margin);
   cp(lower_margin);
   cp(hsync_len);
   cp(vsync_len);
   cp(vmode);
   cp(sync);
   cp(xres);
   cp(yres);
   #undef cp
   uszprintf(temp_timings.config_item, sizeof(temp_timings.config_item), uconvert_ascii("fb_mode_%dx%d", tmp),
	     orig_mode.xres, orig_mode.yres);
}


/* update_timings:
 *  Updates the timing section of the mode info.  Maybe we can make
 *  this algorithmic, as a backup, at some point.  For now it searches
 *  the config file or /etc/fb.modes for the data.
 * 
 *  We could make the init routine give up if the data isn't there, or 
 *  use an algorithmic guesser. 
 * 
 *  If we go right ahead with this system, I think the setup program 
 *  should offer quite a few options -- dotclock probing might be useful,
 *  along with the functionality of xvidtune.
 */
static int update_timings(struct fb_var_screeninfo *mode)
{
   struct timings *t;

   set_default_timings();
   t = _fb_get_timings (mode->xres, mode->yres);
   if (!t) return -1;

   /* for debugging, maybe for the setup program too */
   memcpy (&_fb_current_timings, t, sizeof(struct timings));

   /* update the mode struct */
   mode->pixclock = t->pixclock;
   mode->left_margin = t->left_margin;
   mode->right_margin = t->right_margin;
   mode->upper_margin = t->upper_margin;
   mode->lower_margin = t->lower_margin;
   mode->hsync_len = t->hsync_len;
   mode->vsync_len = t->vsync_len;
   mode->vmode = t->vmode;
   mode->sync = t->sync;

   return 0;
}


/* I'm not sure whether these work or not -- my Matrox seems capable
 * of setting whatever you ask it to. */

#if 0

static int _fb_get_pixclock(void)
{
   struct fb_var_screeninfo mode;
   if (ioctl (fbfd, FBIOGET_VSCREENINFO, &mode)) return -1;
   return mode.pixclock;
}

static void _fb_set_pixclock(int new_val)
{
   struct fb_var_screeninfo mode;
   if (ioctl (fbfd, FBIOGET_VSCREENINFO, &mode)) return;
   mode.pixclock = new_val;
   if (ioctl (fbfd, FBIOPUT_VSCREENINFO, &mode)) return;
}

#endif



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   if (system_driver == SYSTEM_LINUX)
      _unix_register_gfx_driver(GFX_FBCON, &gfx_fbcon, TRUE, FALSE);
}

#endif      /* ifdef ALLEGRO_MODULE */



#endif      /* if (defined ALLEGRO_LINUX_FBCON) ... */
