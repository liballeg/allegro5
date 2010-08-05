#include <stdio.h>
#define ALLEGRO_USE_CONSOLE
#include "allegro5/allegro.h"

#include "common.c"

int main(void)
{
   ALLEGRO_PATH *path;

   /* defaults to allegro */
   al_set_org_name("liballeg.org");

   /* defaults to the exename, set it here to remove the .exe on windows */
   al_set_app_name("ex_get_path");
   
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   open_log();

   path = al_get_standard_path(ALLEGRO_PROGRAM_PATH);
   log_printf("PROGRAM_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_TEMP_PATH);
   log_printf("TEMP_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_SYSTEM_DATA_PATH);
   log_printf("SYSTEM_DATA_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_SYSTEM_SETTINGS_PATH);
   log_printf("SYSTEM_SETTINGS_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
   log_printf("USER_DATA_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
   log_printf("USER_SETTINGS_PATH: %s\n", al_path_cstr(path, '/'));

   path = al_get_standard_path(ALLEGRO_USER_HOME_PATH);
   log_printf("USER_HOME_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
   log_printf("EXENAME_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   close_log(true);
   return 0;
}


/* vim: set sts=3 sw=3 et: */
