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
 *      Top level font reading routines.
 *
 *      By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"

#include "allegro5/a5_font.h"


typedef struct FONT_TYPE_INFO
{
   char *ext;
   A5FONT_FONT *(*load)(AL_CONST char *filename, void *param);
   struct FONT_TYPE_INFO *next;
} FONT_TYPE_INFO;

static FONT_TYPE_INFO *font_type_list = NULL;



/* register_font_file_type:
 *  Informs Allegro of a new font file type, telling it how to load files of 
 *  this format.
 */
void a5font_register_font_file_type(AL_CONST char *ext, A5FONT_FONT *(*load)(AL_CONST char *filename, void *param))
{
   char tmp[32], *aext;
   FONT_TYPE_INFO *iter = font_type_list;

   aext = uconvert_toascii(ext, tmp);
   if (strlen(aext) == 0) return;

   if (!iter) 
      iter = font_type_list = _AL_MALLOC(sizeof(struct FONT_TYPE_INFO));
   else {
      for (iter = font_type_list; iter->next; iter = iter->next);
      iter = iter->next = _AL_MALLOC(sizeof(struct FONT_TYPE_INFO));
   }

   if (iter) {
      iter->load = load;
      iter->ext = strdup(aext);
      iter->next = NULL;
   }
}



/* load_font:
 *  Loads a font from disk. Will try to load a font from a bitmap if all else
 *  fails.
 */
A5FONT_FONT *a5font_load_font(AL_CONST char *filename, void *param)
{
   char tmp[32], *aext;
   FONT_TYPE_INFO *iter;
   ASSERT(filename);

   aext = uconvert_toascii(get_extension(filename), tmp);
   
   for (iter = font_type_list; iter; iter = iter->next) {
      if (stricmp(iter->ext, aext) == 0) {
	 if (iter->load)
	    return iter->load(filename, param);
	 return NULL;
      }
   }
   
   /* Try to load the file as a bitmap image and grab the font from there */
   return a5font_load_bitmap_font(filename, param);
}



/* register_font_file_type_exit:
 *  Free list of registered bitmap file types.
 */
static void a5font_register_font_file_type_exit(void)
{
   FONT_TYPE_INFO *iter = font_type_list, *next;

   while (iter) {
      next = iter->next;
      _AL_FREE(iter->ext);
      _AL_FREE(iter);
      iter = next;
   }
   
   font_type_list = NULL;

   /* If we are using a destructor, then we only want to prune the list
    * down to valid modules. So we clean up as usual, but then reinstall
    * the internal modules.
    */
   #ifdef ALLEGRO_USE_CONSTRUCTOR
      _a5font_register_font_file_type_init();
   #endif

   _remove_exit_func(a5font_register_font_file_type_exit);
}



/* _register_font_file_type_init:
 *  Register built-in font file types.
 */
void _a5font_register_font_file_type_init(void)
{
   _add_exit_func(a5font_register_font_file_type_exit,
		  "a5font_register_font_file_type_exit");
}



#ifdef ALLEGRO_USE_CONSTRUCTOR
   CONSTRUCTOR_FUNCTION(static void font_filetype_constructor(void));
   DESTRUCTOR_FUNCTION(static void font_filetype_destructor(void));

   /* font_filetype_constructor:
    *  Register font filetype functions if this object file is linked
    *  in. This isn't called if the load_font() function isn't used 
    *  in a program, thus saving a little space in statically linked 
    *  programs.
    */
   void font_filetype_constructor(void)
   {
      _a5font_register_font_file_type_init();
   }

   /* font_filetype_destructor:
    *  Since we only want to destroy the whole list when we *actually*
    *  quit, not just when allegro_exit() is called, we need to use a
    *  destructor to accomplish this.
    */
   static void font_filetype_destructor(void)
   {
      FONT_TYPE_INFO *iter = font_type_list, *next;

      while (iter) {
         next = iter->next;
         _AL_FREE(iter->ext);
         _AL_FREE(iter);
         iter = next;
      }
   
      font_type_list = NULL;

      _remove_exit_func(a5font_register_font_file_type_exit);
   }
#endif
