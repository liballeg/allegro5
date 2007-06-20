#include <string.h>
#include "allegro.h"
#include "internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"


static int _bitmap_format = 0;
static int _bitmap_flags = 0;
/* For pushing/popping bitmap parameters */
static int _bitmap_format_backup;
static int _bitmap_flags_backup;

static AL_COLOR _mask_color = { { 0, 0, 0, 0 } };


// FIXME: does not work if the areas overlap or anything is outside
void _al_blit_memory_bitmap(AL_BITMAP *source, AL_BITMAP *dest,
   int source_x, int source_y, int dest_x, int dest_y, int w, int h)
{
	_al_convert_bitmap_data(
		source->memory,
		source->format, _al_pixel_size(source->format)*source->w,
		dest->memory,
		dest->format, _al_pixel_size(dest->format)*dest->w,
		source_x, source_y,
		dest_x, dest_y,
		w, h);
	
	/*
   int y;
   unsigned char *sptr = source->memory + (source_y * source->w + source_x) * 4;
   unsigned char *dptr = dest->memory + (dest_y * dest->w + dest_x) * 4;
   for (y = 0; y < h; y++)
   {
      memcpy(dptr, sptr, w * 4);
      sptr += source->w * 4;
      dptr += dest->w * 4;
   }
   */
}

void al_set_bitmap_parameters(int format, int flags)
{
	_bitmap_format = format;
	_bitmap_flags = flags;
}

void al_get_bitmap_parameters(int *format, int *flags)
{
	if (format)
		*format = _bitmap_format;
	if (flags)
		*flags = _bitmap_flags;
}

/* Creates a memory bitmap. A memory bitmap can only be drawn to other memory
 * bitmaps, not to a display.
 */
static AL_BITMAP *_al_create_memory_bitmap(int w, int h)
{
   AL_BITMAP *bitmap = _AL_MALLOC(sizeof *bitmap);
   memset(bitmap, 0, sizeof *bitmap);
   bitmap->format = _bitmap_format;
   bitmap->flags = _bitmap_flags;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->display = NULL;
   bitmap->locked = false;
   // FIXME: Of course, we do need to handle all the possible different formats,
   // this will easily fill up its own file of 1000 lines, but for now,
   // RGBA with 8-bit per component is hardcoded.
   bitmap->memory = _AL_MALLOC(w * h * _al_pixel_size(_bitmap_format));
   memset(bitmap->memory, 0, w * h * _al_pixel_size(_bitmap_format));
   return bitmap;
}

static void _al_destroy_memory_bitmap(AL_BITMAP *bmp)
{
   _AL_FREE(bmp->memory);
   _AL_FREE(bmp);
}

/* Creates an empty display bitmap. A display bitmap can only be drawn to a
 * display.
 */
AL_BITMAP *al_create_bitmap(int w, int h)
{
   AL_BITMAP *bitmap;
   
   if (_bitmap_flags & AL_MEMORY_BITMAP) {
   	return _al_create_memory_bitmap(w, h);
   }

   /* Else it's a display bitmap */

   bitmap = _al_current_display->vt->create_bitmap(_al_current_display, w, h);

   bitmap->display = _al_current_display;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->locked = false;

   if (!bitmap->memory) {
   	bitmap->memory = _AL_MALLOC(w * h * _al_pixel_size(bitmap->format));
        memset(bitmap->memory, 0, w * h * _al_pixel_size(bitmap->format));
   }

   bitmap->vt->upload_bitmap(bitmap, 0, 0, w, h);

   return bitmap;
}

/* Destroy a (memory or display) bitmap. The passed pointer will be invalid
 * afterwards, so best set it to NULL.
 */
void al_destroy_bitmap(AL_BITMAP *bitmap)
{
   if (bitmap->flags & AL_MEMORY_BITMAP) {
   	_al_destroy_memory_bitmap(bitmap);
	return;
   }

   /* Else it's a display bitmap */

   if (bitmap->locked)
      al_unlock_bitmap(bitmap);

   if (bitmap->vt)
      bitmap->vt->destroy_bitmap(bitmap);

   if (bitmap->memory)
      _AL_FREE(bitmap->memory);

   _AL_FREE(bitmap);
}

