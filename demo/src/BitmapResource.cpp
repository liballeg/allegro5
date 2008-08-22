#include "a5teroids.hpp"

void BitmapResource::destroy(void)
{
   if (!bitmap)
      return;
   al_destroy_bitmap(bitmap);
   bitmap = 0;
}

bool BitmapResource::load(void)
{
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
   bitmap = iio_load(filename.c_str());
   if (!bitmap)
      debug_message("Error loading bitmap %s\n", filename.c_str());
   return bitmap != 0;
}

void* BitmapResource::get(void)
{
   return bitmap;
}

BitmapResource::BitmapResource(const char* filename) :
   bitmap(0)
{
   this->filename = std::string(filename);
}

