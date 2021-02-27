#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include "demo.h"

/* for doing stereo sound effects */
#define PAN(x)       (((x) * 256) / SCREEN_W)
#define SPEED_SHIFT     3

extern int score;
extern int player_x_pos;
extern int player_hit;
extern int ship_state;
extern int skip_count;

void play_game(void);

#endif
