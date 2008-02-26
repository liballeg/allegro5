#include "a5teroids.hpp"

void SampleResource::destroy(void)
{
   if (!sample)
      return;
   destroy_sample(sample);
   sample = 0;
}

bool SampleResource::load(void)
{
   sample = load_sample(filename.c_str());
   if (!sample)
      debug_message("Error loading sample %s\n", filename.c_str());
   return sample != 0;
}

void* SampleResource::get(void)
{
   return sample;
}

SampleResource::SampleResource(const char* filename) :
   sample(0)
{
   this->filename = std::string(filename);
}

