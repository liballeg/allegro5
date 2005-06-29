#ifndef BULLET_H_INCLUDED
#define BULLET_H_INCLUDED

#include "demo.h"

typedef struct BULLET {
   int x;
   int y;
   struct BULLET *next;
} BULLET;

extern BULLET *bullet_list;

BULLET *add_bullet(int x, int y);
BULLET *delete_bullet(BULLET * bullet);
void draw_bullets(BITMAP *bmp);
void move_bullets(void);

#endif
