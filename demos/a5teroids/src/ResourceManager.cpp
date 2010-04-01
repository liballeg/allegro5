#include "a5teroids.hpp"

ResourceManager* ResourceManager::rm = 0;

ResourceManager& ResourceManager::getInstance(void)
{
   if (!rm)
      rm = new ResourceManager();
   return *rm;
}
 
void ResourceManager::destroy(void)
{
   std::vector<Resource*>::reverse_iterator it;
  
   for (it = resources.rbegin(); it != resources.rend(); it++) {
      Resource* r = *it;
      r->destroy();
      delete r;
   }

   resources.clear();

   delete rm;
   rm = 0;
}

bool ResourceManager::add(Resource* res, bool load)
{
   if (load) {
      if (!res->load())
         return false;
   }
   resources.push_back(res);
   return true;
}

Resource* ResourceManager::getResource(int index)
{
   return resources[index];
}

void* ResourceManager::getData(int index)
{
   return getResource(index)->get();
}

ResourceManager::ResourceManager(void)
{
}

