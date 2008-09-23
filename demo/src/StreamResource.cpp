#include "a5teroids.hpp"

void StreamResource::destroy(void)
{
   if (!stream)
      return;
   al_stream_destroy(stream);
   stream = 0;
}

bool StreamResource::load(void)
{
   stream = al_stream_from_file(4, 2048, filename.c_str());
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

