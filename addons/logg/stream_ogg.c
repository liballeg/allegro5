#include <stdio.h>

#include "logg.h"

int main(int argc, char** argv)
{
	LOGG_Stream* s;

	if (argc != 2) {
		printf("usage: %s file.ogg\n", argv[0]);
		return 0;
	}

	if (allegro_init() != 0) {
		printf("Error initialising Allegro.\n");
		return 1;
	}

	if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0) != 0) {
		printf("Error initialising sound: %s\n", allegro_error);
		return 1;
	}
	install_timer();

	s = logg_get_stream(argv[1], 255, 128, 1);
	if (!s) {
		printf("Error getting ogg stream.\n");
		return 1;
	}
	while (logg_update_stream(s)) {
		rest(1);
	}
	logg_destroy_stream(s);
	return 0;
}
END_OF_MAIN()
