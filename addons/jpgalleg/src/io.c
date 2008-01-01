/*
 *         __   _____    ______   ______   ___    ___
 *        /\ \ /\  _ `\ /\  ___\ /\  _  \ /\_ \  /\_ \
 *        \ \ \\ \ \L\ \\ \ \__/ \ \ \L\ \\//\ \ \//\ \      __     __
 *      __ \ \ \\ \  __| \ \ \  __\ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\
 *     /\ \_\/ / \ \ \/   \ \ \L\ \\ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \
 *     \ \____//  \ \_\    \ \____/ \ \_\ \_\/\____\/\____\ \____\ \____ \
 *      \/____/    \/_/     \/___/   \/_/\/_/\/____/\/____/\/____/\/___L\ \
 *                                                                  /\____/
 *                                                                  \_/__/
 *
 *      Version 2.6, by Angelo Mottola, 2000-2006
 *
 *      Input/output helper routines module.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */


#include <internal.h>


static int current_byte = 0;
static int bytes_read = 0;
static int chunk_len = 0;
static void *chunk = NULL;



/* _jpeg_getc:
 *  Reads a byte from the input stream.
 */
int
_jpeg_getc(void)
{
	bytes_read++;
	if (_jpeg_io.current_bit < 8) {
		if (*_jpeg_io.buffer == 0xff)
			_jpeg_io.buffer++;
		_jpeg_io.buffer++;
	}
	_jpeg_io.current_bit = 8;

	if (_jpeg_io.buffer >= _jpeg_io.buffer_end) {
		TRACE("Tried to read memory past buffer size");
		jpgalleg_error = JPG_ERROR_INPUT_BUFFER_TOO_SMALL;
		return -1;
	}
	return *_jpeg_io.buffer++;
}


/* _jpeg_putc:
 *  Writes a byte to the output stream.
 */
int
_jpeg_putc(int c)
{
	if (_jpeg_io.buffer >= _jpeg_io.buffer_end) {
		TRACE("Tried to write memory past buffer size");
		jpgalleg_error = JPG_ERROR_OUTPUT_BUFFER_TOO_SMALL;
		return -1;
	}
	*_jpeg_io.buffer++ = c;
	return 0;
}


/* _jpeg_getw:
 *  Reads a word from the input stream.
 */
int
_jpeg_getw(void)
{
	int result;
	
	result = _jpeg_getc() << 8;
	result |= _jpeg_getc();
	return result;
}


/* _jpeg_putw:
 *  Writes a word to the output stream.
 */
int
_jpeg_putw(int w)
{
	int result;
	
	result = _jpeg_putc((w >> 8) & 0xff);
	result |= _jpeg_putc(w & 0xff);
	return result;
}


/* _jpeg_get_bit:
 *  Reads a single bit from the input stream.
 */
INLINE int
_jpeg_get_bit(void)
{
	if (_jpeg_io.current_bit <= 0) {
		if (_jpeg_io.buffer >= _jpeg_io.buffer_end) {
			TRACE("Tried to read memory past buffer size");
			jpgalleg_error = JPG_ERROR_INPUT_BUFFER_TOO_SMALL;
			return -1;
		}
		if (*_jpeg_io.buffer == 0xff)
			/* Special encoding for 0xff, which in JPGs is encoded like 2 bytes:
			 * 0xff00. Here we skip the next byte (0x00)
			 */
			_jpeg_io.buffer++;
		_jpeg_io.buffer++;
		_jpeg_io.current_bit = 8;
	}
	_jpeg_io.current_bit--;
	return (*_jpeg_io.buffer >> _jpeg_io.current_bit) & 0x1;
}


/* _jpeg_put_bit:
 *  Writes a single bit to the output stream.
 */
int
_jpeg_put_bit(int bit)
{
	current_byte |= (bit << _jpeg_io.current_bit);
	_jpeg_io.current_bit--;
	if (_jpeg_io.current_bit < 0) {
		if (_jpeg_putc(current_byte))
			return -1;
		if (current_byte == 0xff)
			_jpeg_putc(0);
		_jpeg_io.current_bit = 7;
		current_byte = 0;
	}
	return 0;
}


/* _jpeg_flush_bits:
 *  Flushes the current byte by filling unset bits with 1.
 */
void
_jpeg_flush_bits(void)
{
	while (_jpeg_io.current_bit < 7)
		_jpeg_put_bit(1);
}


/* _jpeg_open_chunk:
 *  Opens a chunk for reading.
 */
void
_jpeg_open_chunk(void)
{
	bytes_read = 0;
	chunk_len = _jpeg_getw();
	_jpeg_io.current_bit = 8;
}


/* _jpeg_close_chunk:
 *  Closes the chunk being read, eventually skipping unused bytes.
 */
void
_jpeg_close_chunk(void)
{
	while (bytes_read < chunk_len)
		_jpeg_getc();
}


/* _jpeg_eoc:
 *  Returns true if the end of chunk being read is reached, otherwise false.
 */
int
_jpeg_eoc(void)
{
	return (bytes_read < chunk_len) ? FALSE : TRUE;
}


/* _jpeg_new_chunk:
 *  Creates a new chunk for writing.
 */
void
_jpeg_new_chunk(int type)
{
	char *c = (char *)malloc(65536);
	
	c[0] = 0xff;
	c[1] = type;
	chunk_len = 2;
	chunk = c;
}


/* _jpeg_write_chunk:
 *  Writes the current chunk to the output stream.
 */
void
_jpeg_write_chunk(void)
{
	unsigned char *c;
	
	if (!chunk)
		return;
	c = (unsigned char *)chunk;
	c[2] = (chunk_len >> 8) & 0xff;
	c[3] = chunk_len & 0xff;
	for (chunk_len += 2; chunk_len; chunk_len--)
		_jpeg_putc(*c++);
	free(chunk);
	chunk = NULL;
	_jpeg_io.current_bit = 7;
	current_byte = 0;
}


/* _jpeg_chunk_putc:
 *  Writes a byte to the current chunk.
 */
void
_jpeg_chunk_putc(int c)
{
	char *p = (char *)chunk + chunk_len + 2;
	
	*p = c;
	chunk_len++;
}


/* _jpeg_chunk_putw:
 *  Writes a word to the current chunk.
 */
void
_jpeg_chunk_putw(int w)
{
	_jpeg_chunk_putc((w >> 8) & 0xff);
	_jpeg_chunk_putc(w & 0xff);
}


/* _jpeg_chunk_puts:
 *  Writes a stream of bytes to the current chunk.
 */
void
_jpeg_chunk_puts(unsigned char *s, int size)
{
	for (; size; size--)
		_jpeg_chunk_putc(*s++);
}
