#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/fshook.h"

int main(int argc, char **argv)
{
	char buffer[1024];
	al_init();

	al_get_path(AL_PROGRAM_PATH, buffer, 1024);
	printf("AL_PROGRAM_PATH: %s\n", buffer);

	al_get_path(AL_TEMP_PATH, buffer, 1024);
	printf("AL_TEMP_PATH: %s\n", buffer);

	al_get_path(AL_SYSTEM_DATA_PATH, buffer, 1024);
	printf("AL_SYSTEM_DATA_PATH: %s\n", buffer);

	al_get_path(AL_USER_DATA_PATH, buffer, 1024);
	printf("AL_USER_DATA_PATH: %s\n", buffer);

	al_get_path(AL_USER_HOME_PATH, buffer, 1024);
	printf("AL_USER_HOME_PATH: %s\n", buffer);

	return 0;
}
END_OF_MAIN()
