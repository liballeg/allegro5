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
 *      The grabber utility program.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "datedit.h"

#if (defined ALLEGRO_DJGPP) && (!defined SCAN_DEPEND)
   #include <dos.h>
   #include <sys/exceptn.h>
#endif


typedef struct DATAITEM
{
   DATAFILE *dat;
   DATAFILE **parent;
   int i;
   char name[128];
} DATAITEM;


static DATAFILE *datafile = NULL;
static DATAITEM *data = NULL;
static char *data_sel = NULL;
static int data_count = 0;
static int data_malloced = 0;

static char header_file[FILENAME_LENGTH] = "";
static char prefix_string[256 * MAX_BYTES_PER_CHAR] = "";
static char password[256 * MAX_BYTES_PER_CHAR];
static char xgrid_string[5 * MAX_BYTES_PER_CHAR];
static char ygrid_string[5 * MAX_BYTES_PER_CHAR];

static int current_view_object = -1;
static int current_property_object = -1;

static int is_modified = 0;

static int busy_mouse = FALSE;

static int entered_password = FALSE;
static int no_sound = FALSE;

static int autodetect_card = GFX_AUTODETECT_WINDOWED;

static int view_proc(int, DIALOG *, int);
static int list_proc(int, DIALOG *, int);
static int prop_proc(int, DIALOG *, int);
static int droplist_proc(int, DIALOG *, int);
static int custkey_proc(int, DIALOG *, int);
static int edit_mod_proc(int, DIALOG *, int);
static int droplist_mod_proc(int, DIALOG *, int);
static char *list_getter(int, int *);
static char *pack_getter(int, int *);
static char *prop_getter(int, int *);

static int renewer(void);
static int loader(void);
static int merger(void);
static int saver(void);
static int strip_saver(void);
static int updater(void);
static int sel_updater(void);
static int force_updater(void);
static int force_sel_updater(void);
static int reader(void);
static int viewer(void);
static int quitter(void);
static int grabber(void);
static int exporter(void);
static int deleter(void);
static int mover(int);
static int mover_up(void);
static int mover_down(void);
static int replacer(int);
static int sheller(void);
static int helper(void);
static int sysinfo(void);
static int about(void);
static int renamer(void);
static int hooker(void);
static int backup_toggler(void);
static int dither_toggler(void);
static int index_toggler(void);
static int sort_toggler(void);
static int trans_toggler(void);
static int relf_toggler(void);
static int property_delete(void);
static int property_insert(void);
static int property_change(void);
static int new_object(void);
static int replace_object(void);

static int add_new(int type);

static void rebuild_list(void *old, int clear);



/* variable-sized */
static MENU file_menu[32] =
{
   { "&New",                        renewer,          NULL,       0, NULL  },
   { "&Load\t(ctrl+L)",             loader,           NULL,       0, NULL  },
   { "&Save\t(ctrl+S)",             saver,            NULL,       0, NULL  },
   { "Save S&tripped",              strip_saver,      NULL,       0, NULL  },
   { "&Merge",                      merger,           NULL,       0, NULL  },
   { "&Update\t(ctrl+U)",           updater,          NULL,       0, NULL  },
   { "Update S&election",           sel_updater,      NULL,       0, NULL  },
   { "&Force Update\t(ctrl+F)",     force_updater,    NULL,       0, NULL  },
   { "F&orce Update Selection",     force_sel_updater,NULL,       0, NULL  },
   { "",                            NULL,             NULL,       0, NULL  },
   { "&Read Bitmap\t(ctrl+R)",      reader,           NULL,       0, NULL  },
   { "&View Bitmap\t(ctrl+V)",      viewer,           NULL,       0, NULL  },
   { "",                            NULL,             NULL,       0, NULL  },
   { "&Quit\t(ctrl+Q)",             quitter,          NULL,       0, NULL  },
   { NULL,                          NULL,             NULL,       0, NULL  }
};



static MENU move_menu[2] =
{
   { "&Up\t(shift+U)",              mover_up,         NULL,       0, NULL  },
   { "&Down\t(shift+D)",            mover_down,       NULL,       0, NULL  }
};



/* variable-sized */
static MENU replace_menu[32] =
{
   { "Other",                       replace_object,   NULL,       0, NULL  }
};



/* variable-sized */
static MENU new_menu[32] =
{
   { "Other",                       new_object,       NULL,       0, NULL  },
   { NULL,                          NULL,             NULL,       0, NULL  }
};



/* variable-sized */
static MENU objc_menu[32] =
{
   { "&Grab\t(ctrl+G)",             grabber,          NULL,       0, NULL  },
   { "&Export\t(ctrl+E)",           exporter,         NULL,       0, NULL  },
   { "&Delete\t(ctrl+D)",           deleter,          NULL,       0, NULL  },
   { "&Move",                       NULL,             move_menu,  0, NULL  },
   { "Rep&lace",                    NULL,             replace_menu, 0, NULL},
   { "&Rename\t(ctrl+N)",           renamer,          NULL,       0, NULL  },
   { "Set &Property\t(ctrl+P)",     property_insert,  NULL,       0, NULL  },
   { "&Shell Edit\t(ctrl+Z)",       sheller,          NULL,       0, NULL  },
   { "",                            NULL,             NULL,       0, NULL  },
   { "&New",                        NULL,             new_menu,   0, NULL  },
   { NULL,                          NULL,             NULL,       0, NULL  }
};



static MENU opt_menu[] =
{
   { "&Backup Datafiles",           backup_toggler,   NULL,       0, NULL  },
   { "&Index Objects",              index_toggler,    NULL,       0, NULL  },
   { "&Sort Objects",               sort_toggler,     NULL,       0, NULL  },
   { "Store &Relative Filenames",   relf_toggler,     NULL,       0, NULL  },
   { "&Dither Images",              dither_toggler,   NULL,       0, NULL  },
   { "Preserve &Transparency",      trans_toggler,    NULL,       0, NULL  },
   { NULL,                          NULL,             NULL,       0, NULL  }
};

#define MENU_BACKUP           0
#define MENU_INDEX            1
#define MENU_SORT             2
#define MENU_RELF             3
#define MENU_DITHER           4
#define MENU_TRANS            5



/* variable-sized */
static MENU help_menu[32] =
{
   { "&Help\t(F1)",                 helper,           NULL,       0, NULL  },
   { "&System",                     sysinfo,          NULL,       0, NULL  },
   { "&About",                      about,            NULL,       0, NULL  },
   { NULL,                          NULL,             NULL,       0, NULL  }
};



/* variable-sized */
static MENU menu[32] = 
{ 
   { "&File",                       NULL,             file_menu,  0, NULL  },
   { "&Object",                     NULL,             objc_menu,  0, NULL  },
   { "O&ptions",                    NULL,             opt_menu,   0, NULL  },
   { "&Help",                       NULL,             help_menu,  0, NULL  },
   { NULL,                          NULL,             NULL,       0, NULL  }
};



/* variable-sized */
static MENU popup_menu[32] =
{
   { "&Grab",                       grabber,          NULL,       0, NULL  },
   { "&Export",                     exporter,         NULL,       0, NULL  },
   { "&Delete",                     deleter,          NULL,       0, NULL  },
   { "&Move",                       NULL,             move_menu,  0, NULL  },
   { "Rep&lace",                    NULL,             replace_menu, 0, NULL},
   { "&Rename",                     renamer,          NULL,       0, NULL  },
   { "&Shell Edit",                 sheller,          NULL,       0, NULL  },
   { "",                            NULL,             NULL,       0, NULL  },
   { "&New",                        NULL,             new_menu,   0, NULL  },
   { NULL,                          NULL,             NULL,       0, NULL  }
};



#define C(x)      (x - 'a' + 1)

/*  Note:  See DLG_* constants below. */
static DIALOG main_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_clear_proc,      0,    0,    640,  480,  0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_menu_proc,       0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       menu,             NULL, NULL  },
   { d_text_proc,       20,   30,   0,    0,    0,    0,    0,       0,          0,             0,       "Editing:",       NULL, NULL  },
   { edit_mod_proc,     100,  30,   320,  8,    0,    0,    0,       0,          255,           0,       grabber_data_file,NULL, NULL  },
   { d_text_proc,       20,   42,   0,    0,    0,    0,    0,       0,          0,             0,       "Header:",        NULL, NULL  },
   { edit_mod_proc,     100,  42,   320,  8,    0,    0,    0,       0,          255,           0,       header_file,      NULL, NULL  },
   { d_text_proc,       20,   54,   0,    0,    0,    0,    0,       0,          0,             0,       "Prefix:",        NULL, NULL  },
   { edit_mod_proc,     100,  54,   320,  8,    0,    0,    0,       0,          255,           0,       prefix_string,    NULL, NULL  },
   { d_text_proc,       20,   66,   0,    0,    0,    0,    0,       0,          0,             0,       "Password:",      NULL, NULL  },
   { edit_mod_proc,     100,  66,   320,  8,    0,    0,    0,       0,          255,           0,       password,         NULL, NULL  },
   { d_text_proc,       260,  16,   0,    0,    0,    0,    0,       0,          0,             0,       "X-grid:",        NULL, NULL  },
   { d_edit_proc,       324,  16,   40,   8,    0,    0,    0,       0,          4,             0,       xgrid_string,     NULL, NULL  },
   { d_text_proc,       375,  16,   0,    0,    0,    0,    0,       0,          0,             0,       "Y-grid:",        NULL, NULL  },
   { d_edit_proc,       439,  16,   40,   8,    0,    0,    0,       0,          4,             0,       ygrid_string,     NULL, NULL  },
   { droplist_mod_proc, 430,  48,   195,  28,   0,    0,    0,       0,          0,             0,       pack_getter,      NULL, NULL  },
   { prop_proc,         260,  86,   365,  107,  0,    0,    0,       D_EXIT,     0,             0,       prop_getter,      NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       renewer,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('l'),  0,          0,             0,       loader,           NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('s'),  0,          0,             0,       saver,            NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('u'),  0,          0,             0,       updater,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('f'),  0,          0,             0,       force_updater,    NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('r'),  0,          0,             0,       reader,           NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('v'),  0,          0,             0,       viewer,           NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('q'),  0,          0,             0,       quitter,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('g'),  0,          0,             0,       grabber,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('e'),  0,          0,             0,       exporter,         NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('d'),  0,          0,             0,       deleter,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    'U',     0,          0,             0,       mover_up,         NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    'D',     0,          0,             0,       mover_down,       NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('n'),  0,          0,             0,       renamer,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('p'),  0,          0,             0,       property_insert,  NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('z'),  0,          0,             0,       sheller,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    27,      0,          0,             0,       quitter,          NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,          KEY_F1,        0,       helper,           NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { custkey_proc,      0,    0,    0,    0,    0,    0,    0,       D_DISABLED, 0,             0,       NULL,             NULL, NULL  },
   { list_proc,         20,   86,   209,  388,  0,    0,    0,       D_EXIT,     0,             0,       list_getter,      NULL, NULL  },
   { view_proc,         260,  218,  0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};


#define DLG_FILENAME          3
#define DLG_HEADERNAME        5
#define DLG_PREFIXSTRING      7
#define DLG_PASSWORD          9
#define DLG_XGRIDSTRING       11
#define DLG_YGRIDSTRING       13
#define DLG_PACKLIST          14
#define DLG_PROP              15
#define DLG_FIRSTWHITE        16
#define DLG_LIST              50
#define DLG_VIEW              51


#define SELECTED_ITEM         main_dlg[DLG_LIST].d1
#define SELECTED_PROPERTY     main_dlg[DLG_PROP].d1



#define BOX_W     MIN(512, SCREEN_W-16)
#define BOX_H     MIN(256, (SCREEN_H-64)&0xFFF0)

#define BOX_L     ((SCREEN_W - BOX_W) / 2)
#define BOX_R     ((SCREEN_W + BOX_W) / 2)
#define BOX_T     ((SCREEN_H - BOX_H) / 2)
#define BOX_B     ((SCREEN_H + BOX_H) / 2)

static int box_x = 0;
static int box_y = 0;
static int box_active = FALSE;



/* updates the window title */
static void update_title(void)
{
   char buf[1024];

   if (grabber_data_file[0])
      sprintf(buf, "grabber - %s%s", grabber_data_file, is_modified ? " (modified)" : "");
   else
      strcpy(buf, "grabber");

   set_window_title(buf);
}



/* indicates a possible modification of the currently loaded datafile */
static void set_modified(int modified)
{
   is_modified = modified;
   update_title();
}



/* standard edit proc, enhanced with possible modification detection */
static int edit_mod_proc(int msg, DIALOG *d, int c)
{
   int ret = d_edit_proc(msg, d, c);
   
   if ((msg == MSG_CHAR) || (msg == MSG_UCHAR))
      set_modified(TRUE);

   return ret;
}



/* droplist proc, enhanced with possible modification detection */
static int droplist_mod_proc(int msg, DIALOG *d, int c)
{
   int ret = droplist_proc(msg, d, c);
   
   if ((msg == MSG_CHAR) || (msg == MSG_UCHAR) || (msg == MSG_CLICK))
      set_modified(TRUE);

   return ret;
}



/* starts outputting a progress message */
static void box_start(void)
{
   show_mouse(NULL);

   rectfill(screen, BOX_L, BOX_T, BOX_R, BOX_B, gui_bg_color);
   rect(screen, BOX_L-1, BOX_T-1, BOX_R+1, BOX_B+1, gui_fg_color);
   hline(screen, BOX_L, BOX_B+2, BOX_R+1, gui_fg_color);
   vline(screen, BOX_R+2, BOX_T, BOX_B+2, gui_fg_color);

   show_mouse(screen);

   box_x = box_y = 0;
   box_active = TRUE;
}



/* outputs text to the progress message */
static void box_out(char *msg)
{
   if (box_active) {
      scare_mouse();

      set_clip_rect(screen, BOX_L+8, BOX_T+1, BOX_R-8, BOX_B-1);

      textout_ex(screen, font, msg, BOX_L+(box_x+1)*8, BOX_T+(box_y+1)*8, gui_fg_color, gui_bg_color);

      set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);

      unscare_mouse();

      box_x += strlen(msg);
   }
}



