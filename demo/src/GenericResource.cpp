#include "a5teroids.hpp"


void GenericResource::destroy(void)
{
   (*destroy_func)(data);
}

bool GenericResource::load(void)
{
   return true;
}

void* GenericResource::get(void)
{
   return data;
}

GenericResource::GenericResource(void *data, void (*destroy)(void *))
{
   destroy_func = destroy;
   this->data = data;
}

