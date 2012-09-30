#include "cosmic_protector.hpp"

void FontResource::destroy(void)
{
   al_destroy_font(font);
   font = 0;
}

bool FontResource::load(void)
{
   font = al_load_font(filename.c_str(), 0, 0);
   return font != 0;
}

void* FontResource::get(void)
{
   return font;
}

FontResource::FontResource(const char *filename) :
   font(0),
   filename(filename)
{
}

