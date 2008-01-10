#include "display.h"
#include "dirty.h"

static BITMAP *page1, *page2, *page3;
static int current_page = 0;
static int num_pages;
static BITMAP *memory_buffer, *back;
ANIMATION_TYPE animation_type;

void init_display(int mode, int w, int h, ANIMATION_TYPE type)
{
   char buf[256];

   animation_type = type;

   switch (animation_type) {
      case DOUBLE_BUFFER:
      case DIRTY_RECTANGLE:
         num_pages = 1;
         break;

      case PAGE_FLIP:
         num_pages = 2;
         break;

      case TRIPLE_BUFFER:
         num_pages = 3;
         break;
   }

   set_color_depth(8);
#ifdef ALLEGRO_VRAM_SINGLE_SURFACE
   if (set_gfx_mode(mode, w, h, w, h * num_pages) != 0) {
#else
   if (set_gfx_mode(mode, w, h, 0, 0) != 0) {
#endif
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting 8bpp graphics mode\n%s\n",
                      allegro_error);
      exit(1);
   }

   page1 = NULL;
   page2 = NULL;
   page3 = NULL;
   memory_buffer = NULL;

   switch (animation_type) {

      case DOUBLE_BUFFER:
      case DIRTY_RECTANGLE:
	 memory_buffer = create_bitmap(SCREEN_W, SCREEN_H);

         if (!memory_buffer) {
            set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
            allegro_message("Could not create memory buffer\n");
            exit(1);
         }
	 break;

      case PAGE_FLIP:
         /* set up page flipping bitmaps */
         page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
         page2 = create_video_bitmap(SCREEN_W, SCREEN_H);

         if ((!page1) || (!page2)) {
            set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
            allegro_message("Not enough video memory for page flipping\n");
            exit(1);
         }
         break;

      case TRIPLE_BUFFER:
         /* set up triple buffered bitmaps */
         if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER))
            enable_triple_buffer();

         if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)) {
            strcpy(buf, gfx_driver->name);
            set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

#ifdef ALLEGRO_DOS
            allegro_message
                ("The %s driver does not support triple buffering\n\n"
                 "Try using mode-X in clean DOS mode (not under Windows)\n",
                 buf);
#else
            allegro_message
                ("The %s driver does not support triple buffering\n", buf);
#endif

            exit(1);
         }

         page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
         page2 = create_video_bitmap(SCREEN_W, SCREEN_H);
         page3 = create_video_bitmap(SCREEN_W, SCREEN_H);

         if ((!page1) || (!page2) || (!page3)) {
            set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
            allegro_message
                ("Not enough video memory for triple buffering\n");
            exit(1);
         }
         break;
   }
}



void clear_display(void)
{
    if (num_pages < 2) {
        clear_bitmap(screen);
        clear_bitmap(memory_buffer);
        if (animation_type == DIRTY_RECTANGLE)
            clear_dirty(memory_buffer);
    }
    else {
        clear_bitmap(page1);
        clear_bitmap(page2);
        if (num_pages == 3)
            clear_bitmap(page3);
    }
    current_page = 0;
    show_video_bitmap(screen);
}



BITMAP *prepare_display(void)
{
   switch (animation_type) {
      case DOUBLE_BUFFER:
         /* for double buffering, draw onto the memory bitmap. The first step 
          * is to clear it...
          */
         back = memory_buffer;
         clear_bitmap(back);
         break;
      case PAGE_FLIP:
         /* for page flipping we draw onto one of the video pages.
          */
         if (current_page == 0) {
            back = page2;
            current_page = 1;
         }
         else {
            back = page1;
            current_page = 0;
         }
         clear_bitmap(back);
         break;
      case TRIPLE_BUFFER:
         /* triple buffering works kind of like page flipping, but with three
          * pages (obvious, really :-) The benefit of this is that we can be
          * drawing onto page a, while displaying page b and waiting for the
          * retrace that will flip across to page c, hence there is no need
          * to waste time sitting around waiting for retraces.
          */
         if (current_page == 0) {
            back = page2;
            current_page = 1;
         }
         else if (current_page == 1) {
            back = page3;
            current_page = 2;
         }
         else {
            back = page1;
            current_page = 0;
         }
         clear_bitmap(back);
         break;
      case DIRTY_RECTANGLE:
         /* for dirty rectangle animation we draw onto the memory bitmap, but 
          * we can use information saved during the last draw operation to
          * only clear the areas that have something on them.
          */
         back = memory_buffer;

         clear_dirty(back);

         break;
   }
   return back;
}



void flip_display(void)
{
   switch (animation_type) {
      case DOUBLE_BUFFER:
         /* when double buffering, just copy the memory bitmap to the screen */
         blit(memory_buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
         break;
      case PAGE_FLIP:
         /* for page flipping we display the back page */
         show_video_bitmap(back);
         break;
      case TRIPLE_BUFFER:
         /* make sure the last flip request has actually happened */
         do {
         } while (poll_scroll());

         /* post a request to display the page we just drew */
         request_video_bitmap(back);
         break;
      case DIRTY_RECTANGLE:
         draw_dirty(memory_buffer);
         break;
   }
}


void destroy_display(void)
{
   if (num_pages == 1) {
      destroy_bitmap(memory_buffer);
   }
   else if (num_pages == 2) {
      destroy_bitmap(page1);
      destroy_bitmap(page2);
   }
   else if (num_pages == 3) {
      destroy_bitmap(page1);
      destroy_bitmap(page2);
      destroy_bitmap(page3);
   }
   else {
      ASSERT(FALSE);
   }
}
