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
 *      Grabber plugin for managing font objects.
 *
 *      By Shawn Hargreaves.
 *
 *      GRX font file reader by Mark Wodrich.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "allegro/aintern.h"
#include "../datedit.h"



/* creates a new font object */
static void *makenew_font(long *size)
{
   FONT *f = NULL;
   FONT *fnt = NULL;
   FONT *df = font;
   FONT_GLYPH *g, *dg;
   int count, c, s;

   while (df) {
      if (fnt) {
	 fnt->next = _al_malloc(sizeof(FONT));
	 fnt = fnt->next;
      }
      else
	 f = fnt = _al_malloc(sizeof(FONT));

      fnt->mono = df->mono;
      fnt->start = df->start;
      fnt->end = df->end;
      fnt->next = NULL;
      fnt->renderhook = NULL;
      fnt->widthhook = NULL;
      fnt->heighthook = NULL;

      count = fnt->end-fnt->start+1;

      fnt->glyphs = _al_malloc(count*sizeof(void *));

      for (c=0; c<count; c++) {
	 dg = df->glyphs[c];
	 s = ((dg->w+7)/8) * dg->h;

	 g = _al_malloc(sizeof(FONT_GLYPH)+s);

	 g->w = dg->w;
	 g->h = dg->h;

	 memcpy(g->dat, dg->dat, s);

	 fnt->glyphs[c] = g;
      }

      df = df->next;
   }

   return f;
}



/* displays a font in the grabber object view window */
static void plot_font(AL_CONST DATAFILE *dat, int x, int y)
{
   FONT *f = dat->dat;
   int h = text_height(f) * 3/2;
   char buf[1024];
   int bufpos;
   int c = f->start;

   y += 32;

   while (((c <= f->end) || (f->next)) && (y<SCREEN_H-h)) {
      bufpos = 0;
      usetc(buf, 0);

      while (((c <= f->end) || (f->next)) && (text_length(dat->dat, buf) < SCREEN_W-x-h*2)) {
	 bufpos += usetc(buf+bufpos, c);
	 bufpos += usetc(buf+bufpos, ' ');
	 usetc(buf+bufpos, 0);

	 c++;

	 if ((c > f->end) && (f->next)) {
	    f = f->next;
	    c = f->start;
	 }
      }

      textout(screen, dat->dat, buf, x, y, gui_fg_color);
      y += h;
   }
}



/* returns a description string for a font object */
static void get_font_desc(AL_CONST DATAFILE *dat, char *s)
{
   FONT *fnt = (FONT *)dat->dat;
   char *mono = fnt->mono ? "mono" : "color";
   int ranges = 0;
   int glyphs = 0;

   while (fnt) {
      ranges++;
      glyphs += (fnt->end - fnt->start + 1);
      fnt = fnt->next;
   }

   sprintf(s, "%s font, %d range%s, %d glyphs", mono, ranges, (ranges==1) ? "" : "s", glyphs);
}



