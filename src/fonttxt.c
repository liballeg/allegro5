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
 *
 *      TXT font file reader.
 *
 *      See readme.txt for copyright information.
 */
#include <stdio.h>
#include <string.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"




/* load_txt_font:
 *  Loads a scripted font.
 */
FONT *load_txt_font(AL_CONST char *filename, RGB *pal, void *param)
{
   char buf[1024], *font_str, *start_str = 0, *end_str = 0;
   FONT *f, *f2, *f3, *f4;
   PACKFILE *pack;
   int begin, end;

   pack = pack_fopen(filename, F_READ);
   if (!pack) 
      return NULL;

   f = f2 = f3 = f4 = NULL;

   while(pack_fgets(buf, sizeof(buf)-1, pack)) {
      font_str = strtok(buf, " \t");
      if (font_str) 
         start_str = strtok(0, " \t");
      if (start_str) 
         end_str = strtok(0, " \t");

      if(!font_str || !start_str || !end_str) {
         destroy_font(f);
         pack_fclose(pack);
         return NULL;
      }

      if(font_str[0] == '-')
         font_str[0] = '\0';
         
      begin = strtol(start_str, 0, 0);
        
      if (end_str)
         end = strtol(end_str, 0, 0) + 1;
      else 
         end = -1;

      if(begin <= 0 || (end > 0 && end < begin)) {
         _al_free(f);
         pack_fclose(pack);

         return NULL;
      }

      /* Load the font that needs to be merged with the current font */
      if (font_str[0]) {
         if (f4)
            destroy_font(f4);
         f4 = load_font(font_str, pal, param);
      }
      if(!f4) {
         destroy_font(f);
         pack_fclose(pack);

         return NULL;
      }
      
      /* Extract font range */
      f2 = extract_font_range(f4, begin, end);
      if (f) {
         f3 = f;
         f = merge_fonts(f2, f3);
         destroy_font(f3);
         destroy_font(f2);
      } else {
         f = f2;
         f2 = NULL;
      }
    }
    destroy_font(f4);

    pack_fclose(pack);
    return f;
}
