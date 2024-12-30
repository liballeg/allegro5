#include <allegro5/allegro.h>
#include "defines.h"
#include "global.h"
#include "music.h"

static int currently_playing = -1;
static double volume_music = 1, volume_sound = 1;

void set_music_volume(double v)
{
   volume_music = v;
   if (currently_playing == -1) return;
   al_set_audio_stream_gain(demo_data[currently_playing].dat, volume_music);
}

void set_sound_volume(double v)
{
   volume_sound = v;
}

void play_music(int id, int loop)
{
   (void)loop;
   if (!demo_data[id].dat)
      return;
   if (id == currently_playing) {
      return;
   }
   stop_music();

   al_set_audio_stream_playmode(demo_data[id].dat,
      loop ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE);

   al_set_audio_stream_gain(demo_data[id].dat, volume_music);

   al_attach_audio_stream_to_mixer(demo_data[id].dat,
      al_get_default_mixer());
   al_set_audio_stream_playing(demo_data[id].dat, true);

   currently_playing = id;
}


void stop_music(void)
{
   if (currently_playing == -1) return;
   al_set_audio_stream_playing(demo_data[currently_playing].dat, false);
   currently_playing = -1;
}


void play_sound(ALLEGRO_SAMPLE *s, int vol, int pan, int freq, int loop)
{
   int playmode = loop ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE;
    if (!s)
        return;
   if (freq < 0) {
      freq = 1000 + rand() % (-freq) + freq / 2;
   }

   al_play_sample(s, volume_sound * vol / 255.0,
      (pan - 128) / 128.0, freq / 1000.0, playmode, NULL);
}

void play_sound_id(int id, int vol, int pan, int freq, int loop)
{
   play_sound(demo_data[id].dat, vol, pan, freq, loop);
}
