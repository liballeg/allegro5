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
 *      Display switching interface.
 *
 *      By George Foot.
 *
 *      State saving routines added by Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



#ifdef ALLEGRO_DOS
   static int switch_mode = SWITCH_NONE;
#elif defined ALLEGRO_WINDOWS
   static int switch_mode = SWITCH_AMNESIA;
#else
   static int switch_mode = SWITCH_PAUSE;
#endif



/* remember things about the way our bitmaps are set up */
typedef struct BITMAP_INFORMATION
{
   BITMAP *bmp;                           /* the bitmap */
   BITMAP *other;                         /* replacement during switches */
   struct BITMAP_INFORMATION *sibling;    /* linked list of siblings */
   struct BITMAP_INFORMATION *child;      /* tree of sub-bitmaps */
   void *acquire, *release;               /* need to bodge the vtable, too */
} BITMAP_INFORMATION;



static BITMAP_INFORMATION *info_list = NULL;

int _dispsw_status = SWITCH_NONE;



/* set_display_switch_mode:
 *  Returns zero on success, nonzero on error; unregisters
 *  all callbacks.
 */
int set_display_switch_mode(int mode)
{
   int retval;

   if ((!system_driver))
      return -1;

   if (!system_driver->set_display_switch_mode) {
      if (mode == SWITCH_NONE)
         return 0;
      else
         return -1;
   }

   retval = system_driver->set_display_switch_mode(mode);

   if (!retval)
      switch_mode = mode;

   return retval;
}



/* get_display_switch_mode:
 *  Returns the current display switch mode.
 */
int get_display_switch_mode(void)
{
   return switch_mode;
}



/* set_display_switch_callback:
 *  The first parameter indicates the direction (SWITCH_IN or 
 *  SWITCH_OUT).  Set `cb' to NULL to turn off the feature. 
 *  Returns zero on success, non-zero on failure (e.g. feature 
 *  not supported).
 */
int set_display_switch_callback(int dir, void (*cb)(void))
{
   if ((dir != SWITCH_IN) && (dir != SWITCH_OUT))
      return -1;

   if ((!system_driver) || (!system_driver->set_display_switch_callback))
      return -1;

   return system_driver->set_display_switch_callback(dir, cb);
}



/* remove_display_switch_callback:
 *  Pretty much the opposite of the above.
 */
void remove_display_switch_callback(void (*cb)(void))
{
   if ((system_driver) && (system_driver->remove_display_switch_callback))
      system_driver->remove_display_switch_callback(cb);
}



/* find_switch_bitmap:
 *  Recursively searches the tree for a particular bitmap.
 */
static BITMAP_INFORMATION *find_switch_bitmap(BITMAP_INFORMATION **head, BITMAP *bmp, BITMAP_INFORMATION ***head_ret)
{
   BITMAP_INFORMATION *info = *head, *kid;

   while (info) {
      if (info->bmp == bmp) {
	 *head_ret = head;
	 return info;
      }

      if (info->child) {
	 kid = find_switch_bitmap(&info->child, bmp, head_ret);
	 if (kid)
	    return kid;
      }

      head = &info->sibling;
      info = *head;
   }

   return NULL;
}



/* _register_switch_bitmap:
 *  Lists this bitmap as an interesting thing to remember during console
 *  switches.
 */
void _register_switch_bitmap(BITMAP *bmp, BITMAP *parent)
{
   BITMAP_INFORMATION *info, *parent_info, **head;

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(TRUE, FALSE);

   if (parent) {
      /* add a sub-bitmap */
      parent_info = find_switch_bitmap(&info_list, parent, &head);
      if (!parent_info)
	 goto getout;

      info = malloc(sizeof(BITMAP_INFORMATION));
      if (!info)
	 goto getout;

      info->bmp = bmp;
      info->other = NULL;
      info->sibling = parent_info->child;
      info->child = NULL;
      info->acquire = NULL;
      info->release = NULL;

      parent_info->child = info;
   }
   else {
      /* add a new top-level bitmap: must be in the foreground for this! */
      ASSERT(_dispsw_status == SWITCH_NONE);

      info = malloc(sizeof(BITMAP_INFORMATION));
      if (!info)
	 goto getout;

      info->bmp = bmp;
      info->other = NULL;
      info->sibling = info_list;
      info->child = NULL;
      info->acquire = NULL;
      info->release = NULL;

      info_list = info;
   }

   getout:

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(FALSE, FALSE);
}



/* _unregister_switch_bitmap:
 *  Removes a bitmap from the list of things that need to be saved.
 */
void _unregister_switch_bitmap(BITMAP *bmp)
{
   BITMAP_INFORMATION *info, **head;

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(TRUE, FALSE);

   info = find_switch_bitmap(&info_list, bmp, &head);
   if (!info)
      goto getout;

   /* all the sub-bitmaps should be destroyed before we get to here */
   ASSERT(!info->child);

   /* it's not cool to destroy things that have important state */
   ASSERT(!info->other);

   *head = info->sibling;
   free(info);

   getout:

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(FALSE, FALSE);
}



/* reconstruct_kids:
 *  Recursive helper to rebuild any sub-bitmaps to point at their new
 *  parents.
 */
