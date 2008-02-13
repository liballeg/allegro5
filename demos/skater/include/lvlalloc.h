#ifndef __LVLALLOC_H
#define __LVLALLOC_H

#include "level.h"

extern struct Level *NewLevel(void);
extern struct Material *NewMaterial(void);
extern struct ObjectType *NewObjectType(void);
extern struct Triangle *NewTriangle(void);
extern struct Object *NewObject(void);
extern struct Edge *NewEdge(void);
extern struct Vertex *NewVertex(void);

#endif
