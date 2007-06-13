#ifndef ALLEGRO_BITMAP_NEW_H
#define ALLEGRO_BITMAP_NEW_H

typedef struct AL_BITMAP AL_BITMAP;

/* This is just a proof-of-concept implementation. The basic idea is to leave 
 * things like pixel formats to Allegro usually. The user of course should be
 * able to specify things, as specific as they want. (E.g.
 * "i want a bitmap with 16-bit resolution per color channel"
 * "i want the best bitmap format which is HW accelerated"
 * "i want an RGBA bitmap which does not need to be HW accelerated"
 * )
 * For now, there is only a single flags parameter, which is passed on to the
 * underlying driver, but ignored otherwise.
 */


/*
 * Pixel formats
 */
#define AL_FORMAT_ANY            0
#define AL_FORMAT_ARGB_8888      1
#define AL_FORMAT_ARGB_4444      2
#define AL_FORMAT_RGB_888        3


AL_BITMAP *al_create_bitmap(int w, int h, int flags);
AL_BITMAP *al_create_memory_bitmap(int w, int h, int flags);
void al_destroy_memory_bitmap(AL_BITMAP *bitmap);
AL_BITMAP *al_load_bitmap(char const *filename, int flags);
void al_destroy_bitmap(AL_BITMAP *bitmap);
//void al_draw_bitmap(int flag, AL_BITMAP *bitmap, float x, float y);
void al_draw_sub_bitmap(AL_BITMAP *bitmap, float x, float y,
    float sx, float sy, float sw, float sh);
void al_blit(int flag, AL_BITMAP *src, AL_BITMAP *dest,
	float dx, float dy);
void al_blit_region(int flag, AL_BITMAP *src, float sx, float sy,
	AL_BITMAP *dest, float dx, float dy, float w, float h);
void al_blit_scaled(int flag,
	AL_BITMAP *src,	float sx, float sy, float sw, float sh,
	AL_BITMAP *dest, float dx, float dy, float dw, float dh);
void al_rotate_bitmap(int flag, AL_BITMAP *source,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float angle);
void al_rotate_scaled(int flag, AL_BITMAP *source,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float xscale, float yscale,
	float angle);
void al_blit_region_3(int flag,
	AL_BITMAP *source1, int source1_x, int source1_y,
	AL_BITMAP *source2, int source2_x, int source2_y,
	AL_BITMAP *dest, int dest_x, int dest_y,
	int dest_w, int dest_h);
void al_set_light_color(AL_BITMAP *bitmap, AL_COLOR *light_color);
void al_lock_bitmap(AL_BITMAP *bitmap, unsigned int x, unsigned int y,
	unsigned int width, unsigned int height);
void al_unlock_bitmap(AL_BITMAP *bitmap);
void al_put_pixel(int flag, AL_BITMAP *bitmap, int x, int y, AL_COLOR *color);

#endif
