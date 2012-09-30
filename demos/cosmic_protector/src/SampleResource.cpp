#include "cosmic_protector.hpp"

void SampleResource::destroy(void)
{
   al_destroy_sample(sample_data);
   sample_data = 0;
}

bool SampleResource::load(void)
{
   if (!al_is_audio_installed()) {
      debug_message("Skipped loading sample %s\n", filename.c_str());
      return true;
   }

   sample_data = al_load_sample(filename.c_str());
   if (!sample_data) {
      debug_message("Error loading sample %s\n", filename.c_str());
      return false;
   }

   return true;
}

void* SampleResource::get(void)
{
   return sample_data;
}

SampleResource::SampleResource(const char* filename) :
   sample_data(0),
   filename(filename)
{
}

