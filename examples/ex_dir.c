#include <allegro5/allegro5.h>
#include <stdio.h>

void print_file(ALLEGRO_FS_ENTRY *entry)
{
   int mode = al_get_entry_mode(entry);
   time_t now = time(NULL);
   time_t atime = al_get_entry_atime(entry);
   time_t ctime = al_get_entry_ctime(entry);
   time_t mtime = al_get_entry_mtime(entry);
   ALLEGRO_PATH *path = al_get_entry_name(entry);
   off_t size = al_get_entry_size(entry);
   char const *name;
   if (al_is_directory(entry))
      name = al_path_index(path, -1);
   else
      name = al_path_get_filename(path);
   printf("%-32s %s%s%s%s%s%s %10lu %10lu %10lu %13lu\n", 
      name,
      mode & ALLEGRO_FILEMODE_READ ? "r" : ".",
      mode & ALLEGRO_FILEMODE_WRITE ? "w" : ".",
      mode & ALLEGRO_FILEMODE_EXECUTE ? "e" : ".",
      mode & ALLEGRO_FILEMODE_HIDDEN ? "h" : ".",
      mode & ALLEGRO_FILEMODE_ISFILE ? "f" : ".",
      mode & ALLEGRO_FILEMODE_ISDIR ? "d" : ".",
      now - ctime,
      now - mtime,
      now - atime,
      size);
   al_path_free(path);
}

void print_entry(ALLEGRO_FS_ENTRY *entry)
{
   if (al_is_directory(entry)) {
      ALLEGRO_FS_ENTRY *next;

      al_open_entry(entry, "r");
      while (1) {
         next = al_readdir(entry);
         if (!next)
            break;
         print_file(next);
         al_destroy_entry(next);
      }
   }
   else {
      print_file(entry);
   }
}

int main(int argc, char **argv)
{
   int i;

   al_init();
   
   printf("%-32s %-6s %10s %10s %10s %13s\n",
      "name", "flags", "ctime", "mtime", "atime", "size");

   if (argc == 1) {
      ALLEGRO_FS_ENTRY *entry = al_create_entry(".");
      print_entry(entry);
      al_destroy_entry(entry);
      return 0;
   }

   for (i = 1; i < argc; i++) {
      ALLEGRO_FS_ENTRY *entry = al_create_entry(argv[i]);
      print_entry(entry);
      al_destroy_entry(entry);
   }
   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
