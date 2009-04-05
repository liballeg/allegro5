#include <stdio.h>
#define ALLEGRO_USE_CONSOLE
#include "allegro5/allegro5.h"

int main(void)
{
   ALLEGRO_PATH *path;

   /* defaults to allegro */
   al_set_orgname("liballeg.org");

   /* defaults to the exename, set it here to remove the .exe on windows */
   al_set_appname("ex_get_path");
   
   al_init();

   path = al_get_path(AL_PROGRAM_PATH);
   printf("AL_PROGRAM_PATH: %s\n", al_path_to_string(path, '/'));
   al_path_free(path);

   path = al_get_path(AL_TEMP_PATH);
   printf("AL_TEMP_PATH: %s\n", al_path_to_string(path, '/'));
   al_path_free(path);

   path = al_get_path(AL_SYSTEM_DATA_PATH);
   printf("AL_SYSTEM_DATA_PATH: %s\n", al_path_to_string(path, '/'));
   al_path_free(path);

   path = al_get_path(AL_SYSTEM_SETTINGS_PATH);
   printf("AL_SYSTEM_SETTINGS_PATH: %s\n", al_path_to_string(path, '/'));
   al_path_free(path);

   path = al_get_path(AL_USER_DATA_PATH);
   printf("AL_USER_DATA_PATH: %s\n", al_path_to_string(path, '/'));
   al_path_free(path);

   path = al_get_path(AL_USER_SETTINGS_PATH);
   printf("AL_USER_SETTINGS_PATH: %s\n", al_path_to_string(path, '/'));

   path = al_get_path(AL_USER_HOME_PATH);
   printf("AL_USER_HOME_PATH: %s\n", al_path_to_string(path, '/'));
   al_path_free(path);

   path = al_get_path(AL_EXENAME_PATH);
   printf("AL_EXENAME_PATH: %s\n", al_path_to_string(path, '/'));
   al_path_free(path);

   return 0;
}

END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
