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
 *      Datafile editing functions, for use by the datafile tools.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "datedit.h"


PALETTE datedit_current_palette;
PALETTE datedit_last_read_pal;

static char info_msg[] = "For internal use by the grabber";

DATAFILE datedit_info = { info_msg, DAT_INFO, sizeof(info_msg), NULL };

static int file_datasize;

static DATAFILE_PROPERTY *builtin_prop = NULL;

void (*grabber_sel_palette)(PALETTE pal) = NULL;
void (*grabber_select_property)(int type) = NULL;
void (*grabber_get_grid_size)(int *x, int *y) = NULL;
void (*grabber_rebuild_list)(void *old, int clear) = NULL;
void (*grabber_get_selection_info)(DATAFILE **dat, DATAFILE ***parent) = NULL;
int (*grabber_foreach_selection)(int (*proc)(DATAFILE *, int *, int), int *count, int *param, int param2) = NULL;
DATAFILE *(*grabber_single_selection)(void) = NULL;
void (*grabber_set_selection)(void *object) = NULL;
void (*grabber_busy_mouse)(int busy) = NULL;
void (*grabber_modified)(int modified) = NULL;

char grabber_data_file[FILENAME_LENGTH] = "";
BITMAP *grabber_graphic = NULL;
PALETTE grabber_palette;

char grabber_import_file[256] = "";
char grabber_graphic_origin[GRABBER_GRAPHIC_ORIGIN_SIZE] = "";
char grabber_graphic_date[256] = "";



/* comparison function for sorting the menu list */
static int menu_cmp(MENU *m1, MENU *m2)
{
   if ((m1->child) && (!m2->child))
      return 1;
   else if ((m2->child) && (!m1->child))
      return -1;
   else
      return stricmp(m1->text, m2->text);
}



/* helper function for inserting a list of builtin properties */
static void insert_builtin_prop(AL_CONST char *prop_types)
{
   int type;

   while (TRUE) {
      ASSERT(strlen(prop_types)>=4 && (!prop_types[4] || prop_types[4]==';'));
      type = DAT_ID(prop_types[0], prop_types[1], prop_types[2], prop_types[3]);
      datedit_insert_property(&builtin_prop, type, "*");
      if (!prop_types[4])
         break;
      prop_types += 5;
   }
}



/* main cleanup routine */
static void datedit_exit(void)
{
   DATAFILE_PROPERTY *iter;
   if (builtin_prop) {
      for (iter=builtin_prop; iter->type != DAT_END; iter++)
         _AL_FREE(iter->dat);
      _AL_FREE(builtin_prop);
   }
}



/* main initialisation routine */
void datedit_init(void)
{
   int done, i;
   AL_CONST char *prop_types;

   #include "plugins.h"

   do {
      done = TRUE;

      for (i=0; datedit_object_info[i+1]->type != DAT_END; i++) {
	 if (stricmp(datedit_object_info[i]->desc, datedit_object_info[i+1]->desc) > 0) {
	    DATEDIT_OBJECT_INFO *tmp = datedit_object_info[i];
	    datedit_object_info[i] = datedit_object_info[i+1];
	    datedit_object_info[i+1] = tmp;
	    done = FALSE;
	 }
      }
   } while (!done);

   do {
      done = TRUE;

      for (i=0; (datedit_menu_info[i]) && (datedit_menu_info[i+1]); i++) {
	 if (menu_cmp(datedit_menu_info[i]->menu, datedit_menu_info[i+1]->menu) > 0) {
	    DATEDIT_MENU_INFO *tmp = datedit_menu_info[i];
	    datedit_menu_info[i] = datedit_menu_info[i+1];
	    datedit_menu_info[i+1] = tmp;
	    done = FALSE;
	 }
      }
   } while (!done);

   /* Build the list of builtin properties. */
   datedit_insert_property(&builtin_prop, DAT_ORIG, "*");
   datedit_insert_property(&builtin_prop, DAT_DATE, "*");

   for (i=0; datedit_grabber_info[i]->type != DAT_END; i++) {
      prop_types = datedit_grabber_info[i]->prop_types;
      if (prop_types)
	 insert_builtin_prop(prop_types);
   }

   for (i=0; (datedit_menu_info[i]) && (datedit_menu_info[i+1]); i++) {
      prop_types = datedit_menu_info[i]->prop_types;
      if (prop_types)
	 insert_builtin_prop(prop_types);
   }

   atexit(datedit_exit);
}



/* export raw binary data */
static int export_binary(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   PACKFILE *f = pack_fopen(filename, F_WRITE);
   int ret = TRUE;

   if (f) {
      if (pack_fwrite(dat->dat, dat->size, f) < dat->size)
	 ret = FALSE;

      pack_fclose(f);
   }
   else
      ret = FALSE;

   return ret;
}



/* grab raw binary data */
static DATAFILE *grab_binary(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   void *mem;
   int64_t sz = file_size_ex(filename);
   PACKFILE *f;

   if (sz <= 0)
      return NULL;

   mem = malloc(sz);

   f = pack_fopen(filename, F_READ);
   if (!f) {
      free(mem);
      return NULL; 
   }

   if (pack_fread(mem, sz, f) < sz) {
      pack_fclose(f);
      free(mem);
      return NULL;
   }

   pack_fclose(f);

   return datedit_construct(type, mem, sz, prop);
}



/* save raw binary data */
static int save_binary(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f)
{
   if (pack_fwrite(dat->dat, dat->size, f) < dat->size)
      return FALSE;

   return TRUE;
}



/* export a child datafile */
static int export_datafile(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   DATEDIT_SAVE_DATAFILE_OPTIONS options = {-1,    /* pack      */
					    -1,    /* strip     */
					    -1,    /* sort      */
					    FALSE, /* verbose   */
					    FALSE, /* write_msg */
					    FALSE, /* backup    */
					    FALSE  /* rel. path */ };

   return datedit_save_datafile((DATAFILE *)dat->dat, filename, NULL, &options, NULL);
}



