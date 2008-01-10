#ifndef __LVLALLOC_H
#define __LVLALLOC_H

#include "level.h"

extern struct Level *NewLevel();
extern struct Material *NewMaterial();
extern struct ObjectType *NewObjectType();
extern struct Triangle *NewTriangle();
extern struct Object *NewObject();
extern struct Edge *NewEdge();
extern struct Vertex *NewVertex();

#endif