/* outputs text to the progress message */
static void box_eol(void)
{
   if (box_active) {
      box_x = 0;
      box_y++;

      if ((box_y+2)*8 >= BOX_H) {
	 scare_mouse();

	 blit(screen, screen, BOX_L+8, BOX_T+16, BOX_L+8, BOX_T+8, BOX_W-16, BOX_H-24);
	 rectfill(screen, BOX_L+8, BOX_T+BOX_H-16, BOX_L+BOX_W-8, BOX_T+BOX_H-8, gui_bg_color);

	 unscare_mouse();

	 box_y--;
      }
   }
}



/* ends output of a progress message */
static void box_end(int pause)
{
   if (box_active) {
      if (pause) {
	 box_eol();
	 box_out("-- press a key --");

	 do {
	    poll_mouse();
	 } while (mouse_b);

	 do {
	    poll_mouse();
	 } while ((!keypressed()) && (!mouse_b));

	 do {
	    poll_mouse();
	 } while (mouse_b);

	 clear_keybuf();
      }

      box_active = FALSE;
   }
}



/* returns the currently selected object, or -1 if there is a multiple sel */
static int single_selection(void)
{
   int sel = SELECTED_ITEM;
   int i;

   for (i=0; i<data_count; i++)
      if ((data_sel[i]) && (i != sel))
	 return -1;

   return sel;
}



/* returns a pointer to the currently selected object */
static DATAFILE *get_single_selection(void)
{
   int sel = single_selection();

   if ((sel > 0) && (sel < data_count))
      return data[sel].dat;
   else
      return NULL;
}



/* gets info about the currently selected object */
static void get_selection_info(DATAFILE **dat, DATAFILE ***parent)
{
   if ((SELECTED_ITEM <= 0) || (SELECTED_ITEM >= data_count)) {
      *dat = NULL;
      *parent = &datafile;
   }
   else {
      *dat = data[SELECTED_ITEM].dat;
      if ((*dat)->type == DAT_FILE) {
         void *ptr = &(*dat)->dat;
         *parent = (DATAFILE **)ptr;
      }
      else
	 *parent = data[SELECTED_ITEM].parent;
   }
}



/* worker function for iterating through all the selected objects */
static int foreach_selection(int (*proc)(DATAFILE *, int *, int), int *count, int *param, int param2)
{
   int ret = D_O_K;
   int i;

   *count = 0;

   for (i=1; i<data_count; i++) {
      if ((i == SELECTED_ITEM) || (data_sel[i])) {
	 ret |= proc(data[i].dat, param, param2);
	 (*count)++;
      }
   }

   return ret;
}



/* checks whether two menus have the same name */
static int compare_menu_names(AL_CONST char *n1, AL_CONST char *n2)
{
   char buf1[256], buf2[256];
   int i, j;

   j = 0;

   for (i=0; (n1[i]) && (n1[i] != '\t'); i++)
      if (n1[i] != '&')
	 buf1[j++] = n1[i];

   buf1[j] = 0; 

   j = 0;

   for (i=0; (n2[i]) && (n2[i] != '\t'); i++)
      if (n2[i] != '&')
	 buf2[j++] = n2[i];

   buf2[j] = 0; 

   return stricmp(buf1, buf2);
}



/* inserts a new entry into a menu list */
static void add_to_menu(MENU *menu, MENU *newmenu, int needdp, int (*endproc)(void), MENU *endchild, int endblank)
{
   int i, j;

   for (i=0; menu[i].text; i++)
      if (compare_menu_names(newmenu->text, menu[i].text) == 0)
	 return;

   for (i=0; menu[i].text; i++) {
      if ((needdp) && (!menu[i].dp))
	 break;

      if ((endproc) && (menu[i].proc == endproc))
	 break;

      if ((endchild) && (menu[i].child == endchild))
	 break;

      if ((endblank) && (!menu[i].text[0])) {
	 endblank--;
	 if (!endblank)
	    break;
      }
   }

   j = i;
   while (menu[j].text)
      j++;

   while (j > i) {
      menu[j] = menu[j-1];
      j--;
   }

   menu[i] = *newmenu;
}



/* returns whether the given character is part of a word or not */
static int is_suitable_word_start(int c)
{
   if (uisspace(c)) return 1;
   if (c=='/') return 1;
   return 0;
}



/* automatically generates keyboard shortcuts for a menu */
static void add_menu_shortcuts(MENU *menu)
{
   int alphabet[26];
   int i, j, c;
   int goodpos;
   int word;

   for (i=0; i<26; i++)
      alphabet[i] = FALSE;

   for (i=0; menu[i].text; i++) {
      for (j=0; menu[i].text[j]; j++) {
	 if (menu[i].text[j] == '&') {
	    c = menu[i].text[++j];
	    c = utolower(c)-'a';
	    if ((c >= 0) && (c < 26))
	       alphabet[c] = TRUE;
	 }
      }
   }

   for (i=0; menu[i].text; i++) {
      if (strchr(menu[i].text, '&'))
	 continue;

      goodpos = -1;
      word = TRUE;

      for (j=0; menu[i].text[j]; j++) {
	 c = utolower(menu[i].text[j])-'a';
	 if ((c >= 0) && (c < 26) && (!alphabet[c]) && (word)) {
	    goodpos = j;
	    alphabet[c] = TRUE;
	    break;
	 }
	 word = is_suitable_word_start(menu[i].text[j]);
      }

      if (goodpos < 0) {
	 for (j=0; menu[i].text[j]; j++) {
	    c = utolower(menu[i].text[j])-'a';
	    if ((c >= 0) && (c < 26) && (!alphabet[c])) {
	       goodpos = j;
	       alphabet[c] = TRUE;
	       break;
	    }
	 }
      }

      if (goodpos >= 0) {
	 char *buf = malloc(strlen(menu[i].text)+2);
	 if (goodpos > 0)
	    memcpy(buf, menu[i].text, goodpos);
	 buf[goodpos] = '&';
	 strcpy(buf+goodpos+1, menu[i].text+goodpos);
	 menu[i].text = buf;
      }
   }
}



/* allow plugins to override the default menu actions */
static int check_menu_hook(AL_CONST char *name, int flags, int *ret)
{
   int i;

   *ret = 0;

   for (i=0; datedit_menu_info[i]; i++) {
      if ((datedit_menu_info[i]->flags & flags) == flags) {
	 if (compare_menu_names(name, datedit_menu_info[i]->menu->text) == 0) {
	    if (datedit_menu_info[i]->query)
	       if (!datedit_menu_info[i]->query(FALSE))
		  continue;

	    if (datedit_menu_info[i]->menu->proc) {
	       *ret = datedit_menu_info[i]->menu->proc();
	       return TRUE;
	    }
	 }
      }
   }

   return FALSE;
}



/* helper for overriding the menu actions */
#define CHECK_MENU_HOOK(_name_, _flags_)                 \
{                                                        \
   int _ret_;                                            \
							 \
   if (check_menu_hook(_name_, _flags_, &_ret_))         \
      return _ret_;                                      \
}



/* dummy routine for use by the plugin menus */
static int hooker(void)
{
   CHECK_MENU_HOOK(active_menu->text, 0);

   alert("Error: this command was not", "handled by any plugin!", NULL, "That's funny...", NULL, 13, 0);

   return D_O_K;
}



/* dummy routine for the plugin keyboard shortcuts */
static int custkey_proc(int msg, DIALOG *d, int c)
{
   int ret = D_O_K;

   switch (msg) {

      case MSG_XCHAR:
	 if (((c>>8) != d->d1) && ((c>>8) != d->d2))
	    break;

	 ret |= D_USED_CHAR;
	 /* fall through */

      case MSG_KEY:
	 check_menu_hook(d->dp, 0, &ret);
	 break;
   }

   return ret;
}



/* decides which popup menu to activate, depending on the selection */
static MENU *which_menu(int sel)
{
   static MENU tmp_menu[32];
   static char tmp_text[32][256];
   DATAFILE *dat;
   int i, j, k;
   int ok;

   j = 0;

   for (i=0; popup_menu[i].text; i++) {
      ok = FALSE;

      if ((compare_menu_names(popup_menu[i].text, "Grab") == 0) ||
	  (compare_menu_names(popup_menu[i].text, "Export") == 0) ||
	  (compare_menu_names(popup_menu[i].text, "Move") == 0) ||
	  (compare_menu_names(popup_menu[i].text, "Replace") == 0) ||
	  (compare_menu_names(popup_menu[i].text, "Rename") == 0)) {
	 if (get_single_selection())
	    ok = TRUE;
      }
      else if ((compare_menu_names(popup_menu[i].text, "Delete") == 0) || 
	       (compare_menu_names(popup_menu[i].text, "Convert Filename") == 0)) {
	 for (k=1; k<data_count; k++) {
	    if ((k == SELECTED_ITEM) || (data_sel[k])) {
	       ok = TRUE;
	       break;
	    }
	 }
      }
      else if (compare_menu_names(popup_menu[i].text, "Shell Edit") == 0) {
	 dat = get_single_selection();
	 if ((dat) && (dat->type != DAT_FILE))
	    ok = TRUE;
      }
      else if (compare_menu_names(popup_menu[i].text, "New") == 0) {
	 ok = TRUE;
      }
      else if (!popup_menu[i].text[0]) {
	 if (j > 0)
	    ok = TRUE;
      }

      if (!ok) {
	 for (k=0; datedit_menu_info[k]; k++) {
	    if (datedit_menu_info[k]->flags & DATEDIT_MENU_POPUP) {
	       if (compare_menu_names(popup_menu[i].text, datedit_menu_info[k]->menu->text) == 0) {
		  if (datedit_menu_info[k]->query)
		     if (!datedit_menu_info[k]->query(TRUE))
			continue;

		  ok = TRUE;
		  break;
	       }
	    }
	 }
      }

      if (ok) {
	 for (k=0; (popup_menu[i].text[k]) && (popup_menu[i].text[k] != '\t'); k++)
	    tmp_text[j][k] = popup_menu[i].text[k];

	 tmp_text[j][k] = 0;

	 tmp_menu[j] = popup_menu[i];
	 tmp_menu[j].text = tmp_text[j];

	 j++;
      }
   }

   tmp_menu[j].text = NULL;

   return tmp_menu;
}



static char my_mouse_pointer_data[256] =
{
   2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 2, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 2, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0
};



static char my_busy_pointer_data[256] =
{
   0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0,
   0, 0, 0, 2, 1, 1, 1, 0, 0, 1, 1, 1, 2, 0, 0, 0,
   0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 0, 0,
   0, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 2, 0,
   0, 2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 2, 0,
   2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 2,
   2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 2,
   0, 2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 2, 0,
   0, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 2, 0,
   0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 0, 0,
   0, 0, 0, 2, 1, 1, 1, 0, 0, 1, 1, 1, 2, 0, 0, 0,
   0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0
};



static BITMAP *my_mouse_pointer = NULL;
static BITMAP *my_busy_pointer = NULL;



/* changes the mouse pointer */
static void set_busy_mouse(int busy)
{
   if (busy) {
      set_mouse_sprite(my_busy_pointer);
      busy_mouse = TRUE;
   }
   else {
      set_mouse_sprite(my_mouse_pointer);
      busy_mouse = FALSE;
   }
}



/* selects a palette, sorting out all the colors so things look ok */
static void sel_palette(RGB *pal)
{
   int c, x, y;
   int (*proc)(int, DIALOG *, int);

   memmove(datedit_current_palette, pal, sizeof(PALETTE));
   set_palette(datedit_current_palette);

   gui_fg_color = makecol(0, 0, 0);
   gui_mg_color = makecol(0x80, 0x80, 0x80);
   gui_bg_color = makecol(0xFF, 0xFF, 0xFF);

   proc = main_dlg[DLG_FIRSTWHITE].proc;
   main_dlg[DLG_FIRSTWHITE].proc = NULL;
   set_dialog_color(main_dlg, gui_fg_color, gui_mg_color);

   main_dlg[DLG_FIRSTWHITE].proc = proc;
   set_dialog_color(main_dlg+DLG_FIRSTWHITE, gui_fg_color, gui_bg_color);

   if (!my_mouse_pointer)
      my_mouse_pointer = create_bitmap(16, 16);

   if (!my_busy_pointer)
      my_busy_pointer = create_bitmap(16, 16);

   for (y=0; y<16; y++) {
      for (x=0; x<16; x++) {
	 switch (my_mouse_pointer_data[x+y*16]) {
	    case 1:  c = gui_fg_color; break;
	    case 2:  c = gui_bg_color; break;
	    default: c = screen->vtable->mask_color; break;
	 }
	 putpixel(my_mouse_pointer, x, y, c);

	 switch (my_busy_pointer_data[x+y*16]) {
	    case 1:  c = gui_fg_color; break;
	    case 2:  c = gui_bg_color; break;
	    default: c = screen->vtable->mask_color; break;
	 }
	 putpixel(my_busy_pointer, x, y, c);
      }
   }

   set_busy_mouse(FALSE);
}



/* helper for reading the current grid size settings */
static void get_grid_size(int *x, int *y)
{
   *x = MAX(atoi(xgrid_string), 1);
   *y = MAX(atoi(ygrid_string), 1);
}



/* handles double-clicking on an item in the object list */
static int handle_dclick(DATAFILE *dat)
{
   int i;

   for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
      if ((datedit_object_info[i]->type == dat->type) && (datedit_object_info[i]->dclick))
	 return datedit_object_info[i]->dclick(dat);
   }

   return D_O_K;
}



/* helper for doing a fullscreen bitmap view */
static void show_a_bitmap(BITMAP *bmp, PALETTE pal)
{
   DATAFILE tmpdat;

   tmpdat.dat = bmp;
   tmpdat.type = DAT_BITMAP;
   tmpdat.size = 0;
   tmpdat.prop = NULL;

   set_palette(pal);
   handle_dclick(&tmpdat);
   set_palette(datedit_current_palette);
}



