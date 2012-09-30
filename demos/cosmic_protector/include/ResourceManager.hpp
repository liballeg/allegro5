#ifndef RESOURCEMGR_HPP
#define RESOURCEMGR_HPP

#include "cosmic_protector.hpp"

class ResourceManager {
public:
   static ResourceManager& getInstance(void);
   
   void destroy(void);

   // Add with optional load
   bool add(Resource* res, bool load = true);
   Resource* getResource(int index);
   void* getData(int index);
private:
   static ResourceManager *rm;
   ResourceManager(void);

   std::vector< Resource* > resources;
};

#endif // RESOURCEMGR_HPP

