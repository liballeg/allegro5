#ifndef EXPL_H_INCLUDED
#define EXPL_H_INCLUDED

#include "demo.h"

/* explosion graphics */
#define EXPLODE_FLAG    100
#define EXPLODE_FRAMES  64
#define EXPLODE_SIZE    80

extern RLE_SPRITE *explosion[EXPLODE_FRAMES];

void generate_explosions(void);
void destroy_explosions(void);

#endif
