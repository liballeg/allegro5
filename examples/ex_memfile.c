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
#include <allegro5/allegro.h>
#include <allegro5/allegro_memfile.h>

#include "common.c"

int main(void)
{
   ALLEGRO_FILE *memfile;
   char *data;
   int i;
   const int data_size = 1024;
   char buffer[50];

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   data = calloc(1, data_size);
   if (!data)
      return 1;
   
   printf("Creating memfile\n");
   memfile = al_open_memfile(data, data_size, "rw");
   if (!memfile) {
      printf("Error opening memfile :(\n");
      goto Error;
   }

   printf("Writing data to memfile\n");
   for (i = 0; i < data_size/4; i++) {
      if (al_fwrite32le(memfile, i) < 4) {
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

   if (al_feof(memfile)) {
      printf("EOF indicator prematurely set!\n");
      goto Error;
   }
   
   /* testing the ungetc buffer */
   al_fseek(memfile, 0, ALLEGRO_SEEK_SET);
   
   for (i = 0; al_fungetc(memfile, i) != EOF; ++i) { }
   printf("Length of ungetc buffer: %d\n", i);
   
   if (al_ftell(memfile) != -i) {
      printf("Current position is not correct. Expected -%d, but got %d\n",
         i, (int) al_ftell(memfile));
      goto Error;
   }
   
   while (i--) {
      if (i != al_fgetc(memfile)) {
         printf("Failed to verify ungetc data.\n");
         goto Error;
      }
   }
   
   if (al_ftell(memfile) != 0) {
      printf("Current position is not correct after reading back the ungetc buffer\n");
      printf("Expected 0, but got %d\n", (int) al_ftell(memfile));
      goto Error;
   }
   
   al_fputs(memfile, "legro rocks!");
   al_fseek(memfile, 0, ALLEGRO_SEEK_SET);
   al_fungetc(memfile, 'l');
   al_fungetc(memfile, 'A');
   al_fgets(memfile, buffer, 15);
   if (strcmp(buffer, "Allegro rocks!")) {
      printf("Expected to see 'Allegro rocks!' but got '%s' instead.\n", buffer);
      printf("(Maybe the ungetc buffer isn't big enough.)\n");
      goto Error;
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
/* vim: set sts=3 sw=3 et: */
