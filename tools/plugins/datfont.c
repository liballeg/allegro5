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
#include "allegro/internal/aintern.h"
#include "../datedit.h"



/* creates a new font object */
static void* makenew_font(long* size)
{
    FONT *f;
    FONT_MONO_DATA* mf = 0, * mfread = font->data;

    f = _AL_MALLOC(sizeof(FONT));

    f->height = font->height;
    f->vtable = font->vtable;

    while(mfread) {
        int i;

        if(mf) {
            mf->next = _AL_MALLOC(sizeof(FONT_MONO_DATA));
            mf = mf->next;
        } else f->data = mf = _AL_MALLOC(sizeof(FONT_MONO_DATA));

        mf->begin = mfread->begin;
        mf->end = mfread->end;
        mf->next = 0;
        mf->glyphs = _AL_MALLOC(sizeof(FONT_GLYPH*) * (mf->end - mf->begin));

        for(i = mf->begin; i < mf->end; i++) {
            FONT_GLYPH *gsrc = mfread->glyphs[i - mf->begin], *gdest;
            int sz = ((gsrc->w + 7) / 8) * gsrc->h;

            gdest = _AL_MALLOC(sizeof(FONT_GLYPH) + sz);
            gdest->w = gsrc->w;
            gdest->h = gsrc->h;
            memcpy(gdest->dat, gsrc->dat, sz);

            mf->glyphs[i - mf->begin] = gdest;
        }

        mfread = mfread->next;
    }

    return f;
}



/* displays a font in the grabber object view window */
static void plot_font(AL_CONST DATAFILE *dat, int x, int y)
{
    FONT* f = dat->dat;
    int cx = x, bufpos, ch;
    char buf[8];

    y += 32;

    if(is_mono_font(f)) {
        FONT_MONO_DATA* mf = f->data;

        while(mf) {
            for(ch = mf->begin; ch < mf->end; ch++) {
                bufpos = usetc(buf, ch);
                usetc(buf + bufpos, 0);

                if(text_length(f, buf) + cx + 4 > SCREEN_W) {
                    cx = x;
                    y += text_height(f) + 4;
                }

                if(y > SCREEN_H) break;

                textout_ex(screen, f, buf, cx, y, gui_fg_color, -1);

                cx += text_length(f, buf) + 4;
            }

            mf = mf->next;
        }
    } else {
        FONT_COLOR_DATA* cf = f->data;

        while(cf) {
            for(ch = cf->begin; ch < cf->end; ch++) {
                bufpos = usetc(buf, ch);
                usetc(buf + bufpos, 0);

                if(text_length(f, buf) + cx + 4 > SCREEN_W) {
                    cx = x;
                    y += text_height(f) + 4;
                }

                if(y > SCREEN_H) break;

                textout_ex(screen, f, buf, cx, y, -1, -1);

                cx += text_length(f, buf) + 4;
            }

            cf = cf->next;
        }
    }
}


/* returns a description string for a font object */
static void get_font_desc(AL_CONST DATAFILE *dat, char *s)
{
   FONT *fnt = (FONT *)dat->dat;
   char *mono = (is_mono_font(fnt)) ? "mono" : "color";
   int ranges = 0;
   int glyphs = 0;

    if(is_mono_font(fnt)) {
        FONT_MONO_DATA* mf = fnt->data;

        while(mf) {
            ranges++;
            glyphs += mf->end - mf->begin;
            mf = mf->next;
        }
    } else {
        FONT_COLOR_DATA* cf = fnt->data;

        while(cf) {
            ranges++;
            glyphs += cf->end - cf->begin;
            cf = cf->next;
        }
    }

   sprintf(s, "%s font, %d range%s, %d glyphs", mono, ranges, (ranges==1) ? "" : "s", glyphs);
}



