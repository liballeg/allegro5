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
 *      Grabber plugin for changing between absolute and relative
 *      filenames.
 *
 *      By Eric Botcazou and Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>

#include "allegro.h"
#include "../datedit.h"



/* worker function for counting objects with origin */
static int do_origin_check(DATAFILE *dat, int *param, int param2)
{
   AL_CONST char *orig = get_datafile_property(dat, DAT_ORIG);

   if (orig[0])
      (*param)++;

   return D_O_K;
}



/* checks whether our change command is allowed at the moment */
static int change_query(int popup)
{
   int n, p;

   if (popup) {
      p = 0;
      grabber_foreach_selection(do_origin_check, &n, &p, 0);
      return (p > 0);
   }

   return TRUE;
}



/* worker function for changing to a relative filename */
static int do_change_relative(DATAFILE *dat, int *param, int param2)
{
   AL_CONST char *orig = get_datafile_property(dat, DAT_ORIG);
   char relative[FILENAME_LENGTH];

   if (!orig[0] || is_relative_filename(orig)) {
      (*param)++;
      return D_O_K;
   }

   make_relative_filename(relative, grabber_data_file, orig, FILENAME_LENGTH);
   datedit_set_property(dat, DAT_ORIG, relative);

   return D_REDRAW;
}



/* change the origin to a relative filename */
static int change_relative(void)
{
   char buf[80];
   int ret, n;
   int p = 0;

   grabber_busy_mouse(TRUE);

   ret = grabber_foreach_selection(do_change_relative, &n, &p, 0);

   grabber_busy_mouse(FALSE);

   if (n <= 0) {
      alert ("Nothing to change!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   if (n > p)
      grabber_modified(TRUE);

   return ret;
}



/* worker function for changing to an absolute filename */
static int do_change_absolute(DATAFILE *dat, int *param, int param2)
{
   AL_CONST char *orig = get_datafile_property(dat, DAT_ORIG);
   char absolute[FILENAME_LENGTH];

   if (!orig[0] || !is_relative_filename(orig)) {
      (*param)++;
      return D_O_K;
   }

   make_absolute_filename(absolute, grabber_data_file, orig, FILENAME_LENGTH);
   datedit_set_property(dat, DAT_ORIG, absolute);

   return D_REDRAW;
}



/* change the origin to an absolute filename */
static int change_absolute(void)
{
   char buf[80];
   int ret, n;
   int p = 0;

   grabber_busy_mouse(TRUE);

   ret = grabber_foreach_selection(do_change_absolute, &n, &p, 0);

   grabber_busy_mouse(FALSE);

   if (n <= 0) {
      alert ("Nothing to change!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   if (n > p)
      grabber_modified(TRUE);

   return ret;
}



static MENU filename_menu[] =
{
   { "To &Relative",    change_relative,  NULL,       0, NULL  },
   { "To &Absolute",    change_absolute,  NULL,       0, NULL  },
   { NULL,              NULL,             NULL,       0, NULL  }
};



/* hook ourselves into the grabber menu system */
static MENU change_filename_menu =
{
   "Change Filename",
   NULL,
   filename_menu,
   0,
   NULL
};



DATEDIT_MENU_INFO datfname_type_menu =
{
   &change_filename_menu,
   change_query,
   DATEDIT_MENU_OBJECT | DATEDIT_MENU_POPUP,
   0,
   NULL
};

