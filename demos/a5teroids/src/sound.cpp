#include "a5teroids.hpp"


void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   ALLEGRO_SAMPLE_INSTANCE *s = (ALLEGRO_SAMPLE_INSTANCE *)rm.getData(resourceID);
   if (s) {
      al_stop_sample_instance(s);
      al_play_sample_instance(s);
   }
}

