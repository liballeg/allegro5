/*
 *    Example program for Allegro library.
 *
 *    Demonstrate PhysicsFS addon.
 */


#include <allegro5/allegro5.h>
#include <allegro5/a5_iio.h>
#include <allegro5/a5_physfs.h>
#include <physfs.h>


static void show_image(ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (true) {
      al_draw_bitmap(bmp, 0, 0, 0);
      al_flip_display();
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN
            && event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
   }

   al_destroy_event_queue(queue);
}


int main(int argc, const char *argv[])
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bmp;
   (void)argc;

   if (!al_init())
      return 1;
   al_init_iio_addon();
   al_install_keyboard();

   /* Set up PhysicsFS. */
   if (!PHYSFS_init(argv[0]))
      return 1;
   if (!PHYSFS_setSaneConfig("allegro", "ex_physfs", NULL, 0, 0))
      return 1;
   if (!PHYSFS_addToSearchPath("data/ex_physfs.zip", 1))
      return 1;

   display = al_create_display(640, 480);
   if (!display)
      return 1;

   /* Make future calls to al_fopen() on this thread go to the PhysicsFS
    * backend.
    */
   al_set_physfs_file_interface();

   bmp = al_load_bitmap("02.bmp");
   if (bmp) {
      show_image(bmp);
      al_destroy_bitmap(bmp);
   }

   PHYSFS_deinit();

   return 0;
}
END_OF_MAIN()


/* vim: set sts=3 sw=3 et: */
