/*
 *      Example program for the Allegro library.
 *
 *      Test memfile addon.
 */

#include <stdio.h>
#ifdef _MSC_VER
   #pragma comment ( linker, "/SUBSYSTEM:CONSOLE")
#endif
#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro5.h>
#include <allegro5/memfile.h>

int main(void)
{
   ALLEGRO_FS_ENTRY *memfile = NULL;
   char *data = NULL;
   int i = 0;
   int data_size = 1024;

   al_init();

   data = calloc(1, data_size);
   if (!data)
      return 1;
   
   printf("Creating memfile\n");
   memfile = al_open_memfile(data_size, data);
   if (!memfile) {
      printf("Error opening memfile :(\n");
      goto Error;
   }

   printf("Writing data to memfile\n");
   for (i = 0; i < data_size/4; i++) {
      if (al_fwrite32le(memfile, i) != i) {
         printf("Failed to write %i to memfile\n", i);
         goto Error;
      }
   }

   al_fseek(memfile, 0, ALLEGRO_SEEK_SET);

   printf("Reading and testing data from memfile\n");
   for (i = 0; i < data_size/4; i++) {
      int32_t ret = al_fread32le(memfile);
      if (ret != i || al_feof(memfile)) {
         printf("Item %i failed to verify, got %i\n", i, ret);
         goto Error;
      }
   }

   printf("Done.\n");

   al_fclose(memfile);
   free(data);
   return 0;

Error:

   al_fclose(memfile);
   free(data);
   return 1;
}
END_OF_MAIN()
/* vim: set sts=3 sw=3 et: */