/* loads object names from a header file, if they are missing */
static void load_header(DATAFILE *dat, AL_CONST char *filename)
{
   char buf[160], buf2[160];
   int datsize, i, c, c2;
   PACKFILE *f;

   datsize = 0;
   while (dat[datsize].type != DAT_END)
      datsize++;

   f = pack_fopen(filename, F_READ);

   if (f) {
      while (pack_fgets(buf, 160, f) != 0) {
	 if (strncmp(buf, "#define ", 8) == 0) {
	    c2 = 0;
	    c = 8;

	    while ((buf[c]) && (buf[c] != ' '))
	       buf2[c2++] = buf[c++];

	    buf2[c2] = 0;
	    while (buf[c]==' ')
	       c++;

	    i = 0;
	    while ((buf[c] >= '0') && (buf[c] <= '9')) {
	       i *= 10;
	       i += buf[c] - '0';
	       c++;
	    }

	    if ((i < datsize) && (!*get_datafile_property(dat+i, DAT_NAME)))
	       datedit_set_property(dat+i, DAT_NAME, buf2);
	 }
      }

      pack_fclose(f);
   }
   else {
      /* don't let the error propagate */
      errno = 0;
   }
}



/* generates object names, if they are missing */
static int generate_names(DATAFILE *dat, int n)
{
   int i;

   while (dat->type != DAT_END) {
      if (!*get_datafile_property(dat, DAT_NAME)) {
	 char tmp[32];

	 sprintf(tmp, "%03d_%c%c%c%c", n++,
			(dat->type>>24)&0xFF, (dat->type>>16)&0xFF,
			(dat->type>>8)&0xFF, dat->type&0xFF);

	 for (i=4; tmp[i]; i++) {
	    if (((tmp[i] < '0') || (tmp[i] > '9')) &&
		((tmp[i] < 'A') || (tmp[i] > 'Z')) &&
		((tmp[i] < 'a') || (tmp[i] > 'z')))
	       tmp[i] = 0;
	 }

	 datedit_set_property(dat, DAT_NAME, tmp);
      }

      if (dat->type == DAT_FILE)
	 n = generate_names((DATAFILE *)dat->dat, n);

      dat++;
   }

   return n;
}



/* retrieves whatever grabber information is stored in a datafile */
static DATAFILE *extract_info(DATAFILE *dat, int save)
{
   DATAFILE_PROPERTY *prop;
   int i;

   if (save) {
      if (datedit_info.prop) {
	 prop = datedit_info.prop;
	 while (prop->type != DAT_END) {
	    if (prop->dat)
	       _AL_FREE(prop->dat);
	    prop++;
	 }
	 _AL_FREE(datedit_info.prop);
	 datedit_info.prop = NULL;
      }
   }

   for (i=0; dat[i].type != DAT_END; i++) {
      if (dat[i].type == DAT_INFO) {
	 if (save) {
	    prop = dat[i].prop;
	    while ((prop) && (prop->type != DAT_END)) {
	       datedit_set_property(&datedit_info, prop->type, prop->dat);
	       prop++;
	    }
	 }

	 dat = datedit_delete(dat, i);
	 i--;
      }
   }

   return dat;
}



/* grabs a child datafile */
static DATAFILE *grab_datafile(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   DATAFILE *dat = load_datafile(filename);

   if (dat) {
      load_header(dat, datedit_pretty_name(filename, "h", TRUE));
      generate_names(dat, 0);
      dat = extract_info(dat, FALSE);
      return datedit_construct(type, dat, 0, prop);
   }

   return NULL;
}



/* queries whether this property belongs in the current strip mode */
static int should_save_prop(int type, AL_CONST int *fixed_prop, int strip)
{
   if (strip == 0)
      return TRUE;

   if (fixed_prop) {
      while (*fixed_prop) {
	 if (type == *fixed_prop++)
	    return TRUE;
      }
   }

   if (strip >= 2)
      return FALSE;
   else
      return (datedit_get_property(&builtin_prop, type) == empty_string);  /* not builtin */
}



/* wrapper to avoid division by zero when files are empty */
static int percent(int a, int b)
{
   if (a)
      return (b * 100) / a;
   else
      return 0;
}



/* saves an object */
static int save_object(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids,
                       int strip, int sort, int verbose, PACKFILE * AL_CONST f)
{
   int i, ret;
   DATAFILE_PROPERTY *prop;
   int (*save)(DATAFILE *, AL_CONST int *, int, int, int, int, int, int, PACKFILE *);
   PACKFILE *fchunk;

   ASSERT(f);

   prop = dat->prop;
   datedit_sort_properties(prop);

   while ((prop) && (prop->type != DAT_END)) {
      if (should_save_prop(prop->type, fixed_prop, strip)) {
	 pack_mputl(DAT_PROPERTY, f);
	 pack_mputl(prop->type, f);
	 pack_mputl(strlen(prop->dat), f);
	 if (pack_fwrite(prop->dat, strlen(prop->dat), f) < (signed)strlen(prop->dat))
	    return FALSE;
	 file_datasize += 12 + strlen(prop->dat);
      }

      prop++;
   }

   if (verbose)
      datedit_startmsg("%-28s", get_datafile_property(dat, DAT_NAME));

   pack_mputl(dat->type, f);
   fchunk = pack_fopen_chunk(f, ((!pack) && (pack_kids) && (dat->type != DAT_FILE)));
   if (!fchunk) {
      return FALSE;
   }
   file_datasize += 12;

   save = NULL;

   for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
      if (datedit_object_info[i]->type == dat->type) {
	 save = datedit_object_info[i]->save;
	 break;
      }
   }

   if (!save)
      save = save_binary;

   if (dat->type == DAT_FILE) {
      if (verbose)
	 datedit_endmsg("");

      ret = save((DATAFILE *)dat->dat, fixed_prop, pack, pack_kids, strip, sort, verbose, FALSE, fchunk);

      if (verbose)
	 datedit_startmsg("End of %-21s", get_datafile_property(dat, DAT_NAME));
   }
   else
      ret = save(dat, fixed_prop, (pack || pack_kids), FALSE, strip, sort, verbose, FALSE, fchunk);

   pack_fclose_chunk(fchunk);
   fchunk = NULL;

   if (verbose) {
      if ((!pack) && (pack_kids) && (dat->type != DAT_FILE)) {
	 datedit_endmsg("%7d bytes into %-7d (%d%%)", 
			_packfile_datasize, _packfile_filesize, 
			percent(_packfile_datasize, _packfile_filesize));
      }
      else
	 datedit_endmsg("");
   }

   if (dat->type == DAT_FILE)
      file_datasize += 4;
   else
      file_datasize += _packfile_datasize;

   return ret;
}



/* saves a datafile */
static int save_datafile(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f)
{
   int c, size;

   ASSERT(f);

   if (sort)
      datedit_sort_datafile(dat);

   size = 0;
   while (dat[size].type != DAT_END)
      size++;

   pack_mputl(extra ? size+1 : size, f);

   for (c=0; c<size; c++) {
      if (!save_object(dat+c, fixed_prop, pack, pack_kids, strip, sort, verbose, f))
	 return FALSE;
   }

   return TRUE;
}



