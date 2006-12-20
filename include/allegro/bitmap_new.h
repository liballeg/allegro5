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

AL_BITMAP *al_create_bitmap(int w, int h, int flags);
AL_BITMAP *al_create_memory_bitmap(int w, int h, int flags);
AL_BITMAP *al_load_bitmap(char const *filename, int flags);
void al_destroy_bitmap(AL_BITMAP *bitmap);
void al_draw_bitmap(AL_BITMAP *bitmap, float x, float y);
void al_draw_sub_bitmap(AL_BITMAP *bitmap, float x, float y,
    float sx, float sy, float sw, float sh);

#endif
