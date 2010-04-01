#include "a5teroids.hpp"

void SampleResource::destroy(void)
{
   if (!sample)
      return;
   al_destroy_sample_instance(sample);
   al_destroy_sample(sample_data);
   sample = 0;
   sample_data = 0;
}

bool SampleResource::load(void)
{
   sample_data = al_load_sample(filename.c_str());
   if (!sample_data) {
      debug_message("Error loading sample %s\n", filename.c_str());
      return false;
   }

   sample = al_create_sample_instance(sample_data);
   if (!sample) {
       debug_message("Error creating sample\n");
       al_destroy_sample(sample_data);
       sample_data = 0;
       return false;
   }

   return true;
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

