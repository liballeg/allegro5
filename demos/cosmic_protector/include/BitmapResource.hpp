#ifndef BMPRESOURCE_HPP
#define BMPRESOURCE_HPP

#include "cosmic_protector.hpp"

class BitmapResource : public Resource {
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   BitmapResource(const char* filename);
private:
   A5O_BITMAP *bitmap;
   std::string filename;
};

#endif // BMPRESOURCE_HPP

