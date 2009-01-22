#include <stdio.h>
#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro5.h>
#include <allegro5/memfile.h>

int main(int argc, char **argv)
{
   ALLEGRO_FS_ENTRY *memfile = NULL;
   char *data = NULL;
   int i = 0;
   int data_size = 1024;

   (void)argc;
   (void)argv;
   al_init();

   data = calloc(1, data_size);
   if(!data)
      return 0;
   
   printf("creating memfile\n");
   memfile = al_open_memfile(data_size, data);
   if(!memfile) {
      printf("error opening memfile :(\n");
      return 0;
   }

   printf("writing data to memfile\n");
   for(i = 0; i < data_size/4; i++) {
      if(al_fwrite32le(memfile, i) != i) {
         printf("failed to write %i to memfile\n", i);
         al_fclose(memfile);
         return 0;
      }
   }

   al_fseek(memfile, 0, ALLEGRO_SEEK_SET);

   printf("reading and testing data from memfile\n");
   for(i = 0; i < data_size/4; i++) {
      int32_t ret = al_fread32le(memfile);
      if(ret != i || al_feof(memfile)) {
         printf("item %i failed to verrify, got %i\n", i, ret);
         al_fclose(memfile);
         return 0;
      }
      else
         printf("verrified: [%i] == %i\n", i, ret);
   }

   printf("done.\n");

   return 0;
}
