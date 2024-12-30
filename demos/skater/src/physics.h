#ifndef __PHYSICS_H
#define __PHYSICS_H

#include "anim.h"
#include "level.h"

extern struct QuadTreeNode *RunPhysics(struct Level *lvl, double *pos,
                                       double *vec, double TimeToGo,
                                       struct Animation *PAnim);

#endif