/* exports a font into an external file */
static int export_font(AL_CONST DATAFILE* dat, AL_CONST char* filename)
{
    char buf[1024], tmp[1024];
    PACKFILE *pack;
    FONT* f = dat->dat;

    BITMAP *bmp;
    PALETTE pal;

    int w = 0, h = 0, max = 0, i;

    if(stricmp(get_extension(filename), "txt") == 0) {
        replace_extension(buf, filename, "pcx", sizeof(buf));

        if(exists(buf)) {
            i = datedit_ask("%s already exists, overwrite", buf);
            if(i == 27 || i == 'n' || i == 'N') return TRUE;
        }

        pack = pack_fopen(filename, F_WRITE);
        if(!pack) return FALSE;

        if(is_mono_font(f)) {
            FONT_MONO_DATA* mf = f->data;
            while(mf) {
                sprintf(tmp, "%s 0x%04X 0x%04X\n", (mf == f->data) ? get_filename(buf) : "-", mf->begin, mf->end - 1);
                pack_fputs(tmp, pack);
                mf = mf->next;
            }
        } else {
            FONT_COLOR_DATA* cf = f->data;
            while(cf) {
                sprintf(tmp, "%s 0x%04X 0x%04X\n", (cf == f->data) ? get_filename(buf) : "-", cf->begin, cf->end - 1);
                pack_fputs(tmp, pack);
                cf = cf->next;
            }
        }

        pack_fclose(pack);

        filename = buf;
    } else {
        int multi = 0;

        if(is_mono_font(f)) {
            if( ((FONT_MONO_DATA*)f->data) ->next) multi = 1;
        } else {
            if( ((FONT_COLOR_DATA*)f->data)->next) multi = 1;
        }

        if(multi) {
            i = datedit_ask("Really export multi-range font as bitmap rather than .txt");
            if(i == 27 || i == 'n' || i == 'N') return TRUE;
        }
    }

    if(is_mono_font(f)) {
        FONT_MONO_DATA* mf = f->data;

        while(mf) {
            for(i = mf->begin; i < mf->end; i++) {
                FONT_GLYPH* g = mf->glyphs[i - mf->begin];

                max++;
                if(g->w > w) w = g->w;
                if(g->h > h) h = g->h;
            }

            mf = mf->next;
        }
    } else {
        FONT_COLOR_DATA* cf = f->data;
        int i;

        while(cf) {
            for(i = cf->begin; i < cf->end; i++) {
                BITMAP* g = cf->bitmaps[i - cf->begin];

                max++;
                if(g->w > w) w = g->w;
                if(g->h > h) h = g->h;
            }

            cf = cf->next;
        }
    }

    w = (w + 16) & 0xFFF0;
    h = (h + 16) & 0xFFF0;

    bmp = create_bitmap_ex(8, 1 + w * 16, 1 + h * ((max + 15) / 16) );
    clear_to_color(bmp, 255);

    max = 0;

    if(is_mono_font(f)) {
        FONT_MONO_DATA* mf = f->data;

        while(mf) {
            for(i = mf->begin; i < mf->end; i++) {
                textprintf_ex(bmp, f, 1 + w * (max & 15), 1 + h * (max / 16), 1, 0, "%c", i);
                max++;
            }

            mf = mf->next;
        }
    } else {
        FONT_COLOR_DATA* cf = f->data;

        while(cf) {
            for(i = cf->begin; i < cf->end; i++) {
                textprintf_ex(bmp, f, 1 + w * (max & 15), 1 + h * (max / 16), -1, 0, "%c", i);
                max++;
            }

            cf = cf->next;
        }
    }

    get_palette(pal);

    save_bitmap(filename, bmp, pal);
    destroy_bitmap(bmp);

    return (errno == 0);
}



/* magic number for GRX format font files */
#define FONTMAGIC    0x19590214L



/* import routine for the GRX font copyright message */
static void import_grx_message(AL_CONST char* filename)
{
    PACKFILE *pack;
    int w, h, num, i, end, begin;

    pack = pack_fopen(filename, F_READ);
    if(!pack) return;

    if(pack_igetl(pack) != FONTMAGIC) {
        pack_fclose(pack);
        return;
    }
    pack_igetl(pack);

    w = pack_igetw(pack);
    h = pack_igetw(pack);

    begin = pack_igetw(pack);
    end = pack_igetw(pack) + 1;
    num = end - begin;

    if(pack_igetw(pack) == 0) {
        for(i = 0; i < 38; i++) pack_getc(pack);
        for(i = 0; i < num; i++) pack_igetw(pack);
    } else {
        for(i = 0; i < 38; i++) pack_getc(pack);
    }

    for(i = 0; i < num; i++) {
        int sz;

        sz = ((w + 7) / 8) * h;
        while(sz) {
            pack_getc(pack);
            sz--;
        }
    }

    if(!pack_feof(pack)) {
        char cw_char[1024];

        strcpy(cw_char, filename);
        strcpy(get_extension(cw_char), "txt");
        i = datedit_ask("Save font copyright message into '%s'", cw_char);
        if(i != 27 && i != 'n' && i != 'N') {
            PACKFILE* pout = pack_fopen(cw_char, F_WRITE);
            if(pout) {
                while(!pack_feof(pack)) {
                    i = pack_fread(cw_char, 1024, pack);
                    pack_fwrite(cw_char, i, pout);
                }
            }
            pack_fclose(pout);
        }
    }

    pack_fclose(pack);
}