/* creates a new datafile */
static void *makenew_file(long *size)
{
   DATAFILE *dat = _AL_MALLOC(sizeof(DATAFILE));

   dat->dat = NULL;
   dat->type = DAT_END;
   dat->size = 0;
   dat->prop = NULL;

   return dat;
}



/* header block for datafile objects */
DATEDIT_OBJECT_INFO datfile_info =
{
   DAT_FILE, 
   "Datafile", 
   NULL, 
   makenew_file,
   save_datafile,
   NULL,
   NULL,
   NULL
};



DATEDIT_GRABBER_INFO datfile_grabber =
{
   DAT_FILE, 
   "dat",
   "dat",
   grab_datafile,
   export_datafile,
   NULL
};



/* dummy header block to use as a terminator */
DATEDIT_OBJECT_INFO datend_info =
{
   DAT_END,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};



DATEDIT_GRABBER_INFO datend_grabber =
{
   DAT_END,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};



/* list of available object types */
DATEDIT_OBJECT_INFO *datedit_object_info[32] =
{
   &datfile_info, &datend_info,  &datend_info,  &datend_info,
   &datend_info,  &datend_info,  &datend_info,  &datend_info,
   &datend_info,  &datend_info,  &datend_info,  &datend_info,
   &datend_info,  &datend_info,  &datend_info,  &datend_info,
   &datend_info,  &datend_info,  &datend_info,  &datend_info,
   &datend_info,  &datend_info,  &datend_info,  &datend_info,
   &datend_info,  &datend_info,  &datend_info,  &datend_info,
   &datend_info,  &datend_info,  &datend_info,  &datend_info
};



/* list of available object grabber routines */
DATEDIT_GRABBER_INFO *datedit_grabber_info[32] =
{
   &datfile_grabber, &datend_grabber,  &datend_grabber,  &datend_grabber,
   &datend_grabber,  &datend_grabber,  &datend_grabber,  &datend_grabber,
   &datend_grabber,  &datend_grabber,  &datend_grabber,  &datend_grabber,
   &datend_grabber,  &datend_grabber,  &datend_grabber,  &datend_grabber,
   &datend_grabber,  &datend_grabber,  &datend_grabber,  &datend_grabber,
   &datend_grabber,  &datend_grabber,  &datend_grabber,  &datend_grabber,
   &datend_grabber,  &datend_grabber,  &datend_grabber,  &datend_grabber,
   &datend_grabber,  &datend_grabber,  &datend_grabber,  &datend_grabber
};



/* list of active menu hooks */
DATEDIT_MENU_INFO *datedit_menu_info[32] =
{
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};



/* adds a new object to the list of available methods */
void datedit_register_object(DATEDIT_OBJECT_INFO *info)
{
   int i = 0;

   while (datedit_object_info[i]->type != DAT_END)
      i++;

   datedit_object_info[i] = info;
}



/* adds a new grabber to the list of available methods */
void datedit_register_grabber(DATEDIT_GRABBER_INFO *info)
{
   int i = 0;

   while (datedit_grabber_info[i]->type != DAT_END)
      i++;

   datedit_grabber_info[i] = info;
}



/* adds a new entry to the list of active menus */
void datedit_register_menu(DATEDIT_MENU_INFO *info)
{
   int i = 0;

   while (datedit_menu_info[i])
      i++;

   datedit_menu_info[i] = info;
}



/* adds extensions to filenames if they are missing, or changes them */
char *datedit_pretty_name(AL_CONST char *name, AL_CONST char *ext, int force_ext)
{
   static char buf[256];
   char *s;

   strcpy(buf, name);

   s = get_extension(buf);
   if ((s > buf) && (*(s-1)=='.')) {
      if (force_ext)
	 strcpy(s, ext);
   }
   else {
      *s = '.';
      strcpy(s+1, ext);
   }

   fix_filename_case(buf);
   return buf;
}



/* returns a description string for an object */
AL_CONST char *datedit_desc(AL_CONST DATAFILE *dat)
{
   static char buf[256];
   int i;

   for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
      if (datedit_object_info[i]->type == dat->type) {
	 if (datedit_object_info[i]->get_desc)
	    datedit_object_info[i]->get_desc(dat, buf);
	 else
	    strcpy(buf, datedit_object_info[i]->desc);

	 return buf;
      }
   }

   sprintf(buf, "binary data (%ld bytes)", dat->size);
   return buf;
}



/* qsort callback for comparing datafile objects */
static int dat_cmp(AL_CONST void *e1, AL_CONST void *e2)
{
   DATAFILE *d1 = (DATAFILE *)e1;
   DATAFILE *d2 = (DATAFILE *)e2;

   return stricmp(get_datafile_property((AL_CONST DATAFILE*)d1, DAT_NAME), get_datafile_property((AL_CONST DATAFILE*)d2, DAT_NAME));
}



/* sorts a datafile */
void datedit_sort_datafile(DATAFILE *dat)
{
   int len;

   if (dat) {
      len = 0;
      while (dat[len].type != DAT_END)
	 len++;

      if (len > 1)
	 qsort(dat, len, sizeof(DATAFILE), dat_cmp);
   }
}



/* qsort callback for comparing datafile properties */
static int prop_cmp(AL_CONST void *e1, AL_CONST void *e2)
{
   DATAFILE_PROPERTY *p1 = (DATAFILE_PROPERTY *)e1;
   DATAFILE_PROPERTY *p2 = (DATAFILE_PROPERTY *)e2;

   return p1->type - p2->type;
}



/* sorts a list of object properties */
void datedit_sort_properties(DATAFILE_PROPERTY *prop)
{
   int len;

   if (prop) {
      len = 0;
      while (prop[len].type != DAT_END)
	 len++;

      if (len > 1)
	 qsort(prop, len, sizeof(DATAFILE_PROPERTY), prop_cmp);
   }
}



