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
 *      Top level bitmap reading routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"



typedef struct BITMAP_TYPE_INFO
{
   char ext[8];
   BITMAP *(*load)(char *filename, RGB *pal);
   int (*save)(char *filename, BITMAP *bmp, RGB *pal);
} BITMAP_TYPE_INFO;


#define MAX_BITMAP_TYPES   16


static BITMAP_TYPE_INFO bitmap_types[MAX_BITMAP_TYPES] =
{
   { "bmp", load_bmp, save_bmp },
   { "lbm", load_lbm, NULL     },
   { "pcx", load_pcx, save_pcx },
   { "tga", load_tga, save_tga },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     },
   { "",    NULL,     NULL     }
};



/* register_bitmap_file_type:
 *  Informs Allegro of a new image file type, telling it how to load and
 *  save files of this format (either function may be NULL).
 */
void register_bitmap_file_type(char *ext, BITMAP *(*load)(char *filename, RGB *pal), int (*save)(char *filename, BITMAP *bmp, RGB *pal))
{
   char tmp[16], *aext;
   int i;

   aext = uconvert_toascii(ext, tmp);

   for (i=0; i<MAX_BITMAP_TYPES; i++) {
      if ((!bitmap_types[i].ext[0]) || (stricmp(bitmap_types[i].ext, aext) == 0)) {
	 strncpy(bitmap_types[i].ext, aext, sizeof(bitmap_types[i].ext)-1);
	 bitmap_types[i].ext[sizeof(bitmap_types[i].ext)-1] = 0;
	 bitmap_types[i].load = load;
	 bitmap_types[i].save = save;
	 return;
      }
   }
}



/* load_bitmap:
 *  Loads a bitmap from disk.
 */
BITMAP *load_bitmap(char *filename, RGB *pal)
{
   char tmp[16], *aext;
   int i;

   aext = uconvert_toascii(get_extension(filename), tmp);

   for (i=0; i<MAX_BITMAP_TYPES; i++) {
      if ((bitmap_types[i].ext[0]) && (stricmp(bitmap_types[i].ext, aext) == 0)) {
	 if (bitmap_types[i].load)
	    return bitmap_types[i].load(filename, pal);
	 else
	    return NULL;
      }
   }

   return NULL;
}



/* save_bitmap:
 *  Writes a bitmap to disk.
 */
int save_bitmap(char *filename, BITMAP *bmp, RGB *pal)
{
   char tmp[16], *aext;
   int i;

   aext = uconvert_toascii(get_extension(filename), tmp);

   for (i=0; i<MAX_BITMAP_TYPES; i++) {
      if ((bitmap_types[i].ext[0]) && (stricmp(bitmap_types[i].ext, aext) == 0)) {
	 if (bitmap_types[i].save)
	    return bitmap_types[i].save(filename, bmp, pal);
	 else
	    return 1;
      }
   }

   return 1;
}



/* _fixup_loaded_bitmap:
 *  Helper function for adjusting the color depth of a loaded image.
 */
BITMAP *_fixup_loaded_bitmap(BITMAP *bmp, PALETTE pal, int bpp)
{
   BITMAP *b2;

   b2 = create_bitmap_ex(bpp, bmp->w, bmp->h);
   if (!b2) {
      destroy_bitmap(bmp);
      return NULL;
   }

   if (bpp == 8) {
      RGB_MAP *old_map = rgb_map;

      generate_optimized_palette(bmp, pal, NULL);

      rgb_map = malloc(sizeof(RGB_MAP));
      if (rgb_map != NULL)
	 create_rgb_table(rgb_map, pal, NULL);

      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);

      if (rgb_map != NULL)
	 free(rgb_map);
      rgb_map = old_map;
   }
   else if (bitmap_color_depth(bmp) == 8) {
      select_palette(pal);
      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);
      unselect_palette();
   }
   else {
      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);
   }

   destroy_bitmap(bmp);

   return b2;
}