/* exports a font into an external file */
static int export_font(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   char buf[256], tmp[256];
   FONT *f;
   FONT_GLYPH *g;
   BITMAP *b;
   PALETTE pal;
   PACKFILE *file;
   int w, h, c;
   int chars;
   int col;

   if (stricmp(get_extension(filename), "txt") == 0) {
      replace_extension(buf, filename, "pcx", sizeof(buf));

      if (exists(buf)) {
	 c = datedit_ask("%s already exists, overwrite", buf);
	 if ((c == 27) || (c == 'n') || (c == 'N'))
	    return TRUE;
      }

      file = pack_fopen(filename, F_WRITE);
      if (!file)
	 return FALSE;

      f = dat->dat;

      while (f) {
	 sprintf(tmp, "%s 0x%04X 0x%04X\n", (f == dat->dat) ? get_filename(buf) : "-", f->start, f->end);
	 pack_fputs(tmp, file);
	 f = f->next;
      }

      pack_fclose(file);

      filename = buf;
   }
   else {
      f = dat->dat;
      if (f->next) {
	 c = datedit_ask("Really export multi-range font as bitmap rather than .txt");
	 if ((c == 27) || (c == 'n') || (c == 'N'))
	    return TRUE;
      }
   }

   w = 0;
   h = 0;

   f = dat->dat;
   chars = 0;

   while (f) {
      for (c=f->start; c<=f->end; c++) {
	 if (f->mono) {
	    g = f->glyphs[c-f->start];
	    if (g->w > w)
	       w = g->w;
	    if (g->h > h)
	       h = g->h;
	 }
	 else {
	    b = f->glyphs[c-f->start];
	    if (b->w > w)
	       w = b->w;
	    if (b->h > h)
	       h = b->h;
	 }

	 chars++;
      }

      f = f->next;
   }

   w = (w+16) & 0xFFF0;
   h = (h+16) & 0xFFF0;

   b = create_bitmap_ex(8, 1+w*16, 1+h*((chars+15)/16));
   rectfill(b, 0, 0, b->w, b->h, 255);
   text_mode(0);

   f = dat->dat;
   chars = 0;

   while (f) {
      col = (f->mono) ? 1 : -1;

      for (c=f->start; c<=f->end; c++) {
	 textprintf(b, f, 1+w*(chars&15), 1+h*(chars/16), col, "%c", c);
	 chars++;
      }

      f = f->next;
   }

   pal[0].r = 63;
   pal[0].g = 0;
   pal[0].b = 63;

   for (c=1; c<255; c++) {
      col = (c-1)*63/253;
      pal[c].r = col;
      pal[c].g = col;
      pal[c].b = col;
   }

   pal[255].r = 63;
   pal[255].g = 63;
   pal[255].b = 0;

   save_bitmap(filename, b, pal);
   destroy_bitmap(b);

   return (errno == 0);
}



/* magic number for GRX format font files */
#define FONTMAGIC    0x19590214L



/* import routine for the GRX font format */
static FONT *import_grx_font(AL_CONST char *filename)
{
   PACKFILE *f, *cf;
   FONT *fnt;
   FONT_GLYPH *g;
   char copyright[256];
   int *wtable;
   int width, height;
   int minchar, maxchar, numchar;
   int isfixed;
   int c, s;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   if (pack_igetl(f) != FONTMAGIC) {
      pack_fclose(f);
      return NULL;
   }

   pack_igetl(f);

   width = pack_igetw(f);
   height = pack_igetw(f);
   minchar = pack_igetw(f);
   maxchar = pack_igetw(f);
   isfixed = pack_igetw(f);

   for (c=0; c<38; c++)
      pack_getc(f);

   numchar = maxchar-minchar+1;

   if (!isfixed) {
      wtable = malloc(sizeof(int) * numchar);
      for (c=0; c<numchar; c++)
	 wtable[c] = pack_igetw(f);
   }
   else
      wtable = NULL;

   fnt = _al_malloc(sizeof(FONT));
   fnt->mono = TRUE;
   fnt->start = minchar;
   fnt->end = maxchar;
   fnt->next = NULL;
   fnt->renderhook = NULL;
   fnt->widthhook = NULL;
   fnt->heighthook = NULL;

   fnt->glyphs = _al_malloc(numchar*sizeof(void *));

   for (c=0; c<numchar; c++) {
      if (wtable)
	 width = wtable[c];

      s = ((width+7)/8) * height;

      g = _al_malloc(sizeof(FONT_GLYPH) + s);

      g->w = width;
      g->h = height;

      pack_fread(g->dat, s, f);

      fnt->glyphs[c] = g;
   }

   if (!pack_feof(f)) {
      strcpy(copyright, filename);
      strcpy(get_extension(copyright), "txt");
      c = datedit_ask("Save font copyright message into '%s'", copyright);
      if ((c != 27) && (c != 'n') && (c != 'N')) {
	 cf = pack_fopen(copyright, F_WRITE);
	 if (cf) {
	    while (!pack_feof(f)) {
	       pack_fgets(copyright, sizeof(copyright)-1, f);
	       pack_fputs(copyright, cf);
	       pack_fputs("\n", cf);
	    }
	    pack_fclose(cf);
	 }
      }
   }

   pack_fclose(f);

   if (wtable)
      free(wtable);

   return fnt;
}