static void reconstruct_kids(BITMAP *parent, BITMAP_INFORMATION *info)
{
   int x, y, i;

   while (info) {
      info->bmp->vtable = parent->vtable;
      info->bmp->write_bank = parent->write_bank;
      info->bmp->read_bank = parent->read_bank;
      info->bmp->seg = parent->seg;
      info->bmp->id = parent->id | BMP_ID_SUB;

      x = info->bmp->x_ofs - parent->x_ofs;
      y = info->bmp->y_ofs - parent->y_ofs;

      if (is_planar_bitmap(info->bmp))
	 x /= 4;

      x *= BYTES_PER_PIXEL(bitmap_color_depth(info->bmp));

      for (i=0; i<info->bmp->h; i++)
	 info->bmp->line[i] = parent->line[y+i] + x;

      reconstruct_kids(info->bmp, info->child);
      info = info->sibling;
   }
}



/* fudge_bitmap:
 *  Makes b2 be similar to b1 (duplicate clip settings, ID, etc).
 */
static void fudge_bitmap(BITMAP *b1, BITMAP *b2)
{
   set_clip(b2, 0, 0, 0, 0);

   blit(b1, b2, 0, 0, 0, 0, b1->w, b1->h);

   b2->clip = b1->clip;

   b2->cl = b1->cl;
   b2->cr = b1->cr;
   b2->ct = b1->ct;
   b2->cb = b1->cb;
}



/* swap_bitmap_contents:
 *  Now remember boys and girls, don't try this at home!
 */
static void swap_bitmap_contents(BITMAP *b1, BITMAP *b2)
{
   int size = sizeof(BITMAP) + sizeof(char *) * b1->h;
   unsigned char *s = (unsigned char *)b1;
   unsigned char *d = (unsigned char *)b2;
   unsigned char t;
   int c;

   for (c=0; c<size; c++) {
      t = s[c];
      s[c] = d[c];
      d[c] = t;
   }
}



/* save_bitmap_state:
 *  Remember everything important about a screen bitmap.
 */
static void save_bitmap_state(BITMAP_INFORMATION *info, int switch_mode)
{
   if ((switch_mode == SWITCH_AMNESIA) || (switch_mode == SWITCH_BACKAMNESIA))
      return;

   info->other = create_bitmap_ex(bitmap_color_depth(info->bmp), info->bmp->w, info->bmp->h);
   if (!info->other)
      return;

   fudge_bitmap(info->bmp, info->other);

   info->acquire = info->other->vtable->acquire;
   info->release = info->other->vtable->release;

   info->other->vtable->acquire = info->bmp->vtable->acquire;
   info->other->vtable->release = info->bmp->vtable->release;

   #define INTERESTING_ID_BITS   (BMP_ID_VIDEO | BMP_ID_SYSTEM | \
				  BMP_ID_SUB | BMP_ID_MASK)

   info->other->id = (info->bmp->id & INTERESTING_ID_BITS) | 
		     (info->other->id & ~INTERESTING_ID_BITS);

   swap_bitmap_contents(info->bmp, info->other);
}



/* _save_switch_state:
 *  Saves the graphics state before a console switch.
 */
void _save_switch_state(int switch_mode)
{
   BITMAP_INFORMATION *info = info_list;
   int hadmouse;

   if (!screen)
      return;

   if (_al_linker_mouse && 
       is_same_bitmap(*(_al_linker_mouse->mouse_screen_ptr), screen)) {
      _al_linker_mouse->show_mouse(NULL);
      hadmouse = TRUE;
   }
   else
      hadmouse = FALSE;

   while (info) {
      save_bitmap_state(info, switch_mode);
      reconstruct_kids(info->bmp, info->child);
      info = info->sibling;
   }

   _dispsw_status = switch_mode;

   if (hadmouse)
      _al_linker_mouse->show_mouse(screen);
}



/* restore_bitmap_state:
 *  The King's Men are quite good at doing this with bitmaps...
 */
static void restore_bitmap_state(BITMAP_INFORMATION *info)
{
   if (info->other) {
      swap_bitmap_contents(info->other, info->bmp);
      info->other->vtable->acquire = info->acquire;
      info->other->vtable->release = info->release;
      info->other->id &= ~INTERESTING_ID_BITS;
      fudge_bitmap(info->other, info->bmp);
      destroy_bitmap(info->other);
      info->other = NULL;
   }
   else
      clear_bitmap(info->bmp);
}



/* _restore_switch_state:
 *  Restores the graphics state after a console switch.
 */
void _restore_switch_state()
{
   BITMAP_INFORMATION *info = info_list;
   int hadmouse, hadtimer;

   if (!screen)
      return;

   if (_al_linker_mouse &&
       is_same_bitmap(*(_al_linker_mouse->mouse_screen_ptr), screen)) {
      _al_linker_mouse->show_mouse(NULL);
      hadmouse = TRUE;
   }
   else
      hadmouse = FALSE;

   hadtimer = _timer_installed;
   _timer_installed = FALSE;

   while (info) {
      restore_bitmap_state(info);
      reconstruct_kids(info->bmp, info->child);
      info = info->sibling;
   }

   _dispsw_status = SWITCH_NONE;

   if (bitmap_color_depth(screen) == 8) {
      if (_got_prev_current_palette)
	 gfx_driver->set_palette(_prev_current_palette, 0, 255, FALSE);
      else
	 gfx_driver->set_palette(_current_palette, 0, 255, FALSE);
   }

   if (hadmouse)
      _al_linker_mouse->show_mouse(screen);

   _timer_installed = hadtimer;
}


