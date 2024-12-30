#ifndef __QUADTREES_H
#define __QUADTREES_H

#include "global.h"

#define TRIFLAGS_WIDTH   0x07f
#define TRIFLAGS_EDGE    0x080

#define OBJFLAGS_VISIBLE 0x001
#define OBJFLAGS_DOOR    0x002

#define FLAGS_COLLIDABLE 0x100
#define FLAGS_FOREGROUND 0x200

struct Material {
   ALLEGRO_BITMAP *Edge, *Fill;
   double Friction;
   struct Material *Next;
};

struct Vertex {
   double Pos[2];
   double Normal[2];
   struct Edge *Edges[2];
   struct Vertex *Next;
};

struct SmallVertex {
   double Pos[2];
};

struct BoundingBox {
   struct SmallVertex TL, BR;
};

struct Triangle {
   struct BoundingBox Bounder;
   struct Material *Material;
   struct Triangle *Next;

   struct Vertex *Edges[3];
   unsigned int EdgeFlags[3];

   unsigned int LastFrame;
};

struct ObjectType {
   ALLEGRO_BITMAP *Image;
   ALLEGRO_SAMPLE *CollectNoise;
   int Radius;

   struct ObjectType *Next;
};

struct Object {
   struct BoundingBox Bounder;
   struct ObjectType *ObjType;

   double Pos[2];
   unsigned int Flags;
   double Angle;
   unsigned int LastFrame;

   struct Object *Next;
};

struct Edge {
   struct BoundingBox Bounder;
   struct Material *Material;
   struct Edge *Next;

   /* equation of the edge, as in a*x + b*y + c = 0 */
   double a, b, c;

   /* pointers to the vertices that create this edge */
   struct Vertex *EndPoints[2];
};

/* containers for the two previous elements, used to build lists at tree nodes */
struct Container {
   struct Container *Next;

   enum { TRIANGLE, OBJECT, EDGE } Type;
   union {
      struct Edge *E;
      struct Triangle *T;
      struct Object *O;
   } Content;
};

/* quadtrees of containers */
struct QuadTreeNode {
   struct BoundingBox Bounder;

   int NumContents;
   struct Container *Contents;
   struct Container *PostContents;

   struct QuadTreeNode *Children;

   struct QuadTreeNode *Next;
};

/* level structure, connecting it all */
struct Level {
   struct QuadTreeNode DisplayTree, *VisibleList;
   struct QuadTreeNode CollisionTree;

   double PlayerStartPos[2];
   int TotalObjects, ObjectsRequired;
   struct LevelState *InitialState;

   struct ObjectType *AllObjectTypes, Door;
   struct Material *AllMats;

   struct Vertex *AllVerts;
   struct Edge *AllEdges;

   struct Triangle *AllTris;
   struct Object *AllObjects;

   ALLEGRO_BITMAP *DoorOpen, *DoorShut;
};


extern void SetupQuadTree(struct QuadTreeNode *Tree, int x1, int y1,
                          int x2, int y2);
extern void FreeQuadTree(struct QuadTreeNode *Tree);

extern void BeginQuadTreeDraw(struct Level *Lvl,
                              struct QuadTreeNode *TriTree,
                              struct BoundingBox *ScrBounder,
                              unsigned int framec);
extern void EndQuadTreeDraw(struct Level *Lvl,
                            struct BoundingBox *ScrBounder,
                            unsigned int framec);
extern struct QuadTreeNode *GetCollisionNode(struct Level *lev, double *pos,
                                             double *vec);

extern void AddTriangle(struct Level *Level, struct Triangle *NewTri);
extern void AddEdge(struct Level *level, struct Edge *NewEdge);
extern void AddObject(struct Level *level, struct Object *NewObject,
                      int DisplayTree);

extern void SplitTree(struct QuadTreeNode *Tree);
extern void OrderTree(struct QuadTreeNode *Tree, int PostTree);

#endif