/* helper for updating the color conversion mode */
static void update_colorconv(int flags, int what)
{
   static int current[2] = {-1, -1};
   int mode, wanted[2];

   if (flags & D_SELECTED)
      wanted[what] = 1;
   else
      wanted[what] = 0;

   if (wanted[what] != current[what]) {
      current[what] = wanted[what];

      mode = (current[0] ? COLORCONV_DITHER : 0) | (current[1] ? COLORCONV_KEEP_TRANS : 0);
      if (mode)
         set_color_conversion(mode);
      else
         set_color_conversion(COLORCONV_NONE);
   } 
}



/* helper for toggling index mode on and off */
static void update_index(int flags)
{
   static int current = -1;
   int wanted;
   
   if (flags & D_SELECTED)
      wanted = 1;
   else
      wanted = 0;

   if (wanted != current) {
      current = wanted;

      rebuild_list(datafile->dat, TRUE);
      object_message(main_dlg+DLG_LIST, MSG_DRAW, 0);
   } 
}



/* helper for toggling sort mode on and off */
static void update_sort(int flags)
{
   static int current = -1;
   int wanted;
   
   if (flags & D_SELECTED)
      wanted = 1;
   else
      wanted = 0;

   if (wanted != current) {
      current = wanted;

      if (current) {
         datedit_sort_datafile(datafile);
         rebuild_list(datafile->dat, TRUE);
         object_message(main_dlg+DLG_LIST, MSG_DRAW, 0);
      }
   } 
}



/* dialog procedure for displaying the selected object */
static int view_proc(int msg, DIALOG *d, int c)
{
   DATAFILE *dat;
   char buf[80];
   int i, c1, c2;

   if (msg == MSG_IDLE) {
      if (current_view_object != SELECTED_ITEM)
	 object_message(d, MSG_DRAW, 0);
   }
   else if (msg == MSG_DRAW) {
      current_view_object = SELECTED_ITEM;

      rectfill(screen, d->x, d->y, SCREEN_W, SCREEN_H, gui_mg_color);

      if ((current_view_object > 0) && (current_view_object < data_count)) {
	 dat = data[current_view_object].dat;

	 textout_ex(screen, font, datedit_desc(dat), d->x, d->y, gui_fg_color, -1);

	 for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
	    if ((datedit_object_info[i]->type == dat->type) && (datedit_object_info[i]->plot)) {
	       datedit_object_info[i]->plot(dat, d->x, d->y);
	       return D_O_K;
	    }
	 }

	 if (dat->type != DAT_FILE) {
	    for (c1=0; c1<16; c1++) {
	       for (c2=0; c2<32; c2++) {
		  if ((c1*32+c2) >= dat->size)
		     buf[c2] = ' ';
		  else
		     buf[c2] = ((char *)dat->dat)[c1*32+c2];
		  if ((buf[c2] < 32) || (buf[c2] > 126))
		     buf[c2] = ' ';
	       }
	       buf[32] = 0;
	       textout_ex(screen, font, buf, d->x, d->y+32+c1*8, gui_fg_color, -1);
	    }
	    if (dat->size > 32*16)
	       textout_ex(screen, font, "...", d->x+31*8, d->y+40+16*8, gui_fg_color, -1);
	 }
      }
   }

   return D_O_K;
}



/* dialog procedure for a listbox with a shadow */
static int droplist_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_DRAW) {
      hline(screen, d->x+1, d->y+d->h, d->x+d->w, d->fg);
      vline(screen, d->x+d->w, d->y+1, d->y+d->h, d->fg);
   }

   return d_list_proc(msg, d, c);
}



/* dialog procedure for the main object list */
static int list_proc(int msg, DIALOG *d, int c)
{
   static int in_dclick = FALSE;
   int ret;

   switch (msg) {

      case MSG_CHAR:
	 switch (c >> 8) {

	    case KEY_ESC:
	       position_mouse(d->x+d->w/3, d->y+6+(d->d1-d->d2)*8);
	       if (do_menu(which_menu(d->d1), mouse_x, mouse_y) >= 0)
		  return D_REDRAW | D_USED_CHAR;
	       else
		  return D_USED_CHAR;

	    case KEY_DEL:
	    case KEY_BACKSPACE:
	       return deleter() | D_USED_CHAR;

	    case KEY_INSERT:
	       return add_new(0) | D_USED_CHAR;
	 }
	 break;

      case MSG_CLICK:
	 if ((mouse_b == 2) && (!in_dclick)) {
	    _handle_listbox_click(d);
	    if (do_menu(which_menu(d->d1), mouse_x, mouse_y) >= 0)
	       return D_REDRAW;
	    else
	       return D_O_K;
	 }
	 break;

      case MSG_DCLICK:
	 in_dclick = TRUE;
	 break;
   }

   ret = droplist_proc(msg, d, c);

   if ((msg == MSG_DRAW) && (!keypressed())) {
      if (current_view_object != d->d1)
	 object_message(main_dlg+DLG_VIEW, MSG_DRAW, 0);

      if (current_property_object != d->d1)
	 object_message(main_dlg+DLG_PROP, MSG_DRAW, 0);
   }

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;
      if ((d->d1 > 0) && (d->d1 < data_count))
	 ret |= handle_dclick(data[d->d1].dat);
   }

   in_dclick = FALSE;

   return ret;
}



/* dialog callback for retrieving information about the object list */
static char *list_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = data_count;
      return NULL;
   }

   return data[index].name;
}



#define MAX_PROPERTIES        64
#define MAX_PROPERTY_VALUE    256

static char prop_string[MAX_PROPERTIES][MAX_PROPERTY_VALUE];
static int num_props = 0;



/* dialog procedure for the property list */
static int prop_proc(int msg, DIALOG *d, int c)
{
   DATAFILE *dat;
   int i;
   int name_pos;
   int ret;

   switch (msg) {

      case MSG_CHAR:
	 switch (c >> 8) {

	    case KEY_DEL:
	    case KEY_BACKSPACE:
	       return property_delete() | D_USED_CHAR;

	    case KEY_INSERT:
	       return property_insert() | D_USED_CHAR;
	 }
	 break;

      case MSG_IDLE:
	 if (current_property_object != SELECTED_ITEM)
	    object_message(d, MSG_DRAW, 0);
	 break;

      case MSG_DRAW:
	 num_props = 0;
	 name_pos = 0;

	 if ((SELECTED_ITEM > 0) && (SELECTED_ITEM < data_count)) {
	    dat = data[SELECTED_ITEM].dat;
	    if (dat->prop) {
	       for (i=0; dat->prop[i].type != DAT_END; i++) {
		  if (i >= MAX_PROPERTIES)
		     break;

		  sprintf(prop_string[num_props], "%c%c%c%c - ",
			  (dat->prop[i].type >> 24) & 0xFF,
			  (dat->prop[i].type >> 16) & 0xFF,
			  (dat->prop[i].type >> 8) & 0xFF,
			  (dat->prop[i].type & 0xFF));

		  if (dat->prop[i].type == DAT_DATE) {
		     long dat_prop_time = datedit_asc2ftime(dat->prop[i].dat);
		     strncat(prop_string[num_props++], datedit_ftime2asc_int(dat_prop_time), 200);
		  }
		  else
		     strncat(prop_string[num_props++], dat->prop[i].dat, 200);

		  if (dat->prop[i].type == DAT_NAME)
		     name_pos = i;
	       } 
	    }
	 }

	 if (current_property_object != SELECTED_ITEM) {
	    current_property_object = SELECTED_ITEM;
	    d->d1 = name_pos;
	 }

	 if (d->d1 >= num_props)
	    d->d1 = num_props-1;
	 if (d->d1 < 0) 
	    d->d1 = 0;

	 break;
   }

   ret = droplist_proc(msg, d, c);

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;
      if (SELECTED_ITEM > 0)
	 ret |= property_change();
   }

   return ret;
}



/* dialog callback for retrieving information about the property list */
static char *prop_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = num_props;
      return NULL;
   }

   return prop_string[index];
}



/* selects the specified object property */
static void select_property(int type)
{
   DATAFILE *dat;
   int i;

   SELECTED_PROPERTY = 0;

   if ((SELECTED_ITEM > 0) && (SELECTED_ITEM < data_count)) {
      dat = data[SELECTED_ITEM].dat;
      if (dat->prop) {
	 for (i=0; dat->prop[i].type != DAT_END; i++) {
	    if (dat->prop[i].type == type) {
	       SELECTED_PROPERTY = i;
	       break;
	    }
	    if (dat->prop[i].type == DAT_NAME)
	       SELECTED_PROPERTY = i;
	 }
      }
   }

   object_message(main_dlg+DLG_PROP, MSG_START, 0);
}



/* checks whether an object name is valid */
static void check_valid_name(char *val)
{
   int i;

   if (val) {
      for (i=0; val[i]; i++) {
	 if (((val[i] < '0') || (val[i] > '9')) &&
	     ((val[i] < 'A') || (val[i] > 'Z')) &&
	     ((val[i] < 'a') || (val[i] > 'z')) &&
	     (val[i] != '_')) {
	    alert("Warning: name is not",
		  "a valid CPP identifier",
		  NULL, "Hmm...", NULL, 13, 0);
	    break;
	 }
      }
   }
}



/* helper for changing object properties */
static void set_property(DATAITEM *dat, int type, char *val)
{
   DATAFILE *d = dat->dat;
   void *old = d->dat;

   datedit_set_property(d, type, val);
   datedit_sort_properties(d->prop);

   if (type == DAT_NAME) {
      check_valid_name(val);
      if (opt_menu[MENU_SORT].flags & D_SELECTED)
         datedit_sort_datafile(*dat->parent);
      rebuild_list(old, TRUE);
   }

   select_property(type);
}



static char prop_type_string[8];
static char prop_value_string[256];



