#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include "demo.h"

/* different ways to update the screen */
typedef enum ANIMATION_TYPE ANIMATION_TYPE;

enum ANIMATION_TYPE
{
   DOUBLE_BUFFER = 1,
   PAGE_FLIP,
   TRIPLE_BUFFER,
   DIRTY_RECTANGLE
};

extern ANIMATION_TYPE animation_type;

void init_display(int mode, int w, int h, ANIMATION_TYPE animation_type);
void destroy_display(void);
BITMAP *prepare_display(void);
void flip_display(void);
void clear_display(void);

#endif
