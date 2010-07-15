#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro5.h>
#include <stdio.h>

#include "common.c"

static void print_file(ALLEGRO_FS_ENTRY *entry)
{
   int mode = al_get_fs_entry_mode(entry);
   time_t now = time(NULL);
   time_t atime = al_get_fs_entry_atime(entry);
   time_t ctime = al_get_fs_entry_ctime(entry);
   time_t mtime = al_get_fs_entry_mtime(entry);
   const ALLEGRO_PATH *path = al_get_fs_entry_name(entry);
   off_t size = al_get_fs_entry_size(entry);
   char const *name;
   if (al_fs_entry_is_directory(entry))
      name = al_get_path_component(path, -1);
   else
      name = al_get_path_filename(path);
   log_printf("%-32s %s%s%s%s%s%s %10lu %10lu %10lu %13lu\n", 
      al_path_cstr(path, '/'),
      mode & ALLEGRO_FILEMODE_READ ? "r" : ".",
      mode & ALLEGRO_FILEMODE_WRITE ? "w" : ".",
      mode & ALLEGRO_FILEMODE_EXECUTE ? "x" : ".",
      mode & ALLEGRO_FILEMODE_HIDDEN ? "h" : ".",
      mode & ALLEGRO_FILEMODE_ISFILE ? "f" : ".",
      mode & ALLEGRO_FILEMODE_ISDIR ? "d" : ".",
      now - ctime,
      now - mtime,
      now - atime,
      size);
}

static void print_entry(ALLEGRO_FS_ENTRY *entry)
{
   print_file(entry);

   if (al_fs_entry_is_directory(entry)) {
      ALLEGRO_FS_ENTRY *next;

      al_open_directory(entry);
      while (1) {
         next = al_read_directory(entry);
         if (!next)
            break;

         print_entry(next);
         al_destroy_fs_entry(next);
      }
      al_close_directory(entry);
   }
}

int main(int argc, char **argv)
{
   int i;

   al_init();
   open_log_monospace();
   
   log_printf("%-32s %-6s %10s %10s %10s %13s\n",
      "name", "flags", "ctime", "mtime", "atime", "size");
   log_printf(
      "-------------------------------- "
      "------ "
      "---------- "
      "---------- "
      "---------- "
      "-------------\n");

   if (argc == 1) {
      ALLEGRO_FS_ENTRY *entry = al_create_fs_entry("data");
      print_entry(entry);
      al_destroy_fs_entry(entry);
   }

   for (i = 1; i < argc; i++) {
      ALLEGRO_FS_ENTRY *entry = al_create_fs_entry(argv[i]);
      print_entry(entry);
      al_destroy_fs_entry(entry);
   }

   close_log(true);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
