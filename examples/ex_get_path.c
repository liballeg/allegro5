#include <stdio.h>
#define ALLEGRO_USE_CONSOLE
#include "allegro5/allegro5.h"

#include "common.c"

int main(void)
{
   ALLEGRO_PATH *path;

   /* defaults to allegro */
   al_set_orgname("liballeg.org");

   /* defaults to the exename, set it here to remove the .exe on windows */
   al_set_appname("ex_get_path");
   
   al_init();

   path = al_get_standard_path(ALLEGRO_PROGRAM_PATH);
   printf("PROGRAM_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_TEMP_PATH);
   printf("TEMP_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_SYSTEM_DATA_PATH);
   printf("SYSTEM_DATA_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_SYSTEM_SETTINGS_PATH);
   printf("SYSTEM_SETTINGS_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
   printf("USER_DATA_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
   printf("USER_SETTINGS_PATH: %s\n", al_path_cstr(path, '/'));

   path = al_get_standard_path(ALLEGRO_USER_HOME_PATH);
   printf("USER_HOME_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
   printf("EXENAME_PATH: %s\n", al_path_cstr(path, '/'));
   al_destroy_path(path);

   return 0;
}


/* vim: set sts=3 sw=3 et: */