static DIALOG prop_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                 (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,    401,  113,  0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
   { d_ctext_proc,      200,  8,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
   { d_text_proc,       16,   32,   0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
   { d_edit_proc,       72,   32,   40,   8,    0,    0,    0,       0,          4,             0,       prop_type_string,    NULL, NULL  },
   { d_text_proc,       16,   48,   0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
   { d_edit_proc,       72,   48,   320,  8,    0,    0,    0,       0,          255,           0,       prop_value_string,   NULL, NULL  },
   { d_button_proc,     112,  80,   81,   17,   0,    0,    13,      D_EXIT,     0,             0,       "OK",                NULL, NULL  }, 
   { d_button_proc,     208,  80,   81,   17,   0,    0,    27,      D_EXIT,     0,             0,       "Cancel",            NULL, NULL  }, 
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  }
};


#define PROP_DLG_TITLE           1
#define PROP_DLG_TYPE_STRING     2
#define PROP_DLG_TYPE            3
#define PROP_DLG_VALUE_STRING    4
#define PROP_DLG_VALUE           5
#define PROP_DLG_OK              6
#define PROP_DLG_CANCEL          7



/* brings up the property/new object dialog */
static int do_edit(char *title, char *type_string, char *value_string, int type, AL_CONST char *val, int change_type, int show_type)
{
   prop_dlg[PROP_DLG_TITLE].dp = title;
   prop_dlg[PROP_DLG_TYPE_STRING].dp = type_string;
   prop_dlg[PROP_DLG_VALUE_STRING].dp = value_string;

   if (show_type) {
      prop_dlg[PROP_DLG_TYPE_STRING].flags &= ~D_HIDDEN;
      prop_dlg[PROP_DLG_TYPE].flags &= ~D_HIDDEN;
   }
   else {
      prop_dlg[PROP_DLG_TYPE_STRING].flags |= D_HIDDEN;
      prop_dlg[PROP_DLG_TYPE].flags |= D_HIDDEN;
   }

   if (type)
      sprintf(prop_type_string, "%c%c%c%c", type>>24, (type>>16)&0xFF, (type>>8)&0xFF, type&0xFF);
   else
      prop_type_string[0] = 0;

   if (val)
      strcpy(prop_value_string, val);
   else
      prop_value_string[0] = 0;

   centre_dialog(prop_dlg);
   set_dialog_color(prop_dlg, gui_fg_color, gui_bg_color);

   if (change_type)
      prop_dlg[PROP_DLG_TYPE].proc = d_edit_proc;
   else
      prop_dlg[PROP_DLG_TYPE].proc = d_text_proc;

   set_modified(TRUE);

   return (popup_dialog(prop_dlg, (change_type ? PROP_DLG_TYPE : PROP_DLG_VALUE)) == PROP_DLG_OK);
}



/* helper for informing of multiple selections */
static void alert_multiple_selections(void)
{
   alert("Please select one item for this action.", NULL, NULL, "OK", NULL, 13, 0);
}



/* helper for informing of no selection */
static void alert_no_selection(void)
{
   alert("Please make a valid selection.", NULL, NULL, "OK", NULL, 13, 0);
}



/* brings up the property editing dialog */
static int edit_property(char *title, char *value_string, int type, AL_CONST char *val, int change_type, int show_type)
{
   int sel = single_selection();
   DATAITEM *dat;

   if ((sel > 0) && (sel < data_count)) {
      dat = data+sel;
   }
   else {
      if (sel < 0)
	 alert_multiple_selections();
      else
	 alert_no_selection();
      return D_O_K;
   }

   if (do_edit(title, "ID:", value_string, type, val, change_type, show_type))
      if (prop_type_string[0])
	 set_property(dat, datedit_clean_typename(prop_type_string), prop_value_string);

   return D_REDRAW;
}



/* handle the set property command */
static int property_insert(void)
{
   CHECK_MENU_HOOK("Set Property", DATEDIT_MENU_OBJECT);

   return edit_property("Set Property", "Value:", 0, NULL, TRUE, TRUE);
}



/* handle the change property command */
static int property_change(void)
{
   int sel = single_selection();
   DATAFILE *dat;
   int i;

   if ((sel > 0) && (sel < data_count)) {
      dat = data[sel].dat;
      if (dat->prop) {
	 for (i=0; dat->prop[i].type != DAT_END; i++) {
	    if (i == SELECTED_PROPERTY) {
	       return edit_property("Edit Property", "Value: ", dat->prop[i].type, dat->prop[i].dat, FALSE, TRUE);
	    }
	 }
      }
   }
   else {
      if (sel < 0)
	 alert_multiple_selections();
   }

   return D_O_K;
}



/* handle the rename command */
static int renamer(void)
{
   DATAFILE *dat;
   int sel;

   CHECK_MENU_HOOK("Rename", DATEDIT_MENU_OBJECT);

   sel = single_selection();

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert_multiple_selections();
      else
	 alert_no_selection();
      return D_O_K;
   }

   dat = data[sel].dat;
   return edit_property("Rename", "Name:", DAT_NAME, get_datafile_property(dat, DAT_NAME), FALSE, FALSE);
}



/* handle the delete property command */
static int property_delete(void)
{
   int sel = single_selection();
   DATAFILE *dat;
   int i;

   if ((sel > 0) && (sel < data_count)) {
      dat = data[sel].dat;
      if (dat->prop) {
	 for (i=0; dat->prop[i].type != DAT_END; i++) {
	    if (i == SELECTED_PROPERTY) {
	       set_property(data+SELECTED_ITEM, dat->prop[i].type, NULL);
	       set_modified(TRUE);
	       return D_REDRAW;
	    }
	 }
      }
   }
   else {
      if (sel < 0)
	 alert_multiple_selections();
   }

   return D_O_K;
}



/* helper for adding an item to the object list */
static void add_to_list(DATAFILE *dat, DATAFILE **parent, int i, char *name, int clear)
{
   if (data_count >= data_malloced) {
      data_malloced += 16;
      data = realloc(data, data_malloced * sizeof(DATAITEM));
      data_sel = realloc(data_sel, data_malloced * sizeof(char));
      main_dlg[DLG_LIST].dp2 = data_sel;
   }

   data[data_count].dat = dat;
   data[data_count].parent = parent;
   data[data_count].i = i;
   strcpy(data[data_count].name, name);

   if (clear)
      data_sel[data_count] = FALSE;

   data_count++;
}



/* array to record which nested datafiles are folded */
static void **folded = NULL;
static int folded_capacity = 0;
static int folded_size = 0;



/* helper function for comparing two pointers */
static INLINE int ptr_cmp(AL_CONST void *elem_p1, AL_CONST void *elem_p2)
{
   AL_CONST void *ptr1 = *(AL_CONST void **)elem_p1;
   AL_CONST void *ptr2 = *(AL_CONST void **)elem_p2;

   return (int)((AL_CONST unsigned long)ptr2 - (AL_CONST unsigned long)ptr1);
}



/* marks the specified datafile as folded */
static void fold_datafile(DATAFILE *dat)
{
   void **elem_p;

   if (folded_size == folded_capacity) {
      folded_capacity = (folded_capacity ? folded_capacity*2 : 4);
      folded = _al_sane_realloc(folded, folded_capacity * sizeof(void *));
   }

   /* Linear search but I'm too lazy to implement anything else. And you're
    * not supposed to (un)fold nested datafiles like a crazy dog. Otherwise
    * drop me a mail and I'll give you the address of a veterinary clinic.
    */
   for (elem_p = &folded[0]; elem_p < &folded[folded_size]; elem_p++) {
      if (ptr_cmp(&dat->dat, elem_p) < 0)
	 break;
   }

   memmove(elem_p + 1, elem_p, (size_t)((unsigned long)&folded[folded_size++] - (unsigned long)elem_p));
   *elem_p = dat->dat;
}



/* marks the specified datafile as unfolded */
static void unfold_datafile(DATAFILE *dat)
{
   void **elem_p = bsearch(&dat->dat, folded, folded_size, sizeof(void *), ptr_cmp);

   if (elem_p)
      memmove(elem_p, elem_p + 1, (size_t)((unsigned long)&folded[--folded_size] - (unsigned long)elem_p));
}



/* returns whether the specified datafile is folded */
static int is_datafile_folded(DATAFILE *dat)
{
   return bsearch(&dat->dat, folded, folded_size, sizeof(void *), ptr_cmp) != NULL;
}



/* displays a datafile in the grabber object view window */
static void plot_datafile(AL_CONST DATAFILE *dat, int x, int y)
{
   textout_ex(screen,
	      font,
	      "Double-click in the item list to (un)fold it",
	      x,
	      y+32,
	      gui_fg_color,
	      gui_bg_color);
}



/* handles double-clicking on a datafile */
static int dclick_datafile(DATAFILE *dat)
{
   if (is_datafile_folded(dat))
      unfold_datafile(dat);
   else
      fold_datafile(dat);

   rebuild_list(dat->dat, TRUE);
   object_message(main_dlg+DLG_LIST, MSG_DRAW, 0);

   return D_O_K;
}



/* recursive helper used by rebuild list() */
static void add_datafile_to_list(DATAFILE **dat, char *prefix, int clear)
{
   char tmp[80];
   DATAFILE *d;
   int digits = 1, i;
   int indexed = (opt_menu[MENU_INDEX].flags & D_SELECTED);
   int folded = 0;

   if (indexed) {
      i = 0;

      while ((*dat)[i].type != DAT_END)
	 i++;

      while (i/=10)
	 digits++;
   }

   for (i=0; (*dat)[i].type != DAT_END; i++) {
      d = (*dat)+i;

      if (d->type == DAT_FILE)
         folded = is_datafile_folded(d);

      if (indexed) {
	 sprintf(tmp, "[%*d] %c%c%c%c %s%c ", digits, i, (d->type >> 24) & 0xFF,
		 (d->type >> 16) & 0xFF, (d->type >> 8) & 0xFF,
		 (d->type & 0xFF), prefix, 
		 (d->type == DAT_FILE) && folded ? '+' : '-');
      }
      else {
	 sprintf(tmp, "%c%c%c%c %s%c ", (d->type >> 24) & 0xFF,
		 (d->type >> 16) & 0xFF, (d->type >> 8) & 0xFF,
		 (d->type & 0xFF), prefix, 
		 (d->type == DAT_FILE) && folded ? '+' : '-');
      }

      strncat(tmp, get_datafile_property(d, DAT_NAME), 32);

      add_to_list(d, dat, i, tmp, clear);

      if ((d->type == DAT_FILE) && (!folded)) {
         void *ptr;
         strcpy(tmp, prefix);
         strcat(tmp, "|");
         ptr = &d->dat;
         add_datafile_to_list((DATAFILE **)ptr, tmp, clear);
      }
   }
}



/* expands the datafile into a list of objects, for display in the listbox */
static void rebuild_list(void *old, int clear)
{
   int i;

   data_count = 0;

   add_to_list(NULL, &datafile, 0, "<root>", clear);
   add_datafile_to_list(&datafile, "", clear);

   if (old) {
      SELECTED_ITEM = 0;

      for (i=0; i<data_count; i++) {
	 if ((data[i].dat) && (data[i].dat->dat == old)) {
	    SELECTED_ITEM = i;
	    break;
	 }
      }
   }

   object_message(main_dlg+DLG_LIST, MSG_START, 0);
}



/* sets the multiple-selection flag for this object */
static void set_selection(void *object)
{
   int i;

   for (i=0; i<data_count; i++) {
      if ((data[i].dat) && (data[i].dat->dat == object)) {
	 data_sel[i] = TRUE;
	 break;
      }
   }
}



/* dialog callback for retrieving the contents of the compression type list */
static char *pack_getter(int index, int *list_size)
{
   static char *s[] =
   {
      "No compression",
      "Individual compression",
      "Global compression"
   };

   static char *s2[] =
   {
      "Unpacked",
      "Per-object",
      "Compressed"
   };

   ASSERT(sizeof(s) / sizeof(s[0]) == sizeof(s2) / sizeof(s2[0]));
   if (index < 0) {
      *list_size = sizeof(s) / sizeof(s[0]);
      return NULL;
   }

   if (SCREEN_W < 640)
      return s2[index];
   else
      return s[index];
}



/* updates the info chunk with the current settings */
static void update_info(void)
{
   char buf[8];

   datedit_set_property(&datedit_info, DAT_HNAM, header_file);
   datedit_set_property(&datedit_info, DAT_HPRE, prefix_string);
   datedit_set_property(&datedit_info, DAT_XGRD, xgrid_string);
   datedit_set_property(&datedit_info, DAT_YGRD, ygrid_string);

   datedit_set_property(&datedit_info, DAT_BACK, 
		  (opt_menu[MENU_BACKUP].flags & D_SELECTED) ? "y" : "n");

   datedit_set_property(&datedit_info, DAT_DITH, 
		  (opt_menu[MENU_DITHER].flags & D_SELECTED) ? "y" : "n");

   datedit_set_property(&datedit_info, DAT_TRAN, 
		  (opt_menu[MENU_TRANS].flags & D_SELECTED) ? "y" : "n");

   datedit_set_property(&datedit_info, DAT_SORT, 
		  (opt_menu[MENU_SORT].flags & D_SELECTED) ? "y" : "n");

   datedit_set_property(&datedit_info, DAT_RELF, 
		  (opt_menu[MENU_RELF].flags & D_SELECTED) ? "y" : "n");

   sprintf(buf, "%d", main_dlg[DLG_PACKLIST].d1);
   datedit_set_property(&datedit_info, DAT_PACK, buf);
}



/* helper for recovering data stored in the info chunk */
static void retrieve_property(int object, int type, char *def)
{
   AL_CONST char *p = get_datafile_property(&datedit_info, type);

   if ((p) && (*p))
      strcpy(main_dlg[object].dp, p);
   else
      strcpy(main_dlg[object].dp, def);

   main_dlg[object].d2 = strlen(main_dlg[object].dp);
}



/* do the actual work of loading a file */
static void load(char *filename, int flush)
{
   DATAFILE *new_datafile;
   int obj_count;
   int new_obj_count;
   int items_num;
   int sort;
   AL_CONST char *sort_prop;

   set_busy_mouse(TRUE);

   if ((datafile) && (flush)) {
      unload_datafile(datafile);
      datafile = NULL;
   }

   if (filename) {
      canonicalize_filename(grabber_data_file, filename, sizeof(grabber_data_file));
      strcpy(grabber_data_file, datedit_pretty_name(grabber_data_file, "dat", FALSE));
   }
   else
      grabber_data_file[0] = 0;

   main_dlg[DLG_FILENAME].d2 = strlen(grabber_data_file);

   new_datafile = datedit_load_datafile(filename, FALSE, password);
   if (!new_datafile)
      new_datafile = datedit_load_datafile(NULL, FALSE, NULL);

   /* preserve old behaviour (always sort) if the SORT property is not present */
   sort_prop = get_datafile_property(&datedit_info, DAT_SORT);
   if (*sort_prop)
      sort = (utolower(*sort_prop)=='y');
   else
      sort = TRUE;

   if (!flush) {
      obj_count = 0;
      while (datafile[obj_count].type != DAT_END)
	 obj_count++;

      new_obj_count = 0;
      while (new_datafile[new_obj_count].type != DAT_END)
	 new_obj_count++;

      datafile = realloc(datafile, (obj_count+new_obj_count+1) * sizeof(DATAFILE));
      memmove(datafile+obj_count, new_datafile, (new_obj_count+1) * sizeof(DATAFILE));
      free(new_datafile);

      if (sort)
         datedit_sort_datafile(datafile);

      set_modified(TRUE);
   }
   else {
      datafile = new_datafile;
      set_modified(FALSE);
   }

   SELECTED_ITEM = 0;

   retrieve_property(DLG_HEADERNAME, DAT_HNAM, "");
   retrieve_property(DLG_PREFIXSTRING, DAT_HPRE, "");
   retrieve_property(DLG_XGRIDSTRING, DAT_XGRD, "16");
   retrieve_property(DLG_YGRIDSTRING, DAT_YGRD, "16");

   if (utolower(*get_datafile_property(&datedit_info, DAT_BACK)) == 'y')
      opt_menu[MENU_BACKUP].flags |= D_SELECTED;
   else
      opt_menu[MENU_BACKUP].flags &= ~D_SELECTED;

   if (utolower(*get_datafile_property(&datedit_info, DAT_DITH)) == 'y')
      opt_menu[MENU_DITHER].flags |= D_SELECTED;
   else
      opt_menu[MENU_DITHER].flags &= ~D_SELECTED;

   if (utolower(*get_datafile_property(&datedit_info, DAT_TRAN)) == 'y')
      opt_menu[MENU_TRANS].flags |= D_SELECTED;
   else
      opt_menu[MENU_TRANS].flags &= ~D_SELECTED;

   if (utolower(*get_datafile_property(&datedit_info, DAT_RELF)) == 'y')
      opt_menu[MENU_RELF].flags |= D_SELECTED;
   else
      opt_menu[MENU_RELF].flags &= ~D_SELECTED;

   if (sort)
      opt_menu[MENU_SORT].flags |= D_SELECTED;
   else
      opt_menu[MENU_SORT].flags &= ~D_SELECTED;

   main_dlg[DLG_PACKLIST].d1 = atoi(get_datafile_property(&datedit_info, DAT_PACK));
   pack_getter(-1, &items_num);
   if (main_dlg[DLG_PACKLIST].d1 >= items_num)
      main_dlg[DLG_PACKLIST].d1 = items_num-1;
   else if (main_dlg[DLG_PACKLIST].d1 < 0)
      main_dlg[DLG_PACKLIST].d1 = 0;

   rebuild_list(NULL, TRUE);

   set_busy_mouse(FALSE);
}



/* helper for closing modified files */
static int ask_save_changes(void)
{
   return alert3("Save changes to datafile?",
		 NULL, NULL,
		 "Yes", "No", "Cancel", 'y', 'n', 27);
}



/* handle the load command */
static int loader(void)
{
   char buf[FILENAME_LENGTH];

   CHECK_MENU_HOOK("Load", DATEDIT_MENU_FILE);

   strcpy(buf, grabber_data_file);
   *get_filename(buf) = 0;

   if (file_select_ex("Load datafile", buf, "dat", sizeof(buf), 0, 0)) {
      if (is_modified) {
	 int r = ask_save_changes();

	 switch (r) {

	    case 1:
	       saver();
	       break;

	    case 2:
	       break;

	    case 3:
	    default:
	       return D_REDRAW;
	 }
      }

      fix_filename_case(buf);
      load(buf, 1);
   }

   return D_REDRAW;
}



/* handle the new file command */
static int renewer(void)
{
   CHECK_MENU_HOOK("New", DATEDIT_MENU_FILE);

   if (is_modified) {
      int r = ask_save_changes();

      switch (r) {

	 case 1:
	    saver();
	    break;

	 case 2:
	    break;

	 case 3:
	 default:
	    return D_REDRAW;
      }
   }

   load(NULL, 1);

   return D_REDRAW;
}



/* handle the merge command */
static int merger(void)
{
   char buf[FILENAME_LENGTH];

   CHECK_MENU_HOOK("Merge", DATEDIT_MENU_FILE);

   strcpy(buf, grabber_data_file);
   *get_filename(buf) = 0;

   if (file_select_ex("Merge datafile", buf, "dat", sizeof(buf), 0, 0)) {
      fix_filename_case(buf);
      load(buf, 0);
   }

   return D_REDRAW;
}



/* do the actual work of saving a file */
static int save(int strip)
{
   DATEDIT_SAVE_DATAFILE_OPTIONS options;
   char buf[FILENAME_LENGTH], buf2[256];
   int err = FALSE;

   strcpy(buf, grabber_data_file);

   if (file_select_ex("Save datafile", buf, "dat", sizeof(buf), 0, 0)) {
      if ((stricmp(grabber_data_file, buf) != 0) && (exists(buf))) {
	 sprintf(buf2, "%s already exists, overwrite?", buf);
	 if (alert(buf2, NULL, NULL, "Yes", "No", 'y', 'n') != 1)
	    return D_REDRAW;
      }

      box_start();

      set_busy_mouse(TRUE);

      fix_filename_case(buf);
      strcpy(grabber_data_file, buf);
      main_dlg[DLG_FILENAME].d2 = strlen(grabber_data_file);

      update_info();

      options.pack = -1;
      options.strip = strip;
      options.sort = -1;
      options.verbose = TRUE;
      options.write_msg = FALSE;
      options.backup = (opt_menu[MENU_BACKUP].flags & D_SELECTED);
      options.relative = (opt_menu[MENU_RELF].flags & D_SELECTED);

      if (!datedit_save_datafile(datafile, grabber_data_file, NULL, &options, password))
	 err = TRUE;
      else
	 set_modified(FALSE);

      if ((header_file[0]) && (!err)) {
	 box_eol();

	 if ((!strchr(header_file, OTHER_PATH_SEPARATOR)) && (!strchr(header_file, '/'))) {
	    strcpy(buf, grabber_data_file);
	    strcpy(get_filename(buf), header_file);
	 }
	 else
	    strcpy(buf, header_file);

	 if (!datedit_save_header(datafile, grabber_data_file, buf, "grabber", prefix_string, FALSE))
	    err = TRUE;
      }

      set_busy_mouse(FALSE);

      box_end(!err);
   }

   return D_REDRAW;
}



/* handle the save command */
static int saver(void)
{
   CHECK_MENU_HOOK("Save", DATEDIT_MENU_FILE);

   return save(-1);
}



/* dialog callback for retrieving the contents of the strip mode list */
static char *striplist_getter(int index, int *list_size)
{
   static char *str[] =
   {
      "Save everything",
      "Strip grabber information",
      "Strip all object properties"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(str) / sizeof(str[0]);
      return NULL;
   }

   return str[index];
}



static DIALOG strip_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,    301,  113,  0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_ctext_proc,      150,  8,    0,    0,    0,    0,    0,       0,          0,             0,       "Save Stripped",  NULL, NULL  },
   { d_list_proc,       22,   32,   257,  28,   0,    0,    0,       D_EXIT,     0,             0,       striplist_getter, NULL, NULL  },
   { d_button_proc,     62,   80,   81,   17,   0,    0,    13,      D_EXIT,     0,             0,       "OK",             NULL, NULL  }, 
   { d_button_proc,     158,  80,   81,   17,   0,    0,    27,      D_EXIT,     0,             0,       "Cancel",         NULL, NULL  }, 
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};


