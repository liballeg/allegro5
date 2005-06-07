#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include "demo.h"

/* different ways to update the screen */
#define DOUBLE_BUFFER      1
#define PAGE_FLIP          2
#define TRIPLE_BUFFER      3
#define DIRTY_RECTANGLE    4

extern int animation_type;

void init_display(int mode, int w, int h, int animation_type);
void destroy_display(void);
BITMAP *prepare_display(void);
void flip_display(void);
void clear_display(void);

#endif
