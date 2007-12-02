#include "a5teroids.hpp"

void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   SAMPLE *s = (SAMPLE *)rm.getData(resourceID);
   play_sample(s, 255, 128, 1000, 0);
}

