#ifndef BMPRESOURCE_HPP
#define BMPRESOURCE_HPP

#include "a5teroids.hpp"

class BitmapResource : public Resource {
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   BitmapResource(const char* filename);
private:
   ALLEGRO_BITMAP *bitmap;
   std::string filename;
};

#endif // BMPRESOURCE_HPP

