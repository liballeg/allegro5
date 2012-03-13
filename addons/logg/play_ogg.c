#include <stdio.h>
#include "logg.h"

int main(int argc, char** argv)
{
	SAMPLE* s;
	int voice;

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

	s = logg_load(argv[1]);
	if (!s) {
		printf("Error loading %s\n", argv[1]);
		return 1;
	}

	voice = play_sample(s, 255, 128, 1000, 0);
	rest(s->len*1000/s->freq);
	destroy_sample(s);
	return 0;
}
END_OF_MAIN()
