/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Global defines.
 */

#ifndef __SPEED_H__
#define __SPEED_H__

#include "a4_aux.h"


/* global state variables */
extern int cheat;
extern int low_detail;
extern int no_grid;
extern int no_music;
extern int lives;
extern int score;


/* from title.c */
int title_screen();
void show_results();
void goodbye();


/* from view.c */
void init_view();
void shutdown_view();
int advance_view();
void update_view();
void draw_view();
float view_size();


/* from player.c */
void init_player();
void shutdown_player();
void advance_player(int cycle);
int update_player();
void draw_player(int r, int g, int b, int (*project)(float *f, int *i, int c));
float player_pos();
float find_target(float x);
int player_dying();
int kill_player(float x, float y);


/* from badguys.c */
void init_badguys();
void shutdown_badguys();
int update_badguys();
void draw_badguys(int r, int g, int b, int (*project)(float *f, int *i, int c));
void lay_attack_wave(int reset);


/* from bullets.c */
void init_bullets();
void shutdown_bullets();
void update_bullets();
void draw_bullets(int r, int g, int b, int (*project)(float *f, int *i, int c));
void fire_bullet();
void *get_first_bullet(float *x, float *y);
void *get_next_bullet(void *b, float *x, float *y);
void kill_bullet(void *b);


/* from explode.c */
void init_explode();
void shutdown_explode();
void update_explode();
void draw_explode(int r, int g, int b, int (*project)(float *f, int *i, int c));
void explode(float x, float y, int big);


/* from message.c */
void init_message();
void shutdown_message();
void update_message();
void draw_message();
void message(char *text);


/* from hiscore.c */
void init_hiscore();
void shutdown_hiscore();
void score_table();
int get_hiscore();


/* from sound.c */
void init_sound();
void shutdown_sound();
void sfx_shoot();
void sfx_explode_alien();
void sfx_explode_block();
void sfx_explode_player();
void sfx_ping(int times);



#endif   /* __SPEED_H__ */
