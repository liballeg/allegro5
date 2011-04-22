#ifndef __LVLFILE_H
#define __LVLFILE_H

#include "level.h"

void LoadMaterials(struct Level *NewLev);
void LoadVertices(struct Level *NewLev);
void LoadTriangles(struct Level *NewLev, int radius);
void LoadObjectTypes(struct Level *NewLev, int radius);
void LoadObjects(struct Level *NewLev);
void LoadStats(struct Level *NewLev);

extern int GetNormal(struct Edge *e, double *v1, double *v2);

#endif
