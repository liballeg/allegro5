#include "a5teroids.hpp"


void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   ALLEGRO_SAMPLE *s = (ALLEGRO_SAMPLE *)rm.getData(resourceID);
   al_sample_stop(s);
   al_sample_play(s);
}