/* import routine for the 8x8 and 8x16 BIOS font formats */
static FONT *import_bios_font(AL_CONST char *filename)
{
   unsigned char data[256][16];
   PACKFILE *f;
   FONT *fnt;
   FONT_GLYPH *g;
   int c, y, h;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   h = (f->todo == 2048) ? 8 : 16;

   memset(data, 0, sizeof(data));

   for (c=0; c<256; c++) {
      for (y=0; y<h; y++)
	 data[c][y] = pack_getc(f);
   }

   pack_fclose(f);

   h = 0;

   for (c=1; c<256; c++) {
      for (y=15; y>=0; y--) {
	 if (data[c][y])
	    break;
      }

      y++;

      if (y > h)
	 h = y;
   }

   fnt = _al_malloc(sizeof(FONT));
   fnt->mono = TRUE;
   fnt->start = 1;
   fnt->end = 255;
   fnt->next = NULL;
   fnt->renderhook = NULL;
   fnt->widthhook = NULL;
   fnt->heighthook = NULL;

   fnt->glyphs = _al_malloc(255*sizeof(void *));

   for (c=1; c<256; c++) {
      g = _al_malloc(sizeof(FONT_GLYPH) + h);
      g->w = 8;
      g->h = h;

      for (y=0; y<h; y++)
	 g->dat[y] = data[c][y];

      fnt->glyphs[c-1] = g;
   }

   return fnt;
}



/* state information for the bitmap font importer */
static BITMAP *import_bmp = NULL;

static int import_x = 0;
static int import_y = 0;



/* import routine for the Allegro .pcx font format */
static FONT *import_bitmap_font(AL_CONST char *filename, int minchar, int maxchar, int cleanup)
{
   BITMAP **bmp;
   PALETTE junk;
   FONT *fnt;
   FONT_GLYPH *g;
   int x, y, w, h;
   int c, d;
   int max_h;
   int stride;
   int col;

   if (filename) {
      if (import_bmp)
	 destroy_bitmap(import_bmp);

      import_bmp = load_bitmap(filename, junk);

      import_x = 0;
      import_y = 0;
   }

   if (!import_bmp)
      return NULL;

   if (bitmap_color_depth(import_bmp) != 8) {
      destroy_bitmap(import_bmp);
      import_bmp = NULL;
      return NULL;
   }

   bmp = malloc(65536*sizeof(BITMAP *));
   max_h = 0;
   col = 0;
   c = 0;

   for (;;) {
      datedit_find_character(import_bmp, &import_x, &import_y, &w, &h);

      if ((w <= 0) || (h <= 0))
	 break;

      bmp[c] = create_bitmap_ex(8, w, h);
      blit(import_bmp, bmp[c], import_x+1, import_y+1, 0, 0, w, h);
      max_h = MAX(max_h, h);
      import_x += w;

      if (col >= 0) {
	 for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	       if (bmp[c]->line[y][x]) {
		  if ((col) && (col != bmp[c]->line[y][x]))
		     col = -1;
		  else
		     col = bmp[c]->line[y][x];
	       }
	    }
	 }
      }

      c++;

      if ((maxchar > 0) && (minchar+c > maxchar))
	 break;
   }

   if (c > 0) {
      fnt = _al_malloc(sizeof(FONT));
      fnt->mono = (col >= 0) ? TRUE : FALSE;
      fnt->start = minchar;
      fnt->end = minchar+c-1;
      fnt->next = NULL;
      fnt->renderhook = NULL;
      fnt->widthhook = NULL;
      fnt->heighthook = NULL;

      fnt->glyphs = _al_malloc(c*sizeof(void *));

      for (d=0; d<c; d++) {
	 w = bmp[d]->w;
	 h = max_h;

	 if (col >= 0) {
	    stride = (w+7)/8;

	    g = _al_malloc(sizeof(FONT_GLYPH) + stride*h);
	    g->w = w;
	    g->h = h;

	    memset(g->dat, 0, stride*h);

	    for (y=0; y<bmp[d]->h; y++) {
	       for (x=0; x<bmp[d]->w; x++) {
		  if (bmp[d]->line[y][x])
		     g->dat[y*stride + x/8] |= (0x80 >> (x&7));
	       }
	    }

	    fnt->glyphs[d] = g;
	 }
	 else {
	    fnt->glyphs[d] = create_bitmap_ex(8, w, h);
	    clear(fnt->glyphs[d]);
	    blit(bmp[d], fnt->glyphs[d], 0, 0, 0, 0, w, h);
	 }
      }
   }
   else
      fnt = NULL;

   for (d=0; d<c; d++)
      destroy_bitmap(bmp[d]);

   free(bmp);

   if (cleanup) {
      destroy_bitmap(import_bmp);
      import_bmp = NULL;
   }

   return fnt;
}



