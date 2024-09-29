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

static void show_image(A5O_BITMAP *bmp)
{
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   while (true) {
      al_draw_bitmap(bmp, 0, 0, 0);
      al_flip_display();
      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_KEY_DOWN
            && event.keyboard.keycode == A5O_KEY_ESCAPE) {
         break;
      }
   }

   al_destroy_event_queue(queue);
}

static void print_file(A5O_FS_ENTRY *entry)
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
      mode & A5O_FILEMODE_READ ? "r" : ".",
      mode & A5O_FILEMODE_WRITE ? "w" : ".",
      mode & A5O_FILEMODE_EXECUTE ? "x" : ".",
      mode & A5O_FILEMODE_HIDDEN ? "h" : ".",
      mode & A5O_FILEMODE_ISFILE ? "f" : ".",
      mode & A5O_FILEMODE_ISDIR ? "d" : ".",
      (unsigned)(now - ctime),
      (unsigned)(now - mtime),
      (unsigned)(now - atime),
      (unsigned)size);
}

static void listdir(A5O_FS_ENTRY *entry)
{
   A5O_FS_ENTRY *next;

   al_open_directory(entry);
   while (1) {
      next = al_read_directory(entry);
      if (!next)
         break;

      print_file(next);
      if (al_get_fs_entry_mode(next) & A5O_FILEMODE_ISDIR)
         listdir(next);
      al_destroy_fs_entry(next);
   }
   al_close_directory(entry);
}

static bool add_main_zipfile(void)
{
   A5O_PATH *exe;
   const char *ext;
   const char *zipfile;
   bool ret;

   /* On Android we treat the APK itself as the zip file. */
   exe = al_get_standard_path(A5O_EXENAME_PATH);
   ext = al_get_path_extension(exe);
   if (0 == strcmp(ext, ".apk")) {
      zipfile = al_path_cstr(exe, '/');
   }
   else {
      zipfile = "data/ex_physfs.zip";
   }

   if (PHYSFS_mount(zipfile, NULL, 1)) {
      ret = true;
   }
   else {
      log_printf("Could load the zip file: %s\n", zipfile);
      ret = false;
   }

   al_destroy_path(exe);

   return ret;
}

int main(int argc, char *argv[])
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bmp;
   A5O_FS_ENTRY *entry;
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
   if (!add_main_zipfile()) {
      abort_example("Could not add zip file\n");
   }

   for (i = 1; i < argc; i++) {
      if (!PHYSFS_mount(argv[i], NULL, 1)) {
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
   if (!bmp) {
      /* Fallback for Android. */
      bmp = al_load_bitmap("assets/data/alexlogo.bmp");
   }
   if (bmp) {
      show_image(bmp);
      al_destroy_bitmap(bmp);
   }

   PHYSFS_deinit();

   close_log(false);
   return 0;
}


/* vim: set sts=3 sw=3 et: */
