/*
 *    Example program for the Allegro library, by Elias Pschernig.
 *
 *    This program demonstrates how to use the GUI routines. From
 *    the simple dialog controls that display a text or a bitmap to
 *    more complex multiple choice selection lists, Allegro provides
 *    a framework which can be customised to suit your needs.
 */


#include <stdio.h>

#include <allegro.h>
#include "example.h"


/* maximum number of bytes a single (Unicode) character can have */
#define MAX_BYTES_PER_CHAR 4

/* for the d_edit_proc object */
#define LEN 32
char the_string[(LEN + 1) * MAX_BYTES_PER_CHAR] = "Change Me!";

/* for the d_text_box_proc object */
char the_text[] =
   "I'm text inside a text box.\n\n"
   "I can have multiple lines.\n\n"
   "If I grow too big to fit into my box, I get a scrollbar to "
   "the right, so you can scroll me in the vertical direction. I will never "
   "let you scroll in the horizontal direction, but instead I will try to "
   "word wrap the text.";

/* for the multiple selection list */
char sel[11];

/* for the example bitmap */
DATAFILE *datafile;



/* callback function to specify the contents of the listbox */
char *listbox_getter(int index, int *list_size)
{
   static char *strings[] =
   {
      "Zero",  "One",   "Two",   "Three", "Four",  "Five", 
      "Six",   "Seven", "Eight", "Nine",  "Ten"
   };

   if (index < 0) {
      *list_size = 11;
      return NULL;
   }
   else {
      return strings[index]; 
   }
}



/* Used as a menu-callback, and by the quit button. */
int quit(void)
{
   if (alert("Really Quit?", NULL, NULL, "&Yes", "&No", 'y', 'n') == 1)
      return D_CLOSE;
   else
      return D_O_K;
}



/* A custom dialog procedure, derived from d_button_proc. It intercepts
 * the D_CLOSE return of d_button_proc, and calls the function in dp3.
 */
int my_button_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);
   if (ret == D_CLOSE && d->dp3)
      return ((int (*)(void))d->dp3)();
   return ret;
}



/* Our about box. */
int about(void)
{
   alert("* exgui *",
         "",
         "Allegro GUI Example",
         "Ok", 0, 0, 0); 
   return D_O_K;
}



/* Another menu callback. */
int menu_callback(void)
{
   char str[256];

   ustrzcpy(str, sizeof str, active_menu->text);
   alert("Selected menu item:", "", ustrtok(str, "\t"), "Ok", NULL, 0, 0);
   return D_O_K;
}



/* Menu callback which toggles the checked status. */
int check_callback(void)
{
   active_menu->flags ^= D_SELECTED; 
   if (active_menu->flags & D_SELECTED)
      active_menu->text = "Checked";
   else
      active_menu->text = "Unchecked";

   alert("Menu item has been toggled!", NULL, NULL, "Ok", NULL, 0, 0);
   return D_O_K;
}



/* the submenu */
MENU submenu[] =
{
   { "Submenu",            NULL,   NULL, D_DISABLED,       NULL  },
   { "",                   NULL,   NULL, 0,                NULL  },
   { "Checked",  check_callback,   NULL, D_SELECTED,       NULL  },
   { "Disabled",           NULL,   NULL, D_DISABLED,       NULL  },
   { NULL,                 NULL,   NULL, 0,                NULL  }
};


/* the first menu in the menubar */
MENU menu1[] =
{
   { "Test &1 \t1", menu_callback,  NULL,      0,  NULL  },
   { "Test &2 \t2", menu_callback,  NULL,      0,  NULL  },
   { "&Quit \tq/Esc",        quit,  NULL,      0,  NULL  },
   { NULL,                   NULL,  NULL,      0,  NULL  }
};



/* the second menu in the menubar */
MENU menu2[] =
{
   { "&Test",    menu_callback,     NULL,   0,  NULL  },
   { "&Submenu",          NULL,  submenu,   0,  NULL  },
   { NULL,                NULL,     NULL,   0,  NULL  }
};



/* the help menu */
MENU helpmenu[] =
{
   { "&About \tF1",     about,  NULL,      0,  NULL  },
   { NULL,               NULL,  NULL,      0,  NULL  }
};



/* the main menu-bar */
MENU the_menu[] =
{
   { "&First",  NULL,   menu1,          0,      NULL  },
   { "&Second", NULL,   menu2,          0,      NULL  },
   { "&Help",   NULL,   helpmenu,       0,      NULL  },
   { NULL,      NULL,   NULL,           0,      NULL  }
};



extern int info1(void);
extern int info2(void);
extern int info3(void);

#define LIST_OBJECT     26
#define TEXTLIST_OBJECT 27
#define SLIDER_OBJECT   29
#define BITMAP_OBJECT   32
#define ICON_OBJECT     33