/* import routine for the multiple range .txt font format */
static FONT *import_scripted_font(AL_CONST char *filename)
{
   char buf[256], *bmp_str, *start_str, *end_str;
   FONT *fnt = NULL;
   FONT *range, *r;
   PACKFILE *f;
   int start, end;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   while (pack_fgets(buf, sizeof(buf)-1, f)) {
      bmp_str = strtok(buf, " \t");
      if (!bmp_str)
	 goto badness;

      start_str = strtok(NULL, " \t");
      if (!start_str)
	 goto badness;

      end_str = strtok(NULL, " \t");

      if (bmp_str[0] == '-')
	 bmp_str = NULL;

      start = strtol(start_str, NULL, 0);

      if (end_str)
	 end = strtol(end_str, NULL, 0);
      else
	 end = -1;

      if ((start <= 0) || ((end > 0) && (end < start)))
	 goto badness;

      range = import_bitmap_font(bmp_str, start, end, FALSE);

      if (!range) {
	 if (bmp_str)
	    datedit_error("Unable to read font images from %s", bmp_str);
	 else
	    datedit_error("Unable to read continuation font images");

	 destroy_font(fnt);
	 fnt = NULL;
	 break;
      }

      if (fnt) {
	 r = fnt;
	 while (r->next)
	    r = r->next;
	 r->next = range;
      }
      else
	 fnt = range;
   }

   pack_fclose(f);

   destroy_bitmap(import_bmp);
   import_bmp = NULL;

   return fnt;

   badness:

   datedit_error("Bad font description (expecting 'file.pcx start end')");

   destroy_font(fnt);

   pack_fclose(f);
   destroy_bitmap(import_bmp);

   import_bmp = NULL;

   return NULL;
}



/* imports a font from an external file (handles various formats) */
static void *grab_font(AL_CONST char *filename, long *size, int x, int y, int w, int h, int depth)
{
   PACKFILE *f;
   int id;

   if (stricmp(get_extension(filename), "fnt") == 0) {
      f = pack_fopen(filename, F_READ);
      if (!f)
	 return NULL;

      id = pack_igetl(f);
      pack_fclose(f);

      if (id == FONTMAGIC)
	 return import_grx_font(filename);
      else
	 return import_bios_font(filename);
   }
   else if (stricmp(get_extension(filename), "txt") == 0) {
      return import_scripted_font(filename);
   }
   else {
      return import_bitmap_font(filename, ' ', -1, TRUE);
   }
}



/* saves a font into a datafile */
static void save_font(DATAFILE *dat, int packed, int packkids, int strip, int verbose, int extra, PACKFILE *f)
{
   FONT *fnt;
   FONT_GLYPH *g;
   BITMAP *b;
   int c, x, y;

   fnt = dat->dat;
   c = 0;

   while (fnt) {
      c++;
      fnt = fnt->next;
   }

   pack_mputw(0, f);
   pack_mputw(c, f);

   fnt = dat->dat;

   while (fnt) {
      pack_putc(fnt->mono, f);
      pack_mputl(fnt->start, f);
      pack_mputl(fnt->end, f);

      for (c=fnt->start; c<=fnt->end; c++) {
	 if (fnt->mono) {
	    g = fnt->glyphs[c-fnt->start];

	    pack_mputw(g->w, f);
	    pack_mputw(g->h, f);

	    pack_fwrite(g->dat, ((g->w+7)/8) * g->h, f);
	 }
	 else {
	    b = fnt->glyphs[c-fnt->start];

	    pack_mputw(b->w, f);
	    pack_mputw(b->h, f);

	    for (y=0; y<b->h; y++)
	       for (x=0; x<b->w; x++)
		  pack_putc(b->line[y][x], f);
	 }
      }

      fnt = fnt->next;
   }
}



