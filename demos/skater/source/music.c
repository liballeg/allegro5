#include <allegro.h>
#include "../include/defines.h"
#include "../include/global.h"

static int currently_playing = -1;

void play_music(int id, int loop)
{
   if (id == currently_playing && midi_pos >= 0) {
      return;
   }

   if (play_midi(demo_data[id].dat, loop) == 0) {
      currently_playing = id;
   } else {
      currently_playing = -1;
   }
}


void stop_music(void)
{
   stop_midi();
   currently_playing = -1;
}


void play_sound(int id, int vol, int pan, int freq, int loop)
{
   if (freq < 0) {
      freq = 1000 + rand() % (-freq) + freq / 2;
   }

   play_sample(demo_data[id].dat, vol, pan, freq, loop);
}
