/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use the GUI routines.
 */


#include <stdio.h>

#include "allegro.h"
#include "example.h"



/* we need to load example.dat to access the big font */
DATAFILE *datafile;


/* for the d_edit_proc() object */
char the_string[32] = "Change Me!";


/* since we change the font, we need to store a copy of the original one */
FONT *original_font;



/* A custom dialog procedure for the 'change font' button. This uses a
 * simple form of inheritance: it calls d_button_proc() to do most of
 * the work, so it behaves exactly like any other button, but when the
 * button is clicked and d_button_proc() returns D_CLOSE, it intercepts
 * the message and changes the font instead.
 */
int change_font_proc(int msg, DIALOG *d, int c)
{
   int ret;

   /* call the parent object */
   ret = d_button_proc(msg, d, c);

   /* trap the close return value and change the font */
   if (ret == D_CLOSE) {
      if (font == original_font)
	 font = datafile[BIG_FONT].dat;
      else
	 font = original_font;

      return D_REDRAW; 
   }

   /* otherwise just return */
   return ret;
}



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
   else
      return strings[index]; 
}



DIALOG the_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)                    (d2)  (dp)              (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    255,  0,    0,    0,       0,                      0,    NULL,             NULL, NULL  },
   { d_edit_proc,       80,   32,   512,  48,   255,  0,    0,    0,       sizeof(the_string)-1,   0,    the_string,       NULL, NULL  },
   { d_button_proc,     80,   132,  161,  49,   255,  0,    't',  0,       0,                      0,    "&Toggle Me",     NULL, NULL  },
   { d_list_proc,       360,  100,  207,  207,  255,  0,    0,    0,       0,                      0,    listbox_getter,   NULL, NULL  },
   { change_font_proc,  80,   232,  161,  49,   255,  0,    'f',  D_EXIT,  0,                      0,    "Change &Font",   NULL, NULL  },
   { d_button_proc,     80,   400,  161,  49,   255,  0,    0,    D_EXIT,  0,                      0,    "OK",             NULL, NULL  },
   { d_button_proc,     360,  400,  161,  49,   255,  0,    0,    D_EXIT,  0,                      0,    "Cancel",         NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,       0,                      0,    NULL,             NULL, NULL  }
};



/* index of the listbox object in the dialog array */
#define LISTBOX_OBJECT     3



int main(int argc, char *argv[])
{
   char buf1[256], buf2[80], buf3[80];
   int ret;

   /* initialise everything */
   allegro_init();
   install_keyboard(); 
   install_mouse();
   install_timer();
   if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);

   /* We set up colors to match screen color depth (in case it changed) */
   for (ret = 0; the_dialog[ret].proc; ret++) {
      the_dialog[ret].fg = makecol(0, 0, 0);
      the_dialog[ret].bg = makecol(255, 255, 255);
   }

   /* load the datafile */
   replace_filename(buf1, argv[0], "example.dat", sizeof(buf1));
   datafile = load_datafile(buf1);
   if (!datafile) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s!\n", buf1);
      return 1;
   }

   /* store a copy of the default font */
   original_font = font;

   /* do the dialog */
   ret = do_dialog(the_dialog, -1);

   /* and report the results */
   sprintf(buf1, "do_dialog() returned %d", ret);
   sprintf(buf2, "string is '%s'", the_string);
   sprintf(buf3, "listbox selection is %d", the_dialog[LISTBOX_OBJECT].d1);
   alert(buf1, buf2, buf3, "OK", NULL, 0, 0);

   unload_datafile(datafile);
   return 0;
}

END_OF_MAIN();
