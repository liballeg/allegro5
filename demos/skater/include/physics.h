#ifndef __PHYSICS_H
#define __PHYSICS_H

#include "anim.h"
#include "level.h"

extern struct QuadTreeNode *RunPhysics(struct Level *lvl, float *pos,
				       float *vec, float TimeToGo,
				       struct Animation *PAnim);

#endif
