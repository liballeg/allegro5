#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"

#include "common.c"

#define EURO      "\xe2\x82\xac"

static void wait_for_esc(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_BITMAP *screen_clone;
   al_install_keyboard();
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   screen_clone = al_clone_bitmap(al_get_target_bitmap());
   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE) {
         int x = event.display.x;
         int y = event.display.y;
         int w = event.display.width;
         int h = event.display.height;

         al_draw_bitmap_region(screen_clone, x, y, w, h,
            x, y, 0);
         al_update_display_region(x, y, w, h);
      }
   }
   al_destroy_bitmap(screen_clone);
   al_destroy_event_queue(queue);
}

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap, *font_bitmap;
    ALLEGRO_FONT *f1, *f2, *f3;

    int ranges[] = {
       0x0020, 0x007F,  /* ASCII */
       0x00A1, 0x00FF,  /* Latin 1 */
       0x0100, 0x017F,  /* Extended-A */
       0x20AC, 0x20AC}; /* Euro */

    if (!al_init()) {
        abort_example("Could not init Allegro.\n");
    }

    al_init_image_addon();
    al_init_font_addon();

    al_set_new_display_option(ALLEGRO_SINGLE_BUFFER, true, ALLEGRO_SUGGEST);
    al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
    display = al_create_display(320, 200);
    if (!display) {
        abort_example("Failed to create display\n");
    }
    bitmap = al_load_bitmap("data/mysha.pcx");
    if (!bitmap) {
        abort_example("Failed to load mysha.pcx\n");
    }

    f1 = al_load_font("data/bmpfont.tga", 0, 0);
    if (!f1) {
        abort_example("Failed to load bmpfont.tga\n");
    }
    
    font_bitmap = al_load_bitmap("data/a4_font.tga");
    if (!font_bitmap) {
        abort_example("Failed to load a4_font.tga\n");
    }
    f2 = al_grab_font_from_bitmap(font_bitmap, 4, ranges);

    f3 = al_create_builtin_font();
    if (!f3) {
        abort_example("Failed to create builtin font.\n");
    }

    /* Draw background */
    al_draw_bitmap(bitmap, 0, 0, 0);

    /* Draw red text */
    al_draw_textf(f1, al_map_rgb(255, 0, 0), 10, 10, 0, "red");

    /* Draw green text */
    al_draw_textf(f1, al_map_rgb(0, 255, 0), 120, 10, 0, "green");
    
    /* Draw a unicode symbol */
    al_draw_textf(f2, al_map_rgb(0, 0, 255), 60, 60, 0, "Mysha's 0.02" EURO);

    /* Draw a yellow text with the builtin font */
    al_draw_textf(f3, al_map_rgb(255, 255, 0), 160, 160, ALLEGRO_ALIGN_CENTER,
        "a string from builtin font data");

    al_flip_display();

    wait_for_esc(display);

    al_destroy_bitmap(bitmap);
    al_destroy_bitmap(font_bitmap);
    al_destroy_font(f1);
    al_destroy_font(f2);
    al_destroy_font(f3);
    return 0;
}

/* vim: set sts=4 sw=4 et: */
