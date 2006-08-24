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
 *      File attributes test program for the Allegro library.
 *
 *      By Eric Botcazou.
 *
 *      Based on src/fsel.c by Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


/* Define to test the low-level al_find*() functions */
#undef USE_FINDFIRST


#include <time.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"


#define FLIST_SIZE      2048

typedef struct FLIST
{
   char dir[1024];
   int size;
   char *name[FLIST_SIZE];
} FLIST;

static FLIST *flist = NULL;
static char updir[1024];


#define ATTR_OFFSET 0
#define TIME_OFFSET 7
#define SIZE_OFFSET 24
#define NAME_OFFSET 33

static int global_attr = 0;


static int fa_button_proc(int, DIALOG *, int);
static int fa_filename_proc(int, DIALOG *, int);
static int fa_flist_proc(int, DIALOG *, int);
static char *fa_flist_getter(int, int *);


static DIALOG fa_viewer[] =
{
   /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)              (dp2) (dp3) */
   { d_shadow_box_proc,    15,   47,   610,  385,   0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
   { d_ctext_proc,        319,   66,     1,    8,   0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
   { d_button_proc,       495,  378,   110,   17,   0,    0,    27,   D_EXIT,  0,    0,    NULL,             NULL, NULL  },
   { fa_filename_proc,     35,   97,   557,    8,   0,    0,    0,    0,       79,   0,    NULL,             NULL, NULL  },
   { fa_flist_proc,        35,  126,   440,  269,   0,    0,    0,    D_EXIT,  0,    0,    fa_flist_getter,  NULL, NULL  },
   { d_box_proc,          495,  126,   110,  232,   0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
   { fa_button_proc,      505,  146,    90,   17,   0,    0,    0,    0,       1,    FA_RDONLY,"FA_RDONLY",  NULL, NULL  },
   { fa_button_proc,      505,  183,    90,   17,   0,    0,    0,    0,       1,    FA_HIDDEN,"FA_HIDDEN",  NULL, NULL  },
   { fa_button_proc,      505,  220,    90,   17,   0,    0,    0,    0,       1,    FA_SYSTEM,"FA_SYSTEM",  NULL, NULL  },
   { fa_button_proc,      505,  257,    90,   17,   0,    0,    0,    0,       1,    FA_LABEL,"FA_LABEL",    NULL, NULL  },
   { fa_button_proc,      505,  294,    90,   17,   0,    0,    0,    0,       1,    FA_DIREC,"FA_DIREC",    NULL, NULL  },
   { fa_button_proc,      505,  331,    90,   17,   0,    0,    0,    0,       1,    FA_ARCH,"FA_ARCH",      NULL, NULL  },
   { d_yield_proc,          0,    0,     0,    0,   0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
   { NULL,                  0,    0,     0,    0,   0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  }
};


#define FA_FRAME	0
#define FA_MESSAGE      1
#define FA_OK           2
#define FA_TEXT         3
#define FA_FILES        4
#define FA_BOX          5



/* fa_button_proc:
 *  Dialog procedure for the file attribute selection buttons.
 */
static int fa_button_proc(int msg, DIALOG *d, int c)
{
   int ret;

   if ((msg == MSG_CLICK) || (msg == MSG_KEY)) {
      ret = d_check_proc(msg, d, c);

      if (d->flags & D_SELECTED)
         global_attr |= d->d2;
      else
         global_attr &= ~(d->d2);

      object_message(fa_viewer+FA_FILES, MSG_START, 0);
      object_message(fa_viewer+FA_FILES, MSG_DRAW, 0);

      return ret;
   }
   else
      return d_check_proc(msg, d, c);
}



/* fa_filename_proc:
 *  Dialog procedure for the filename displayer.
 */
static int fa_filename_proc(int msg, DIALOG *d, int c)
{
   char *s = d->dp;
   int size = (d->d1 + 1) * uwidth_max(U_CURRENT); /* of s (in bytes) */
   int list_size;
   int found = 0;
   char b[1024], tmp[16];
   int ch, attr;
   int i;

   if (msg == MSG_START) {
      canonicalize_filename(b, s, sizeof(b));
      ustrzcpy(s, size, b);
   }

   if (msg == MSG_KEY) {
      if ((!ugetc(s)) || (ugetat(s, -1) == DEVICE_SEPARATOR))
	 ustrzcat(s, size, uconvert_ascii("./", tmp));

      canonicalize_filename(b, s, sizeof(b));
      ustrzcpy(s, size - ucwidth(OTHER_PATH_SEPARATOR), b);

      ch = ugetat(s, -1);
      if ((ch != '/') && (ch != OTHER_PATH_SEPARATOR)) {
	 if (file_exists(s, FA_RDONLY | FA_HIDDEN | FA_DIREC, &attr)) {
	    if (attr & FA_DIREC)
               put_backslash(s);
	    else
	       return D_O_K;
	 }
	 else
	    return D_O_K;
      }

      object_message(fa_viewer+FA_FILES, MSG_START, 0);
      /* did we `cd ..' ? */
      if (ustrlen(updir)) {
	 /* now we have to find a directory name equal to updir */
	 for (i = 0; i<flist->size; i++) {
	    if (!ustrcmp(updir, flist->name[i]+NAME_OFFSET)) {  /* we got it ! */
	       fa_viewer[FA_FILES].d1 = i;
	       /* we have to know the number of visible lines in the filelist */
	       /* -1 to avoid an off-by-one problem */
               list_size = (fa_viewer[FA_FILES].h-4) / text_height(font) - 1;
               if (i>list_size)
		  fa_viewer[FA_FILES].d2 = i-list_size;
	       else
		  fa_viewer[FA_FILES].d2 = 0;
               found = 1;
	       break;  /* ok, our work is done... */
	    }
	 }
	 /* by some strange reason, we didn't find the old directory... */
         if (!found) {
            fa_viewer[FA_FILES].d1 = 0;
            fa_viewer[FA_FILES].d2 = 0;
         }
      }
      /* and continue... */
      object_message(fa_viewer+FA_FILES, MSG_DRAW, 0);
      object_message(d, MSG_START, 0);
      object_message(d, MSG_DRAW, 0);

      return D_O_K;
   }

   if (msg == MSG_WANTFOCUS)
      return D_O_K;
   else
      return d_edit_proc(msg, d, c);
}



/* ustrfilecmp:
 *  ustricmp for filenames: makes sure that eg "foo.bar" comes before
 *  "foo-1.bar", and also that "foo9.bar" comes before "foo10.bar".
 */
static int ustrfilecmp(AL_CONST char *s1, AL_CONST char *s2)
{
   int c1, c2;
   int x1, x2;
   char *t1, *t2;

   for (;;) {
      c1 = utolower(ugetxc(&s1));
      c2 = utolower(ugetxc(&s2));

      if ((c1 >= '0') && (c1 <= '9') && (c2 >= '0') && (c2 <= '9')) {
	 x1 = ustrtol(s1 - ucwidth(c1), &t1, 10);
	 x2 = ustrtol(s2 - ucwidth(c2), &t2, 10);
	 if (x1 != x2)
	    return x1 - x2;
	 else if (t1 - s1 != t2 - s2)
	    return (t2 - s2) - (t1 - s1);
	 s1 = t1;
	 s2 = t2;
      }
      else if (c1 != c2) {
	 if (!c1)
	    return -1;
	 else if (!c2)
	    return 1;
	 else if (c1 == '.')
	    return -1;
	 else if (c2 == '.')
	    return 1;
	 return c1 - c2;
      }

      if (!c1)
	 return 0;
   }
}



/* put_attrib:
 *  Helper function for displaying the file attributes.
 */
static void put_attrib(char *buffer, int attrib)
{
   int pos;

   if (attrib & FA_RDONLY)
      pos = usetc(buffer, 'r');
   else
      pos = usetc(buffer, '-');

   if (attrib & FA_HIDDEN)
      pos += usetc(buffer+pos, 'h');
   else
      pos += usetc(buffer+pos, '-');

   if (attrib & FA_SYSTEM)
      pos += usetc(buffer+pos, 's');
   else
      pos += usetc(buffer+pos, '-');

   if (attrib & FA_LABEL)
      pos += usetc(buffer+pos, 'l');
   else
      pos += usetc(buffer+pos, '-');

   if (attrib & FA_DIREC)
      pos += usetc(buffer+pos, 'd');
   else
      pos += usetc(buffer+pos, '-');

   if (attrib & FA_ARCH)
      pos += usetc(buffer+pos, 'a');
   else
      pos += usetc(buffer+pos, '-');

   pos += usetc(buffer+pos, ' ');
   usetc(buffer+pos, 0);
}



/* put_time:
 *  Helper function for displaying the file time.
 */
static void put_time(char *buffer, time_t time)
{
   char tmp1[64], tmp2[256];

   strftime(tmp1, sizeof(tmp1), "%m/%d/%Y %H:%M ", localtime(&time));
   
   ustrcpy(buffer, uconvert_ascii(tmp1, tmp2));
}



/* put_size:
 *  Helper function for displaying the file size.
 */
static void put_size(char *buffer, uint64_t size)
{
   char tmp1[128];

   if (size)
      usprintf(buffer, uconvert_ascii("%5d kb ", tmp1), 1 + size/1024);
   else
      ustrcpy(buffer, uconvert_ascii("         ", tmp1));
}



/* fa_flist_putter:
 *  Callback function for for_each_file() to fill the listbox.
 */
#ifdef USE_FINDFIRST
static void fa_flist_putter(struct al_ffblk *info)
#else
static int fa_flist_putter(AL_CONST char *str, int attrib, void *param)
#endif
{
   char *s, *name;
   int c, c2;

#ifdef USE_FINDFIRST
   int attrib = info->attrib;
   s = info->name;
#else
   s = get_filename(str);
#endif

   fix_filename_case(s);

   if ((flist->size < FLIST_SIZE) && ((ugetc(s) != '.') || (ugetat(s, 1)))) {
      name = malloc(NAME_OFFSET + ustrsizez(s) + ((attrib & FA_DIREC) ? ucwidth(OTHER_PATH_SEPARATOR) : 0));
      if (!name)
#ifdef USE_FINDFIRST
	 return;
#else
	 return -1;
#endif

      for (c=0; c<flist->size; c++) {
	 if (ugetat(flist->name[c], -1) == OTHER_PATH_SEPARATOR) {
	    if (attrib & FA_DIREC)
	       if (ustrfilecmp(s, flist->name[c] + NAME_OFFSET) < 0)
		  break;
	 }
	 else {
	    if (attrib & FA_DIREC)
	       break;
	    if (ustrfilecmp(s, flist->name[c] + NAME_OFFSET) < 0)
	       break;
	 }
      }

      for (c2=flist->size; c2>c; c2--)
	 flist->name[c2] = flist->name[c2-1];

      flist->name[c] = name;
      put_attrib(flist->name[c] + ATTR_OFFSET, attrib);

#ifdef USE_FINDFIRST
      put_time(flist->name[c] + TIME_OFFSET, info->time);
      put_size(flist->name[c] + SIZE_OFFSET, al_ffblk_get_size(info));
#else
      put_time(flist->name[c] + TIME_OFFSET, file_time(str));
      put_size(flist->name[c] + SIZE_OFFSET, file_size_ex(str));
#endif

      ustrcpy(flist->name[c] + NAME_OFFSET, s);

      if (attrib & FA_DIREC)
	 put_backslash(flist->name[c]);

      flist->size++;
   }
  
#ifdef USE_FINDFIRST
   return;
#else
   return 0;
#endif
}



/* fa_flist_getter:
 *  Listbox data getter routine for the file list.
 */
static char *fa_flist_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = flist->size;
      return NULL;
   }

   return flist->name[index];
}



/* fa_flist_proc:
 *  Dialog procedure for the file list.
 */
static int fa_flist_proc(int msg, DIALOG *d, int c)
{
   static int recurse_flag = 0;
   char *s = fa_viewer[FA_TEXT].dp;
   char tmp[32];
   int size = (fa_viewer[FA_TEXT].d1 + 1) * uwidth_max(U_CURRENT); /* of s (in bytes) */
   int sel = d->d1;
   int i, ret;
   int ch, count;

#ifdef USE_FINDFIRST
   struct al_ffblk info;
#endif

   if (msg == MSG_START) {
      if (!flist) {
	 flist = malloc(sizeof(FLIST));

	 if (!flist) {
	    *allegro_errno = ENOMEM;
	    return D_CLOSE;
	 }
      }
      else {
	 for (i=0; i<flist->size; i++)
	    if (flist->name[i])
	       free(flist->name[i]);
      }

      flist->size = 0;

      replace_filename(flist->dir, s, uconvert_ascii("*.*", tmp), sizeof(flist->dir));

#ifdef USE_FINDFIRST
      if (al_findfirst(flist->dir, &info, global_attr) == 0) {
         do {
            fa_flist_putter(&info);

         } while (al_findnext(&info) == 0);

         al_findclose(&info);
      }

      if (*allegro_errno == ENOENT)
         *allegro_errno = 0;
#else
      for_each_file_ex(flist->dir, global_attr, 0, fa_flist_putter, NULL);
#endif

      if (*allegro_errno)
	 alert(NULL, get_config_text("Disk error"), NULL, get_config_text("OK"), NULL, 13, 0);

      usetc(get_filename(flist->dir), 0);
      d->d1 = d->d2 = 0;
      sel = 0;
   }

   if (msg == MSG_END) {
      if (flist) {
	 for (i=0; i<flist->size; i++)
	    if (flist->name[i])
	       free(flist->name[i]);
	 free(flist);
	 flist = NULL;
      }
   }

   recurse_flag++;
   ret = d_text_list_proc(msg, d, c);     /* call the parent procedure */
   recurse_flag--;

   if (((sel != d->d1) || (ret == D_CLOSE)) && (recurse_flag == 0)) {
      replace_filename(s, flist->dir, flist->name[d->d1]+NAME_OFFSET, size);
      /* check if we want to `cd ..' */
      if ((!ustrncmp(flist->name[d->d1]+NAME_OFFSET, uconvert_ascii("..", tmp), 2)) && (ret == D_CLOSE)) {
	 /* let's remember the previous directory */
	 usetc(updir, 0);
	 i = ustrlen(flist->dir);
	 count = 0;
	 while (i>0) {
	    ch = ugetat(flist->dir, i);
	    if ((ch == '/') || (ch == OTHER_PATH_SEPARATOR)) {
	       if (++count == 2)
		  break;
	    }
	    uinsert(updir, 0, ch);
	    i--;
	 }
	 /* ok, we have the dirname in updir */
      }
      else {
	 usetc(updir, 0);
      }

      object_message(fa_viewer+FA_TEXT, MSG_START, 0);
      object_message(fa_viewer+FA_TEXT, MSG_DRAW, 0);

      if (ret == D_CLOSE)
	 return object_message(fa_viewer+FA_TEXT, MSG_KEY, 0);
   }

   return ret;
}



int main(void)
{
   char path[1024] = EMPTY_STRING;

   if (allegro_init() != 0)
      return -1;
   install_keyboard();
   install_mouse();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
         allegro_message("Unable to set the display mode\n");
         return -1;
      }
   }

   fa_viewer[FA_MESSAGE].dp = "File attribute viewer";
   fa_viewer[FA_TEXT].d1 = sizeof(path)/uwidth_max(U_CURRENT) - 1;
   fa_viewer[FA_TEXT].dp = path;
   fa_viewer[FA_OK].dp = (void*)get_config_text("OK");

   if (!ugetc(path)) {

   #if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')

      int drive = _al_getdrive();

   #else

      int drive = 0;

   #endif

      _al_getdcwd(drive, path, sizeof(path) - ucwidth(OTHER_PATH_SEPARATOR));
      fix_filename_case(path);
      fix_filename_slashes(path);
      put_backslash(path);
   }

   clear_keybuf();

   do {
   } while (gui_mouse_b());

   set_dialog_color(fa_viewer, makecol(0, 0, 0), makecol(255, 255, 255));

   popup_dialog(fa_viewer, FA_TEXT);

   return 0;
}
END_OF_MAIN()

