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
 *      The file selector.
 *
 *      By Shawn Hargreaves.
 *
 *      Guilherme Silveira and Theuzifan Sumachingun both independently 
 *      modified it to only list valid drive letters.
 *
 *      Peter Pavlovic modified it not to list the logical drives, such
 *      as the b: drive assigned as a logical drive for a: on single
 *      floppy disk drive equipped systems and improved the browsing
 *      through directories.
 *
 *      Peter Wang and Eric Botcazou modified it to stretch to screen and
 *      font sizes.
 *
 *      Annie Testes modified it so that buffer overflows cannot occur
 *      anymore.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"


#if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')
   #define HAVE_DIR_LIST
#endif


static int fs_edit_proc(int, DIALOG *, int);
static int fs_flist_proc(int, DIALOG *, int);
static char *fs_flist_getter(int, int *);


#ifdef HAVE_DIR_LIST

static int fs_dlist_proc(int, DIALOG *, int);
static char *fs_dlist_getter(int, int *);

#endif


#define FLIST_SIZE      2048

typedef struct FLIST
{
   char dir[1024];
   int size;
   char *name[FLIST_SIZE];
} FLIST;

static FLIST *flist = NULL;

static AL_CONST char *fext = NULL;

static char updir[1024];

static DIALOG file_selector[] =
{
   #ifdef HAVE_DIR_LIST

      /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)              (dp2) (dp3) */
      { _gui_shadow_box_proc, 0,    0,    305,  161,  0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
      { _gui_ctext_proc,      152,  8,    1,    1,    0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
      { _gui_button_proc,     208,  107,  81,   17,   0,    0,    0,    D_EXIT,  0,    0,    NULL,             NULL, NULL  },
      { _gui_button_proc,     208,  129,  81,   17,   0,    0,    27,   D_EXIT,  0,    0,    NULL,             NULL, NULL  },
      { fs_edit_proc,         16,   28,   272,  8,    0,    0,    0,    0,       79,   0,    NULL,             NULL, NULL  },
      { fs_flist_proc,        16,   46,   177,  100,  0,    0,    0,    D_EXIT,  0,    0,    fs_flist_getter,  NULL, NULL  },
      { fs_dlist_proc,        208,  46,   81,   52,   0,    0,    0,    D_EXIT,  0,    0,    fs_dlist_getter,  NULL, NULL  },
      { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },

   #else

      /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)              (dp2) (dp3) */
      { _gui_shadow_box_proc, 0,    0,    305,  189,  0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
      { _gui_ctext_proc,      152,  8,    1,    1,    0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },
      { _gui_button_proc,     64,   160,  81,   17,   0,    0,    0,    D_EXIT,  0,    0,    NULL,             NULL, NULL  },
      { _gui_button_proc,     160,  160,  81,   17,   0,    0,    27,   D_EXIT,  0,    0,    NULL,             NULL, NULL  },
      { fs_edit_proc,         16,   28,   272,  8,    0,    0,    0,    0,       79,   0,    NULL,             NULL, NULL  },
      { fs_flist_proc,        16,   46,   273,  100,  0,    0,    0,    D_EXIT,  0,    0,    fs_flist_getter,  NULL, NULL  },
      { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,             NULL, NULL  },

   #endif

   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};



#define FS_FRAME	0
#define FS_MESSAGE      1
#define FS_OK           2
#define FS_CANCEL       3
#define FS_EDIT         4
#define FS_FILES        5



#ifdef HAVE_DIR_LIST       /* not all platforms need a directory list */

#define FS_DISKS        6
#define FS_YIELD        7



/* count_disks:
 *  Counts the number of valid drives.
 */
static int count_disks()
{
   int c, i;

   c = 0;

   for (i=0; i<26; i++)
      if (_al_drive_exists(i))
	 c++;

   return c;
}



/* get_x_drive:
 *  Returns the drive letter matching the specified list index.
 */
static int get_x_drive(int index)
{
   int c, i;

   c = 0;

   for (i=0; i<26; i++) {
      if (_al_drive_exists(i)) {
	 c++;
	 if (c==index)
	    return i;
      }
   }

   return -1;
}



/* fs_dlist_getter:
 *  Listbox data getter routine for the file selector disk list.
 */
static char *fs_dlist_getter(int index, int *list_size)
{
   static char d[8];
   int pos, c;

   if (index < 0) {
      if (list_size)
	 *list_size = count_disks();
      return NULL;
   }

   c = 'A' + get_x_drive(index+1);
   if ((c < 'A') || (c > 'Z'))
      c = 'C';

   pos = usetc(d, c);
   pos += usetc(d+pos, DEVICE_SEPARATOR);
   pos += usetc(d+pos, OTHER_PATH_SEPARATOR);
   usetc(d+pos, 0);

   return d;
}



/* fs_dlist_proc:
 *  Dialog procedure for the file selector disk list.
 */
static int fs_dlist_proc(int msg, DIALOG *d, int c)
{
   char *s = file_selector[FS_EDIT].dp;
   int ret, i, temp;

   if (msg == MSG_START) {
      d->d1 = d->d2 = 0;
      temp = utoupper(ugetc(s));
      if (((temp >= 'A') && (temp <= 'Z')) && (ugetat(s, 1) == DEVICE_SEPARATOR)) {
	 temp -= 'A';
	 for (i=0; i<temp; i++)
	    if (_al_drive_exists(i))
	       d->d1++;
      }
   }

   ret = _gui_text_list_proc(msg, d, c);

   if (ret == D_CLOSE) {
      temp = 'A' + get_x_drive(d->d1+1);
      if ((temp < 'A') || (temp > 'Z'))
	 temp = 'C';

      s += usetc(s, temp);
      s += usetc(s, DEVICE_SEPARATOR);
      s += usetc(s, OTHER_PATH_SEPARATOR);
      usetc(s, 0);

      scare_mouse();
      object_message(file_selector+FS_FILES, MSG_START, 0);
      object_message(file_selector+FS_FILES, MSG_DRAW, 0);
      object_message(file_selector+FS_EDIT, MSG_START, 0);
      object_message(file_selector+FS_EDIT, MSG_DRAW, 0);
      unscare_mouse();

      return D_O_K;
   }

   return ret;
}



#else

#define FS_YIELD        6

#endif      /* HAVE_DIR_LIST */



/* fs_edit_proc:
 *  Dialog procedure for the file selector editable string.
 */
static int fs_edit_proc(int msg, DIALOG *d, int c)
{
   char *s = d->dp;
   int size = (d->d1 + 1) * uwidth_max(U_CURRENT); /* of s (in bytes) */
   int list_size;
   int found = 0;
   char b[1024], tmp[16];
   int ch, attr;
   int i;

   if (msg == MSG_START) {
      fix_filename_path(b, s, sizeof(b));
      ustrzcpy(s, size, b);
   }

   if (msg == MSG_KEY) {
      if ((!ugetc(s)) || (ugetat(s, -1) == DEVICE_SEPARATOR))
	 ustrzcat(s, size, uconvert_ascii("./", tmp));

      fix_filename_path(b, s, sizeof(b));
      ustrzcpy(s, size - ucwidth(OTHER_PATH_SEPARATOR), b);

      ch = ugetat(s, -1);
      if ((ch != '/') && (ch != OTHER_PATH_SEPARATOR)) {
	 if (file_exists(s, FA_RDONLY | FA_HIDDEN | FA_DIREC, &attr)) {
	    if (attr & FA_DIREC)
               put_backslash(s);
	    else
	       return D_CLOSE;
	 }
	 else
	    return D_CLOSE;
      }

      scare_mouse();
      object_message(file_selector+FS_FILES, MSG_START, 0);
      /* did we `cd ..' ? */
      if (ustrlen(updir)) {
	 /* now we have to find a directory name equal to updir */
	 for (i = 0; i<flist->size; i++) {
	    if (!ustrcmp(updir, flist->name[i])) {  /* we got it ! */
	       file_selector[FS_FILES].d1 = i;
	       /* we have to know the number of visible lines in the filelist */
	       /* -1 to avoid an off-by-one problem */
               list_size = (file_selector[FS_FILES].h-4) / text_height(font) - 1;
               if (i>list_size)
		  file_selector[FS_FILES].d2 = i-list_size;
	       else
		  file_selector[FS_FILES].d2 = 0;
               found = 1;
	       break;  /* ok, our work is done... */
	    }
	 }
	 /* by some strange reason, we didn't find the old directory... */
         if (!found) {
            file_selector[FS_FILES].d1 = 0;
            file_selector[FS_FILES].d2 = 0;
         }
      }
      /* and continue... */
      object_message(file_selector+FS_FILES, MSG_DRAW, 0);
      object_message(d, MSG_START, 0);
      object_message(d, MSG_DRAW, 0);
      unscare_mouse();

      return D_O_K;
   }

   if (msg == MSG_UCHAR) {
      if ((c >= 'a') && (c <= 'z')) {
	 if (!ALLEGRO_LFN)
	    c = utoupper(c);
      }
      else if (c == '/') {
	 c = OTHER_PATH_SEPARATOR;
      }
      else if (ALLEGRO_LFN) {
	 if ((c > 127) || (c < 32))
	    return D_O_K;
      }
      else {
	 if ((c != OTHER_PATH_SEPARATOR) && (c != '_') &&
	     (c != DEVICE_SEPARATOR) && (c != '.') &&
	     ((c < 'A') || (c > 'Z')) && ((c < '0') || (c > '9')))
	    return D_O_K;
      }
   }

   return _gui_edit_proc(msg, d, c); 
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



/* fs_flist_putter:
 *  Callback routine for for_each_file() to fill the file selector listbox.
 */
static void fs_flist_putter(AL_CONST char *str, int attrib, int param)
{
   char ext_tokens[32];
   char *s, *ext, *tok, *name, *last;
   char tmp[512], tmp2[32];
   int c, c2, i, k, sign;

   /* attribute flags (rhsda order)
    * 0 = not required, 1 = must be set, -1 = must be unset
    */
   int attr_flag[5+5] = {
      0, -1, -1, 0, 0,
      FA_RDONLY, FA_HIDDEN, FA_SYSTEM, FA_DIREC, FA_ARCH
   };

   c = usetc(ext_tokens, ' ');
   c += usetc(ext_tokens+c, ',');
   c += usetc(ext_tokens+c, ';');
   usetc(ext_tokens+c, 0);

   s = get_filename(str);
   fix_filename_case(s);

   if (fext) {
      ustrzcpy(tmp, sizeof(tmp), fext);
      ustrtok_r(tmp, ext_tokens, &last);
      c = (ustrtok_r(NULL, ext_tokens, &last) ? 1 : 0);
      if (!c) {
	 if (!ustrchr(fext, '/'))
	    c = 1;
      }

      if (c && (!(attrib & FA_DIREC))) {
	 ustrzcpy(tmp, sizeof(tmp), fext);
	 ext = get_extension(s);
	 tok = ustrtok_r(tmp, ext_tokens, &last);

	 while (tok) {
	    if (ustricmp(ext, tok) == 0)
	       break;

	    tok = ustrtok_r(NULL, ext_tokens, &last);
	 }

	 if (!tok)
	    return;
      }

      c = usetc(ext_tokens, ' ');
      c += usetc(ext_tokens+c, ',');
      c += usetc(ext_tokens+c, ';');
      c += usetc(ext_tokens+c, '/');
      usetc(ext_tokens+c, 0);

      ustrzcpy(tmp, sizeof(tmp), fext);
      tok = ustrchr(tmp, '/');

      if (tok)
	 tok = ustrtok_r(tok, ext_tokens, &last);

      if (tok) {
	 sign = 1;
	 c = usetc(tmp2, 'r');
	 c += usetc(tmp2+c, 'h');
	 c += usetc(tmp2+c, 's');
	 c += usetc(tmp2+c, 'd');
	 c += usetc(tmp2+c, 'a');
	 c += usetc(tmp2+c, '+');
	 c += usetc(tmp2+c, '-');
	 usetc(tmp2+c, 0);

	 /* scan the string */
	 i = 0;
	 while ((c = utolower(ugetat(tok, i)))) {
	    k = 0;
	    while ((c2 = ugetat(tmp2, k))!=0) {
	       if (c == c2) {
		  if (k<5) {
		     attr_flag[k] = sign;
		     break;
		  }
		  else
		     sign = (k==5) ? 1 : -1;
	       }
	       k++;
	    }
	    i++;
	 }
      }
   }

   /* check if file attributes match */
   if (!(attr_flag[3+5] & attrib)) {
      /* if not a directory, we check all attributes except FA_DIREC */
      for (c=0; c<5; c++) {
	 if (c == 3)
	    continue;
	 if ((attr_flag[c] == 1) && (!(attrib & attr_flag[c+5])))
	    return;
	 if ((attr_flag[c] == -1) && (attrib & attr_flag[c+5]))
	    return;
      }
   }
   else {
      /* if a directory, we check only FA_DIREC */
      if (attr_flag[3] == -1)
	 return;
   }

   if ((flist->size < FLIST_SIZE) && ((ugetc(s) != '.') || (ugetat(s, 1)))) {
      name = malloc(ustrsizez(s) + ((attrib & FA_DIREC) ? ucwidth(OTHER_PATH_SEPARATOR) : 0));
      if (!name)
	 return;

      for (c=0; c<flist->size; c++) {
	 if (ugetat(flist->name[c], -1) == OTHER_PATH_SEPARATOR) {
	    if (attrib & FA_DIREC)
	       if (ustrfilecmp(s, flist->name[c]) < 0)
		  break;
	 }
	 else {
	    if (attrib & FA_DIREC)
	       break;
	    if (ustrfilecmp(s, flist->name[c]) < 0)
	       break;
	 }
      }

      for (c2=flist->size; c2>c; c2--)
	 flist->name[c2] = flist->name[c2-1];

      flist->name[c] = name;
      ustrcpy(flist->name[c], s);  /* ustrzcpy() not needed */

      if (attrib & FA_DIREC)
	 put_backslash(flist->name[c]);

      flist->size++;
   }
}



/* fs_flist_getter:
 *  Listbox data getter routine for the file selector list.
 */
static char *fs_flist_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = flist->size;
      return NULL;
   }

   return flist->name[index];
}