/* splits bitmaps into sub-sprites, using regions bounded by col #255 */
void datedit_find_character(BITMAP *bmp, int *x, int *y, int *w, int *h)
{
   int c1;
   int c2;

   if (bitmap_color_depth(bmp) == 8) {
      c1 = 255;
      c2 = 255;
   }
   else {
      c1 = makecol_depth(bitmap_color_depth(bmp), 255, 255, 0);
      c2 = makecol_depth(bitmap_color_depth(bmp), 0, 255, 255);
   }

   /* look for top left corner of character */
   while ((getpixel(bmp, *x, *y) != c1) || 
	  (getpixel(bmp, *x+1, *y) != c2) ||
	  (getpixel(bmp, *x, *y+1) != c2) ||
	  (getpixel(bmp, *x+1, *y+1) == c1) ||
	  (getpixel(bmp, *x+1, *y+1) == c2)) {
      (*x)++;
      if (*x >= bmp->w) {
	 *x = 0;
	 (*y)++;
	 if (*y >= bmp->h) {
	    *w = 0;
	    *h = 0;
	    return;
	 }
      }
   }

   /* look for right edge of character */
   *w = 0;
   while ((getpixel(bmp, *x+*w+1, *y) == c2) &&
	  (getpixel(bmp, *x+*w+1, *y+1) != c2) &&
	  (*x+*w+1 <= bmp->w))
      (*w)++;

   /* look for bottom edge of character */
   *h = 0;
   while ((getpixel(bmp, *x, *y+*h+1) == c2) &&
	  (getpixel(bmp, *x+1, *y+*h+1) != c2) &&
	  (*y+*h+1 <= bmp->h))
      (*h)++;
}



/* cleans up an object type string, and packs it */
int datedit_clean_typename(AL_CONST char *type)
{
   int c1, c2, c3, c4;

   if (!type)
      return 0;

   c1 = (*type) ? utoupper(*(type++)) : ' ';
   c2 = (*type) ? utoupper(*(type++)) : ' ';
   c3 = (*type) ? utoupper(*(type++)) : ' ';
   c4 = (*type) ? utoupper(*(type++)) : ' ';

   return DAT_ID(c1, c2, c3, c4);
}



/* returns the property string for the specified property (if any) */
AL_CONST char *datedit_get_property(DATAFILE_PROPERTY **prop, int type)
{
   DATAFILE_PROPERTY *iter = *prop;

   if (iter) {
      while (iter->type != DAT_END) {
	 if (iter->type == type)
	    return (iter->dat ? iter->dat : empty_string);

	 iter++;
      }
   }

   return empty_string;
}



/* insert a new property into a list without duplication */
void datedit_insert_property(DATAFILE_PROPERTY **prop, int type, AL_CONST char *value)
{
   DATAFILE_PROPERTY *iter = *prop, *the_prop;
   int size;

   if (iter) {
      size = 0;
      the_prop = NULL;

      for (; iter->type != DAT_END; iter++) {
         size++;
	 if (iter->type == type)
	    the_prop = iter;
	    /* no break, we need the full size of the array */
      }

      if ((value) && (strlen(value) > 0)) {
	 if (the_prop) {
	    the_prop->dat = _AL_REALLOC(the_prop->dat, strlen(value)+1);
	    strcpy(the_prop->dat, value);
	 }
	 else {
	    *prop = _AL_REALLOC(*prop, sizeof(DATAFILE_PROPERTY)*(size+2));
	    the_prop = *prop + size;
	    the_prop->type = type;
	    the_prop->dat = _AL_MALLOC(strlen(value)+1);
	    strcpy(the_prop->dat, value);
	    the_prop++;
	    the_prop->type = DAT_END;
	    the_prop->dat = NULL;
	 }
      }
      else {
	 if (the_prop) {
	    _AL_FREE(the_prop->dat);
	    for (iter = the_prop; iter->type != DAT_END; iter++)
	       *iter = *(iter+1);
	    *prop = _AL_REALLOC(*prop, sizeof(DATAFILE_PROPERTY)*size);
	 }
      }
   }
   else {
      if ((value) && (strlen(value) > 0)) {
	 *prop = _AL_MALLOC(sizeof(DATAFILE_PROPERTY)*2);
	 the_prop = *prop;
	 the_prop->type = type;
	 the_prop->dat = _AL_MALLOC(strlen(value)+1);
	 strcpy(the_prop->dat, value);
	 the_prop++;
	 the_prop->type = DAT_END;
	 the_prop->dat = NULL;
      }
   }
}



/* sets an object property string */
void datedit_set_property(DATAFILE *dat, int type, AL_CONST char *value)
{
   datedit_insert_property(&dat->prop, type, value);
}



/* loads a datafile */
DATAFILE *datedit_load_datafile(AL_CONST char *name, int compile_sprites, AL_CONST char *password)
{
   char *pretty_name;
   DATAFILE *datafile;

   _compile_sprites = compile_sprites;

   if (!compile_sprites) {
      typedef void (*DF)(void *);
      register_datafile_object(DAT_C_SPRITE, NULL, (void (*)(void *))destroy_bitmap);
      register_datafile_object(DAT_XC_SPRITE, NULL, (void (*)(void *))destroy_bitmap);
   }

   if (name)
      pretty_name = datedit_pretty_name(name, "dat", FALSE);
   else
      pretty_name = NULL;

   if ((pretty_name) && (exists(pretty_name))) {
      datedit_msg("Reading %s", pretty_name);

      packfile_password(password);

      datafile = load_datafile(pretty_name);

      packfile_password(NULL);

      if (!datafile) {
	 datedit_error("Error reading %s", pretty_name);
	 return NULL;
      }
      else {
	 load_header(datafile, datedit_pretty_name(name, "h", TRUE));
	 generate_names(datafile, 0);
      }
   }
   else {
      if (pretty_name)
	 datedit_msg("%s not found: creating new datafile", pretty_name);

      datafile = _AL_MALLOC(sizeof(DATAFILE));
      datafile->dat = NULL;
      datafile->type = DAT_END;
      datafile->size = 0;
      datafile->prop = NULL;
   }

   datafile = extract_info(datafile, TRUE);

   return datafile;
}



/* works out what name to give an exported object */
void datedit_export_name(AL_CONST DATAFILE *dat, AL_CONST char *name, AL_CONST char *ext, char *buf)
{
   AL_CONST char *obname = get_datafile_property(dat, DAT_NAME);
   AL_CONST char *oborig = get_datafile_property(dat, DAT_ORIG);
   char tmp[32];
   int i;

   if (name)
      strcpy(buf, name);
   else
      strcpy(buf, oborig);

   if (*get_filename(buf) == 0) {
      if (*oborig) {
	 strcat(buf, get_filename(oborig));
      }
      else {
	 strcat(buf, obname);
	 if (!ALLEGRO_LFN)
	    get_filename(buf)[8] = 0;
      }
   }

   if (ext) {
      strcpy(tmp, ext);
      for (i=0; tmp[i]; i++) {
	 if (tmp[i] == ';') {
	    tmp[i] = 0;
	    break;
	 }
      }
      strcpy(buf, datedit_pretty_name(buf, tmp, ((name == NULL) || (!*get_extension(name)))));
   }
   else
      fix_filename_case(buf);
}



