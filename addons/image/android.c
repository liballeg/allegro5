#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_image.h"

ALLEGRO_BITMAP *_al_load_android_bitmap_f(ALLEGRO_FILE *fp, int flags)
{
   return _al_android_load_image_f(fp, flags);
}

ALLEGRO_BITMAP *_al_load_android_bitmap(const char *filename, int flags)
{
   return _al_android_load_image(filename, flags);
}

