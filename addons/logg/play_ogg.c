#include "logg.h"

int main(int argc, char** argv)
{
	allegro_init();
	install_sound(DIGI_AUTODETECT, MIDI_NONE, 0);
	install_timer();
	SAMPLE* s = logg_load(argv[1]);
	int voice = play_sample(s, 255, 128, 1000, 0);
	rest(s->len*1000/s->freq);
	destroy_sample(s);
	return 0;
}
END_OF_MAIN()