/* upgrade_to_color, upgrade_to_color_data:
 *  Helper functions. Upgrades a monochrome font to a color font.
 */
static FONT_COLOR_DATA* upgrade_to_color_data(FONT_MONO_DATA* mf)
{
    FONT_COLOR_DATA* cf = _AL_MALLOC(sizeof(FONT_COLOR_DATA));
    BITMAP** bits = _AL_MALLOC(sizeof(BITMAP*) * (mf->end - mf->begin));
    int i;

    cf->begin = mf->begin;
    cf->end = mf->end;
    cf->bitmaps = bits;
    cf->next = 0;

    for(i = mf->begin; i < mf->end; i++) {
        FONT_GLYPH* g = mf->glyphs[i - mf->begin];
        BITMAP* b = create_bitmap_ex(8, g->w, g->h);
        clear_to_color(b, 0);
        b->vtable->draw_glyph(b, g, 0, 0, 1, 0);

        bits[i - mf->begin] = b;
        free(g);
    }

    free(mf->glyphs);
    free(mf);

    return cf;
}



static void upgrade_to_color(FONT* f)
{
    FONT_MONO_DATA* mf = f->data;
    FONT_COLOR_DATA * cf, *cf_write = 0;

    if(is_color_font(f)) return;
    f->vtable = font_vtable_color;

    while(mf) {
        FONT_MONO_DATA* mf_next = mf->next;

        cf = upgrade_to_color_data(mf);
        if(!cf_write) f->data = cf;
        else cf_write->next = cf;

        cf_write = cf;
        mf = mf_next;
    }
}



static DATAFILE *fonts_datafile;

/* Get a list of fonts for a font datafile */
static AL_CONST char *datafile_fontname_getter(int index, int *list_size)
{
   DATAFILE *datf = fonts_datafile;
   int n;
   
   if (index<0) {
      *list_size = 0;
      if (datf) {
         for(n=0; datf[n].type != DAT_END; n++) {
            if (datf[n].type == DAT_FONT) (*list_size)++;
         }
      }
      
      return NULL;
   }

   if (datf) {
      for(n=0; datf[n].type != DAT_END && index>-1; n++)
         if (datf[n].type == DAT_FONT) index--;
      return get_datafile_property(&(datf[n-1]), DAT_ID('N','A','M','E'));
   }
   else {
      return NULL;
   }
}



/* List the fonts available in a datafile */
static FONT *grab_datafile_font(AL_CONST char *filename)
{
   const char *names[2] = { NULL, NULL };
   FONT *f = NULL;
   int c;
   
   fonts_datafile = load_datafile(filename);

   c = datedit_select(datafile_fontname_getter, "Select font:");
   if (c>=0) {
      names[0] = datafile_fontname_getter(c, NULL);
      f = load_font(filename, NULL, (void*)names);
   }
   
   unload_datafile(fonts_datafile);
   fonts_datafile = NULL;
   
   return f;
}

/* imports a font from an external file (handles various formats) */
static DATAFILE *grab_font(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   PACKFILE *f;
   FONT *font;
   int id;

   if (stricmp(get_extension(filename), "dat") == 0) {
      font = grab_datafile_font(filename);
   }
   else {
      font = load_font(filename, NULL, NULL);
   }

   /* Safe copyright message for GRX fonts */
   if (stricmp(get_extension(filename), "fnt") == 0) {
      f = pack_fopen(filename, F_READ);
      if (!f)
	 return NULL;

      id = pack_igetl(f);
      pack_fclose(f);

      if (font && id == FONTMAGIC) {
	 /* GRX font; safe copyright message? */
         import_grx_message(filename);
      }
   }
   return datedit_construct(type, font, 0, prop);
}



