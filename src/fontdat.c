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
 *      Font loading routine for reading a font from a datafile.
 *
 *      By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"

/* load_dat_font:
 *  Loads a font from the datafile named filename. The palette is set to the
 *  last palette encountered before the font. param points to an integer that
 *  specifies what font to load, if there is more than one in the datafile.
 *  Either of these may be NULL, in which case the palette is ignored and a
 *  value of 0 is used for the font index
 */
FONT *load_dat_font(AL_CONST char *filename, RGB *pal, void *param)
{
   FONT *fnt;
   DATAFILE *df;
   RGB *p = NULL;
   int font_index, c, n;
   ASSERT(filename);
   
   fnt = NULL;
   font_index = param?*(int *)param:0;
   df = load_datafile(filename);
   if (!df) {
      return NULL;
   }
   n = 0;
   for (c=0; df[c].type!=DAT_END; c++) {
      if (df[c].type==DAT_PALETTE)
         p = df[c].dat;
      if (df[c].type==DAT_FONT) {
         if (font_index==n) {
            fnt = df[c].dat;
            df[c].dat = NULL;
            break;
         }
         else {
            n++;
         }
      }
   }
   if (p && pal && fnt) {
      memcpy(pal, p, sizeof(PALETTE));
   }
   
   unload_datafile(df);
   return fnt;
}