/* here it comes - the big bad ugly DIALOG array for our main dialog */
DIALOG the_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */
   
   /* this element just clears the screen, therefore it should come before the others */
   { d_clear_proc,        0,   0,    0,    0,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   
   /* these just display text, either left aligned, centered, or right aligned */
   { d_text_proc,         0,  20,    0,    0,   0,  0,    0,      0,       0,   0,    "d_text_proc",          NULL, NULL  },
   { d_ctext_proc,      318,  20,    0,    0,   0,  0,    0,      0,       0,   0,    "d_ctext_proc",         NULL, NULL  },
   { d_rtext_proc,      636,  20,    0,    0,   0,  0,    0,      0,       0,   0,    "d_rtext_proc",         NULL, NULL  },
   
   /* lots of descriptive text elements */
   { d_text_proc,         0,   0,    0,    0,   0,  0,    0,      0,       0,   0,    "d_menu_proc->",        NULL, NULL  },
   { d_text_proc,         0,  40,    0,    0,   0,  0,    0,      0,       0,   0,    "d_button_proc->",      NULL, NULL  },
   { d_text_proc,         0,  70,    0,    0,   0,  0,    0,      0,       0,   0,    "d_check_proc->",       NULL, NULL  },
   { d_text_proc,         0, 100,    0,    0,   0,  0,    0,      0,       0,   0,    "d_radio_proc->",       NULL, NULL  },
   { d_text_proc,         0, 130,    0,    0,   0,  0,    0,      0,       0,   0,    "d_edit_proc->",        NULL, NULL  },
   { d_text_proc,         0, 150,    0,    0,   0,  0,    0,      0,       0,   0,    "d_list_proc->",        NULL, NULL  },
   { d_text_proc,         0, 200,    0,    0,   0,  0,    0,      0,       0,   0,    "d_text_list_proc->",   NULL, NULL  },
   { d_text_proc,         0, 250,    0,    0,   0,  0,    0,      0,       0,   0,    "d_textbox_proc->",     NULL, NULL  },
   { d_text_proc,         0, 300,    0,    0,   0,  0,    0,      0,       0,   0,    "d_slider_proc->",      NULL, NULL  },
   { d_text_proc,         0, 330,    0,    0,   0,  0,    0,      0,       0,   0,    "d_box_proc->",         NULL, NULL  },
   { d_text_proc,         0, 360,    0,    0,   0,  0,    0,      0,       0,   0,    "d_shadow_box_proc->",  NULL, NULL  },
   { d_text_proc,         0, 390,    0,    0,   0,  0,    0,      0,       0,   0,    "d_keyboard_proc. Press F1 to see me trigger the about box.", NULL, NULL  },
   { d_text_proc,         0, 410,    0,    0,   0,  0,    0,      0,       0,   0,    "d_clear_proc. I draw the white background.", NULL, NULL  },
   { d_text_proc,         0, 430,    0,    0,   0,  0,    0,      0,       0,   0,    "d_yield_proc. I make us play nice with the OS scheduler.", NULL, NULL  },
   { d_rtext_proc,      636,  40,    0,    0,   0,  0,    0,      0,       0,   0,    "<-d_bitmap_proc",      NULL, NULL  },
   { d_rtext_proc,      636,  80,    0,    0,   0,  0,    0,      0,       0,   0,    "<-d_icon_proc",        NULL, NULL  },
   
   /* a menu bar - note how it auto-calculates its dimension if they are not given */
   { d_menu_proc,       160,   0,    0,    0,   0,  0,    0,      0,       0,   0,    the_menu,               NULL, NULL  },
   
   /* some more GUI elements, all of which require you to specify their dimensions */
   { d_button_proc,     160,  40,  160,   20,   0,  0,  't',      0,       0,   0,    "&Toggle Me!",          NULL, NULL  },
   { d_check_proc,      160,  70,  160,   20,   0,  0,  'c',      0,       0,   0,    "&Check Me!",           NULL, NULL  },
   { d_radio_proc,      160, 100,  160,   19,   0,  0,  's',      0,       0,   0,    "&Select Me!",          NULL, NULL  },
   { d_radio_proc,      320, 100,  160,   19,   0,  0,  'o',      0,       0,   0,    "&Or Me!",              NULL, NULL  },
   { d_edit_proc,       160, 130,  160,    8,   0,  0,    0,      0,     LEN,   0,    the_string,             NULL, NULL  },
   { d_list_proc,       160, 150,  160,   44,   0,  0,    0,      0,       0,   0,    (void *)listbox_getter, sel,  NULL  },
   { d_text_list_proc,  160, 200,  160,   44,   0,  0,    0,      0,       0,   0,    (void *)listbox_getter, NULL, NULL  },
   { d_textbox_proc,    160, 250,  160,   48,   0,  0,    0,      0,       0,   0,    (void *)the_text,       NULL, NULL  },
   { d_slider_proc,     160, 300,  160,   12,   0,  0,    0,      0,     100,   0,    NULL,                   NULL, NULL  },
   { d_box_proc,        160, 330,  160,   20,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   { d_shadow_box_proc, 160, 360,  160,   20,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   
   /* note how we don't fill in the dp field yet, because we first need to load the bitmap */
   { d_bitmap_proc,     480,  40,   30,   30,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   { d_icon_proc,       480,  80,   30,   30,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   
   /* the quit and info buttons use our customized dialog procedure, using dp3 as callback */
   { my_button_proc,      0, 450,  160,   20,   0,  0,  'q', D_EXIT,       0,   0,    "&Quit",                NULL, (void *)quit  },
   { my_button_proc,    400, 150,  160,   20,   0,  0,  'i', D_EXIT,       0,   0,    "&Info",                NULL, (void *)info1 },
   { my_button_proc,    400, 200,  160,   20,   0,  0,  'n', D_EXIT,       0,   0,    "I&nfo",                NULL, (void *)info2 },
   { my_button_proc,    400, 300,  160,   20,   0,  0,  'f', D_EXIT,       0,   0,    "In&fo",                NULL, (void *)info3 },

   /* the next two elements don't draw anything */
   { d_keyboard_proc,     0,   0,    0,    0,   0,  0,    0,      0,  KEY_F1,   0,    (void *)about,          NULL, NULL  },
   { d_yield_proc,        0,   0,    0,    0,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   { NULL,                0,   0,    0,    0,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  }
};



/* These three functions demonstrate how to query dialog elements.
 */
int info1(void)
{
   char buf1[256];
   char buf2[256] = "";
   int i, s = 0, n;

   listbox_getter(-1, &n);
   /* query the list proc */
   for (i = 0; i < n; i++) {
      if (sel[i]) {
         uszprintf(buf1, sizeof buf1, "%i ", i);
         ustrzcat(buf2, sizeof buf2, buf1);
         s = 1;
      }
   }
   if (s)
      ustrzcat(buf2, sizeof buf2, "are in the multiple selection!");
   else
      ustrzcat(buf2, sizeof buf2, "There is no multiple selection!");
   uszprintf(buf1, sizeof buf1, "Item number %i is selected!",
      the_dialog[LIST_OBJECT].d1);
   alert("Info about the list:", buf1, buf2, "Ok", NULL, 0, 0);
   return D_O_K;
}

int info2(void)
{
   char buf[256];

   /* query the textlist proc */
   uszprintf(buf, sizeof buf, "Item number %i is selected!",
      the_dialog[TEXTLIST_OBJECT].d1);
   alert("Info about the text list:", NULL, buf, "Ok", NULL, 0, 0);
   return D_O_K;
}

int info3(void)
{
   char buf[256];

   /* query the slider proc */
   uszprintf(buf, sizeof buf, "Slider position is %i!",
     the_dialog[SLIDER_OBJECT].d2);
   alert("Info about the slider:", NULL, buf, "Ok", NULL, 0, 0);
   return D_O_K;
}



int main(int argc, char *argv[])
{
   char buf[256];
   int i;

   /* initialise everything */
   if (allegro_init() != 0)
      return 1;
   install_keyboard(); 
   install_mouse();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   /* load the datafile */
   replace_filename(buf, argv[0], "example.dat", sizeof(buf));
   datafile = load_datafile(buf);
   if (!datafile) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s!\n", buf);
      return 1;
   }

   set_palette(datafile[THE_PALETTE].dat);

   /* set up colors */
   gui_fg_color = makecol(0, 0, 0);
   gui_mg_color = makecol(128, 128, 128);
   gui_bg_color = makecol(200, 240, 200);
   set_dialog_color(the_dialog, gui_fg_color, gui_bg_color);

   /* white color for d_clear_proc and the d_?text_procs */
   the_dialog[0].bg = makecol(255, 255, 255);
   for (i = 4; the_dialog[i].proc; i++) {
      if (the_dialog[i].proc == d_text_proc ||
          the_dialog[i].proc == d_ctext_proc ||
          the_dialog[i].proc == d_rtext_proc)
      {
         the_dialog[i].bg = the_dialog[0].bg;
      }
   }
   
   /* fill in bitmap pointers */
   the_dialog[BITMAP_OBJECT].dp = datafile[SILLY_BITMAP].dat;
   the_dialog[ICON_OBJECT].dp = datafile[SILLY_BITMAP].dat;
   
   /* shift the dialog 2 pixels away from the border */
   position_dialog(the_dialog, 2, 2);
   
   /* do the dialog */
   do_dialog(the_dialog, -1);

   unload_datafile(datafile);
   
   return 0;
}
END_OF_MAIN()
