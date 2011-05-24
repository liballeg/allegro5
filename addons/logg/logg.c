#include <string.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"

#include "loggint.h"

/* XXX requires testing */
#ifdef ALLEGRO_BIG_ENDIAN
	const int ENDIANNESS = 1;
#endif
#ifdef ALLEGRO_LITTLE_ENDIAN
	const int ENDIANNESS = 0;
#endif

static int logg_bufsize = 1024*64;

SAMPLE* logg_load(const char* filename)
{
	OggVorbis_File ovf;
	FILE* file;
	vorbis_info* vi;
	SAMPLE* samp;
	int numRead;
	int offset = 0;
	int bitstream;
	char *buf = malloc(logg_bufsize);

	file = fopen(filename, "rb");
	if (!file) {
		uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, "Unable to open file: %s", filename);
		free(buf);
		return 0;
	}

	if (ov_open_callbacks(file, &ovf, 0, 0, OV_CALLBACKS_DEFAULT) != 0) {
		strncpy(allegro_error, "ov_open_callbacks failed.", ALLEGRO_ERROR_SIZE);
		fclose(file);
		free(buf);
		return 0;
	}

	vi = ov_info(&ovf, -1);

	samp = (SAMPLE*)_al_malloc(sizeof(SAMPLE));
	if (!samp) {
		ov_clear(&ovf);
		free(buf);
		return 0;
	}

	samp->bits = 16;
	samp->stereo = vi->channels > 1 ? 1 : 0;
	samp->freq = vi->rate;
	samp->priority = 128;
	samp->len = ov_pcm_total(&ovf, -1);
	samp->loop_start = 0;
	samp->loop_end = samp->len;
	samp->data = _al_malloc(sizeof(unsigned short) * samp->len * 2);

	while ((numRead = ov_read(&ovf, buf, logg_bufsize,
				ENDIANNESS, 2, 0, &bitstream)) != 0) {
		memcpy((unsigned char*)samp->data+offset, buf, numRead);
		offset += numRead;
	}

	ov_clear(&ovf);
	free(buf);

	return samp;
}

int logg_get_buffer_size(void)
{
	return logg_bufsize;
}

void logg_set_buffer_size(int size)
{
	ASSERT(size > 0);
	logg_bufsize = size;
}

static int logg_open_file_for_streaming(LOGG_Stream* s)
{
	FILE* file;
	vorbis_info* vi;

	file = fopen(s->filename, "rb");
	if (!file) {
		uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, "Unable to open file: %s", s->filename);
		return 1;
	}

	if (ov_open_callbacks(file, &s->ovf, 0, 0, OV_CALLBACKS_DEFAULT) != 0) {
		strncpy(allegro_error, "ov_open_callbacks failed.", ALLEGRO_ERROR_SIZE);
		fclose(file);
		return 1;
	}

	vi = ov_info(&s->ovf, -1);

	s->bits = 16;
	s->stereo = vi->channels > 1 ? 1 : 0;
	s->freq = vi->rate;
	s->len = ov_pcm_total(&s->ovf, -1);

	return 0;
}

static int read_ogg_data(LOGG_Stream* s)
{
	int read = 0;
	int bitstream;

	int page = s->current_page;
	s->current_page++;
	s->current_page %= OGG_PAGES_TO_BUFFER;

	memset(s->buf[page], 0, logg_bufsize);

	while (read < logg_bufsize) {
		int thisRead = ov_read(&s->ovf, s->buf[page]+read,
				logg_bufsize-read,
				ENDIANNESS, 2, 0, &bitstream);
		if (thisRead == 0) {
			if (s->loop) {
				ov_clear(&s->ovf);
				if (logg_open_file_for_streaming(s)) {
					return -1;
				}
			}
			else {
				return read;
			}
		}
		read += thisRead;
	}

	return read;
}

static int logg_play_stream(LOGG_Stream* s)
{
	int len;
	int i;

	s->current_page = 0;
	s->playing_page = -1;

	len = logg_bufsize / (s->stereo ? 2 : 1)
		/ (s->bits / (sizeof(char)*8));

	s->audio_stream = play_audio_stream(len,
		       	s->bits, s->stereo,
			s->freq, s->volume, s->pan);

	if (!s->audio_stream) {
		return 1;
	}

	for (i = 0; i < OGG_PAGES_TO_BUFFER; i++) {
		s->buf[i] = malloc(logg_bufsize);
		if (!s->buf[i]) {
			logg_destroy_stream(s);
			return 1;
		}
		if (read_ogg_data(s) < 0) {
			return 1;
		}
	}

	return 0;
}

LOGG_Stream* logg_get_stream(const char* filename, int volume, int pan, int loop)
{
	LOGG_Stream* s = calloc(1, sizeof(LOGG_Stream));
	if (!s) {
		return 0;
	}

	s->filename = strdup(filename);

	if (!s->filename) {
		free(s);
		return 0;
	}

	if (logg_open_file_for_streaming(s)) {
		logg_destroy_stream(s);
		return 0;
	}

	s->volume = volume;
	s->pan = pan;
	s->loop = loop;

	if (logg_play_stream(s)) {
		logg_destroy_stream(s);
		return 0;
	}

	return s;
}

int logg_update_stream(LOGG_Stream* s)
{
	unsigned char* data = get_audio_stream_buffer(s->audio_stream);

	if (!data) {
		if (s->current_page != s->playing_page) {
			int read = read_ogg_data(s);
			if (read < logg_bufsize) {
				return 0;
			}
			else {
				return 1;
			}
		}
		else {
			return 1;
		}
	}

	s->playing_page++;
	s->playing_page %= OGG_PAGES_TO_BUFFER;
	memcpy(data, s->buf[s->playing_page], logg_bufsize);

	free_audio_stream_buffer(s->audio_stream);

	return 1;
}

void logg_stop_stream(LOGG_Stream* s)
{
	int i;

	stop_audio_stream(s->audio_stream);
	for (i = 0; i < OGG_PAGES_TO_BUFFER; i++) {
		free(s->buf[i]);
		s->buf[i] = 0;
	}
}

int logg_restart_stream(LOGG_Stream* s)
{
	return logg_play_stream(s);
}

void logg_destroy_stream(LOGG_Stream* s)
{
	int i;

	if (s->audio_stream) {
		stop_audio_stream(s->audio_stream);
	}
	ov_clear(&s->ovf);
	for (i = 0; i < OGG_PAGES_TO_BUFFER; i++) {
		if (s->buf[i]) {
			free(s->buf[i]);
		}
	}
	free(s->filename);
	free(s);
}

