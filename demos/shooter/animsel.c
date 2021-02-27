#include "animsel.h"

/* d_list_proc() callback for the animation mode dialog */
static char *anim_list_getter(int index, int *list_size)
{
   static char *s[] = {
      "Double buffered",
      "Page flipping",
      "Triple buffered",
      "Dirty rectangles"
   };

   if (index < 0) {
      *list_size = sizeof(s) / sizeof(s[0]);
      return NULL;
   }

   return s[index];
}



static int anim_list_proc(int msg, DIALOG * d, int c);
static int anim_desc_proc(int msg, DIALOG * d, int c);

static DIALOG anim_type_dlg[] = {
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)     (d1)  (d2)  (dp)                 (dp2) (dp3) */
   {d_shadow_box_proc, 0, 0, 281, 151, 0, 1, 0, 0, 0, 0, NULL, NULL, NULL},
   {d_ctext_proc, 140, 8, 1, 1, 0, 1, 0, 0, 0, 0, "Animation Method", NULL,
    NULL},
   {anim_list_proc, 16, 28, 153, 36, 0, 1, 0, D_EXIT, 3, 0,
    anim_list_getter, NULL, NULL},
   {d_check_proc, 16, 70, 248, 12, 0, 0, 0, 0, 0, 0, "Maximum FPS (uses 100% CPU)", NULL, NULL},
   {anim_desc_proc, 16, 90, 248, 48, 0, 1, 0, 0, 0, 0, 0, NULL, NULL},
   {d_button_proc, 184, 28, 80, 16, 0, 1, 0, D_EXIT, 0, 0, "OK", NULL,
    NULL},
   {d_button_proc, 184, 50, 80, 16, 0, 1, 27, D_EXIT, 0, 0, "Cancel", NULL,
    NULL},
   {d_yield_proc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL},
   {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
};



static int anim_list_proc(int msg, DIALOG * d, int c)
{
   int sel, ret;

   sel = d->d1;

   ret = d_list_proc(msg, d, c);

   if (sel != d->d1)
      ret |= D_REDRAW;

   return ret;
}



/* dialog procedure for the animation type dialog */
static int anim_desc_proc(int msg, DIALOG * d, int c)
{
   static char *double_buffer_desc[] = {
      "Draws onto a memory bitmap,",
      "and then uses a brute-force",
      "blit to copy the entire",
      "image across to the screen.",
      NULL
   };

   static char *page_flip_desc[] = {
      "Uses two pages of video",
      "memory, and flips back and",
      "forth between them. It will",
      "only work if there is enough",
      "video memory to set up dual",
      "pages.",
      NULL
   };

   static char *triple_buffer_desc[] = {
      "Uses three pages of video",
      "memory, to avoid wasting time",
      "waiting for retraces. Only",
      "some drivers and hardware",
      "support this.",
      NULL
   };

   static char *dirty_rectangle_desc[] = {
      "This is similar to double",
      "buffering, but stores a list",
      "of which parts of the screen",
      "have changed, to minimise the",
      "amount of drawing that needs",
      "to be done.",
      NULL
   };

   static char **descs[] = {
      double_buffer_desc,
      page_flip_desc,
      triple_buffer_desc,
      dirty_rectangle_desc
   };

   char **p;
   int y;

   if (msg == MSG_DRAW) {
      rectfill(screen, d->x, d->y, d->x + d->w, d->y + d->h, d->bg);

      p = descs[anim_type_dlg[2].d1];
      y = d->y;

      while (*p) {
         textout_ex(screen, font, *p, d->x, y, d->fg, d->bg);
         y += 8;
         p++;
      }
   }

   return D_O_K;
}



/* allows the user to choose an animation type */
int pick_animation_type(ANIMATION_TYPE *type)
{
   int ret;

   centre_dialog(anim_type_dlg);

   clear_bitmap(screen);

   /* we set up colors to match screen color depth */
   for (ret = 0; anim_type_dlg[ret].proc; ret++) {
      anim_type_dlg[ret].fg = palette_color[0];
      anim_type_dlg[ret].bg = palette_color[1];
   }

   ret = do_dialog(anim_type_dlg, 2);

   *type = anim_type_dlg[2].d1 + 1;
   max_fps = anim_type_dlg[3].flags & D_SELECTED;

   ASSERT(*type >= DOUBLE_BUFFER && *type <= DIRTY_RECTANGLE);

   return (ret == 6) ? -1 : ret;
}
