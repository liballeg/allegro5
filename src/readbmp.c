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
#include "allegro/aintern.h"



typedef struct BITMAP_TYPE_INFO
{
   char *ext;
   BITMAP *(*load)(AL_CONST char *filename, RGB *pal);
   int (*save)(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal);
} BITMAP_TYPE_INFO;


struct bitmap_loader_type
{
   BITMAP_TYPE_INFO type;
   struct bitmap_loader_type *next;
};

static struct bitmap_loader_type *first_loader_type = NULL;



/* register_bitmap_file_type:
 *  Informs Allegro of a new image file type, telling it how to load and
 *  save files of this format (either function may be NULL).
 */
void register_bitmap_file_type(AL_CONST char *ext, BITMAP *(*load)(AL_CONST char *filename, RGB *pal), int (*save)(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal))
{
   char tmp[32], *aext;
   struct bitmap_loader_type *iter = first_loader_type;

   aext = uconvert_toascii(ext, tmp);
   if (strlen(aext) == 0) return;

   if (!iter) {
      first_loader_type = malloc(sizeof(struct bitmap_loader_type));
      if (!first_loader_type) return;
      iter = first_loader_type;
   } 
   else {
      while (iter) {
	 if (stricmp(iter->type.ext, aext) == 0) {
	    iter->type.load = load;
	    iter->type.save = save;
	    return;
	 }
	 iter = iter->next;
      }

      iter = first_loader_type;
      while (iter->next) iter = iter->next;

      iter->next = malloc(sizeof(struct bitmap_loader_type));
      if (!iter->next) return;
      iter = iter->next;
   }

   iter->type.load = load;
   iter->type.save = save;
   iter->type.ext = strdup(aext);
   iter->next = NULL;
}



/* load_bitmap:
 *  Loads a bitmap from disk.
 */
BITMAP *load_bitmap(AL_CONST char *filename, RGB *pal)
{
   char tmp[16], *aext;
   struct bitmap_loader_type* iter = first_loader_type;

   aext = uconvert_toascii(get_extension(filename), tmp);
   
   while (iter) {
      if (stricmp(iter->type.ext, aext) == 0) {
	 if (iter->type.load) 
	    return iter->type.load(filename, pal);
	 return NULL;
      }
      iter = iter->next;
   }

   return NULL;
}



/* save_bitmap:
 *  Writes a bitmap to disk.
 */
int save_bitmap(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal)
{
   char tmp[16], *aext;
   struct bitmap_loader_type *iter = first_loader_type;

   aext = uconvert_toascii(get_extension(filename), tmp);

   while (iter) {
      if (stricmp(iter->type.ext, aext) == 0) {
	 if (iter->type.save) 
	    return iter->type.save(filename, bmp, pal);
	 return 1;
      }
      iter = iter->next;
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



/* register_bitmap_file_type_exit:
 *  Free list of registered bitmap file types.
 */
static void register_bitmap_file_type_exit(void)
{
   struct bitmap_loader_type *iter = first_loader_type, *iter2 = NULL;

   while (iter) {
      iter2 = iter->next;
      free(iter->type.ext);
      free(iter);
      iter = iter2;
   }
   
   first_loader_type = NULL;

   _remove_exit_func(register_bitmap_file_type_exit);
}



/* register_bitmap_file_type_init:
 *  Register built-in bitmap file types.
 */
void register_bitmap_file_type_init(void)
{
   char buf[16];

   _add_exit_func(register_bitmap_file_type_exit);

   register_bitmap_file_type(uconvert_ascii("bmp", buf), load_bmp, save_bmp);
   register_bitmap_file_type(uconvert_ascii("lbm", buf), load_lbm, NULL);
   register_bitmap_file_type(uconvert_ascii("pcx", buf), load_pcx, save_pcx);
   register_bitmap_file_type(uconvert_ascii("tga", buf), load_tga, save_tga);
}