/* exports a datafile object */
int datedit_export(AL_CONST DATAFILE *dat, AL_CONST char *name)
{
   AL_CONST char *obname = get_datafile_property(dat, DAT_NAME);
   int (*export)(AL_CONST DATAFILE *dat, AL_CONST char *filename) = NULL;
   char buf[256], tmp[256];
   char *ext = NULL;
   char *tok;
   int i;

   for (i=0; datedit_grabber_info[i]->type != DAT_END; i++) {
      if ((datedit_grabber_info[i]->type == dat->type) && (datedit_grabber_info[i]->export_ext)) {
	 ext = datedit_grabber_info[i]->export_ext;
	 break;
      }
   }

   datedit_export_name(dat, name, ext, buf);

   if (exists(buf)) {
      i = datedit_ask("%s already exists, overwrite", buf);
      if (i == 27)
	 return FALSE;
      else if ((i == 'n') || (i == 'N'))
	 return TRUE;
   }

   datedit_msg("Exporting %s -> %s", obname, buf);

   ext = get_extension(buf);

   for (i=0; datedit_grabber_info[i]->type != DAT_END; i++) {
      if ((datedit_grabber_info[i]->type == dat->type) && (datedit_grabber_info[i]->export_ext) && (datedit_grabber_info[i]->exporter)) {
	 strcpy(tmp, datedit_grabber_info[i]->export_ext);
	 tok = strtok(tmp, ";");
	 while (tok) {
	    if (stricmp(tok, ext) == 0) {
	       export = datedit_grabber_info[i]->exporter;
	       break;
	    }
	    tok = strtok(NULL, ";");
	 }
      }
   }

   if (!export)
      export = export_binary;

   if (!export(dat, buf)) {
      delete_file(buf);
      datedit_error("Error writing %s", buf);
      return FALSE;
   }

   return TRUE;
}



/* deletes a datafile object */
DATAFILE *datedit_delete(DATAFILE *dat, int i)
{
   _unload_datafile_object(dat+i);

   do {
      dat[i] = dat[i+1];
      i++;
   } while (dat[i].type != DAT_END);

   return _AL_REALLOC(dat, sizeof(DATAFILE)*i);
}



/* swaps two datafile objects */
void datedit_swap(DATAFILE *dat, int i1, int i2)
{
   DATAFILE temp = dat[i1];

   dat[i1] = dat[i2];
   dat[i2] = temp;
}



/* fixup function for the strip options */
int datedit_striptype(int strip)
{
   if (strip >= 0)
      return strip;
   else
      return 0;
}



/* fixup function for the pack options */
int datedit_packtype(int pack)
{
   if (pack >= 0) {
      char buf[80];

      sprintf(buf, "%d", pack);
      datedit_set_property(&datedit_info, DAT_PACK, buf);

      return pack;
   }
   else {
      AL_CONST char *p = get_datafile_property(&datedit_info, DAT_PACK);

      if (*p)
	 return atoi(p);
      else
	 return 0;
   }
}



/* fixup function for the sort options */
int datedit_sorttype(int sort)
{
   if (sort >= 0) {
      sort = (sort == 1 ? TRUE : FALSE);
      datedit_set_property(&datedit_info, DAT_SORT, sort ? "y" : "n");

      return sort;
   }
   else {
      AL_CONST char *p = get_datafile_property(&datedit_info, DAT_SORT);

      if (*p)
	 return (utolower(*p)=='y');
      else
	 return TRUE;  /* sort if SORT property not present */
   }
}



/* saves a datafile */
int datedit_save_datafile(DATAFILE *dat, AL_CONST char *name, AL_CONST int *fixed_prop, AL_CONST DATEDIT_SAVE_DATAFILE_OPTIONS *options, AL_CONST char *password)
{
   char *pretty_name;
   char backup_name[256];
   int pack, strip, sort;
   PACKFILE *f;
   int ret;

   packfile_password(password);

   pack = datedit_packtype(options->pack);
   strip = datedit_striptype(options->strip);
   sort = datedit_sorttype(options->sort);

   strcpy(backup_name, datedit_pretty_name(name, "bak", TRUE));
   pretty_name = datedit_pretty_name(name, "dat", FALSE);

   if (options->write_msg)
      datedit_msg("Writing %s", pretty_name);

   delete_file(backup_name);
   rename(pretty_name, backup_name);

   f = pack_fopen(pretty_name, (pack >= 2) ? F_WRITE_PACKED : F_WRITE_NOPACK);

   if (f) {
      pack_mputl(DAT_MAGIC, f);
      file_datasize = 12;

      ret = save_datafile(dat, fixed_prop, (pack >= 2), (pack >= 1), strip, sort, options->verbose, (strip <= 0), f);

      if ((ret == TRUE) && (strip <= 0)) {
	 datedit_set_property(&datedit_info, DAT_NAME, "GrabberInfo");
	 ret = save_object(&datedit_info, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, f);
      }

      pack_fclose(f); 
   }
   else
      ret = FALSE;

   if (ret == FALSE) {
      delete_file(pretty_name);
      datedit_error("Error writing %s", pretty_name);
      packfile_password(NULL);
      return FALSE;
   }
   
   if (!options->backup)
      delete_file(backup_name);

   if (options->verbose) {
      uint64_t file_filesize = file_size_ex(pretty_name);
      datedit_msg("%-28s%7d bytes into %-7d (%d%%)", "- GLOBAL COMPRESSION -",
		  file_datasize, file_filesize, percent(file_datasize, file_filesize));
   }

   packfile_password(NULL);
   return TRUE;
}



/* writes object definitions into a header file */
static void save_header(AL_CONST DATAFILE *dat, FILE *f, AL_CONST char *prefix)
{
   int c;

   fprintf(f, "\n");

   for (c=0; dat[c].type != DAT_END; c++) {
      fprintf(f, "#define %s%-*s %-8d /* %c%c%c%c */\n", 
	      prefix, 32-(int)strlen(prefix),
	      get_datafile_property(dat+c, DAT_NAME), c,
	      (dat[c].type>>24), (dat[c].type>>16)&0xFF, 
	      (dat[c].type>>8)&0xFF, (dat[c].type&0xFF));

      if (dat[c].type == DAT_FILE) {
	 char p[256];

	 strcpy(p, prefix);
	 strcat(p, get_datafile_property(dat+c, DAT_NAME));
	 strcat(p, "_");

	 save_header((DATAFILE *)dat[c].dat, f, p);
      }
   }

   if (*prefix)
      fprintf(f, "#define %s%-*s %d\n", prefix, 32-(int)strlen(prefix), "COUNT", c);

   fprintf(f, "\n");
}



