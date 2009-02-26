#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"

static void wait_for_esc(void)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_BITMAP *screen_clone;
   al_install_keyboard();
   queue = al_create_event_queue();
   al_register_event_source(queue, (void *)al_get_keyboard());
   al_register_event_source(queue, (void *)al_get_current_display());
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
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
            al_map_rgb_f(1, 1, 1));
         al_draw_bitmap_region(screen_clone, x, y - 2, w, h + 4,
            x, y - 2, 0);
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
    ALLEGRO_FONT *f, *a4f;
    int ranges[] = {
       0x0020, 0x007F,  /* ASCII */
       0x00A1, 0x00FF,  /* Latin 1 */
       0x0100, 0x017F,  /* Extended-A */
       0x20AC, 0x20AC}; /* Euro */

    if (!al_init()) {
        TRACE("Could not init Allegro.\n");
        return 1;
    }

    al_iio_init();
    al_font_init();

    al_set_new_display_option(ALLEGRO_DOUBLEBUFFERED, false, ALLEGRO_SUGGEST);
    al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
    display = al_create_display(320, 200);
    if (!display) {
        TRACE("Failed to create display\n");
        return 1;
    }
    bitmap = al_iio_load("data/mysha.pcx");
    if (!bitmap) {
        TRACE("Failed to load mysha.pcx\n");
        return 1;
    }

    f = al_font_load_font("data/bmpfont.tga", NULL);
    if (!f) {
        TRACE("Failed to load bmpfont.tga\n");
        return 1;
    }
    
    font_bitmap = al_iio_load("data/a4_font.png");
    if (!font_bitmap) {
        TRACE("Failed to load a4_font.png\n");
        return 1;
    }
    a4f = al_font_grab_font_from_bitmap(font_bitmap, 4, ranges);

    /* Draw background */
    al_draw_bitmap(bitmap, 0, 0, 0);

    /* Draw red text */
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(255, 0, 0));
    al_font_textout(f, 10, 10, "red", -1);

    /* Draw green text */
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(0, 255, 0));
    al_font_textout(f, 10, 50, "green", -1);
    
    /* Draw a unicode symbol */
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(0, 0, 255));
    al_font_textout(a4f, 10, 90, "Mysha's 0.02€", -1);

    al_flip_display();

    wait_for_esc();

    al_destroy_bitmap(bitmap);
    al_destroy_bitmap(font_bitmap);
    al_font_destroy_font(f);
    al_font_destroy_font(a4f);
    return 0;
}
END_OF_MAIN()

/* vim: set sts=4 sw=4 et: */