/* helper for save_font, below */
static int save_mono_font(FONT* f, PACKFILE* pack)
{
    FONT_MONO_DATA* mf = f->data;
    int i = 0;

   *allegro_errno = 0;

    /* count number of ranges */
    while(mf) {
        i++;
        mf = mf->next;
    }
    pack_mputw(i, pack);

    mf = f->data;

    while(mf) {

        /* mono, begin, end-1 */
        pack_putc(1, pack);
        pack_mputl(mf->begin, pack);
        pack_mputl(mf->end - 1, pack);

        for(i = mf->begin; i < mf->end; i++) {
            FONT_GLYPH* g = mf->glyphs[i - mf->begin];

            pack_mputw(g->w, pack);
            pack_mputw(g->h, pack);

	    pack_fwrite(g->dat, ((g->w + 7) / 8) * g->h, pack);
        }

        mf = mf->next;

    }

   if (*allegro_errno)
      return FALSE;
   else
      return TRUE;
}



/* helper for save_font, below */
static int save_color_font(FONT* f, PACKFILE* pack)
{
    FONT_COLOR_DATA* cf = f->data;
    int i = 0;
    int depth;

   *allegro_errno = 0;

    /* count number of ranges */
    while(cf) {
        i++;
        cf = cf->next;
    }
    pack_mputw(i, pack);

    cf = f->data;

    while(cf) {
         
        /* Get font color depth */
        depth = bitmap_color_depth(cf->bitmaps[0]);

        /* Compatibility with older versions: depth=0 means 8 bit */
        if (depth == 8)
            depth = 0;
        
        /* color, begin, end-1 */
        pack_putc(depth, pack);
        pack_mputl(cf->begin, pack);
        pack_mputl(cf->end - 1, pack);

        /* Save pixmap data */
        for(i = cf->begin; i < cf->end; i++) {
            BITMAP* glyph = cf->bitmaps[i - cf->begin];
            int x, y, c, r, g, b, a;
            uint16_t *p16;
            uint8_t *p24;
            uint32_t *p32;

            pack_mputw(glyph->w, pack);
            pack_mputw(glyph->h, pack);

            switch (depth) {

               case 0:
	          /* 256 colors */
	          for (y=0; y<glyph->h; y++)
	             for (x=0; x<glyph->w; x++)
	                pack_putc(glyph->line[y][x], pack);
	          break;

               case 15:
               case 16:
	          /* hicolor */
	          for (y=0; y<glyph->h; y++) {
	             p16 = (uint16_t *)glyph->line[y];

	             for (x=0; x<glyph->w; x++) {
	                c = p16[x];
	                r = getr_depth(depth, c);
	                g = getg_depth(depth, c);
	                b = getb_depth(depth, c);
	                c = ((r<<8)&0xF800) | ((g<<3)&0x7E0) | ((b>>3)&0x1F);
	                pack_iputw(c, pack);
	             }
	          }
	          break;

               case 24:
	          /* 24 bit truecolor */
	          for (y=0; y<glyph->h; y++) {
	             p24 = (unsigned char *)glyph->line[y];

	             for (x=0; x<glyph->w; x++) {
	                c = READ3BYTES(p24);
	                r = getr24(c);
	                g = getg24(c);
	                b = getb24(c);
	                pack_putc(r, pack);
	                pack_putc(g, pack);
	                pack_putc(b, pack);
	                p24 += 3;
	             }
	          }
	          break;

               case 32:
	          /* 32 bit truecolor */
	          for (y=0; y<glyph->h; y++) {
	             p32 = (uint32_t *)glyph->line[y];

	             for (x=0; x<glyph->w; x++) {
	                c = p32[x];
	                r = getr32(c);
	                g = getg32(c);
	                b = getb32(c);
	                pack_putc(r, pack);
	                pack_putc(g, pack);
	                pack_putc(b, pack);
	             }
	          }
	          break;

               case -32:
	          /* 32 bit truecolor with alpha channel */
	          for (y=0; y<glyph->h; y++) {
	             p32 = (uint32_t *)glyph->line[y];

	             for (x=0; x<glyph->w; x++) {
	                c = p32[x];
	                r = getr32(c);
	                g = getg32(c);
	                b = getb32(c);
	                a = geta32(c);
	                pack_putc(r, pack);
	                pack_putc(g, pack);
	                pack_putc(b, pack);
	                pack_putc(a, pack);
	             }
	          }
	          break;
            }

        }

        cf = cf->next;

    }

   if (*allegro_errno)
      return FALSE;
   else
      return TRUE;
}



