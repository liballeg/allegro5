#include "a5teroids.hpp"

#define MAX_VOICES 16
static ALLEGRO_VOICE *voices[MAX_VOICES];

// FIXME: I want my play_sample back, this new API is.. not for me :(
static void bleh_need_better_sound_api_in_A5(ALLEGRO_SAMPLE *s)
{
   
   for (int i = 0; i < MAX_VOICES; i++) {
      if (!voices[i]) {
         voices[i] = al_voice_create(s);
         al_voice_start(voices[i]);
      }
      else {
         if (!al_voice_is_playing(voices[i])) {
            al_voice_destroy(voices[i]);
            voices[i] = NULL;
         }
      }
   }
}

void my_play_sample(int resourceID)
{
   ResourceManager &rm = ResourceManager::getInstance();
   ALLEGRO_SAMPLE *s = (ALLEGRO_SAMPLE *)rm.getData(resourceID);
   bleh_need_better_sound_api_in_A5(s);
}

