#include "dirty.h"

/* dirty rectangle list */
typedef struct DIRTY_RECT {
   int x, y, w, h;
} DIRTY_RECT;

typedef struct DIRTY_LIST {
   int size;
   int count;
   DIRTY_RECT *rect;
} DIRTY_LIST;

static DIRTY_LIST dirty, old_dirty;


/* adds a new screen area to the dirty rectangle list */
void add_to_list(DIRTY_LIST *list, int x, int y, int w, int h)
{
   /* dynamicall grow the list of dirty rectangles */
   if (list->count >= list->size) {
      if (list->size == 0)
         list->size = 256;
      else
         list->size *= 2;
      list->rect = realloc(list->rect, list->size * sizeof(DIRTY_RECT));
   }
   list->rect[list->count].x = x;
   list->rect[list->count].y = y;
   list->rect[list->count].w = w;
   list->rect[list->count].h = h;
   list->count++;
}



void dirty_rectangle(int x, int y, int w, int h)
{
   add_to_list(&dirty, x, y, w, h);
}



/* qsort() callback for sorting the dirty rectangle list */
int rect_cmp(const void *p1, const void *p2)
{
   DIRTY_RECT *r1 = (DIRTY_RECT *) p1;
   DIRTY_RECT *r2 = (DIRTY_RECT *) p2;

   return (r1->y - r2->y);
}



void clear_dirty(BITMAP *bmp)
{
   int c;
   DIRTY_LIST swap;
   /* clear everything drawn during the previous update */
   for (c = 0; c < dirty.count; c++) {
      if ((dirty.rect[c].w == 1) && (dirty.rect[c].h == 1)) {
         putpixel(bmp, dirty.rect[c].x, dirty.rect[c].y, 0);
      }
      else {
         rectfill(bmp, dirty.rect[c].x, dirty.rect[c].y,
                  dirty.rect[c].x + dirty.rect[c].w - 1,
                  dirty.rect[c].y + dirty.rect[c].h - 1, 0);
      }
   }

   /* swap lists, so the just cleared rects automatically are in this update */
   swap = old_dirty;
   old_dirty = dirty;
   dirty = swap;
   dirty.count = 0;
}



void draw_dirty(BITMAP *bmp)
{
   int c;
   /* for dirty rectangle animation, only copy the areas that changed */
   for (c = 0; c < dirty.count; c++)
      add_to_list(&old_dirty, dirty.rect[c].x, dirty.rect[c].y,
                  dirty.rect[c].w, dirty.rect[c].h);

   /* sorting the objects really cuts down on bank switching */
   if (!gfx_driver->linear)
      qsort(old_dirty.rect, old_dirty.count, sizeof(DIRTY_RECT),
            rect_cmp);

   acquire_screen();

   for (c = 0; c < old_dirty.count; c++) {
      if ((old_dirty.rect[c].w == 1) && (old_dirty.rect[c].h == 1)) {
         putpixel(screen, old_dirty.rect[c].x, old_dirty.rect[c].y,
                  getpixel(bmp, old_dirty.rect[c].x, old_dirty.rect[c].y));
      }
      else {
         blit(bmp, screen, old_dirty.rect[c].x, old_dirty.rect[c].y,
              old_dirty.rect[c].x, old_dirty.rect[c].y,
              old_dirty.rect[c].w, old_dirty.rect[c].h);
      }
   }

   release_screen();
}
