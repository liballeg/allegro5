#include "a5teroids.hpp"

void StreamResource::destroy(void)
{
   if (!stream)
      return;
   al_destroy_audio_stream(stream);
   stream = 0;
}

bool StreamResource::load(void)
{
   stream = al_load_audio_stream(filename.c_str(), 4, 2048);
   if (!stream) {
       debug_message("Error creating stream\n");
       return false;
   }

   return true;
}

void* StreamResource::get(void)
{
   return stream;
}

StreamResource::StreamResource(const char* filename) :
   stream(0)
{
   this->filename = std::string(filename);
}