/* helper for renaming files (works across drives) */
static int rename_file(AL_CONST char *oldname, AL_CONST char *newname)
{
   PACKFILE *oldfile, *newfile;
   int c;

   oldfile = pack_fopen(oldname, F_READ);
   if (!oldfile)
      return -1;

   newfile = pack_fopen(newname, F_WRITE);
   if (!newfile) {
      pack_fclose(oldfile);
      return -1;
   }

   c = pack_getc(oldfile);

   while (c != EOF) {
      pack_putc(c, newfile);
      c = pack_getc(oldfile);
   } 

   pack_fclose(oldfile);
   pack_fclose(newfile);

   delete_file(oldname); 

   return 0;
}



/* checks whether the header needs updating */
static int cond_update_header(AL_CONST char *tn, AL_CONST char *n, int verbose)
{
   PACKFILE *f1, *f2;
   char b1[256], b2[256];
   int i;
   int differ = FALSE;

   if (!exists(n)) {
      if (rename_file(tn, n) != 0)
	 return FALSE;
   }
   else {
      f1 = pack_fopen(tn, F_READ);
      if (!f1)
	 return FALSE;

      f2 = pack_fopen(n, F_READ);
      if (!f2) {
	 pack_fclose(f1);
	 return FALSE;
      }

      for (i=0; i<4; i++) {
	 /* skip date, which may differ */
	 pack_fgets(b1, 255, f1);
	 pack_fgets(b2, 255, f2);
      }

      while ((!pack_feof(f1)) && (!pack_feof(f2)) && (!differ)) {
	 pack_fgets(b1, 255, f1);
	 pack_fgets(b2, 255, f2);
	 if (strcmp(b1, b2) != 0)
	    differ = TRUE;
      }

      if ((!pack_feof(f1)) || (!pack_feof(f2)))
	 differ = TRUE;

      pack_fclose(f1);
      pack_fclose(f2);

      if (differ) {
	 if (verbose)
	    datedit_msg("%s has changed: updating", n);

	 delete_file(n);
	 rename_file(tn, n);
      }
      else {
	 if (verbose)
	    datedit_msg("%s has not changed: no update", n);

	 delete_file(tn);
      }
   }

   return TRUE;
}



/* exports a datafile header */
int datedit_save_header(AL_CONST DATAFILE *dat, AL_CONST char *name, AL_CONST char *headername, AL_CONST char *progname, AL_CONST char *prefix, int verbose)
{
   char *pretty_name, *tmp_name;
   char tm[80];
   char p[80];
   time_t now;
   FILE *f;
   int c;

   #ifdef ALLEGRO_HAVE_MKSTEMP

      char tmp_buf[] = "XXXXXX";
      char tmp[512];
      int tmp_fd;

      tmp_fd = mkstemp(tmp_buf);
      if (tmp_fd < 0) {
	 datedit_error("Error creating temporary file");
	 return FALSE;
      }
      close(tmp_fd);
      tmp_name = uconvert_ascii(tmp_buf, tmp);

   #else

      tmp_name = tmpnam(NULL);

   #endif

   if (prefix)
      strcpy(p, prefix);
   else
      strcpy(p, get_datafile_property(&datedit_info, DAT_HPRE));

   if ((p[0]) && (p[(strlen(p)-1)] != '_'))
      strcat(p, "_");

   pretty_name = datedit_pretty_name(headername, "h", FALSE);
   datedit_msg("Writing ID's into %s", pretty_name);

   f = fopen(tmp_name, "w");
   if (f) {
      time(&now);
      strcpy(tm, asctime(localtime(&now)));
      for (c=0; tm[c]; c++)
	 if ((tm[c] == '\r') || (tm[c] == '\n'))
	    tm[c] = 0;

      fprintf(f, "/* Allegro datafile object indexes, produced by %s v" ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR " */\n", progname);
      fprintf(f, "/* Datafile: %s */\n", name);
      fprintf(f, "/* Date: %s */\n", tm);
      fprintf(f, "/* Do not hand edit! */\n");

      save_header(dat, f, p);

      fclose(f);

      if (!cond_update_header(tmp_name, pretty_name, verbose)) {
	 datedit_error("Error writing %s", pretty_name);
	 return FALSE;
      }
   }
   else {
      datedit_error("Error writing %s", pretty_name);
      return FALSE;
   }

   return TRUE;
}



/* converts a file timestamp from ASCII to integer representation */
long datedit_asc2ftime(AL_CONST char *time)
{
   static char *sep = "-,: ";
   char tmp[256], *tok;
   struct tm t;

   memset(&t, 0, sizeof(struct tm));

   strcpy(tmp, time);
   tok = strtok(tmp, sep);

   if (tok) {
      t.tm_mon = atoi(tok)-1;
      tok = strtok(NULL, sep);
      if (tok) {
	 t.tm_mday = atoi(tok);
	 tok = strtok(NULL, sep);
	 if (tok) {
	    t.tm_year = atoi(tok)-1900;
	    tok = strtok(NULL, sep);
	    if (tok) {
	       t.tm_hour = atoi(tok);
	       tok = strtok(NULL, sep);
	       if (tok) {
		  t.tm_min = atoi(tok);
	       }
	    }
	 }
      }
   }

   {
      /* make timezone adjustments by converting to time_t with adjustment 
       * from local time, then back again as GMT (=UTC) */
      time_t tm = mktime (&t);
      if (tm != (time_t)-1) {               /* cast needed in djgpp */
	 struct tm *temp = gmtime (&tm);
	 if (temp) memcpy (&t, temp, sizeof t);
      }
   }

   return mktime(&t);
}



/* converts a file timestamp from integer to ASCII representation */
AL_CONST char *datedit_ftime2asc(long time)
{
   static char buf[80];

   time_t tim = time;
   struct tm *t = gmtime(&tim);

   sprintf(buf, "%d-%02d-%d, %d:%02d",
		t->tm_mon+1, t->tm_mday, t->tm_year+1900,
		t->tm_hour, t->tm_min);

   return buf;
}



/* converts a file timestamp to international ASCII representation */
AL_CONST char *datedit_ftime2asc_int(long time)
{
   static char month[12][4] =
   {
       "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
   };

   static char buf[80];

   time_t tim = time;
   struct tm *t = gmtime(&tim);

   sprintf(buf, "%.3s-%02d-%d, %d:%02d",
		month[t->tm_mon%12], t->tm_mday, t->tm_year+1900,
		t->tm_hour, t->tm_min);

   return buf;
}