#define STRIP_DLG_LIST        2
#define STRIP_DLG_OK          3
#define STRIP_DLG_CANCEL      4



/* handle the save stripped command */
static int strip_saver(void)
{
   CHECK_MENU_HOOK("Save Stripped", DATEDIT_MENU_FILE);

   centre_dialog(strip_dlg);
   set_dialog_color(strip_dlg, gui_fg_color, gui_bg_color);

   if (do_dialog(strip_dlg, STRIP_DLG_LIST) == STRIP_DLG_CANCEL)
      return D_REDRAW;

   return save(strip_dlg[STRIP_DLG_LIST].d1);
}



/* worker function for the update commands */
static int update_worker(AL_CONST char *name, int selection_only, int force)
{
   int c;
   int nowhere;
   int err = FALSE;

   CHECK_MENU_HOOK(name, DATEDIT_MENU_FILE);

   box_start();

   set_busy_mouse(TRUE);

   for (c=1; c<data_count; c++) {
      if (!selection_only || (c==SELECTED_ITEM) || data_sel[c]) {
	 if (data[c].dat->type != DAT_FILE) {
	    if (!datedit_update(data[c].dat, grabber_data_file, force, FALSE, &nowhere)) {
	       err = TRUE;
	       break;
	    }
	    datedit_sort_properties(data[c].dat->prop);
	 }
      }
   }

   set_busy_mouse(FALSE);

   if (!err) {
      box_out("Done!");
      box_eol();
   }

   box_end(!err);

   select_property(DAT_NAME);

   set_modified(TRUE);

   return D_REDRAW;
}



/* handle the update command */
static int updater(void)
{
   return update_worker("Update", FALSE, FALSE);
}



/* handle the update_selection command */
static int sel_updater(void)
{
   return update_worker("Update selection", TRUE, FALSE);
}



/* handle the force_update command */
static int force_updater(void)
{
   return update_worker("Force Update", FALSE, TRUE);
}



/* handle the force_update_selection command */
static int force_sel_updater(void)
{
   return update_worker("Force Update selection", TRUE, TRUE);
}



/* formats a heading for the file select dialog */
static void format_file_select_heading(char *dest, char *s1, char *s2, AL_CONST char *ext)
{
   int len;

   if (ext) {
      if (s2)
	 len = strlen(s1) + strlen(s2) + strlen(ext) + 4;
      else
	 len = strlen(s1) + strlen(ext) + 3;

      if (len > 36) {
	 if (s2)
	    sprintf(dest, "%s %s", s1, s2);
	 else
	    strcpy(dest, s1);
      }
      else {
	 if (s2)
	    sprintf(dest, "%s %s (%s)", s1, s2, ext);
	 else
	    sprintf(dest, "%s (%s)", s1, ext);
      }
   }
   else if (s2)
      sprintf(dest, "%s %s", s1, s2);
   else
      strcpy(dest, s1);
}



/* handle the read command */
static int reader(void)
{
   DATAFILE *dat;
   char buf[FILENAME_LENGTH], buf2[256];
   DATEDIT_GRAB_PARAMETERS params;
   AL_CONST char *s;

   CHECK_MENU_HOOK("Read", DATEDIT_MENU_FILE);

   strcpy(buf, grabber_import_file);
   *get_filename(buf) = 0;

   s = datedit_grab_ext(DAT_BITMAP);

   if (!s) {
      alert("You seem to have removed",
	    "the BITMAP plugin!",
	    NULL, "Not good...", NULL, 13, 0);
      return D_O_K;
   }

   format_file_select_heading(buf2, "Read bitmap file", NULL, s);

   if (file_select_ex(buf2, buf, s, sizeof(buf), 0, 0)) {
      fix_filename_case(buf);

      set_busy_mouse(TRUE);

      if (grabber_graphic)
	 destroy_bitmap(grabber_graphic);

      strcpy(grabber_import_file, buf);

      params.datafile = grabber_data_file;
      params.filename = grabber_import_file;
      params.name = grabber_import_file;
      params.type = DAT_BITMAP;
      params.x = -1;
      params.y = -1;
      params.w = -1;
      params.h = -1;
      params.colordepth = -1;
      params.relative = (opt_menu[MENU_RELF].flags & D_SELECTED);

      dat = datedit_grab(NULL, &params);

      set_busy_mouse(FALSE);

      if (dat) {
	 grabber_graphic = dat->dat;
	 memcpy(grabber_palette, datedit_last_read_pal, sizeof(PALETTE));

	 dat->dat = NULL;
	 _unload_datafile_object(dat);

	 if (bitmap_color_depth(grabber_graphic) > 8)
	    generate_optimized_palette(grabber_graphic, grabber_palette, NULL);

	 show_a_bitmap(grabber_graphic, grabber_palette);

	 if (opt_menu[MENU_RELF].flags & D_SELECTED)
	    make_relative_filename(grabber_graphic_origin, grabber_data_file, grabber_import_file, GRABBER_GRAPHIC_ORIGIN_SIZE);
	 else
	    strcpy(grabber_graphic_origin, grabber_import_file);

	 strcpy(grabber_graphic_date, datedit_ftime2asc(file_time(grabber_import_file)));
      }
      else {
	 grabber_graphic_origin[0] = 0;
	 grabber_graphic_date[0] = 0;
      }
   }

   return D_REDRAW;
}



/* handle the view command */
static int viewer(void)
{
   CHECK_MENU_HOOK("View", DATEDIT_MENU_FILE);

   if (grabber_graphic) {
      show_a_bitmap(grabber_graphic, grabber_palette);
      return D_REDRAW;
   }
   else {
      alert("Nothing to view!",
	    "First you must read in a bitmap file", 
	    NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }
}



/* handle the quit command */
static int quitter(void)
{
   int r;

   CHECK_MENU_HOOK("Quit", DATEDIT_MENU_FILE);

   if (!is_modified)
      return D_CLOSE;

   r = ask_save_changes();

   if (r == 2)
      return D_CLOSE;

   if (r == 3)
      return D_O_K;

   saver();
   return D_CLOSE;
}



/* handle the grab command */
static int grabber(void)
{
   DATAFILE *dat;
   char *desc = "binary data";
   AL_CONST char *ext, *origin;
   DATEDIT_GRAB_PARAMETERS params;
   char buf[256], name[FILENAME_LENGTH];
   int sel;
   int i;

   CHECK_MENU_HOOK("Grab", DATEDIT_MENU_OBJECT);

   sel = single_selection();

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert_multiple_selections();
      else
	 alert_no_selection();
      return D_O_K;
   }

   dat = data[sel].dat;

   for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
      if (datedit_object_info[i]->type == dat->type) {
	 desc = datedit_object_info[i]->desc;
	 break;
      }
   }

   ext = datedit_grab_ext(dat->type);

   origin = get_datafile_property(dat, DAT_ORIG);
   if (origin[0]) {
      if (is_relative_filename(origin))
         make_absolute_filename(name, grabber_data_file, origin, sizeof(name));
      else
         strcpy(name, origin);
   }
   else {
      strcpy(name, grabber_import_file);
      *get_filename(name) = 0;
   }

   format_file_select_heading(buf, "Grab", desc, ext);

   if (file_select_ex(buf, name, ext, sizeof(name), 0, 0)) {
      fix_filename_case(name);

      set_busy_mouse(TRUE);

      strcpy(grabber_import_file, name);

      params.datafile = grabber_data_file;
      params.filename = name;
      params.name = get_datafile_property(dat, DAT_NAME);
      params.type = dat->type;
      params.x = -1;
      params.y = -1;
      params.w = -1;
      params.h = -1;
      params.colordepth = -1;
      params.relative = (opt_menu[MENU_RELF].flags & D_SELECTED);

      datedit_grabreplace(dat, &params);

      set_modified(TRUE);

      if (dat->type == DAT_FILE)
	 rebuild_list(NULL, TRUE);

      set_busy_mouse(FALSE);
   }

   datedit_sort_properties(dat->prop);
   select_property(DAT_NAME);

   return D_REDRAW;
}



/* handle the export command */
static int exporter(void)
{
   char *desc = "binary data";
   AL_CONST char *ext;
   char buf[256], name[FILENAME_LENGTH];
   DATAFILE *dat;
   int sel;
   int i;

   CHECK_MENU_HOOK("Export", DATEDIT_MENU_OBJECT);

   sel = single_selection();

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert_multiple_selections();
      else
	 alert_no_selection();
      return D_O_K;
   }

   dat = data[sel].dat;

   for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
      if (datedit_object_info[i]->type == dat->type) {
	 desc = datedit_object_info[i]->desc;
	 break;
      }
   }

   ext = datedit_export_ext(dat->type);

   strcpy(name, grabber_import_file);
   *get_filename(name) = 0;

   if (*get_datafile_property(dat, DAT_ORIG)) {
      datedit_export_name(dat, NULL, ext, buf);
      strcat(name, get_filename(buf));
   }

   format_file_select_heading(buf, "Export", desc, ext);

   if (file_select_ex(buf, name, ext, sizeof(name), 0, 0)) {
      fix_filename_case(name);

      set_busy_mouse(TRUE);

      strcpy(grabber_import_file, name);
      update_info();
      datedit_export(dat, name);

      set_busy_mouse(FALSE);
   }

   return D_REDRAW;
}



/* handle the delete command */
static int deleter(void)
{
   void *dat;
   void **todel = NULL;
   int todel_count = 0;
   int todel_alloc = 0;
   char buf[256];
   AL_CONST char *name = "";
   int first = 0;
   int i;

   CHECK_MENU_HOOK("Delete", DATEDIT_MENU_OBJECT);

   for (i=1; i<data_count; i++) {
      if ((i == SELECTED_ITEM) || (data_sel[i])) {
	 if (todel_count == 0) {
	    name = get_datafile_property(data[i].dat, DAT_NAME);
	    first = i;
	 }
	 if (todel_count >= todel_alloc) {
	    todel_alloc += 16;
	    todel = realloc(todel, todel_alloc * sizeof(void *));
	 }
	 todel[todel_count++] = data[i].dat->dat;
      }
   }

   if (todel_count <= 0) {
      alert_no_selection();
      return D_O_K;
   }

   if (todel_count == 1)
      sprintf(buf, "%s?", name[0] ? name : "this item");
   else
      sprintf(buf, "these %d items?", todel_count);

   if (alert("Really delete", buf, NULL, "Yes", "No", 'y', 'n') != 1)
      return D_O_K;

   SELECTED_ITEM = first;

   while (todel_count > 0) {
      dat = todel[--todel_count];
      for (i=1; i<data_count; i++) { 
	 if (data[i].dat->dat == dat) {
	    *data[i].parent = datedit_delete(*data[i].parent, data[i].i);
	    rebuild_list(NULL, TRUE);
	    break;
	 }
      }
   }

   free(todel);

   set_modified(TRUE);

   return D_REDRAW;
}



