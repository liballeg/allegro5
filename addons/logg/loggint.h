#ifndef LOGGINT_H
#define LOGGINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <allegro.h>
#include <vorbis/vorbisfile.h>

#define OGG_PAGES_TO_BUFFER 2

struct LOGG_Stream {
	char *buf[OGG_PAGES_TO_BUFFER];
	int current_page;
	int playing_page;
	AUDIOSTREAM* audio_stream;
	OggVorbis_File ovf;
	int bits;
	int stereo;
	int freq;
	int len;
	char* filename;
	int loop;
	int volume;
	int pan;
};

#include "logg.h"

#ifdef __cplusplus
}
#endif

#endif

