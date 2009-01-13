#include <stdio.h>
#define ALLEGRO_USE_CONSOLE
#include "allegro5/allegro5.h"

int main(void)
{
   char buffer[1024];

   /* defaults to allegro */
   al_set_orgname("liballeg.org");

   /* defaults to the exename, set it here to remove the .exe on windows */
   al_set_appname("ex_get_path");
   
   al_init();

   al_get_path(AL_PROGRAM_PATH, buffer, 1024);
   printf("AL_PROGRAM_PATH: %s\n", buffer);

   al_get_path(AL_TEMP_PATH, buffer, 1024);
   printf("AL_TEMP_PATH: %s\n", buffer);

   al_get_path(AL_SYSTEM_DATA_PATH, buffer, 1024);
   printf("AL_SYSTEM_DATA_PATH: %s\n", buffer);

   al_get_path(AL_SYSTEM_SETTINGS_PATH, buffer, 1024);
   printf("AL_SYSTEM_SETTINGS_PATH: %s\n", buffer);

   al_get_path(AL_USER_DATA_PATH, buffer, 1024);
   printf("AL_USER_DATA_PATH: %s\n", buffer);

   al_get_path(AL_USER_SETTINGS_PATH, buffer, 1024);
   printf("AL_USER_SETTINGS_PATH: %s\n", buffer);

   al_get_path(AL_USER_HOME_PATH, buffer, 1024);
   printf("AL_USER_HOME_PATH: %s\n", buffer);

   al_get_path(AL_EXENAME_PATH, buffer, 1024);
   printf("AL_EXENAME_PATH: %s\n", buffer);

   return 0;
}

END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
