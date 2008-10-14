#include <stdio.h>
#include <allegro5/allegro5.h>

int main(int argc, char **argv)
{
   AL_PATH *dyn = NULL;
   AL_PATH sta;
   AL_PATH *tostring = NULL;

   al_init();
   set_uformat(U_ASCII);

   if(argc < 2) {
      AL_PATH *exe = al_path_create(argv[0]);
      if(exe) {
         printf("useage1: %s <path>\n", al_path_get_filename(exe));
         al_path_free(exe);
      }
      else {
         printf("useage2: %s <path>\n", argv[0]);
      }

      exit(-1);
   }

   dyn = al_path_create(argv[1]);
   if(!dyn) {
      printf("failed to create path structure for '%s'\n", argv[1]);
   }
   else {
      printf("dyn: drive=\"%s\", file=\"%s\"\n", al_path_get_drive(dyn), al_path_get_filename(dyn));
      al_path_free(dyn);
   }

   if(al_path_init(&sta, argv[1])) {
      printf("sta: drive=\"%s\", file=\"%s\"\n", al_path_get_drive(&sta), al_path_get_filename(&sta));
      al_path_free(&sta);
   }
   else {
      printf("failed to init path structure for '%s'\n", argv[1]);
   }

   tostring = al_path_create(argv[1]);
   if(!tostring) {
      printf("failed to create path structure for tostring test\n");
   } else {
      char tostring_string[1024];
      int32_t i = 0;
      printf("tostring: '%s'\n", al_path_to_string(tostring, tostring_string, 1024, '/'));
      printf("tostring: drive:'%s'", al_path_get_drive(tostring));
      printf(" dirs:");
      for(i=0; i<al_path_num_components(tostring); i++) {
         if(i > 0) printf(",");
         printf(" '%s'", al_path_index(tostring, i));
      }
      printf(" filename:'%s'\n", al_path_get_filename(tostring));
      al_path_free(tostring);
   }

   /* FIXME: test out more of the al_path_ functions, ie: insert, remove, concat, cannonocalize, absolute, relative */

   return 0;
}
END_OF_MAIN()
