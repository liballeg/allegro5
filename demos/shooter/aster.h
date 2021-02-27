#ifndef ASTER_H_INCLUDED
#define ASTER_H_INCLUDED

#include "demo.h"

void init_asteroids(void);
void add_asteroid(void);
void move_asteroids(void);
void scroll_asteroids(void);
void draw_asteroids(BITMAP *bmp);
int asteroid_collision(int x, int y, int s);

#endif