/* TODO
 * This will load a bitmap from a file into a memory bitmap, keeping it in the
 * same format as on disk. That is, a paletted image will be stored as indices
 * plus the palette, 16-bit color channels stay as 16-bit, and so on. If
 * a format is not supported by Allegro, the closest one will be used. (E.g. we
 * likely will throw away things like frame timings or gamma correction.)
 */
static AL_BITMAP *_al_load_memory_bitmap(char const *filename)
{
   // TODO:
   // The idea is, load_bitmap returns a memory representation of the bitmap,
   // maybe in fixed RGBA or whatever format is set as default (e.g. for HDR
   // images, should not reduce to 8-bit channels).
   // Then, it is converted to a display specific bitmap, which can be used
   // for blitting to the current display.
   set_color_conversion(COLORCONV_NONE);
   // FIXME: should not use the 4.2 function here of course
   PALETTE pal;
   BITMAP *file_data = load_bitmap(filename, pal);
   select_palette(pal);
   if (!file_data)
      return NULL;
   AL_BITMAP *bitmap = al_create_bitmap(file_data->w, file_data->h);

   _al_convert_compat_bitmap(
   	file_data,
	bitmap->memory, bitmap->format, _al_pixel_size(bitmap->format)*bitmap->w,
	0, 0, 0, 0,
	file_data->w, file_data->h);


   /*
   int x, y;
   int alpha = _bitmap_has_alpha(file_data);
   unsigned char *ptr = bitmap->memory;
   for (y = 0; y < file_data->h; y++) {
      for (x = 0; x < file_data->w; x++) {
         int c = getpixel(file_data, x, y);
         *(ptr++) = getr(c);
         *(ptr++) = getg(c);
         *(ptr++) = getb(c);
         *(ptr++) = alpha ? geta(c) : 255;
      }
   }
   */

   destroy_bitmap(file_data);
   return bitmap;
}

/* Load a bitmap from a file into a display bitmap, ready to be drawn.
 */
AL_BITMAP *al_load_bitmap(char const *filename)
{
   AL_BITMAP *bitmap;
   
   if (_bitmap_flags & AL_MEMORY_BITMAP) {
   	return _al_load_memory_bitmap(filename);
   }

   /* Else it's a display bitmap */

   bitmap = _al_load_memory_bitmap(filename);

   bitmap->vt->upload_bitmap(bitmap, 0, 0, bitmap->w, bitmap->h);

   return bitmap;
}

/*
 * Returns true if the bitmap can be drawn to the current display.
 */
bool al_is_compatible_bitmap(AL_BITMAP *bitmap)
{
	/* FIXME */
	return true;
}

void al_draw_bitmap(AL_BITMAP *bitmap, float x, float y, int flags)
{
   if (al_is_compatible_bitmap(bitmap))
      bitmap->vt->draw_bitmap(bitmap, x, y, flags);
}


#if 0
/*
 * Draw an entire bitmap to another. Use NULL for the destination
 * bitmap to draw to the current display.
 */
void al_blit(int flag, AL_BITMAP *src, AL_BITMAP *dest, float dx, float dy)
{
	src->vt->blit(flag, src, dest, dx, dy);
}

/*
 * Draw a rectangle of one bitmap to another. Use NULL for the
 * destination bitmap to draw to the current display.
 */
void al_blit_region(int flag, AL_BITMAP *src, float sx, float sy,
	AL_BITMAP *dest, float dx, float dy, float w, float h)
{
	src->vt->blit_region(flag, src, sx, sy, dest, dx, dy, w, h);
}

/*
 * Draw a rectangle of one bitmap to another. Use NULL for the
 * destination bitmap to draw to the current display.
 */
void al_blit_scaled(int flag,
	AL_BITMAP *src,	float sx, float sy, float sw, float sh,
	AL_BITMAP *dest, float dx, float dy, float dw, float dh)
{
	src->vt->blit_scaled(flag, src, sx, sy, sw, sh, dest, dx, dy, dw, dh);
}

void al_rotate_bitmap(int flag,
	AL_BITMAP *src,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float angle)
{
	src->vt->rotate_bitmap(flag, src, source_center_x, source_center_y, dest, dest_x, dest_y, angle);
}

