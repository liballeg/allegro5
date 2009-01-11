#include <stdio.h>
#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro5.h>

int main(int argc, char **argv)
{
   ALLEGRO_PATH *dyn = NULL;
   ALLEGRO_PATH *tostring = NULL;

   al_init();
   set_uformat(U_ASCII);

   if (argc < 2) {
      ALLEGRO_PATH *exe = al_path_create(argv[0]);
      if (exe) {
         printf("usage1: %s <path>\n", al_path_get_filename(exe));
         al_path_free(exe);
      }
      else {
         printf("usage2: %s <path>\n", argv[0]);
      }

      return 0;
   }

   dyn = al_path_create(argv[1]);
   if (!dyn) {
      printf("Failed to create path structure for '%s'.\n", argv[1]);
   }
   else {
      printf("dyn: drive=\"%s\", file=\"%s\"\n",
	  al_path_get_drive(dyn),
	  al_path_get_filename(dyn));
      al_path_free(dyn);
   }

   tostring = al_path_create(argv[1]);
   if (!tostring) {
      printf("Failed to create path structure for tostring test\n");
   }
   else {
      char tostring_string[1024];
      int i;

      al_path_to_string(tostring, tostring_string, 1024, '/');
      printf("tostring: '%s'\n", tostring_string);
      printf("tostring: drive:'%s'", al_path_get_drive(tostring));
      printf(" dirs:");
      for (i = 0; i < al_path_num_components(tostring); i++) {
         if (i > 0)
            printf(",");
         printf(" '%s'", al_path_index(tostring, i));
      }
      printf(" filename:'%s'\n", al_path_get_filename(tostring));
      al_path_free(tostring);
   }

   /* FIXME: test out more of the al_path_ functions, ie: insert, remove,
    * concat, canonicalize, absolute, relative
    */

   return 0;
}

END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
