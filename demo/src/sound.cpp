#include "a5teroids.hpp"


void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   ALLEGRO_SAMPLE *s = (ALLEGRO_SAMPLE *)rm.getData(resourceID);
   al_stop_sample(s);
   al_play_sample(s);
}

