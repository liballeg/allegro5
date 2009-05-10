#include <stdio.h>
#define ALLEGRO_USE_CONSOLE
#include <allegro5/allegro5.h>

int main(int argc, char **argv)
{
   ALLEGRO_PATH *dyn = NULL;
   ALLEGRO_PATH *tostring = NULL;
   ALLEGRO_PATH *cloned = NULL;
   
   al_init();

   if (argc < 2) {
      ALLEGRO_PATH *exe = al_create_path(argv[0]);
      if (exe) {
         printf("usage1: %s <path>\n", al_get_path_filename(exe));
         al_free_path(exe);
      }
      else {
         printf("usage2: %s <path>\n", argv[0]);
      }

      return 0;
   }

   dyn = al_create_path(argv[1]);
   if (!dyn) {
      printf("Failed to create path structure for '%s'.\n", argv[1]);
   }
   else {
      printf("dyn: drive=\"%s\", file=\"%s\"\n",
	  al_get_path_drive(dyn),
	  al_get_path_filename(dyn));
      al_free_path(dyn);
   }

   tostring = al_create_path(argv[1]);
   if (!tostring) {
      printf("Failed to create path structure for tostring test\n");
   }
   else {
      int i;

      printf("tostring: '%s'\n", al_path_cstr(tostring, '/'));
      printf("tostring: drive:'%s'", al_get_path_drive(tostring));
      printf(" dirs:");
      for (i = 0; i < al_get_path_num_components(tostring); i++) {
         if (i > 0)
            printf(",");
         printf(" '%s'", al_get_path_component(tostring, i));
      }
      printf(" filename:'%s'\n", al_get_path_filename(tostring));
      al_free_path(tostring);
   }

   /* FIXME: test out more of the al_path_ functions, ie: insert, remove,
    * concat, canonicalize, absolute, relative
    */

   dyn = al_create_path(argv[1]);
   if(dyn) {
      cloned = al_clone_path(dyn);
      if(cloned) {
         printf("dyn: '%s'\n", al_path_cstr(dyn, '/'));

         al_make_path_canonical(cloned);
         printf("can: '%s'\n", al_path_cstr(cloned, '/'));

         al_make_path_absolute(cloned);
         printf("abs: '%s'\n", al_path_cstr(cloned, '/'));

         al_free_path(dyn);
         al_free_path(cloned);
      }
      else {
         printf("failed to clone ALLEGRO_PATH :(\n");
         al_free_path(dyn);
      }
   }
   else {
      printf("failed to create new ALLEGRO_PATH for cloning...");
   }

   return 0;
}

END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
