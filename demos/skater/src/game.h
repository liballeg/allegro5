#ifndef                        __GAME_H__
#define                        __GAME_H__

/* functions for Miran's framework */
#include "gamestate.h"                /* gamestate.h */
#include "global.h"
extern void create_new_game(GAMESTATE * state);
extern void create_continue_game(GAMESTATE * game);
extern void destroy_game(void);

extern char *load_game_resources(const char *data_path);
extern void unload_game_resources(void);

/* global game state defines and variables */
#define KEYFLAG_LEFT               0x01
#define KEYFLAG_RIGHT              0x02
#define KEYFLAG_JUMP               0x04
#define KEYFLAG_JUMP_ISSUED        0x08
#define KEYFLAG_JUMPING            0x10
#define KEYFLAG_FLIP               0x20
extern int KeyFlags;
extern double Pusher;

#endif
