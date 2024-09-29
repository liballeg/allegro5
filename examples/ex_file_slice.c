/*
 *  ex_file_slice - Use slices to pack many objects into a single file.
 *
 *  This example packs two strings into a single file, and then uses a
 *  file slice to open them one at a time. While this usage is contrived,
 *  the same principle can be used to pack multiple images (for example)
 *  into a single file, and later read them back via Allegro's image loader. 
 *
 */
#include "allegro5/allegro.h"

#include "common.c"

#define BUFFER_SIZE 1024

static void pack_object(A5O_FILE *file, const void *object, size_t len)
{
   /* First write the length of the object, so we know how big to make
      the slice when it is opened later. */      
   al_fwrite32le(file, len);
   al_fwrite(file, object, len);
}

static A5O_FILE *get_next_chunk(A5O_FILE *file)
{
   /* Reads the length of the next chunk, and if not at end of file, returns a 
      slice that represents that portion of the file. */ 
   const uint32_t length = al_fread32le(file);
   return !al_feof(file) ? al_fopen_slice(file, length, "rw") : NULL;
}

int main(int argc, const char *argv[])
{
   A5O_FILE *master, *slice;
   A5O_PATH *tmp_path;

   const char *first_string = "Hello, World!";
   const char *second_string = "The quick brown fox jumps over the lazy dog.";
   char buffer[BUFFER_SIZE];

   (void) argc, (void) argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   master = al_make_temp_file("ex_file_slice_XXXX", &tmp_path);
   if (!master) {
      abort_example("Unable to create temporary file\n");
   }

   /* Pack both strings into the master file. */
   pack_object(master, first_string, strlen(first_string));
   pack_object(master, second_string, strlen(second_string));

   /* Seek back to the beginning of the file, as if we had just opened it */
   al_fseek(master, 0, A5O_SEEK_SET);

   /* Loop through the main file, opening a slice for each object */
   while ((slice = get_next_chunk(master))) {
      /* Note: While the slice is open, we must avoid using the master file!
         If you were dealing with packed images, this is where you would pass 'slice'
         to al_load_bitmap_f(). */

      if (al_fsize(slice) < BUFFER_SIZE) {
         /* We could have used al_fgets(), but just to show that the file slice
            is constrained to the string object, we'll read the entire slice. */
         al_fread(slice, buffer, al_fsize(slice));
         buffer[al_fsize(slice)] = 0;
         log_printf("Chunk of size %d: '%s'\n", (int) al_fsize(slice), buffer);
      }

      /* The slice must be closed before the next slice is opened. Closing
         the slice will advanced the master file to the end of the slice. */
      al_fclose(slice);
   }

   al_fclose(master);

   al_remove_filename(al_path_cstr(tmp_path, '/'));

   close_log(true);

   return 0;
}