/* returns the datafile's index in the display list (helper for mover) */
static int data_index(DATAFILE *dat)
{
   int i;
   for (i=1; i<data_count; i++) {
      if(data[i].dat == dat)
	 return i;
   }
   return -1;
}



/* handle the move commands */
static int mover(int direction)
{
   DATAITEM *item;
   DATAFILE *dest;

   CHECK_MENU_HOOK("Move", DATEDIT_MENU_OBJECT);

   if (SELECTED_ITEM == 0) {
      alert_no_selection();
      return D_O_K;
   }
   else if (single_selection() < 0) {
      alert_multiple_selections();
      return D_O_K;
   }

   item = data+SELECTED_ITEM;
   dest = &(*item->parent)[item->i+direction];

   if (item->i==0 && direction<0)
      return D_O_K;
   else if (item->dat[1].type==DAT_END && direction>0)
      return D_O_K;

   datedit_swap(*item->parent, item->i, item->i+direction);
   rebuild_list(NULL, TRUE);
   SELECTED_ITEM = data_index(dest);

   set_modified(TRUE);

   return D_REDRAW;
}



/* moves a datafile to the previous index, if possible */
static int mover_up(void)
{
   return mover(-1);
}



/* moves a datafile to the next index, if possible */
static int mover_down(void)
{
   return mover(1);
}



/* creates a new binary data object */
static void *makenew_data(long *size)
{
   static char msg[] = "Binary Data";

   void *v = _AL_MALLOC(sizeof(msg));

   strcpy(v, msg);
   *size = sizeof(msg);

   return v;
}



/* handle the replace command */
static int replacer(int type)
{
   DATAFILE *dat;
   char buf[256];
   AL_CONST char *name;
   int i, sel;
   void *v = NULL;
   long size;

   CHECK_MENU_HOOK("Replace", DATEDIT_MENU_OBJECT);

   sel = single_selection();

   if ((sel > 0) && (sel < data_count)) {
      dat = data[sel].dat;
   }
   else {
      if (sel < 0)
	 alert_multiple_selections();
      else
	 alert_no_selection();

      return D_O_K;
   }

   name = get_datafile_property(dat, DAT_NAME);
   sprintf(buf, "%s?", name[0] ? name : "this item");

   if (alert("Really delete", buf, NULL, "Yes", "No", 'y', 'n') != 1)
      return D_O_K;

   if (!do_edit("New Object", "Type:", "Name:", type, NULL, (type == 0), TRUE))
      return D_O_K;

   if (prop_type_string[0]) {
      type = datedit_clean_typename(prop_type_string);
      check_valid_name(prop_value_string);

      for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
	 if ((datedit_object_info[i]->type == type) && (datedit_object_info[i]->makenew)) {
	    v = datedit_object_info[i]->makenew(&size);
	    break;
	 }
      }

      if (!v)
	 v = makenew_data(&size);
   }
   else {
      alert("Invalid type!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   _unload_datafile_object(dat);
   dat->dat = v;
   dat->size = size;
   dat->type = type;
   dat->prop = NULL;
   datedit_set_property(dat, DAT_NAME, prop_value_string);

   rebuild_list(NULL, TRUE);
   select_property(DAT_NAME);

   set_modified(TRUE);

   return D_REDRAW;
}



/* handle the backup option */
static int backup_toggler(void)
{
   opt_menu[MENU_BACKUP].flags ^= D_SELECTED;
   return D_O_K;
}



/* handle the dither option */
static int dither_toggler(void)
{
   opt_menu[MENU_DITHER].flags ^= D_SELECTED;
   update_colorconv(opt_menu[MENU_SORT].flags, 0);
   return D_O_K;
}



/* handle the index option */
static int index_toggler(void)
{
   opt_menu[MENU_INDEX].flags ^= D_SELECTED;
   update_index(opt_menu[MENU_INDEX].flags);
   return D_O_K;
}



/* handle the sort option */
static int sort_toggler(void)
{
   opt_menu[MENU_SORT].flags ^= D_SELECTED;
   update_sort(opt_menu[MENU_SORT].flags);
   return D_O_K;
}



/* handle the preserve transparency option */
static int trans_toggler(void)
{
   opt_menu[MENU_TRANS].flags ^= D_SELECTED;
   update_colorconv(opt_menu[MENU_SORT].flags, 1);
   return D_O_K;
}




/* handle the relative paths option */
static int relf_toggler(void)
{
   opt_menu[MENU_RELF].flags ^= D_SELECTED;
   return D_O_K;
}



/* help system by Doug Eleveld */
#define N_HELP_TEXT_SECTIONS  9

static char *grabber_last_help = NULL;
static char *grabber_help_text = NULL;
static char *grabber_help_text_section[N_HELP_TEXT_SECTIONS];


static int set_grabber_help_main(void)
{
   grabber_help_text = grabber_help_text_section[0];
   return D_O_K;
}


static int set_grabber_help_using_grabber(void)
{
   grabber_help_text = grabber_help_text_section[1];
   return D_O_K;
}


static int set_grabber_help_using_archiver(void)
{
   grabber_help_text = grabber_help_text_section[2];
   return D_O_K;
}


static int set_grabber_help_misc(void)
{
   grabber_help_text = grabber_help_text_section[3];
   return D_O_K;
}


static int set_grabber_help_accessing_datafiles(void)
{
   grabber_help_text = grabber_help_text_section[4];
   return D_O_K;
}


static int set_grabber_help_compiling_datafiles_to_asm(void)
{
   grabber_help_text = grabber_help_text_section[5];
   return D_O_K;
}


static int set_grabber_help_compiling_datafiles_to_c(void)
{
   grabber_help_text = grabber_help_text_section[6];
   return D_O_K;
}


static int set_grabber_help_custom_objects(void)
{
   grabber_help_text = grabber_help_text_section[7];
   return D_O_K;
}


static int set_grabber_help_file_format(void)
{
   grabber_help_text = grabber_help_text_section[8];
   return D_O_K;
}


static int do_grabber_help_exit(void)
{
   return D_CLOSE;
}



static MENU grabber_help_datafiles[] =
{
   { "&Accessing",        set_grabber_help_accessing_datafiles,        NULL, 0, NULL },
   { "&Compiling to asm", set_grabber_help_compiling_datafiles_to_asm, NULL, 0, NULL },
   { "&Compiling to C",   set_grabber_help_compiling_datafiles_to_c,   NULL, 0, NULL },
   { NULL,                NULL,                                        NULL, 0, NULL }
};


static MENU grabber_help_choose_topic[] =
{
   { "&Main",           set_grabber_help_main,                 NULL, 0, NULL },
   { "&Grabber",        set_grabber_help_using_grabber,        NULL, 0, NULL },
   { "&Archiver",       set_grabber_help_using_archiver,       NULL, 0, NULL },
   { "&Datafiles",      NULL,                                  grabber_help_datafiles, 0, NULL },
   { "&Custom Objects", set_grabber_help_custom_objects,       NULL, 0, NULL },
   { "&File format",    set_grabber_help_file_format,          NULL, 0, NULL },
   { "M&isc",           set_grabber_help_misc,                 NULL, 0, NULL },
   { NULL,              NULL,                                  NULL, 0, NULL }
};


static MENU grabber_help_topic[] =
{
   { "&Topic",          NULL,                                  grabber_help_choose_topic, 0, NULL },
   { "E&xit",           do_grabber_help_exit,                  NULL, 0, NULL },
   { NULL,              NULL,                                  NULL, 0, NULL }
};



static int d_helptext_proc(int msg, DIALOG *d, int c)
{
   if (d->dp == NULL)
      d->dp = grabber_help_text;

   /* if the text needs changing changed, redraw */
   if ((msg == MSG_IDLE) && (grabber_help_text != grabber_last_help)) {
      d->dp = grabber_help_text;
      grabber_last_help = grabber_help_text;

      d->d2 = 0;

      object_message(d, MSG_DRAW, c);
      return D_O_K;
   }
   else
      return d_textbox_proc(msg, d, c);
};



static DIALOG grabber_help[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)  (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    8,    8,    0,    0,       0,    0,    NULL, NULL, NULL },
   { d_text_proc,       0,    4,    0,    0,    255,  8,    0,    0,       0,    0,    "Help viewer by Doug Eleveld", NULL, NULL },
   { d_menu_proc,       0,    0,    0,    0,    255,  0,    0,    0,       0,    0,    grabber_help_topic, NULL, NULL },
   { d_helptext_proc,   0,    20,   0,    0,    255,  0,    0,    0,       0,    0,    NULL, NULL, NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL }
};



/* handle the help command */
static int helper(void)
{
   char buf[256];
   PACKFILE *f;
   char *help_text, *last, *s; 
   int i, j, cr;
   int64_t size;

   CHECK_MENU_HOOK("Help", DATEDIT_MENU_HELP);

   get_executable_name(buf, sizeof(buf));
   strcpy(get_filename(buf), "grabber.txt");

   size = file_size_ex(buf);
   if (size <= 0) {
      alert("Error reading grabber.txt", NULL, NULL, "Oh dear", NULL, 13, 0);
      return D_REDRAW;
   }

   grabber_help_text = help_text = malloc(size+1); 

   f = pack_fopen(buf, F_READ);
   if (!f) {
      alert("Error reading grabber.txt", NULL, NULL, "Oh dear", NULL, 13, 0);
      return D_REDRAW;
   }

   pack_fread(grabber_help_text, size, f);
   pack_fclose(f);

   grabber_help_text[size] = 0;
   grabber_help_text_section[0] = grabber_help_text;

   cr = (strstr(grabber_help_text, "\r\n") != NULL) ? 1 : 0;

   last = grabber_help_text;

   while ((s = strstr(last, (cr ? " \r\n" : " \n"))) != NULL) {
      last = s+2+cr;
      if ((SCREEN_W >= 640) && (uisspace(*last)))
	 continue;
      j = 0;
      while (uisspace(last[j]))
	 j++;
      s++;
      memmove(s, s+1+cr+j, size - ((long)s-(long)grabber_help_text) - 1 - j);
      size -= 1+cr+j;
      last = s;
   }

   last = grabber_help_text;

   for (i=1; i<N_HELP_TEXT_SECTIONS; i++) {
      s = strstr(last , (cr ? "\r\n\r\n====" : "\n\n===="));

      if (s) {
	 grabber_help_text_section[i] = last = s+2+cr*2;
	 s[1+cr*2] = 0;
      }
      else
	 grabber_help_text_section[i] = NULL;
   }

   grabber_last_help = NULL;

   set_dialog_color(grabber_help, gui_fg_color, gui_bg_color);
   grabber_help[0].fg = gui_mg_color;
   grabber_help[0].bg = gui_mg_color;
   grabber_help[1].bg = gui_mg_color;

   grabber_help[3].d1 = 0;
   grabber_help[3].d2 = 0;
   grabber_help[3].dp = grabber_help_text;

   if (SCREEN_W > 512)
      grabber_help[1].x = SCREEN_W - 220;
   else
      grabber_help[1].x = SCREEN_W;

   grabber_help[3].w = SCREEN_W;
   grabber_help[3].h = SCREEN_H - 20;

   do_dialog(grabber_help, 3);

   free(help_text);

   return D_REDRAW;
}



static DIALOG sys_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)     (d2)     (dp)     (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,    0,    0,    0,    0,    0,       0,          0,       0,       NULL,    NULL, NULL  },
   { d_textbox_proc,    0,    0,    0,    0,    0,    0,    0,       0,          0,       0,       NULL,    NULL, NULL  },
   { d_button_proc,     0,    0,    0,    0,    0,    0,    0,       D_EXIT,     0,       0,       "Exit",  NULL, NULL  },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,       0,       NULL,    NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,       0,       NULL,    NULL, NULL  }
};



