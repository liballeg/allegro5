#ifndef __al_included_allegro5_bitmap_h
#define __al_included_allegro5_bitmap_h

#include "allegro5/color.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: A5O_BITMAP
 */
typedef struct A5O_BITMAP A5O_BITMAP;

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
/* Enum: A5O_BITMAP_WRAP
 */
typedef enum A5O_BITMAP_WRAP {
   A5O_BITMAP_WRAP_DEFAULT = 0,
   A5O_BITMAP_WRAP_REPEAT = 1,
   A5O_BITMAP_WRAP_CLAMP = 2,
   A5O_BITMAP_WRAP_MIRROR = 3,
} A5O_BITMAP_WRAP;
#endif

/*
 * Bitmap flags
 */
enum {
   A5O_MEMORY_BITMAP            = 0x0001,
   _A5O_KEEP_BITMAP_FORMAT      = 0x0002,	/* now a bitmap loader flag */
   A5O_FORCE_LOCKING            = 0x0004,	/* no longer honoured */
   A5O_NO_PRESERVE_TEXTURE      = 0x0008,
   _A5O_ALPHA_TEST              = 0x0010,   /* now a render state flag */
   _A5O_INTERNAL_OPENGL         = 0x0020,
   A5O_MIN_LINEAR               = 0x0040,
   A5O_MAG_LINEAR               = 0x0080,
   A5O_MIPMAP                   = 0x0100,
   _A5O_NO_PREMULTIPLIED_ALPHA  = 0x0200,	/* now a bitmap loader flag */
   A5O_VIDEO_BITMAP             = 0x0400,
   A5O_CONVERT_BITMAP           = 0x1000
};


AL_FUNC(void, al_set_new_bitmap_format, (int format));
AL_FUNC(void, al_set_new_bitmap_flags, (int flags));
AL_FUNC(int, al_get_new_bitmap_format, (void));
AL_FUNC(int, al_get_new_bitmap_flags, (void));
AL_FUNC(void, al_add_new_bitmap_flag, (int flag));

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(int, al_get_new_bitmap_depth, (void));
AL_FUNC(void, al_set_new_bitmap_depth, (int depth));
AL_FUNC(int, al_get_new_bitmap_samples, (void));
AL_FUNC(void, al_set_new_bitmap_samples, (int samples));
AL_FUNC(void, al_get_new_bitmap_wrap, (A5O_BITMAP_WRAP *u, A5O_BITMAP_WRAP *v));
AL_FUNC(void, al_set_new_bitmap_wrap, (A5O_BITMAP_WRAP u, A5O_BITMAP_WRAP v));
#endif

AL_FUNC(int, al_get_bitmap_width, (A5O_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_height, (A5O_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_format, (A5O_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_flags, (A5O_BITMAP *bitmap));

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(int, al_get_bitmap_depth, (A5O_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_samples, (A5O_BITMAP *bitmap));
#endif

AL_FUNC(A5O_BITMAP*, al_create_bitmap, (int w, int h));
AL_FUNC(void, al_destroy_bitmap, (A5O_BITMAP *bitmap));

AL_FUNC(void, al_put_pixel, (int x, int y, A5O_COLOR color));
AL_FUNC(void, al_put_blended_pixel, (int x, int y, A5O_COLOR color));
AL_FUNC(A5O_COLOR, al_get_pixel, (A5O_BITMAP *bitmap, int x, int y));

/* Masking */
AL_FUNC(void, al_convert_mask_to_alpha, (A5O_BITMAP *bitmap, A5O_COLOR mask_color));

/* Blending */
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(A5O_COLOR, al_get_bitmap_blend_color, (void));
AL_FUNC(void, al_get_bitmap_blender, (int *op, int *src, int *dst));
AL_FUNC(void, al_get_separate_bitmap_blender, (int *op, int *src, int *dst, int *alpha_op, int *alpha_src, int *alpha_dst));
AL_FUNC(void, al_set_bitmap_blend_color, (A5O_COLOR color));
AL_FUNC(void, al_set_bitmap_blender, (int op, int src, int dst));
AL_FUNC(void, al_set_separate_bitmap_blender, (int op, int src, int dst, int alpha_op, int alpha_src, int alpha_dst));
AL_FUNC(void, al_reset_bitmap_blender, (void));
#endif

/* Clipping */
AL_FUNC(void, al_set_clipping_rectangle, (int x, int y, int width, int height));
AL_FUNC(void, al_reset_clipping_rectangle, (void));
AL_FUNC(void, al_get_clipping_rectangle, (int *x, int *y, int *w, int *h));

/* Sub bitmaps */
AL_FUNC(A5O_BITMAP *, al_create_sub_bitmap, (A5O_BITMAP *parent, int x, int y, int w, int h));
AL_FUNC(bool, al_is_sub_bitmap, (A5O_BITMAP *bitmap));
AL_FUNC(A5O_BITMAP *, al_get_parent_bitmap, (A5O_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_x, (A5O_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_y, (A5O_BITMAP *bitmap));
AL_FUNC(void, al_reparent_bitmap, (A5O_BITMAP *bitmap,
   A5O_BITMAP *parent, int x, int y, int w, int h));

/* Miscellaneous */
AL_FUNC(A5O_BITMAP *, al_clone_bitmap, (A5O_BITMAP *bitmap));
AL_FUNC(void, al_convert_bitmap, (A5O_BITMAP *bitmap));
AL_FUNC(void, al_convert_memory_bitmaps, (void));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(void, al_backup_dirty_bitmap, (A5O_BITMAP *bitmap));
#endif

#ifdef __cplusplus
   }
#endif

#endif
