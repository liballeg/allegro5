#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_image.h"

A5O_BITMAP *_al_load_android_bitmap_f(A5O_FILE *fp, int flags)
{
   return _al_android_load_image_f(fp, flags);   
}

A5O_BITMAP *_al_load_android_bitmap(const char *filename, int flags)
{
   return _al_android_load_image(filename, flags);
}

