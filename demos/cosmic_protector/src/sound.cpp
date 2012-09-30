#include "cosmic_protector.hpp"


void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   ALLEGRO_SAMPLE *s = (ALLEGRO_SAMPLE *)rm.getData(resourceID);
   if (s) {
      al_play_sample(s, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
   }
}

