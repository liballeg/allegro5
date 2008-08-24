#include "a5teroids.hpp"

#define MAX_VOICES 16
static ALLEGRO_VOICE *voices[MAX_VOICES];


void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   ALLEGRO_SAMPLE *s = (ALLEGRO_SAMPLE *)rm.getData(resourceID);
   al_sample_play(s);
}