/* fs_flist_proc:
 *  Dialog procedure for the file selector list.
 */
static int fs_flist_proc(int msg, DIALOG *d, int c)
{
   static int recurse_flag = 0;
   char *s = file_selector[FS_EDIT].dp;
   char tmp[32];
   int size = (file_selector[FS_EDIT].d1 + 1) * uwidth_max(U_CURRENT); /* of s (in bytes) */
   int sel = d->d1;
   int i, ret;
   int ch, count;

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

      for_each_file(flist->dir, FA_RDONLY | FA_DIREC | FA_ARCH | FA_HIDDEN | FA_SYSTEM, fs_flist_putter, 0);

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
   ret = _gui_text_list_proc(msg, d, c);     /* call the parent procedure */
   recurse_flag--;

   if (((sel != d->d1) || (ret == D_CLOSE)) && (recurse_flag == 0)) {
      replace_filename(s, flist->dir, flist->name[d->d1], size);
      /* check if we want to `cd ..' */
      if ((!ustrncmp(flist->name[d->d1], uconvert_ascii("..", tmp), 2)) && (ret == D_CLOSE)) {
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
      scare_mouse();
      object_message(file_selector+FS_EDIT, MSG_START, 0);
      object_message(file_selector+FS_EDIT, MSG_DRAW, 0);
      unscare_mouse();

      if (ret == D_CLOSE)
	 return object_message(file_selector+FS_EDIT, MSG_KEY, 0);
   }

   return ret;
}



/* stretch_dialog:
 *  Stretch the dialog horizontally and vertically to the specified
 *   size and the font in use.
 *   (all the magic numbers come from the "historical" file selector)
 */
static void stretch_dialog(DIALOG *d, int width, int height)
{
   int font_w, font_h, hpad, vpad;
   char tmp[16];
    
   #ifdef HAVE_DIR_LIST

      /* horizontal settings */
      font_w = text_length(font, uconvert_ascii("A", tmp));

      if (width == 0)
          width = 0.95*SCREEN_W + 1;

      hpad = 0.05*width + 1;
    
      d[FS_FRAME].w   = width;
      d[FS_FRAME].x   = 0;
      d[FS_MESSAGE].w = 1;
      d[FS_MESSAGE].x = width/2;
      d[FS_EDIT].w    = d[FS_FRAME].w - 2*hpad - 1;
      d[FS_EDIT].x    = hpad;
      d[FS_CANCEL].w  = 10*font_w + 1;
      d[FS_CANCEL].x  = d[FS_FRAME].w - hpad - d[FS_CANCEL].w; 
      d[FS_OK].w      = d[FS_CANCEL].w;
      d[FS_OK].x      = d[FS_CANCEL].x;
      d[FS_DISKS].w   = d[FS_OK].w;
      d[FS_DISKS].x   = d[FS_OK].x;
      d[FS_FILES].x   = hpad;
      d[FS_FILES].w   = d[FS_CANCEL].x - d[FS_FILES].x - hpad + 1;
      d[FS_YIELD].x   = 0;

      /* vertical settings */     
      font_h = text_height(font);

      if (height == 0)
          height = 0.80*SCREEN_H + 1;

      vpad = 0.05*height;

      d[FS_FRAME].h   = height;
      d[FS_FRAME].y   = 0;
      d[FS_MESSAGE].h = font_h;
      d[FS_MESSAGE].y = vpad;
      d[FS_EDIT].h    = font_h;
      d[FS_EDIT].y    = 2*vpad + d[FS_MESSAGE].h + 4;
      d[FS_CANCEL].h  = font_h + 9;
      d[FS_CANCEL].y  = d[FS_FRAME].h - 2*vpad - d[FS_CANCEL].h + 1;
      d[FS_OK].h      = d[FS_CANCEL].h;
      d[FS_OK].y      = d[FS_CANCEL].y - vpad/2 - d[FS_OK].h - 1; 
      d[FS_DISKS].y   = d[FS_EDIT].y + d[FS_EDIT].h + vpad + 2;
      d[FS_DISKS].h   = d[FS_OK].y - d[FS_DISKS].y - vpad - 1;    
      d[FS_FILES].y   = d[FS_DISKS].y;
      d[FS_FILES].h   = d[FS_FRAME].h - d[FS_FILES].y - 2*vpad + 1;
      d[FS_YIELD].y   = 0;

   #else

      /* horizontal settings */
      font_w = text_length(font, uconvert_ascii("A", tmp));

      if (width == 0)
          width = 0.95*SCREEN_W + 1;

      hpad = 0.05*width + 1;
    
      d[FS_FRAME].w   = width;
      d[FS_FRAME].x   = 0;
      d[FS_MESSAGE].w = 1;
      d[FS_MESSAGE].x = width/2;
      d[FS_EDIT].w    = d[FS_FRAME].w - 2*hpad - 1;
      d[FS_EDIT].x    = hpad;
      d[FS_FILES].w   = d[FS_FRAME].w - 2*hpad;
      d[FS_FILES].x   = hpad;
      d[FS_OK].w      = 10*font_w + 1;
      d[FS_OK].x      = (d[FS_FRAME].w - 2*d[FS_OK].w - hpad + 1)/2; 
      d[FS_CANCEL].w  = d[FS_OK].w;
      d[FS_CANCEL].x  = d[FS_FRAME].w - d[FS_OK].x - d[FS_CANCEL].w;
      d[FS_YIELD].x   = 0;
      
      /* vertical settings */     
      font_h = text_height(font);

      if (height == 0)
          height = 0.95*SCREEN_H - 1;

      vpad = 0.04*height + 1;

      d[FS_FRAME].h   = height;
      d[FS_FRAME].y   = 0;
      d[FS_MESSAGE].h = font_h;
      d[FS_MESSAGE].y = vpad;
      d[FS_EDIT].h    = font_h;
      d[FS_EDIT].y    = 2*vpad + d[FS_MESSAGE].h + 4;
      d[FS_OK].h      = font_h + 9;
      d[FS_OK].y      = d[FS_FRAME].h - 1.5*vpad - d[FS_OK].h;
      d[FS_CANCEL].h  = d[FS_OK].h;
      d[FS_CANCEL].y  = d[FS_OK].y;
      d[FS_FILES].y   = d[FS_EDIT].y + d[FS_EDIT].h + vpad + 2;
      d[FS_FILES].h   = d[FS_OK].y - d[FS_FILES].y - vpad - 6;
      d[FS_YIELD].y   = 0;

   #endif
}



/* file_select_ex:
 *  Displays the Allegro file selector, with the message as caption. 
 *  Allows the user to select a file, and stores the selection in the 
 *  path buffer, whose length in bytes is given by size and should have 
 *  room for at least 80 characters. The files are filtered according to 
 *  the file extensions in ext. Passing NULL includes all files, "PCX;BMP" 
 *  includes only files with .PCX or .BMP extensions. Returns zero if it 
 *  was closed with the Cancel button or non-zero if it was OK'd.
 */
int file_select_ex(AL_CONST char *message, char *path, AL_CONST char *ext, int size, int width, int height)
{
   int ret;
   char *p;
   char tmp[32];

   if (width == OLD_FILESEL_WIDTH)
      width = 305;

   #ifdef HAVE_DIR_LIST

      if (height == OLD_FILESEL_HEIGHT)
         height = 161;

   #else

      if (height == OLD_FILESEL_HEIGHT)
         height = 189;

   #endif

   /* for fs_dlist_proc() */
   ASSERT(size >= 4 * uwidth_max(U_CURRENT));

   usetc(updir, 0);
   file_selector[FS_MESSAGE].dp = (char *)message;
   file_selector[FS_EDIT].d1 = size/uwidth_max(U_CURRENT) - 1;
   file_selector[FS_EDIT].dp = path;
   file_selector[FS_OK].dp = (void*)get_config_text("OK");
   file_selector[FS_CANCEL].dp = (void*)get_config_text("Cancel");
   fext = ext;

   if (!ugetc(path)) {

   #if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')

      int drive = _al_getdrive();

   #else

      int drive = 0;

   #endif

      _al_getdcwd(drive, path, size - ucwidth(OTHER_PATH_SEPARATOR));
      fix_filename_case(path);
      fix_filename_slashes(path);
      put_backslash(path);
   }

   clear_keybuf();

   do {
   } while (gui_mouse_b());

   stretch_dialog(file_selector, width, height);
   centre_dialog(file_selector);
   set_dialog_color(file_selector, gui_fg_color, gui_bg_color);
   ret = popup_dialog(file_selector, FS_EDIT);

   if ((ret == FS_CANCEL) || (!ugetc(get_filename(path))))
      return FALSE;

   p = get_extension(path);
   if ((!ugetc(p)) && (ext) && (!ustrpbrk(ext, uconvert_ascii(" ,;", tmp)))) {
      size -= ((long)p - (long)path + ucwidth('.'));
      if (size >= uwidth_max(U_CURRENT) + ucwidth(0)) {  /* do not end with '.' */
         p += usetc(p, '.');
         ustrzcpy(p, size, ext);
      }
   }

   return TRUE; 
}



/* file_select:
 *  The good old file selector.
 */
int file_select(AL_CONST char *message, char *path, AL_CONST char *ext)
{
   return file_select_ex(message, path, ext, 1024, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT);
}