/* handle the system info */
static int sysinfo(void)
{
   char *systext;
   AL_CONST char* s;
   int i, type;

   CHECK_MENU_HOOK("System", DATEDIT_MENU_HELP);

   systext = malloc(65536);
   systext[0] = 0;

   strcpy(systext, "System Status\n=============\n\n\n\n");

   switch (os_type) {
      case OSTYPE_UNKNOWN:    s = "DOS";                        break;
      case OSTYPE_WIN3:       s = "Windows 3.x";                break;
      case OSTYPE_WIN95:      s = "Windows 95";                 break;
      case OSTYPE_WIN98:      s = "Windows 98";                 break;
      case OSTYPE_WINME:      s = "Windows ME";                 break;
      case OSTYPE_WINNT:      s = "Windows NT";                 break;
      case OSTYPE_WIN2000:    s = "Windows 2000";               break;
      case OSTYPE_WINXP:      s = "Windows XP";                 break;
      case OSTYPE_OS2:        s = "OS/2";                       break;
      case OSTYPE_WARP:       s = "OS/2 Warp 3";                break;
      case OSTYPE_DOSEMU:     s = "Linux DOSEMU";               break;
      case OSTYPE_OPENDOS:    s = "Caldera OpenDOS";            break;
      case OSTYPE_LINUX:      s = "Linux";                      break;
      case OSTYPE_SUNOS:      s = "SunOS/Solaris";              break;
      case OSTYPE_FREEBSD:    s = "FreeBSD";                    break;
      case OSTYPE_NETBSD:     s = "NetBSD";                     break;
      case OSTYPE_OPENBSD:    s = "OpenBSD";                    break;
      case OSTYPE_IRIX:       s = "IRIX";                       break;
      case OSTYPE_DARWIN:     s = "Darwin";                     break;
      case OSTYPE_QNX:        s = "QNX";                        break;
      case OSTYPE_UNIX:       s = "Unix";                       break;
      case OSTYPE_BEOS:       s = "BeOS";                       break;
      case OSTYPE_HAIKU:      s = "Haiku";                      break;
      case OSTYPE_MACOS:      s = "MacOS";                      break;
      case OSTYPE_MACOSX:     s = "MacOS X";                    break;
      default:                s = "Unknown";                    break;
   }

   sprintf(systext+strlen(systext), "Platform: %s (version %d.%d)\n\n\n\n", s, os_version, os_revision);

   #ifdef ALLEGRO_I386

      sprintf(systext+strlen(systext), "CPU: %s %d86", cpu_vendor, cpu_family);

      if (cpu_capabilities & CPU_FPU)
	 strcat(systext, " / FPU");

      if (cpu_capabilities & CPU_SSE)
	 strcat(systext, " / SSE");
	 
      if (cpu_capabilities & CPU_SSE2)
	 strcat(systext, " / SSE2");
	 
      if (cpu_capabilities & CPU_MMX)
	 strcat(systext, " / MMX");

      if (cpu_capabilities & CPU_MMXPLUS)
	 strcat(systext, " / MMX+");

      if (cpu_capabilities & CPU_3DNOW)
	 strcat(systext, " / 3DNow!");

      if (cpu_capabilities & CPU_ENH3DNOW)
	 strcat(systext, " / Enh 3DNow!");

      strcat(systext, "\n\n\n\n");

   #endif

   sprintf(systext+strlen(systext), "Video: %s, %dx%d, %d bpp\n\n%s\n\n\n\n", gfx_driver->name, SCREEN_W, SCREEN_H, bitmap_color_depth(screen), gfx_driver->desc);

   sprintf(systext+strlen(systext), "Audio: %s\n\n%s\n\n\n\n", digi_driver->name, digi_driver->desc);

   sprintf(systext+strlen(systext), "MIDI: %s\n\n%s\n\n\n\n", midi_driver->name, midi_driver->desc);

   strcat(systext, "Object plugins:\n\n\n\n");

   for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
      type = datedit_object_info[i]->type;
      sprintf(systext+strlen(systext), "  %c%c%c%c - %s\n\n", (type>>24)&0xFF, (type>>16)&0xFF, (type>>8)&0xFF, type&0xFF, datedit_object_info[i]->desc);

      s = datedit_grab_ext(type);

      if (s)
	 sprintf(systext+strlen(systext), "     Import: %s\n", s);
      else
	 strcat(systext, "     No import plugins!\n");

      s = datedit_export_ext(type);

      if (s)
	 sprintf(systext+strlen(systext), "     Export: %s\n", s);
      else
	 strcat(systext, "     No export plugins!\n");

      strcat(systext, "\n\n\n");
   }

   sys_dlg[0].x = 0;
   sys_dlg[0].y = 0;
   sys_dlg[0].w = SCREEN_W*3/4+1;
   sys_dlg[0].h = SCREEN_H*3/4+1;

   sys_dlg[1].x = 0;
   sys_dlg[1].y = 0;
   sys_dlg[1].w = sys_dlg[0].w;
   sys_dlg[1].h = sys_dlg[0].h-31;

   sys_dlg[2].x = (sys_dlg[0].w-80)/2;
   sys_dlg[2].y = sys_dlg[0].h-24;
   sys_dlg[2].w = 81;
   sys_dlg[2].h = 17;

   sys_dlg[1].d1 = 0;
   sys_dlg[1].d2 = 0;
   sys_dlg[1].dp = systext;

   centre_dialog(sys_dlg);
   set_dialog_color(sys_dlg, gui_fg_color, gui_bg_color);

   do_dialog(sys_dlg, 1);

   free(systext);

   return D_REDRAW;
}



/* handle the about command */
static int about(void)
{
   CHECK_MENU_HOOK("About", DATEDIT_MENU_HELP);

   alert("Allegro Datafile Editor, version " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR,
	 "By Shawn Hargreaves, " ALLEGRO_DATE_STR,
	 NULL, "OK", NULL, 13, 0);

   return D_O_K;
}



/* worker for creating new objects */
static int add_new(int type)
{
   DATAITEM *dat;
   DATAFILE **df;
   void *v = NULL;
   long size = 0;
   int i;

   if ((SELECTED_ITEM >= 0) && (SELECTED_ITEM < data_count))
      dat = data+SELECTED_ITEM;
   else
      return D_O_K;

   if (do_edit("New Object", "Type:", "Name:", type, NULL, (type == 0), TRUE)) {
      if (prop_type_string[0]) {
	 type = datedit_clean_typename(prop_type_string);
	 check_valid_name(prop_value_string);

	 for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
	    if ((datedit_object_info[i]->type == type) && (datedit_object_info[i]->makenew)) {
	       v = datedit_object_info[i]->makenew(&size);
	       break;
	    }
	 }

	 if (!v)
	    v = makenew_data(&size);

         if ((dat->dat) && (dat->dat->type == DAT_FILE)) {
            void *ptr = &dat->dat->dat;
            df = (DATAFILE **)ptr;
         }
	 else
	    df = dat->parent;

	 *df = datedit_insert(*df, NULL, prop_value_string, type, v, size);
	 if (opt_menu[MENU_SORT].flags & D_SELECTED)
	    datedit_sort_datafile(*df);
	 rebuild_list(v, TRUE);
	 select_property(DAT_NAME);
         
	 set_modified(TRUE);
      }
      else {
	 alert("Invalid type!", NULL, NULL, "OK", NULL, 13, 0);
	 return D_O_K;
      }
   }

   return D_REDRAW;
}



/* handle the new object command */
static int new_object(void)
{
   return add_new((int)((unsigned long)active_menu->dp));
}



/* handle the replace object command */
static int replace_object(void)
{
   return replacer((int)((unsigned long)active_menu->dp));
}



/* menu callback to activate an external shell tool */
static int sheller(void)
{
   DATAFILE *dat;
   char buf[256], cmd[256], ext[256], filename[256];
   DATEDIT_GRAB_PARAMETERS params;
   AL_CONST char *s, * s2, *origin;

   int oldw = SCREEN_W;
   int oldh = SCREEN_H;
   int export, delfile;
   int ret, c, i;
   int sel;
   int relf = FALSE;

   CHECK_MENU_HOOK("Shell Edit", DATEDIT_MENU_OBJECT);

   sel = single_selection();

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert_multiple_selections();
      else
	 alert_no_selection();
      return D_O_K;
   }

   dat = data[sel].dat;

   if (dat->type == DAT_FILE) {
      alert("Can't Shell Edit a datafile object!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   buf[0] = (dat->type >> 24) & 0xFF;
   buf[1] = (dat->type >> 16) & 0xFF;
   buf[2] = (dat->type >> 8) & 0xFF;
   buf[3] = dat->type & 0xFF;
   buf[4] = 0;

   for (i=3; (i>0) && (buf[i] == ' '); i--)
      buf[i] = 0;

   s = get_config_string("grabber", buf, "");
   if ((!s) || (!s[0])) {
      sprintf(cmd, "Add a \"%s=command\" line to the [grabber]", buf);
      alert("No shell association for this object type!", cmd, "section in allegro.cfg", "OK", NULL, 13, 0);
      return D_O_K;
   }

   origin = get_datafile_property(dat, DAT_ORIG);
   if (origin[0]) {
      if (is_relative_filename(origin)) {
	 make_absolute_filename(filename, grabber_data_file, origin, sizeof(filename));
	 relf = TRUE;
      }
      else {
	 strcpy(filename, origin);
      }

      export = !exists(filename);
      delfile = FALSE;
   }
   else {
      strcpy(ext, "tmp");

      s2 = datedit_export_ext(dat->type);

      if (s2) {
	 strcpy(ext, s2);
	 for (c=0; ext[c]; c++) {
	    if (ext[c] == ';') {
	       ext[c] = 0;
	       break;
	    }
	 }
      }

      s2 = getenv("TMPDIR");
      if (s2)
	 strcpy(buf, s2);
      else {
	 s2 = getenv("TEMP");
	 if (s2)
	    strcpy(buf, s2);
	 else {
	    s2 = getenv("TMP");
	    if (s2)
	       strcpy(buf, s2);
	    else
	     #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
	       strcpy(buf, "c:\\");
	     #else
	       strcpy(buf, "/tmp/");
	     #endif
	 }
      }
      put_backslash(buf);

      i = 0;
      do {
	 sprintf(filename, "%sgrab%04d.%s", buf, i, ext);
	 i++;
      } while (exists(filename));

      export = TRUE;
      delfile = TRUE;
   }

   fix_filename_case(filename);

   if (export) {
      if (!datedit_export(dat, filename))
	 goto ohwellitwasaniceidea;
   }

   sprintf(cmd, "%s %s", s, filename);

   allegro_exit();

   #ifdef ALLEGRO_CONSOLE_OK
      printf("%s\n", cmd);
   #endif

   #ifdef ALLEGRO_DOS
      /* fool Windows into noticing that the sound hardware is now free */
      if ((os_type == OSTYPE_WIN95) || (os_type == OSTYPE_WIN98))
	 system("command.com /c");
   #endif

   ret = system(cmd);

   #ifdef ALLEGRO_DOS
      /* bring our window back to the foreground */
      if ((os_type == OSTYPE_WIN95) || (os_type == OSTYPE_WIN98)) {
	 __dpmi_regs r;
	 r.x.ax = 0x168B;
	 r.x.bx = 0;
	 __dpmi_int(0x2F, &r);
      }
   #endif

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();

   if (set_gfx_mode(autodetect_card, oldw, oldh, 0, 0) != 0) {
      destroy_bitmap(my_mouse_pointer);
      my_mouse_pointer = NULL;
      destroy_bitmap(my_busy_pointer);
      my_busy_pointer = NULL;
      set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);
      sel_palette(datedit_current_palette);
      alert("bad, Bad, BAD error!", "Unable to restore the graphics mode...", NULL, "Shit", NULL, 13, 0);
   }
   else
      sel_palette(datedit_current_palette);

   clear_to_color(screen, gui_mg_color);

   if (no_sound) {
      install_sound(DIGI_NONE, MIDI_NONE, NULL);
   }
   else {
      if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
	 alert("bad, Bad, BAD error!", "Unable to reset the soundcard...", NULL, "Shit", NULL, 13, 0);
	 install_sound(DIGI_NONE, MIDI_NONE, NULL);
      }
   }

   if (ret != 0) {
      if (alert("Tool returned an error status!", "Do you still want to regrab", "the modified data?", "Yes", "No", 'y', 'n') != 1)
	 goto ohwellitwasaniceidea;
   }

   params.datafile = grabber_data_file;
   params.filename = filename;
   /* params.name = */
   /* params.type = */
   /* params.x = */
   /* params.y = */
   /* params.w = */
   /* params.h = */
   /* params.colordepth */
   params.relative = relf;

   datedit_grabupdate(dat, &params);
   datedit_sort_properties(dat->prop);
   select_property(DAT_NAME);

   set_modified(TRUE);

   ohwellitwasaniceidea:

   if (delfile)
      delete_file(filename);

   clear_keybuf();
   show_mouse(screen);

   return D_REDRAW;
}



/* callback for the datedit functions to display a message */
void datedit_msg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   uvszprintf(buf, 1024, fmt, args);
   va_end(args);

   show_mouse(NULL);
   acquire_screen();

   box_out(buf);
   box_eol();

   release_screen();
   show_mouse(screen);
}



/* callback for the datedit functions to start a multi-part message */
void datedit_startmsg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   uvszprintf(buf, 1024, fmt, args);
   va_end(args);

   box_out(buf);
}



/* callback for the datedit functions to end a multi-part message */
void datedit_endmsg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   uvszprintf(buf, 1024, fmt, args);
   va_end(args);

   show_mouse(NULL);
   acquire_screen();

   box_out(buf);
   box_eol();

   release_screen();
   show_mouse(screen);
}



/* callback for the datedit functions to report an error */
void datedit_error(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   uvszprintf(buf, 1024, fmt, args);
   va_end(args);

   set_mouse_sprite(my_mouse_pointer);

   alert(buf, NULL, NULL, "Oh dear", NULL, 13, 0);

   if (busy_mouse)
      set_mouse_sprite(my_busy_pointer);
}



/* callback for the datedit functions to ask a question */
int datedit_ask(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];
   int ret;

   va_start(args, fmt);
   uvszprintf(buf, 1024, fmt, args);
   va_end(args);

   strcat(buf, "?");

   set_mouse_sprite(my_mouse_pointer);

   ret = alert(buf, NULL, NULL, "Yes", "No", 'y', 'n');

   if (busy_mouse)
      set_mouse_sprite(my_busy_pointer);

   if (ret == 1)
      return 'y';
   else
      return 'n';
}