/* for the font viewer/editor */
static FONT *the_font;

static char char_string[8] = "0x0020";

static char *range_getter(int index, int *list_size);
static int import_proc(int msg, DIALOG *d, int c);
static int delete_proc(int msg, DIALOG *d, int c);
static int font_view_proc(int msg, DIALOG *d, int c);



static DIALOG char_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                 (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,    224,  72,   0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
   { d_text_proc,       16,   16,   0,    0,    0,    0,    0,       0,          0,             0,       "Base character:",   NULL, NULL  },
   { d_edit_proc,       144,  16,   56,   8,    0,    0,    0,       0,          6,             0,       char_string,         NULL, NULL  },
   { d_button_proc,     72,   44,   80,   16,   0,    0,    13,      D_EXIT,     0,             0,       "OK",                NULL, NULL  }, 
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  }
};



static DIALOG view_font_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { font_view_proc,    0,    100,  0,    0,    0,    0,    0,       D_EXIT,     0,             0,       NULL,             NULL, NULL  }, 
   { d_list_proc,       0,    0,    160,  99,   0,    0,    0,       D_EXIT,     0,             0,       range_getter,     NULL, NULL  },
   { import_proc,       180,  8,    112,  16,   0,    0,    'i',     D_EXIT,     0,             0,       "&Import Range",  NULL, NULL  }, 
   { delete_proc,       180,  40,   112,  16,   0,    0,    'd',     D_EXIT,     0,             0,       "&Delete Range",  NULL, NULL  }, 
   { d_button_proc,     180,  72,   112,  16,   0,    0,    27,      D_EXIT,     0,             0,       "Exit",           NULL, NULL  }, 
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};


#define VIEWER          1
#define RANGE_LIST      2



/* dialog callback for retrieving information about the font range list */
static char *range_getter(int index, int *list_size)
{
   static char buf[64];
   FONT *fnt = the_font;

   if (index < 0) {
      if (list_size) {
	 *list_size = 0;
	 while (fnt) {
	    (*list_size)++;
	    fnt = fnt->next;
	 }
      }
      return NULL;
   }

   while ((index--) && (fnt->next))
      fnt = fnt->next;

   sprintf(buf, "%04X-%04X, %s", fnt->start, fnt->end, (fnt->mono) ? "mono" : "color");

   return buf;
}



