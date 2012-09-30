#ifndef SAMPLERESOURCE_HPP
#define SAMPLERESOURCE_HPP

#include "cosmic_protector.hpp"

class SampleResource : public Resource {
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   SampleResource(const char* filename);
private:
   ALLEGRO_SAMPLE *sample_data;
   std::string filename;
};

#endif // SAMPLERESOURCE_HPP

