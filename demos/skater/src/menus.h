#ifndef __DEMO_GAME_MENUS_H__
#define __DEMO_GAME_MENUS_H__

#include "gamestate.h" /* gamestate.h */

void create_main_menu(GAMESTATE * state);
void create_options_menu(GAMESTATE * state);
void create_gfx_menu(GAMESTATE * state);
void create_sound_menu(GAMESTATE * state);
void create_controls_menu(GAMESTATE * state);
void create_misc_menu(GAMESTATE * state);
void create_about_menu(GAMESTATE * state);
void create_success_menu(GAMESTATE * state);
void create_intro(GAMESTATE * state);

void enable_continue_game(void);
void disable_continue_game(void);

#endif /* __DEMO_GAME_MENUS_H__ */