/* imports a new font range */
static int import_proc(int msg, DIALOG *d, int c)
{
   char name[256];
   int ret = d_button_proc(msg, d, c);
   FONT *fnt, *f, *prev;
   long size;
   int base;
   int i;

   if (ret & D_CLOSE) {
      #define EXT_LIST  "bmp;fnt;lbm;pcx;tga"

      strcpy(name, grabber_import_file);
      *get_filename(name) = 0;

      if (file_select("Import range (" EXT_LIST ")", name, EXT_LIST)) {
	 fix_filename_case(name);
	 strcpy(grabber_import_file, name);

	 grabber_busy_mouse(TRUE);
	 fnt = grab_font(name, &size, -1, -1, -1, -1, -1);
	 grabber_busy_mouse(FALSE);

	 if (!fnt) {
	    datedit_error("Error importing %s", name);
	 }
	 else {
	    if (fnt->next) {
	       alert("Whoah, this import gave multiple ranges!", NULL, NULL, "That's strange...", NULL, 13, 0);
	       destroy_font(fnt->next);
	       fnt->next = NULL;
	    }

	    centre_dialog(char_dlg);
	    set_dialog_color(char_dlg, gui_fg_color, gui_bg_color);
	    do_dialog(char_dlg, 2);

	    base = strtol(char_string, NULL, 0);

	    fnt->end += (base - fnt->start);
	    fnt->start = base;

	    f = the_font;

	    while (f) {
	       if ((f->end >= fnt->start) && (f->start <= fnt->end)) {
		  alert("Warning, data overlaps with an", "existing range. This almost", "certainly isn't what you want", "Silly me", NULL, 13, 0);
		  break;
	       }

	       f = f->next;
	    }

	    f = the_font;
	    prev = NULL;
	    i = 0;

	    while ((f) && (f->start < fnt->start)) {
	       prev = f;
	       f = f->next;
	       i++;
	    }

	    if (prev)
	       prev->next = fnt;
	    else
	       the_font = fnt;

	    fnt->next = f;

	    view_font_dlg[RANGE_LIST].d1 = i;

	    SEND_MESSAGE(view_font_dlg+VIEWER, MSG_START, 0);
	    SEND_MESSAGE(view_font_dlg+RANGE_LIST, MSG_START, 0);
	 }
      }

      return D_REDRAW;
   }

   return ret;
}



/* deletes a font range */
static int delete_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);
   FONT *fnt, *prev;
   int i;

   if (ret & D_CLOSE) {
      fnt = the_font;

      if (!fnt->next) {
	 alert("Deletion not possible:", "fonts must always have at", "least one character range", "Sorry", NULL, 13, 0);
	 return D_O_K;
      }

      prev = NULL;

      for (i=0; i<view_font_dlg[RANGE_LIST].d1; i++) {
	 prev = fnt;
	 fnt = fnt->next;
      }

      if (prev)
	 prev->next = fnt->next;
      else
	 the_font = fnt->next;

      fnt->next = NULL;
      destroy_font(fnt);

      SEND_MESSAGE(view_font_dlg+VIEWER, MSG_START, 0);
      SEND_MESSAGE(view_font_dlg+RANGE_LIST, MSG_START, 0);

      return D_REDRAW;
   }

   return ret;
}



