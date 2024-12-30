#include <stdio.h>
#include <allegro5/allegro.h>

#include "common.c"

int main(int argc, char **argv)
{
   ALLEGRO_PATH *dyn = NULL;
   ALLEGRO_PATH *tostring = NULL;
   ALLEGRO_PATH *cloned = NULL;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   open_log();

   if (argc < 2) {
      ALLEGRO_PATH *exe = al_create_path(argv[0]);
      if (exe) {
         log_printf("This example needs to be run from the command line.\nUsage1: %s <path>\n", al_get_path_filename(exe));
         al_destroy_path(exe);
      }
      else {
         log_printf("This example needs to be run from the command line.\nUsage2: %s <path>\n", argv[0]);
      }
      goto done;
   }

   dyn = al_create_path(argv[1]);
   if (!dyn) {
      log_printf("Failed to create path structure for '%s'.\n", argv[1]);
   }
   else {
      log_printf("dyn: drive=\"%s\", file=\"%s\"\n",
      al_get_path_drive(dyn),
      al_get_path_filename(dyn));
      al_destroy_path(dyn);
   }

   tostring = al_create_path(argv[1]);
   if (!tostring) {
      log_printf("Failed to create path structure for tostring test\n");
   }
   else {
      int i;

      log_printf("tostring: '%s'\n", al_path_cstr(tostring, '/'));
      log_printf("tostring: drive:'%s'", al_get_path_drive(tostring));
      log_printf(" dirs:");
      for (i = 0; i < al_get_path_num_components(tostring); i++) {
         if (i > 0)
            log_printf(",");
         log_printf(" '%s'", al_get_path_component(tostring, i));
      }
      log_printf(" filename:'%s'\n", al_get_path_filename(tostring));
      al_destroy_path(tostring);
   }

   /* FIXME: test out more of the al_path_ functions, ie: insert, remove,
    * concat, relative
    */

   dyn = al_create_path(argv[1]);
   if(dyn) {
      cloned = al_clone_path(dyn);
      if(cloned) {
         log_printf("dyn: '%s'\n", al_path_cstr(dyn, '/'));

         al_make_path_canonical(cloned);
         log_printf("can: '%s'\n", al_path_cstr(cloned, '/'));

         al_destroy_path(dyn);
         al_destroy_path(cloned);
      }
      else {
         log_printf("failed to clone ALLEGRO_PATH :(\n");
         al_destroy_path(dyn);
      }
   }
   else {
      log_printf("failed to create new ALLEGRO_PATH for cloning...");
   }

done:

   close_log(true);
   return 0;
}


/* vim: set sts=3 sw=3 et: */
