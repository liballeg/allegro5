#include "cosmic_protector.hpp"


void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   A5O_SAMPLE *s = (A5O_SAMPLE *)rm.getData(resourceID);
   if (s) {
      al_play_sample(s, 1.0, 0.0, 1.0, A5O_PLAYMODE_ONCE, NULL);
   }
}