void al_rotate_scaled(int flag,
	AL_BITMAP *src,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float xscale, float yscale,
	float angle)
{
	src->vt->rotate_scaled(flag, src, source_center_x, source_center_y, dest, dest_x, dest_y, xscale, yscale, angle);
}

void al_draw_sub_bitmap(AL_BITMAP *bitmap, float x, float y,
    float sx, float sy, float sw, float sh)
{
   // TODO
}

void al_set_light_color(AL_BITMAP *bitmap, AL_COLOR *light_color)
{
	memcpy(&bitmap->light_color, light_color, sizeof(AL_COLOR));
}
#endif

AL_LOCKED_RECTANGLE *al_lock_bitmap_region(AL_BITMAP *bitmap,
	int x, int y,
	int width, int height,
	AL_LOCKED_RECTANGLE *locked_rectangle,
	int flags)
{
	if (bitmap->locked)
		return NULL;

	bitmap->locked = true;
	bitmap->lock_x = x;
	bitmap->lock_y = y;
	bitmap->lock_w = width;
	bitmap->lock_h = height;
	bitmap->lock_flags = flags;

	if (bitmap->flags & AL_MEMORY_BITMAP || bitmap->flags & AL_SYNC_MEMORY_COPY) {
		locked_rectangle->data = bitmap->memory+(bitmap->w*y+x)*_al_pixel_size(bitmap->format);
		locked_rectangle->format = bitmap->format;
		locked_rectangle->pitch = bitmap->w*_al_pixel_size(bitmap->format);
	}
	else {
		locked_rectangle = bitmap->vt->lock_region(bitmap,
			x, y, width, height,
			locked_rectangle,
			flags);
	}

	memcpy(&bitmap->locked_rectangle, locked_rectangle, sizeof(AL_LOCKED_RECTANGLE));

	return locked_rectangle;
}

AL_LOCKED_RECTANGLE *al_lock_bitmap(AL_BITMAP *bitmap,
	AL_LOCKED_RECTANGLE *locked_rectangle,
	int flags)
{
	return al_lock_bitmap_region(bitmap, 0, 0, bitmap->w, bitmap->h,
		locked_rectangle, flags);
}

void al_unlock_bitmap(AL_BITMAP *bitmap)
{
	if (bitmap->flags & AL_SYNC_MEMORY_COPY && !(bitmap->flags & AL_MEMORY_BITMAP)) {
		bitmap->vt->upload_bitmap(bitmap,
			bitmap->lock_x,
			bitmap->lock_y,
			bitmap->lock_w,
			bitmap->lock_h);
	}
	else if (!(bitmap->flags & AL_MEMORY_BITMAP)) {
		bitmap->vt->unlock_region(bitmap);
	}

	bitmap->locked = false;
}

AL_COLOR *al_get_mask_color(AL_COLOR *color)
{
	memcpy(color, &_mask_color, sizeof(AL_COLOR));
	return color;
}

void al_set_mask_color(AL_COLOR *color)
{
	memcpy(&_mask_color, color, sizeof(AL_COLOR));
}

void al_convert_mask_to_alpha(AL_BITMAP *bitmap, AL_COLOR *mask_color)
{
	AL_LOCKED_RECTANGLE lr;
	int x, y;
	AL_COLOR pixel;
	AL_COLOR alpha_pixel;

	if (!al_lock_bitmap(bitmap, &lr, 0)) {
		TRACE("al_convert_mask_to_alpha: Couldn't lock bitmap.\n");
		return;
	}

	al_map_rgba(bitmap, &alpha_pixel, 0, 0, 0, 0);

	for (y = 0; y < bitmap->h; y++) {
		for (x = 0; x < bitmap->w; x++) {
			al_get_pixel(bitmap, x, y, &pixel);
			if (memcmp(&pixel, mask_color, sizeof(AL_COLOR)) == 0) {
				al_put_pixel(bitmap, x, y, &alpha_pixel);
			}
		}
	}

	al_unlock_bitmap(bitmap);
}

void _al_push_bitmap_parameters()
{
	_bitmap_format_backup = _bitmap_format;
	_bitmap_flags_backup = _bitmap_flags;
}

void _al_pop_bitmap_parameters()
{
	_bitmap_format = _bitmap_format_backup;
	_bitmap_flags = _bitmap_flags_backup;
}
