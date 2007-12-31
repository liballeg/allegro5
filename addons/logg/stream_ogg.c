#include <stdio.h>

#include "logg.h"

int main(int argc, char** argv)
{
	allegro_init();
	install_sound(DIGI_AUTODETECT, MIDI_NONE, 0);
	install_timer();
	LOGG_Stream* s = logg_get_stream(argv[1], 255, 128, 1);
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
