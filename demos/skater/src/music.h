#ifndef __DEMO_MUSIC_PLAYER_H__
#define __DEMO_MUSIC_PLAYER_H__

#include "global.h"

void play_music(int id, int loop);
void stop_music(void);
void play_sound(ALLEGRO_SAMPLE *s, int vol, int pan, int freq, int loop);
void play_sound_id(int id, int vol, int pan, int freq, int loop);
void set_music_volume(double v);
void set_sound_volume(double v);

#endif /* __DEMO_MUSIC_PLAYER_H__ */
