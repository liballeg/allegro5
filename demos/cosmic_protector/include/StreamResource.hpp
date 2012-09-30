#ifndef STREAMRESOURCE_HPP
#define STREAMRESOURCE_HPP

#include "cosmic_protector.hpp"

class StreamResource : public Resource {
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   StreamResource(const char* filename);
private:
   ALLEGRO_AUDIO_STREAM *stream;
   std::string filename;
};

#endif // STREAMRESOURCE_HPP