/* constructs a datafile object
 * WARNING: the object is private so it must be duplicated.
 */
DATAFILE *datedit_construct(int type, void *dat, long size, DATAFILE_PROPERTY **prop)
{
   static DATAFILE datafile;

   if (!dat)
      return NULL;

   ASSERT(size >= 0);

   datafile.type = type;
   datafile.dat = dat;
   datafile.size = size;
   if (prop != NULL) {
      datafile.prop = *prop;

      /* Make sure we own the property list. */
      *prop = NULL;
   }
   else {
      datafile.prop = NULL;
   }

   return &datafile;
}



/* clone properties */
static DATAFILE_PROPERTY *clone_properties(DATAFILE_PROPERTY *prop)
{
   DATAFILE_PROPERTY *clone, *iter;
   int size = 0, i;

   if (!prop)
      return NULL;

   for (iter = prop; iter->type != DAT_END; iter++) {
      size++;
   }

   clone = _AL_MALLOC(sizeof(DATAFILE_PROPERTY)*(size+1));

   for (i = 0; i <= size; i++) {
       clone[i].type = prop[i].type;
       if (prop[i].dat)
          clone[i].dat = ustrdup(prop[i].dat);
       else
          clone[i].dat = NULL;
   }

   return clone;
}



/* grabs an object from a disk file
 * WARNING: the object is private so it must be duplicated.
 */
DATAFILE *datedit_grab(DATAFILE_PROPERTY *prop, AL_CONST DATEDIT_GRAB_PARAMETERS *params)
{
   DATAFILE *dat;
   DATAFILE *(*grab)(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth);
   char *ext = get_extension(params->filename);
   char *tok;
   char tmp[1024];
   int type, type_index, c;

   type = params->type;

   if (!type) {
      type = DAT_DATA;

      if ((ext) && (*ext)) {
	 for (c=0; datedit_grabber_info[c]->type != DAT_END; c++) {
	    if (datedit_grabber_info[c]->grab_ext) {
	       strcpy(tmp, datedit_grabber_info[c]->grab_ext);
	       tok = strtok(tmp, ";");
	       while (tok) {
		  if (stricmp(tok, ext) == 0) {
		     type = datedit_grabber_info[c]->type;
		     goto found_type;
		  }
		  tok = strtok(NULL, ";");
	       }
	    }
	 }
      }
   }

  found_type:
   type_index = -1;

   for (c=0; datedit_grabber_info[c]->type != DAT_END; c++) {
      if ((datedit_grabber_info[c]->type == type) && (datedit_grabber_info[c]->grab_ext) && (datedit_grabber_info[c]->grab)) {
	 if ((ext) && (*ext)) {
	    strcpy(tmp, datedit_grabber_info[c]->grab_ext);
	    tok = strtok(tmp, ";");
	    while (tok) {
	       if (stricmp(tok, ext) == 0) {
		  type_index = c;
		  goto found_grabber;
	       }
	       tok = strtok(NULL, ";");
	    }
	 }
	 if (!type_index < 0)
	    type_index = c;
      }
   }

  found_grabber:
   if (type_index >= 0)
      grab = datedit_grabber_info[type_index]->grab;
   else
      grab = grab_binary;

   if (prop)
      prop = clone_properties(prop);
   else {
      /* Set standard properties. */
      datedit_insert_property(&prop, DAT_NAME, params->name);
      if (params->relative) {
	 make_relative_filename(tmp, params->datafile, params->filename, sizeof(tmp));
	 datedit_insert_property(&prop, DAT_ORIG, tmp);
      }
      else
	 datedit_insert_property(&prop, DAT_ORIG, params->filename);

      /* Set specific properties. */
      if ((type_index >= 0) && (datedit_grabber_info[type_index]->prop_types)) {
	 strcpy(tmp, datedit_grabber_info[type_index]->prop_types);
	 tok = strtok(tmp, ";");
	 while (tok) {
	    if (stricmp(tok, "XPOS") == 0) {
	       sprintf(tmp, "%d", params->x);
	       datedit_insert_property(&prop, DAT_XPOS, tmp);
	    }
	    else if (stricmp(tok, "YPOS") == 0) {
	       sprintf(tmp, "%d", params->y);
	       datedit_insert_property(&prop, DAT_YPOS, tmp);
	    }
	    else if (stricmp(tok, "XSIZ") == 0) {
	       sprintf(tmp, "%d", params->w);
	       datedit_insert_property(&prop, DAT_XSIZ, tmp);
	    }
	    else if (stricmp(tok, "YSIZ") == 0) {
	       sprintf(tmp, "%d", params->h);
	       datedit_insert_property(&prop, DAT_YSIZ, tmp);
	    }

	    tok = strtok(NULL, ";");
	 }
      }
   }

   /* Always modify the date. */
   datedit_insert_property(&prop, DAT_DATE, datedit_ftime2asc(file_time(params->filename)));

   dat = grab(type, params->filename, &prop, params->colordepth);

   if (!dat) {
      datedit_error("Error reading %s as type %c%c%c%c", params->filename, 
		    type>>24, (type>>16)&0xFF, (type>>8)&0xFF, type&0xFF);
      if (prop)
	 free(prop);
   }

   return dat;
}



/* grabs an object over the top of an existing one */
int datedit_grabreplace(DATAFILE *dat, AL_CONST DATEDIT_GRAB_PARAMETERS *params)
{
   DATAFILE *tmp = datedit_grab(NULL, params);

   if (tmp) {
      _unload_datafile_object(dat);
      *dat = *tmp;
      return TRUE;
   }
   else
      return FALSE;
}



