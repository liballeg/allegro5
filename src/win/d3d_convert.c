#include "allegro.h"
#include "allegro/bitmap_new.h"
#include "internal/aintern.h"
#include "internal/aintern_bitmap.h"


/* Pixel conversions */
#define AL_CONVERT_ARGB_8888_TO_ARGB_4444(p) \
	(((p & 0xFF000000) >> 16) | \
 	 ((p & 0x00FF0000) >> 12) | \
	 ((p & 0x0000FF00) >> 8) | \
	 ((p & 0x000000FF) >> 4))

#define AL_CONVERT_ARGB_8888_TO_RGB_888(p) \
	(p | 0xFF000000)

#define AL_CONVERT_ARGB_4444_TO_ARGB_8888(p) \
	(((p & 0xF000) << 16) | (0x0F000000) | /* make sure alpha remains full */ \
	 ((p & 0x0F00) << 12) | \
	 ((p & 0x00F0) << 8) | \
	 ((p & 0x000F) << 4))

#define AL_CONVERT_ARGB_4444_TO_RGB_888(p) \
	((0xFF000000) | \
	 ((p & 0x0F00) << 12) | \
	 ((p & 0x00F0) << 8) | \
	 ((p & 0x000F) << 4))

#define AL_CONVERT_RGB_888_TO_ARGB_8888(p) \
	(p | 0xFF000000)

#define AL_CONVERT_RGB_888_TO_ARGB_4444(p) \
	((0xF000) | \
 	 ((p & 0x00FF0000) >> 12) | \
	 ((p & 0x0000FF00) >> 8) | \
	 ((p & 0x000000FF) >> 4))


/* Fast copy for when pixel formats match */
#define FAST_CONVERT(src, spitch, dst, dpitch, size, x, y, w, h) \
	void *sptr = (void *)(src + y*spitch + x*size); \
	void *dptr = (void *)(dst + y*dpitch + x*size); \
	int numbytes = w * size; \
	unsigned int y; \
	for (y = 0; y < h; y++) { \
		memcpy(dptr, sptr, numbytes); \
		sptr += spitch; \
		sptr += dpitch; \
	}

/* Copy with conversion */
#define DO_CONVERT(convert, src, stype, ssize, spitch, get, \
			dst, dtype, dsize, dpitch, set, \
			x, y, w, h) \
	stype *sstart = (stype *)(src + y*spitch + x*ssize); \
	stype *sptr; \
	dtype *dstart = (dtype *)(dst + y*dpitch + x*dsize); \
	dtype *dptr; \
	stype *send; \
	unsigned int y; \
	for (y = 0; y < h; y++) { \
		send = sstart + w * ssize; \
		for (sptr = sstart, dptr = dstart; sptr < send; sptr += ssize, dptr += dsize) { \
			set(dptr, convert(get(sptr))); \
		} \
		sstart += spitch; \
		dstart += dpitch; \
	}


void _al_convert_bitmap_data(void *src, int src_format, int src_pitch,
	void *dst, int dst_format, int dst_pitch,
	unsigned int x, unsigned int y,
	unsigned int width, unsigned int height)
{
	if (src_format == AL_FORMAT_ARGB_8888) {
		if (dst_format == AL_FORMAT_ARGB_8888) {
			FAST_CONVERT(src, src_pitch,
				dst, dst_pitch,
				4, x, y, width, height);
		}
		else if (dst_format == AL_FORMAT_ARGB_4444) {
			DO_CONVERT(AL_CONVERT_ARGB_8888_TO_ARGB_4444,
				   src, uint32_t, 4, src_pitch, *,
				   dst, uint16_t, 2, dst_pitch, bmp_write16,
				   x, y, width, height);
		}
		else if (dst_format == AL_FORMAT_RGB_888) {
			DO_CONVERT(AL_CONVERT_ARGB_8888_TO_RGB_888,
				   src, uint32_t, 4, src_pitch, *,
				   dst, unsigned char, 3, dst_pitch, WRITE3BYTES,
				   x, y, width, height);
		}
	}
	else if (src_format == AL_FORMAT_ARGB_4444) {
		if (dst_format == AL_FORMAT_ARGB_4444) {
			FAST_CONVERT(src, src_pitch,
				dst, dst_pitch,
				2, x, y, width, height);
		}
		else if (dst_format == AL_FORMAT_ARGB_8888) {
			DO_CONVERT(AL_CONVERT_ARGB_4444_TO_ARGB_8888,
				   src, uint16_t, 2, src_pitch, *,
				   dst, uint32_t, 4, dst_pitch, bmp_write32,
				   x, y, width, height);
		}
		else if (dst_format == AL_FORMAT_RGB_888) {
			DO_CONVERT(AL_CONVERT_ARGB_4444_TO_RGB_888,
				   src, uint16_t, 2, src_pitch, *,
				   dst, unsigned char, 3, dst_pitch, WRITE3BYTES,
				   x, y, width, height);
		}
	}
	else if (src_format == AL_FORMAT_RGB_888) {
		if (dst_format == AL_FORMAT_RGB_888) {
			FAST_CONVERT(src, src_pitch,
				dst, dst_pitch,
				3, x, y, width, height);
		}
		else if (dst_format == AL_FORMAT_ARGB_8888) {
			DO_CONVERT(AL_CONVERT_RGB_888_TO_ARGB_8888,
				   src, unsigned char, 3, src_pitch, READ3BYTES,
				   dst, uint32_t, 4, dst_pitch, bmp_write32,
				   x, y, width, height);
		}
		else if (dst_format == AL_FORMAT_ARGB_4444) {
			DO_CONVERT(AL_CONVERT_RGB_888_TO_ARGB_4444,
				   src, unsigned char, 3, src_pitch, READ3BYTES,
				   dst, uint16_t, 2, dst_pitch, bmp_write16,
				   x, y, width, height);
		}
	}
}


