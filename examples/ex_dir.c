#include <allegro5/allegro.h>
#include <stdio.h>

#include "common.c"

static void print_file(ALLEGRO_FS_ENTRY *entry)
{
   int mode = al_get_fs_entry_mode(entry);
   time_t now = time(NULL);
   time_t atime = al_get_fs_entry_atime(entry);
   time_t ctime = al_get_fs_entry_ctime(entry);
   time_t mtime = al_get_fs_entry_mtime(entry);
   off_t size = al_get_fs_entry_size(entry);
   const char *name = al_get_fs_entry_name(entry);

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

static void print_entry(ALLEGRO_FS_ENTRY *entry)
{
   ALLEGRO_FS_ENTRY *next;

   print_file(entry);

   if (!(al_get_fs_entry_mode(entry) & ALLEGRO_FILEMODE_ISDIR))
      return;

   if (!al_open_directory(entry)) {
      log_printf("Error opening directory: %s\n", al_get_fs_entry_name(entry));
      return;
   }

   while (1) {
      next = al_read_directory(entry);
      if (!next)
         break;

      print_entry(next);
      al_destroy_fs_entry(next);
   }

   al_close_directory(entry);
}


static int print_fs_entry_cb(ALLEGRO_FS_ENTRY * entry, void * extra) {
   (void) extra;
   print_file(entry);
   return ALLEGRO_FOR_EACH_FS_ENTRY_OK;
}

static int print_fs_entry_cb_norecurse(ALLEGRO_FS_ENTRY * entry, void * extra) {
   (void) extra;
   print_file(entry);
   return ALLEGRO_FOR_EACH_FS_ENTRY_SKIP;
}

static void print_fs_entry(ALLEGRO_FS_ENTRY * dir)
{
   log_printf("\n------------------------------------\nExample of al_for_each_fs_entry with recursion:\n\n");
   al_for_each_fs_entry(dir, print_fs_entry_cb,
      (void*) al_get_fs_entry_name(dir));
}

static void print_fs_entry_norecurse(ALLEGRO_FS_ENTRY * dir)
{
   log_printf("\n------------------------------------\nExample of al_for_each_fs_entry without recursion:\n\n");
   al_for_each_fs_entry(dir, print_fs_entry_cb_norecurse,
      (void*) al_get_fs_entry_name(dir));
}

int main(int argc, char **argv)
{
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   open_log_monospace();

   log_printf("Example of filesystem entry functions:\n\n%-36s %-6s %8s %8s %8s %8s\n",
      "name", "flags", "ctime", "mtime", "atime", "size");
   log_printf(
      "------------------------------------ "
      "------ "
      "-------- "
      "-------- "
      "-------- "
      "--------\n");

   #ifdef ALLEGRO_ANDROID
   al_android_set_apk_fs_interface();
   #endif

   if (argc == 1) {
      ALLEGRO_FS_ENTRY *entry = al_create_fs_entry("data");
      print_entry(entry);
      print_fs_entry(entry);
      print_fs_entry_norecurse(entry);
      al_destroy_fs_entry(entry);
   }

   for (i = 1; i < argc; i++) {
      ALLEGRO_FS_ENTRY *entry = al_create_fs_entry(argv[i]);
      print_entry(entry);
      print_fs_entry(entry);
      print_fs_entry_norecurse(entry);
      al_destroy_fs_entry(entry);
   }

   close_log(true);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