/* callback for the datedit functions to show a list of options */
/* Returns -1 if canceled */
int datedit_select(AL_CONST char *(*list_getter)(int index, int *list_size), AL_CONST char *fmt, ...)
{
   DIALOG datedit_select_dlg[] = {
      /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                 (dp2) (dp3) */
      { d_shadow_box_proc, 0,    0,    224,  113,  0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
      { d_ctext_proc,      0,    2,    220,  15,   0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  },
      { d_list_proc,       28,   24,   161,  50,   0,    0,    0,       0,          0,             0,       NULL/*list_getter*/, NULL, NULL  },
      { d_button_proc,     16,   80,   81,   17,   0,    0,    13,      D_EXIT,     0,             0,       "OK",                NULL, NULL  }, 
      { d_button_proc,     127,  80,   81,   17,   0,    0,    27,      D_EXIT,     0,             0,       "Cancel",            NULL, NULL  }, 
      { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                NULL, NULL  }
   };
   va_list args;
   char buf[1024];
   int ret, c;
 
   /* Some compilers don't like non-constant initialisers */
   ASSERT(datedit_select_dlg[2].proc == d_list_proc);
   ASSERT(datedit_select_dlg[2].dp == NULL);
   datedit_select_dlg[2].dp = (void *)list_getter;

   va_start(args, fmt);
   uvszprintf(buf, 1024, fmt, args);
   va_end(args);
   
   /* If there is only one choice, select it automatically */
   list_getter(-1, &c);
   if (c<=1) return c-1;
   
   datedit_select_dlg[1].dp = buf;
   centre_dialog(datedit_select_dlg);
   set_dialog_color(datedit_select_dlg, gui_fg_color, gui_bg_color);
   datedit_select_dlg[2].d1 = 0;
   datedit_select_dlg[2].d2 = 0;
      
   set_mouse_sprite(my_mouse_pointer);

   ret = popup_dialog(datedit_select_dlg, -1);

   if (busy_mouse)
      set_mouse_sprite(my_busy_pointer);

   if (ret == 3)
      return datedit_select_dlg[2].d1;
      
   return -1;
}



/* close button callback */
static void close_callback(void)
{
   simulate_keypress(27 + (KEY_ESC << 8));
}



int main(int argc, char *argv[])
{ 
   extern DATEDIT_OBJECT_INFO datfile_info;
   int i, j;
   int ret = -1;
   int bpp = -1;
   int w = 640;
   int h = 480;
   int custom_resolution = 0;
   int force_window = 0;
   char *s, tmp[256];
   char *fname = NULL;
   static int color_depths[] = { -1, 32, 16, 15, 8, 0 };

   if (allegro_init() != 0)
      return 1;
   
   /* If possible, use the desktop color depth as the default. */
   if (!(color_depths[0] = desktop_color_depth())) 
      color_depths[0] = -1; 

   for (i=1; i<argc; i++) {
      if (argv[i][0] == '-') {
	 if (strcmp(argv[i]+1, "nosound") == 0) {
	    no_sound = TRUE;
	 }
	 else if (strcmp(argv[i]+1, "windowed") == 0) {
	    autodetect_card = GFX_AUTODETECT_WINDOWED;
            force_window = 1;
	 }
	 else if (strcmp(argv[i]+1, "fullscreen") == 0) {
	    autodetect_card = GFX_AUTODETECT_FULLSCREEN;
            /* If no resolution has been specified yet, try to use the desktop's resolution */
            if (!custom_resolution && get_desktop_resolution(&w, &h)) {
               w = 640;
               h = 480; 
            }
	 }
	 else if ((argv[i][1] == 'p') || (argv[i][1] == 'P')) {
	    strcpy(password, argv[i]+2);
	    entered_password = TRUE;
	 }
	 else if ((strchr(argv[i], 'x')) || (strchr(argv[i], 'X'))) {
	    strcpy(tmp, argv[i]+1);
	    s = strtok(tmp, "xX");
	    if (s) {
	       w = atoi(s);
	       s = strtok(NULL, "xX");
	       if (s)
		  h = atoi(s);
	       else
		  h = 0;
	    }
	    else 
	       w = 0;
	    if ((w < 320) || (h < 200)) {
	       allegro_message("Invalid display resolution '%s'\n", argv[i]+1);
	       return 1;
	    }
            custom_resolution = 1;
	 }
	 else {
	    bpp = atoi(argv[i]+1);
	    if ((bpp != 8) && (bpp != 15) && (bpp != 16) && (bpp != 24) && (bpp != 32)) {
	       allegro_message("Invalid color depth '%s'\n", argv[i]+1);
	       return 1;
	    }
	 }
      }
      else
	 fname = argv[i];
   }

   #ifdef ALLEGRO_DJGPP
      __djgpp_set_ctrl_c(0);
      setcbrk(0);
   #endif

   install_keyboard();
   install_mouse();
   install_timer();

   set_color_conversion(COLORCONV_NONE);

   while(1)
   {
      if (bpp > 0) {
         set_color_depth(bpp);
         ret = set_gfx_mode(autodetect_card, w, h, 0, 0);
      }
      else {
         for (i=0; color_depths[i]; i++) {
            if ((bpp = color_depths[i]) > 0) {
               set_color_depth(bpp);
               ret = set_gfx_mode(autodetect_card, w, h, 0, 0);
               if (ret == 0)
                  break;
            }
         }
      }
      if (ret == 0 || force_window || autodetect_card == GFX_AUTODETECT) break;

      /* If the platform doesn't support windows and the user didn't request a window, try again. */
      autodetect_card = GFX_AUTODETECT;
   }

   if (ret != 0) {
      ret = set_gfx_mode(GFX_SAFE, w, h, 0, 0);

      if (ret != 0) {
         set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
         allegro_message("Error setting %dx%d %d bpp graphics mode\n%s\n", w, h, bpp, allegro_error);
         return 1;
      }
   }

   update_title();

   set_close_button_callback(close_callback);

   if (no_sound) {
      install_sound(DIGI_NONE, MIDI_NONE, NULL);
   }
   else {
      if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
	 alert ("Error initialising sound", allegro_error, NULL, "OK", NULL, 0, 0);
	 install_sound(DIGI_NONE, MIDI_NONE, NULL);
      }
   }

   if (SCREEN_W < 512) {
      prop_dlg[0].w = prop_dlg[0].w * 3/4;
      for (i=1; prop_dlg[i].proc; i++) {
	 if (prop_dlg[i].x > 100)
	    prop_dlg[i].x = prop_dlg[i].x * 3/4;
	 if (prop_dlg[i].w > 100)
	    prop_dlg[i].w = prop_dlg[i].w * 2/3;
	 else if (prop_dlg[i].w > 50)
	    prop_dlg[i].w = prop_dlg[i].w * 3/4;
      }
   }

   main_dlg[0].w = SCREEN_W;
   main_dlg[0].h = SCREEN_H;

   if (SCREEN_W < 640) {
      for (i=DLG_FILENAME-1; i<DLG_PASSWORD; i+=2)
	 main_dlg[i].x = main_dlg[i].x * SCREEN_W / 640;

      main_dlg[DLG_FILENAME].w = SCREEN_W / 3;
      main_dlg[DLG_HEADERNAME].w = SCREEN_W / 3;
      main_dlg[DLG_PREFIXSTRING].w = SCREEN_W / 3;
      main_dlg[DLG_PASSWORD].w = SCREEN_W / 3;

      main_dlg[DLG_XGRIDSTRING-1].dp = "X:";
      main_dlg[DLG_XGRIDSTRING-1].x -= 76;
      main_dlg[DLG_XGRIDSTRING].x -= 116;

      main_dlg[DLG_YGRIDSTRING-1].dp = "Y:";
      main_dlg[DLG_YGRIDSTRING-1].x -= 124;
      main_dlg[DLG_YGRIDSTRING].x -= 164;

      main_dlg[DLG_PACKLIST].x = main_dlg[DLG_PACKLIST].x * SCREEN_W / 640;
      main_dlg[DLG_PACKLIST].w = main_dlg[DLG_PACKLIST].w * SCREEN_W / 640;

      for (i=DLG_PROP; main_dlg[i].proc; i++) {
	 main_dlg[i].x = main_dlg[i].x * SCREEN_W / 640;
	 main_dlg[i].w = main_dlg[i].w * SCREEN_W / 640;
      }

      while (main_dlg[DLG_LIST].y + main_dlg[DLG_LIST].h >= SCREEN_H)
	 main_dlg[DLG_LIST].h -= 8;

      if (SCREEN_H < 400) {
	 main_dlg[DLG_PROP].h -= 80;
	 main_dlg[DLG_VIEW].y -= 80;
      }
   }
   else {
      while (main_dlg[DLG_LIST].y + main_dlg[DLG_LIST].h < SCREEN_H)
	 main_dlg[DLG_LIST].h += 8;

      while (main_dlg[DLG_LIST].y + main_dlg[DLG_LIST].h >= SCREEN_H)
	 main_dlg[DLG_LIST].h -= 8;

      for (i=DLG_PACKLIST; main_dlg[i].proc; i++) {
	 if (i != DLG_LIST)
	    main_dlg[i].x = main_dlg[i].x * SCREEN_W / 640;
	 main_dlg[i].w = main_dlg[i].w * SCREEN_W / 640;
      }
   }

   memcpy(datedit_current_palette, default_palette, sizeof(PALETTE));
   sel_palette(datedit_current_palette);
   clear_to_color(screen, gui_mg_color);

   if (!entered_password)
      strcpy(password, get_config_string("grabber", "password", ""));

   grabber_sel_palette = sel_palette;
   grabber_select_property = select_property;
   grabber_get_grid_size = get_grid_size;
   grabber_rebuild_list = rebuild_list;
   grabber_get_selection_info = get_selection_info;
   grabber_foreach_selection = foreach_selection;
   grabber_single_selection = get_single_selection;
   grabber_set_selection = set_selection;
   grabber_busy_mouse = set_busy_mouse;
   grabber_modified = set_modified;

   datedit_init();
   datfile_info.plot = plot_datafile;
   datfile_info.dclick = dclick_datafile;

   for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
      if (datedit_object_info[i]->makenew) {
	 MENU tmpmenu;

	 tmpmenu.text = datedit_object_info[i]->desc;
	 tmpmenu.proc = new_object;
	 tmpmenu.child = NULL;
	 tmpmenu.flags = 0;
	 tmpmenu.dp = (void *)(unsigned long)datedit_object_info[i]->type;

	 add_to_menu(new_menu, &tmpmenu, TRUE, NULL, NULL, 0);

	 tmpmenu.proc = replace_object;

	 add_to_menu(replace_menu, &tmpmenu, TRUE, NULL, NULL, 0);
      }
   }

   for (i=0; datedit_menu_info[i]; i++) {
      MENU tmpmenu;

      tmpmenu.text = datedit_menu_info[i]->menu->text;
      tmpmenu.proc = hooker;
      tmpmenu.child = datedit_menu_info[i]->menu->child;
      tmpmenu.flags = 0;
      tmpmenu.dp = NULL;

      if (datedit_menu_info[i]->flags & DATEDIT_MENU_FILE)
	 add_to_menu(file_menu, &tmpmenu, FALSE, NULL, NULL, 2); 

      if (datedit_menu_info[i]->flags & DATEDIT_MENU_OBJECT)
	 add_to_menu(objc_menu, &tmpmenu, FALSE, sheller, NULL, 0); 

      if (datedit_menu_info[i]->flags & DATEDIT_MENU_HELP)
	 add_to_menu(help_menu, &tmpmenu, FALSE, about, NULL, 0); 

      if (datedit_menu_info[i]->flags & DATEDIT_MENU_POPUP)
	 add_to_menu(popup_menu, &tmpmenu, FALSE, sheller, NULL, 0); 

      if (datedit_menu_info[i]->flags & DATEDIT_MENU_TOP)
	 add_to_menu(menu, &tmpmenu, FALSE, NULL, help_menu, 0); 

      if (datedit_menu_info[i]->key) {
	 for (j=0; main_dlg[j].proc; j++) {
	    if ((main_dlg[j].proc == custkey_proc) && (main_dlg[j].flags & D_DISABLED)) {
	       main_dlg[j].key = datedit_menu_info[i]->key;
	       main_dlg[j].dp = datedit_menu_info[i]->menu->text;
	       main_dlg[j].flags &= ~D_DISABLED;
	       break;
	    }
	 }
      }
   }

   add_menu_shortcuts(new_menu);
   add_menu_shortcuts(replace_menu);
   add_menu_shortcuts(file_menu);
   add_menu_shortcuts(objc_menu);
   add_menu_shortcuts(help_menu);
   add_menu_shortcuts(popup_menu);
   add_menu_shortcuts(menu);

   if (fname)
      load(fname,1);
   else
      load(NULL,1);

   if (!fname) {
      sprintf(xgrid_string, "%d", get_config_int("grabber", "xgrid", 16));
      sprintf(ygrid_string, "%d", get_config_int("grabber", "ygrid", 16));

      if (strpbrk(get_config_string("grabber", "backups", ""), "yY1"))
         opt_menu[MENU_BACKUP].flags |= D_SELECTED;
      else
         opt_menu[MENU_BACKUP].flags &= ~D_SELECTED;

      if (strpbrk(get_config_string("grabber", "index", ""), "yY1"))
         opt_menu[MENU_INDEX].flags |= D_SELECTED;
      else
         opt_menu[MENU_INDEX].flags &= ~D_SELECTED;

      if (strpbrk(get_config_string("grabber", "sort", ""), "yY1"))
         opt_menu[MENU_SORT].flags |= D_SELECTED;
      else
         opt_menu[MENU_SORT].flags &= ~D_SELECTED;

      if (strpbrk(get_config_string("grabber", "relative", ""), "yY1"))
         opt_menu[MENU_RELF].flags |= D_SELECTED;
      else
         opt_menu[MENU_RELF].flags &= ~D_SELECTED;

      if (strpbrk(get_config_string("grabber", "dither", ""), "yY1"))
         opt_menu[MENU_DITHER].flags |= D_SELECTED;
      else
         opt_menu[MENU_DITHER].flags &= ~D_SELECTED;

      if (strpbrk(get_config_string("grabber", "transparency", ""), "yY1"))
         opt_menu[MENU_TRANS].flags |= D_SELECTED;
      else
         opt_menu[MENU_TRANS].flags &= ~D_SELECTED;
   }

   do_dialog(main_dlg, DLG_LIST);

   if (datafile)
      unload_datafile(datafile);

   if (data)
      free(data);

   if (data_sel)
      free(data_sel);

   if (folded)
      free(folded);

   if (!entered_password)
      set_config_string("grabber", "password", password);

   set_config_string("grabber", "xgrid", xgrid_string);
   set_config_string("grabber", "ygrid", ygrid_string);

   if (opt_menu[MENU_BACKUP].flags & D_SELECTED)
      set_config_string("grabber", "backups", "y");
   else
      set_config_string("grabber", "backups", "n");

   if (opt_menu[MENU_INDEX].flags & D_SELECTED)
      set_config_string("grabber", "index", "y");
   else
      set_config_string("grabber", "index", "n");

   if (opt_menu[MENU_SORT].flags & D_SELECTED)
      set_config_string("grabber", "sort", "y");
   else
      set_config_string("grabber", "sort", "n");

   if (opt_menu[MENU_RELF].flags & D_SELECTED)
      set_config_string("grabber", "relative", "y");
   else
      set_config_string("grabber", "relative", "n");

   if (opt_menu[MENU_DITHER].flags & D_SELECTED)
      set_config_string("grabber", "dither", "y");
   else
      set_config_string("grabber", "dither", "n");

   if (opt_menu[MENU_TRANS].flags & D_SELECTED)
      set_config_string("grabber", "transparency", "y");
   else
      set_config_string("grabber", "transparency", "n");

   return 0;
}

END_OF_MAIN()