/* saves a font into a datafile */
static int save_font(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f)
{
    FONT* font = dat->dat;

    pack_mputw(0, f);

    if (is_mono_font(font))
       return save_mono_font(font, f);
    else
       return save_color_font(font, f);
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
   { d_shadow_box_proc, 0,    0,    225,  73,   0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
   { d_text_proc,       16,   16,   0,    0,    0,    0,    0,       0,          0,             0,       "Base character:",   NULL, NULL  },
   { d_edit_proc,       144,  16,   56,   8,    0,    0,    0,       0,          6,             0,       char_string,         NULL, NULL  },
   { d_button_proc,     72,   44,   81,   17,   0,    0,    13,      D_EXIT,     0,             0,       "OK",                NULL, NULL  }, 
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  }
};



static DIALOG view_font_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { font_view_proc,    0,    100,  0,    0,    0,    0,    0,       D_EXIT,     0,             0,       NULL,             NULL, NULL  }, 
   { d_list_proc,       0,    0,    161,  100,  0,    0,    0,       D_EXIT,     0,             0,       range_getter,     NULL, NULL  },
   { import_proc,       180,  8,    113,  17,   0,    0,    'i',     D_EXIT,     0,             0,       "&Import Range",  NULL, NULL  }, 
   { delete_proc,       180,  40,   113,  17,   0,    0,    'd',     D_EXIT,     0,             0,       "&Delete Range",  NULL, NULL  }, 
   { d_button_proc,     180,  72,   113,  17,   0,    0,    27,      D_EXIT,     0,             0,       "Exit",           NULL, NULL  }, 
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};


#define VIEWER          1
#define RANGE_LIST      2



/* dialog callback for retrieving information about the font range list */
static char *range_getter(int index, int *list_size)
{
    static char buf[64];
    FONT* f = the_font;

    if(index < 0) {
        if(!list_size) return 0;

        *list_size = get_font_ranges(f);
        if (*list_size < 0)
           *list_size = 0;

        return 0;
    }

    sprintf(buf, "%04X-%04X, color", get_font_range_begin(f, index), get_font_range_end(f, index));

    return buf;
}



