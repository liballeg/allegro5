/*
 *    Example program for Allegro library.
 *
 *    Demonstrate PhysicsFS addon.
 */


#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_physfs.h>
#include <physfs.h>

#include "common.c"

static void show_image(ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

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

static void print_file(ALLEGRO_FS_ENTRY *entry)
{
   int mode = al_get_fs_entry_mode(entry);
   time_t now = time(NULL);
   time_t atime = al_get_fs_entry_atime(entry);
   time_t ctime = al_get_fs_entry_ctime(entry);
   time_t mtime = al_get_fs_entry_mtime(entry);
   const char *name = al_get_fs_entry_name(entry);
   off_t size = al_get_fs_entry_size(entry);

   log_printf("%-36s %s%s%s%s%s%s %8u %8u %8u %8u\n",
      name,
      mode & ALLEGRO_FILEMODE_READ ? "r" : ".",
      mode & ALLEGRO_FILEMODE_WRITE ? "w" : ".",
      mode & ALLEGRO_FILEMODE_EXECUTE ? "x" : ".",
      mode & ALLEGRO_FILEMODE_HIDDEN ? "h" : ".",
      mode & ALLEGRO_FILEMODE_ISFILE ? "f" : ".",
      mode & ALLEGRO_FILEMODE_ISDIR ? "d" : ".",
      (unsigned)(now - ctime),
      (unsigned)(now - mtime),
      (unsigned)(now - atime),
      (unsigned)size);
}

static void listdir(ALLEGRO_FS_ENTRY *entry)
{
   ALLEGRO_FS_ENTRY *next;

   al_open_directory(entry);
   while (1) {
      next = al_read_directory(entry);
      if (!next)
         break;

      print_file(next);
      if (al_get_fs_entry_mode(next) & ALLEGRO_FILEMODE_ISDIR)
         listdir(next);
      al_destroy_fs_entry(next);
   }
   al_close_directory(entry);
}

int main(int argc, const char *argv[])
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_FS_ENTRY *entry;
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro\n");
   }
   al_init_image_addon();
   al_install_keyboard();
   open_log_monospace();

   /* Set up PhysicsFS. */
   if (!PHYSFS_init(argv[0])) {
      abort_example("Could not init PhysFS\n");
   }
   // This creates a ~/.allegro directory, which is very annoying to say the
   // least - and no need for it in this example.
   //  if (!PHYSFS_setSaneConfig("allegro", "ex_physfs", NULL, 0, 0))
   //     return 1;
   if (!PHYSFS_addToSearchPath("data/ex_physfs.zip", 1)) {
      abort_example("Could load the zip file\n");
   }

   for (i = 1; i < argc; i++) {
      if (!PHYSFS_addToSearchPath(argv[i], 1)) {
         abort_example("Couldn't add %s\n", argv[i]);
      }
   }

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   /* Make future calls to al_fopen() on this thread go to the PhysicsFS
    * backend.
    */
   al_set_physfs_file_interface();

   /* List the contents of our example zip recursively. */
   log_printf("%-36s %-6s %8s %8s %8s %8s\n",
      "name", "flags", "ctime", "mtime", "atime", "size");
   log_printf(
          "------------------------------------ "
          "------ "
          "-------- "
          "-------- "
          "-------- "
          "--------\n");
   entry = al_create_fs_entry("");
   listdir(entry);
   al_destroy_fs_entry(entry);

   bmp = al_load_bitmap("02.bmp");
   if (bmp) {
      show_image(bmp);
      al_destroy_bitmap(bmp);
   }

   PHYSFS_deinit();

   close_log(false);
   return 0;
}


/* vim: set sts=3 sw=3 et: */