/* updates an object in-place */
int datedit_grabupdate(DATAFILE *dat, DATEDIT_GRAB_PARAMETERS *params)
{
   DATAFILE *tmp;

   params->name = "dummyname";
   params->type = dat->type;
   params->colordepth = -1;

   tmp = datedit_grab(dat->prop, params);

   if (tmp) {
      /* adjust color depth? */
      if ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
	  (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) {

	 int src_depth, dest_depth;

	 if (dat->type == DAT_RLE_SPRITE) {
	    dest_depth = ((RLE_SPRITE *)dat->dat)->color_depth;
	    src_depth = ((RLE_SPRITE *)tmp->dat)->color_depth;
	 }
	 else {
	    dest_depth = bitmap_color_depth(dat->dat);
	    src_depth = bitmap_color_depth(tmp->dat);
	 }

	 if (src_depth != dest_depth) {
	    BITMAP *b1, *b2;

	    if (dat->type == DAT_RLE_SPRITE) {
	       RLE_SPRITE *spr = (RLE_SPRITE *)tmp->dat;
	       b1 = create_bitmap_ex(src_depth, spr->w, spr->h);
	       clear_to_color(b1, b1->vtable->mask_color);
	       draw_rle_sprite(b1, spr, 0, 0);
	       destroy_rle_sprite(spr);
	    }
	    else
	       b1 = (BITMAP *)tmp->dat;

	    if (dest_depth == 8)
	       datedit_msg("Warning: lossy conversion from truecolor to 256 colors!");

	    if ((dat->type == DAT_RLE_SPRITE) ||
		(dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) {
	       datedit_last_read_pal[0].r = 63;
	       datedit_last_read_pal[0].g = 0;
	       datedit_last_read_pal[0].b = 63;
	    }

	    select_palette(datedit_last_read_pal);
	    b2 = create_bitmap_ex(dest_depth, b1->w, b1->h);
	    blit(b1, b2, 0, 0, 0, 0, b1->w, b1->h);
	    unselect_palette();

	    if (dat->type == DAT_RLE_SPRITE) {
	       tmp->dat = get_rle_sprite(b2);
	       destroy_bitmap(b1);
	       destroy_bitmap(b2);
	    }
	    else {
	       tmp->dat = b2;
	       destroy_bitmap(b1);
	    }
	 }
      }

      _unload_datafile_object(dat);
      *dat = *tmp;
      return TRUE;
   }
   else
      return FALSE;
}



/* grabs a new object, inserting it into the datafile */
DATAFILE *datedit_grabnew(DATAFILE *dat, AL_CONST DATEDIT_GRAB_PARAMETERS *params)
{
   DATAFILE *tmp = datedit_grab(NULL, params);
   int len;

   if (tmp) {
      len = 0;
      while (dat[len].type != DAT_END)
	 len++;

      dat = _AL_REALLOC(dat, sizeof(DATAFILE)*(len+2));
      dat[len+1] = dat[len];
      dat[len] = *tmp;
      return dat;
   }
   else
      return NULL;
}



/* inserts a new object into the datafile */
DATAFILE *datedit_insert(DATAFILE *dat, DATAFILE **ret, AL_CONST char *name, int type, void *v, long size)
{
   int len;

   len = 0;
   while (dat[len].type != DAT_END)
      len++;

   dat = _AL_REALLOC(dat, sizeof(DATAFILE)*(len+2));
   dat[len+1] = dat[len];

   dat[len].dat = v;
   dat[len].type = type;
   dat[len].size = size;
   dat[len].prop = NULL;
   datedit_set_property(dat+len, DAT_NAME, name);

   if (ret)
      *ret = dat+len;

   return dat;
}



/* wrapper for examining numeric property values */
int datedit_numprop(DATAFILE_PROPERTY **prop, int type)
{
   AL_CONST char *p = datedit_get_property(prop, type);

   if (*p)
      return atoi(p);
   else 
      return -1;
}



/* scans plugins to find available import/export file extensions */
static AL_CONST char *make_ext_list(int type, int grab)
{
   static char buf[256];
   char extlist[256][16];
   int num_ext = 0;
   char *s;
   int i, j;
   int done;

   for (i=0; datedit_grabber_info[i]->type != DAT_END; i++) {
      if (datedit_grabber_info[i]->type == type) {
	 if (grab)
	    s = datedit_grabber_info[i]->grab_ext;
	 else if (!grab)
	    s = datedit_grabber_info[i]->export_ext;
	 else
	    s = NULL;

	 if (s) {
	    strcpy(buf, s);
	    s = strtok(buf, ";");
	    while (s) {
	       for (j=0; j<num_ext; j++) {
		  if (stricmp(s, extlist[j]) == 0)
		     break;
	       }
	       if (j >= num_ext) {
		  strcpy(extlist[num_ext], s);
		  num_ext++;
	       }
	       s = strtok(NULL, ";");
	    }
	 }
      }
   }

   if (num_ext <= 0)
      return NULL;

   do {
      done = TRUE;

      for (i=0; i<num_ext-1; i++) {
	 if (stricmp(extlist[i], extlist[i+1]) > 0) {
	    strcpy(buf, extlist[i]);
	    strcpy(extlist[i], extlist[i+1]);
	    strcpy(extlist[i+1], buf);
	    done = FALSE;
	 }
      }
   } while (!done);

   buf[0] = 0;

   for (i=0; i<num_ext; i++) {
      strcat(buf, extlist[i]);
      if (i<num_ext-1)
	 strcat(buf, ";");
   }

   return buf;
}



/* returns a list of suitable file extensions for this object type */
AL_CONST char *datedit_grab_ext(int type)
{
   return make_ext_list(type, TRUE);
}



/* returns a list of suitable file extensions for this object type */
AL_CONST char *datedit_export_ext(int type)
{
   return make_ext_list(type, FALSE);
}


/* (un)conditionally update an object */
int datedit_update(DATAFILE *dat, AL_CONST char *datafile, int force, int verbose, int *changed)
{
   AL_CONST char *name = get_datafile_property(dat, DAT_NAME);
   AL_CONST char *origin = get_datafile_property(dat, DAT_ORIG);
   DATEDIT_GRAB_PARAMETERS params;
   char filename[1024];
   int relf;

   if (!*origin) {
      datedit_msg("%s has no origin data - skipping", name);
      return TRUE;
   }

   if (is_relative_filename(origin)) {
      make_absolute_filename(filename, datafile, origin, sizeof(filename));
      relf = TRUE;
   }
   else {
      strcpy(filename, origin);
      relf = FALSE;
   }
   
   if (!exists(filename)) {
      datedit_msg("%s: %s not found - skipping", name, origin);
      return TRUE;
   }

   if (!force) {
      AL_CONST char *date = get_datafile_property(dat, DAT_DATE);

      if (date[0]) {
	 time_t origt = file_time(filename);
	 time_t datat = datedit_asc2ftime(date);

	 if ((origt/60) <= (datat/60)) {
	    if (verbose)
	       datedit_msg("%s: %s has not changed - skipping", name, origin);
	    return TRUE;
	 }
      }
   }

   datedit_msg("Updating %s -> %s", origin, name);

   if (changed)
      *changed = TRUE;

   params.datafile = datafile;
   params.filename = filename;
   /* params.name = */
   /* params.type = */
   /* params.x = */
   /* params.y = */
   /* params.w = */
   /* params.h = */
   /* params.colordepth = */
   params.relative = relf;

   return datedit_grabupdate(dat, &params);
}