/* imports a new font range */
static int import_proc(int msg, DIALOG *d, int c)
{
   char name[80*6]; /* 80 chars * max UTF8 char width */
   int ret = d_button_proc(msg, d, c);
   DATAFILE *dat;
   FONT *fnt, *f;
   int base;
   int i;

   if (ret & D_CLOSE) {
      #define EXT_LIST  "bmp;fnt;lbm;pcx;tga"

      strcpy(name, grabber_import_file);
      *get_filename(name) = 0;

      if (file_select_ex("Import range (" EXT_LIST ")", name, EXT_LIST, sizeof(name), 0, 0)) {
	 fix_filename_case(name);
	 strcpy(grabber_import_file, name);

	 grabber_busy_mouse(TRUE);
	 dat = grab_font(DAT_FONT, name, NULL, -1);
	 grabber_busy_mouse(FALSE);

	 if (!dat) {
	    datedit_error("Error importing %s", name);
	 }
	 else {
	    int import_begin, import_end;

	    fnt = dat->dat;

	    centre_dialog(char_dlg);
	    set_dialog_color(char_dlg, gui_fg_color, gui_bg_color);
	    do_dialog(char_dlg, 2);

	    base = strtol(char_string, NULL, 0);

        if(is_mono_font(fnt)) {
            FONT_MONO_DATA* mf = fnt->data;
            import_end = (mf->end += (base - mf->begin));
            import_begin = mf->begin = base;
        } else {
            FONT_COLOR_DATA* cf = fnt->data;
            import_end = (cf->end += (base - cf->begin));
            import_begin = cf->begin = base;
        }

	    f = the_font;

	    if(is_mono_font(fnt)) {
	        FONT_MONO_DATA* mf = f->data;
	        while(mf) {
	            if(mf->end > import_begin && mf->begin < import_end) {
                    alert("Warning, data overlaps with an", "existing range. This almost", "certainly isn't what you want", "Silly me", NULL, 13, 0);
                    break;
	            }

	            mf = mf->next;
	        }
	    }

	    if(!is_compatible_font(f, fnt)) {
	        upgrade_to_color(f);
	        upgrade_to_color(fnt);
	    }

	    f = the_font;
	    i = 0;

        if(is_mono_font(f)) {
            FONT_MONO_DATA* mf = f->data, * mf2 = fnt->data;

            if(mf->begin > import_begin) {
                mf2->next = mf;
                f->data = mf2;
            } else {
                i++;
                while(mf->next && mf->next->begin < import_begin) {
                    mf = mf->next;
                    i++;
                }
                mf2->next = mf->next;
                mf->next = mf2;
            }
        } else {
            FONT_COLOR_DATA* cf = f->data, * cf2 = fnt->data;

            if(cf->begin > import_begin) {
                cf2->next = cf;
                f->data = cf2;
            } else {
                i++;
                while(cf->next && cf->next->begin < import_begin) {
                    cf = cf->next;
                    i++;
                }
                cf2->next = cf->next;
                cf->next = cf2;
            }
        }
            
	    view_font_dlg[RANGE_LIST].d1 = i;

	    object_message(view_font_dlg+VIEWER, MSG_START, 0);
	    object_message(view_font_dlg+RANGE_LIST, MSG_START, 0);
	    
	    grabber_modified(TRUE);
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
   FONT *fnt;
   int i;

   if (ret & D_CLOSE) {
      fnt = the_font;

        if(is_mono_font(fnt)) {

            FONT_MONO_DATA* mf = fnt->data, * mf_prev = 0;

            if(!mf->next) {
                alert("Deletion not possible:", "fonts must always have at", "least one character range", "Sorry", NULL, 13, 0);
                return D_O_K;
            }

            i = view_font_dlg[RANGE_LIST].d1;
            while(i--) {
                mf_prev = mf;
                mf = mf->next;
            }

            if(mf_prev) mf_prev->next = mf->next;
            else fnt->data = mf->next;

            for(i = mf->begin; i < mf->end; i++) free(mf->glyphs[i - mf->begin]);
            free(mf->glyphs);
            free(mf);

        } else {

            FONT_COLOR_DATA* cf = fnt->data, * cf_prev = 0;

            if(!cf->next) {
                alert("Deletion not possible:", "fonts must always have at", "least one character range", "Sorry", NULL, 13, 0);
                return D_O_K;
            }

            i = view_font_dlg[RANGE_LIST].d1;
            while(i--) {
                cf_prev = cf;
                cf = cf->next;
            }

            if(cf_prev) cf_prev->next = cf->next;
            else fnt->data = cf->next;

            for(i = cf->begin; i < cf->end; i++) destroy_bitmap(cf->bitmaps[i - cf->begin]);
            free(cf->bitmaps);
            free(cf);

        }

      object_message(view_font_dlg+VIEWER, MSG_START, 0);
      object_message(view_font_dlg+RANGE_LIST, MSG_START, 0);

      grabber_modified(TRUE);

      return D_REDRAW;
   }

   return ret;
}



/* displays the font contents */
static int font_view_proc(int msg, DIALOG *d, int c)
{
   BITMAP** bits = 0;
   FONT_GLYPH** gl = 0;
   FONT *fnt;
   int x, y, w, h, i;
   int font_h;
   int begin, end;

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

     if(is_mono_font(fnt)) {

        FONT_MONO_DATA* mf = fnt->data;
        while(i--) mf = mf->next;

        gl = mf->glyphs;
        begin = mf->begin;
        end = mf->end;

     } else {

        FONT_COLOR_DATA* cf = fnt->data;
        while(i--) cf = cf->next;

        bits = cf->bitmaps;
        begin = cf->begin;
        end = cf->end;

     }

	 font_h = text_height(fnt);
	 y = d->d1 + 8;

	 for (i = begin; i < end; i++) {
	    if (gl) {
	       w = gl[i - begin]->w;
	       h = gl[i - begin]->h;
	    }
	    else {
	        w = bits[i - begin]->w;
	        h = bits[i - begin]->h;
	    }

	    textprintf_ex(d->dp, font, 32, y+font_h/2-4, gui_fg_color, gui_mg_color, "U+%04X %3dx%-3d %c", i, w, h, i);
	    textprintf_ex(d->dp, fnt, 200, y, (gl ? gui_fg_color : -1), makecol(0xC0, 0xC0, 0xC0), "%c", i);

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

	       object_message(d, MSG_DRAW, 0);
	    }
	 }

	 show_mouse(screen);
	 return D_O_K;

      case MSG_KEY:
	 return D_CLOSE;

	  case MSG_IDLE:
	 if(d->fg != view_font_dlg[RANGE_LIST].d1) {
	    d->fg = view_font_dlg[RANGE_LIST].d1;
	    d->d1 = 0;
	    object_message(d, MSG_DRAW, 0);
	 }
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
   "txt;fnt;bmp;lbm;pcx;tga;dat",
   "txt;bmp;pcx;tga",
   grab_font,
   export_font,
   NULL
};