/* displays the font contents */
static int font_view_proc(int msg, DIALOG *d, int c)
{
   BITMAP *b;
   FONT_GLYPH *g;
   FONT *fnt;
   int x, y, w, h, i;
   int font_h;

   switch (msg) {

      case MSG_START:
	 if (!d->dp)
	    d->dp = create_bitmap(d->w, d->h);
	 d->dp2 = NULL;
	 d->d1 = 0;
	 d->d2 = 0;
	 break;

      case MSG_END:
	 destroy_bitmap(d->dp);
	 d->dp = NULL;
	 break;

      case MSG_DRAW:
	 clear_to_color(d->dp, gui_mg_color);

	 fnt = the_font;
	 i = view_font_dlg[RANGE_LIST].d1;

	 while ((i--) && (fnt->next))
	    fnt = fnt->next;

	 font_h = text_height(fnt);
	 y = d->d1 + 8;

	 for (i=fnt->start; i<=fnt->end; i++) {
	    if (fnt->mono) {
	       g = fnt->glyphs[i-fnt->start];
	       w = g->w;
	       h = g->h;
	    }
	    else {
	       b = fnt->glyphs[i-fnt->start];
	       w = b->w;
	       h = b->h;
	    }

	    text_mode(gui_mg_color);
	    textprintf(d->dp, font, 32, y+font_h/2-4, gui_fg_color, "U+%04X %3dx%-3d %c", i, w, h, i);

	    text_mode(makecol(0xC0, 0xC0, 0xC0));
	    textprintf(d->dp, fnt, 200, y, gui_fg_color, "%c", i);

	    y += font_h * 3/2;
	 }

	 if (d->flags & D_GOTFOCUS) {
	    for (i=0; i<d->w; i+=2) {
	       putpixel(d->dp, i,   0,      gui_fg_color);
	       putpixel(d->dp, i+1, 0,      gui_bg_color);
	       putpixel(d->dp, i,   d->h-1, gui_bg_color);
	       putpixel(d->dp, i+1, d->h-1, gui_fg_color);
	    }
	    for (i=1; i<d->h-1; i+=2) {
	       putpixel(d->dp, 0,      i,   gui_bg_color);
	       putpixel(d->dp, 0,      i+1, gui_fg_color);
	       putpixel(d->dp, d->w-1, i,   gui_fg_color);
	       putpixel(d->dp, d->w-1, i+1, gui_bg_color);
	    }
	 }

	 blit(d->dp, screen, 0, 0, d->x, d->y, d->w, d->h);

	 d->d2 = y - d->d1;
	 d->dp2 = fnt;
	 break; 

      case MSG_WANTFOCUS:
	 return D_WANTFOCUS;

      case MSG_CHAR:
	 switch (c >> 8) {

	    case KEY_UP:
	       d->d1 += 8;
	       break;

	    case KEY_DOWN:
	       d->d1 -= 8;
	       break;

	    case KEY_PGUP:
	       d->d1 += d->h*2/3;
	       break;

	    case KEY_PGDN:
	       d->d1 -= d->h*2/3;
	       break;

	    case KEY_HOME:
	       d->d1 = 0;
	       break;

	    case KEY_END:
	       d->d1 = -d->d2 + d->h;
	       break;

	    default:
	       return D_O_K;
	 }

	 if (d->d1 < -d->d2 + d->h)
	    d->d1 = -d->d2 + d->h;

	 if (d->d1 > 0)
	    d->d1 = 0;

	 d->flags |= D_DIRTY;
	 return D_USED_CHAR;

      case MSG_CLICK:
	 if (mouse_b & 2)
	    return D_CLOSE;

	 x = mouse_x;
	 y = mouse_y;

	 show_mouse(NULL);

	 while (mouse_b) {
	    poll_mouse();

	    if (mouse_y != y) {
	       d->d1 += (y - mouse_y);
	       position_mouse(x, y);

	       if (d->d1 < -d->d2 + d->h)
		  d->d1 = -d->d2 + d->h;

	       if (d->d1 > 0)
		  d->d1 = 0;

	       SEND_MESSAGE(d, MSG_DRAW, 0);
	    }
	 }

	 show_mouse(screen);
	 return D_O_K;

      case MSG_KEY:
	 return D_CLOSE;

      case MSG_IDLE:
	 fnt = the_font;
	 i = view_font_dlg[RANGE_LIST].d1;

	 while ((i--) && (fnt->next))
	    fnt = fnt->next;

	 if (d->dp2 != fnt) {
	    d->d1 = 0;
	    scare_mouse();
	    SEND_MESSAGE(d, MSG_DRAW, 0);
	    unscare_mouse();
	 }
	 break;
   }

   return D_O_K;
}



/* handles double-clicking on a font in the grabber */
static int view_font(DATAFILE *dat)
{
   show_mouse(NULL);
   clear_to_color(screen, gui_mg_color);

   the_font = dat->dat;

   view_font_dlg[RANGE_LIST].d1 = 0;
   view_font_dlg[RANGE_LIST].d2 = 0;

   view_font_dlg[VIEWER].w = SCREEN_W;
   view_font_dlg[VIEWER].h = SCREEN_H - view_font_dlg[VIEWER].y;

   set_dialog_color(view_font_dlg, gui_fg_color, gui_bg_color);

   view_font_dlg[0].bg = gui_mg_color;

   do_dialog(view_font_dlg, VIEWER);

   dat->dat = the_font;

   show_mouse(screen);
   return D_REDRAW;
}



/* plugin interface header */
DATEDIT_OBJECT_INFO datfont_info =
{
   DAT_FONT, 
   "Font", 
   get_font_desc,
   makenew_font,
   save_font,
   plot_font,
   view_font,
   NULL
};



DATEDIT_GRABBER_INFO datfont_grabber =
{
   DAT_FONT, 
   "txt;fnt;bmp;lbm;pcx;tga",
   "txt;bmp;pcx;tga",
   grab_font,
   export_font
};

