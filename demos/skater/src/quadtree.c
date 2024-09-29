/*

	QuadTree.c
	==========

	A quad tree is a hierarchical tree based structure. For further information
	on quad trees see http://en.wikipedia.org/wiki/Quadtree

	In this code, every node has a pointer named 'Children' which is either NULL
	or points to an array of four children. The children are indexed as follows:

		0 - top left
		1 - top right
		2 - bottom left
		3 - bottom right

	This allows indexing according to a simple bitwise calculation:

		childnum = 0;
		if(x > midpoint) childnum |= 1;
		if(y > midpoint) childnum |= 2;

	The QuadTreeNode structs look like this:

		struct QuadTreeNode
		{
			struct BoundingBox Bounder;

			int NumContents;
			struct Container *Contents;

			struct QuadTreeNode *Children;
		};

	BoundingBox is the axis aligned bounding box for the region of space
	covered, NumContents is a count of the number of items contained in the
	linked list 'Contents'. It is redundant but convenient.
	
	'Children' is as explained above - NULL if this is a leaf node, otherwise
	a pointer to an array of four QuadTreeNode structs that subdivide the area
	of this node.
*/

#include <allegro5/allegro_primitives.h>
#include "level.h"
#include "global.h"

/*

	CentreX and CentreY are macros that evaluate to the centre point of the
	bounding box b

*/
#define CentreX(b) (((b)->TL.Pos[0]+(b)->BR.Pos[0]) * 0.5f)
#define CentreY(b) (((b)->TL.Pos[1]+(b)->BR.Pos[1]) * 0.5f)
#define ERROR_BOUNDARY	2

/*

	GetChild. Returns the child index for a point (qx, qy) in the bounding box
	b. Uses the logic expressed in the comments at the top of this file

*/
static int GetChild(struct BoundingBox *b, double qx, double qy)
{
   int ret = 0;

   if (qx > CentreX(b))
      ret |= 1;
   if (qy > CentreY(b))
      ret |= 2;
   return ret;
}

/*

	Get[X1/2]/[Y1/2] - if passed a child id and the bounding box of the parent,
	these return the corresponding co-ordinate of the child

*/
static double GetX1(int child, struct BoundingBox *b)
{
   return (child & 1) ? CentreX(b) : b->TL.Pos[0];
}
static double GetX2(int child, struct BoundingBox *b)
{
   return (child & 1) ? b->BR.Pos[0] : CentreX(b);
}
static double GetY1(int child, struct BoundingBox *b)
{
   return (child & 2) ? CentreY(b) : b->TL.Pos[1];
}
static double GetY2(int child, struct BoundingBox *b)
{
   return (child & 2) ? b->BR.Pos[1] : CentreY(b);
}

/*

	ToggleChildPtr[X/Y] relate to the way movement is handled by the physics
	code. They use a continuous time method (see Physics.c for more details),
	which means that they need to be able to calculate when a travelling point
	will leave a quad tree node.

	Because there may be rounding errors close to the border, the function for
	obtaining collision edges inspects velocity to determine which child a
	point near the boundary is actually interested in.
	
	For that purpose, ToggleChildPtr[X/Y] affect which child is looked at
	according to velocity. If the point is heading right it wants the right
	hand side child, if it is heading down it wants the lower child, etc.

*/
static int ToggleChildPtrY(double yvec, int oldchild)
{
   if (((yvec > 0) && (oldchild & 2)) || ((yvec < 0) && !(oldchild & 2))
      )
      oldchild ^= 2;
   return oldchild;
}

static int ToggleChildPtrX(double xvec, int oldchild)
{
   if (((xvec > 0) && (oldchild & 1)) || ((xvec < 0) && !(oldchild & 1))
      )
      oldchild ^= 1;
   return oldchild;
}

/*

	Separated does a test on two bounding boxes to determine whether they
	overlap - returning true if they don't and false if they do.
	
	It achieves this using the 'separating planes' algorithm, which is a general
	case test that can be expressed very simply for boxes. If the leftmost point
	of one box is to the right of the rightmost point of the other then they
	must not overlap.

	Ditto for the topmost point of one versus the bottommost of the other and 
	all variations on those tests.

	These tests will catch all possible ways in which two boxes DO NOT
	overlap. Therefore the only possible conclusion if they all fail is that the
	boxes DO overlap.

	A small error boundary in which non-overlapping objects are returned as
	overlapping anyway is coded here so that we don't get rounding problems
	when doing collisions near the edge of a node

*/
static int Separated(struct BoundingBox *a, struct BoundingBox *b)
{
   if (a->TL.Pos[0] - ERROR_BOUNDARY > b->BR.Pos[0])
      return true;
   if (a->TL.Pos[1] - ERROR_BOUNDARY > b->BR.Pos[1])
      return true;

   if (b->TL.Pos[0] - ERROR_BOUNDARY > a->BR.Pos[0])
      return true;
   if (b->TL.Pos[1] - ERROR_BOUNDARY > a->BR.Pos[1])
      return true;

   return false;
}

/*

	GENERIC QUAD TREE FUNCTIONS

*/

/*

	SetupQuadTree just initiates a QuadTreeNode struct, filling its bounding
	box appropriately and setting it up to have no contents and no children

*/
void SetupQuadTree(struct QuadTreeNode *Tree, int x1, int y1, int x2, int y2)
{
   Tree->Bounder.TL.Pos[0] = x1;
   Tree->Bounder.TL.Pos[1] = y1;
   Tree->Bounder.BR.Pos[0] = x2;
   Tree->Bounder.BR.Pos[1] = y2;

   Tree->NumContents = 0;
   Tree->Contents = Tree->PostContents = NULL;
   Tree->Children = NULL;
}

/*

	FreeQuadTree frees all the memory malloc'd to a QuadTree. It calls itself
	recursively for any children

*/
void FreeQuadTree(struct QuadTreeNode *Tree)
{
   int c;
   struct Container *CNext;

   /*

      if this node has children then free them

    */
   if (Tree->Children) {
      c = 4;
      while (c--)
         FreeQuadTree(&Tree->Children[c]);

      free((void *)Tree->Children);
      Tree->Children = NULL;
   }

   /* free all edge containers stored here */
   while (Tree->Contents) {
      CNext = Tree->Contents->Next;
      free((void *)Tree->Contents);
      Tree->Contents = CNext;
   }
}

/*

	AddContent adds new content to a QuadTreeNode. The basic steps are these:

		1. does the new content fit into the space covered by this node? If not,
		reject it

		2. is this node subdivided? If so then pass the new content to the
		children to deal with. If not then insert at this node

		3. if inserted at this node, have we now hit the limit for items
		storable at any node? If so, subdivide and pass all contents that were
		stored here to the children

*/
static void AddContent(struct QuadTreeNode *Tree, struct Container *NewContent,
                int divider)
{
   int c;
   struct Container *CPtr, *Next;

   /*

      First check: does the refered new content actually overlap with this
      node? If not, do nothing

      NB: this code explicitly checks the bounding box of an 'edge' held in
      NewContent, even though it may hold an edge, triangle or object. This is
      fine because we've set up our structs so that the bounding box
      information is at the same location regardless

    */
   if (Separated(&NewContent->Content.E->Bounder, &Tree->Bounder))
      return;

   /*

      Second check: has this node been subdivided? If so, pass on for
      children to deal with

    */
   if (Tree->Children) {
      c = 4;
      while (c--)
         AddContent(&Tree->Children[c], NewContent, divider);
   } else {
      /*

         If we're here, then the edge really does need to be added to the
         current node, so do that

       */
      Tree->NumContents++;
      CPtr = Tree->Contents;
      Tree->Contents = (struct Container *)malloc(sizeof(struct Container));
      Tree->Contents->Content = NewContent->Content;
      Tree->Contents->Type = NewContent->Type;
      Tree->Contents->Next = CPtr;

      /*

         Now check if we've hit the maximum content count for this node. If
         so, subdivide

       */
      if ((Tree->NumContents == divider)
          && ((Tree->Bounder.BR.Pos[0] - Tree->Bounder.TL.Pos[0]) > screen_width)
          && ((Tree->Bounder.BR.Pos[1] - Tree->Bounder.TL.Pos[1]) > screen_height)) {
         /* allocate new memory and set up structures */
         Tree->Children =
            (struct QuadTreeNode *)malloc(sizeof(struct QuadTreeNode)
                                          * 4);
         c = 4;
         while (c--)
            SetupQuadTree(&Tree->Children[c],
                          GetX1(c, &Tree->Bounder), GetY1(c,
                                                          &Tree->
                                                          Bounder),
                          GetX2(c, &Tree->Bounder), GetY2(c, &Tree->Bounder));

         /* redistribute contents currently stored here */
         CPtr = Tree->Contents;
         while (CPtr) {
            Next = CPtr->Next;

            c = 4;
            while (c--)
               AddContent(&Tree->Children[c], CPtr, divider);

            free(CPtr);
            CPtr = Next;
         }
         Tree->Contents = NULL;
      }
   }
}

/*

	STUFF FOR DEALING WITH 'EDGES'

*/


/*

	GetNode returns the leaf node that a point is currently in from
	the edge tree, for collisions & physics

*/
static struct QuadTreeNode *GetNode(struct QuadTreeNode *Ptr, double *pos, double *vec)
{
   int Child;
   int MidX, MidY;

   /* continue moving down the tree if we aren't at a leaf */
   while (Ptr->Children) {
      /* figure out which child we should prima facie be considering */
      Child = GetChild(&Ptr->Bounder, pos[0], pos[1]);

      /* toggle child according to velocity if sufficiently near the boundary */
      MidX = CentreX(&Ptr->Bounder);
      MidY = CentreY(&Ptr->Bounder);

      if (abs((int)pos[0] - MidX) < ERROR_BOUNDARY)
         Child = ToggleChildPtrX(vec[0], Child);

      if (abs((int)pos[1] - MidY) < ERROR_BOUNDARY)
         Child = ToggleChildPtrY(vec[1], Child);

      /* and now move ourselves so that we are now looking at that child */
      Ptr = &Ptr->Children[Child];
   }

   /* return the leaf node found */
   return Ptr;
}

struct QuadTreeNode *GetCollisionNode(struct Level *lvl, double *pos,
                                      double *vec)
{
   return GetNode(&lvl->CollisionTree, pos, vec);
}

/*

	STUFF FOR DEALING WITH 'TRIANGLES'

*/

static void set_v(A5O_VERTEX *vt, double x, double y, double u, double v)
{
   vt->x = x;
   vt->y = y;
   vt->z = 0;
   vt->u = u;
   vt->v = v;
   vt->color = al_map_rgb_f(1, 1, 1);
}

/*

	DrawTriEdge is a drawing function that adds a textured edge to a triangle

*/
static void DrawTriEdge(struct Triangle *tri, struct BoundingBox *ScrBounder)
{
   A5O_VERTEX PolyEdges[4];
   int c, c2;
   double x, y, w;

   /* quick bounding box check - if the triangle isn't on screen then we can
      forget about it */
   if (Separated(&tri->Bounder, ScrBounder))
      return;
   c = 3;
   while (c--) {
      c2 = (c + 1) % 3;
      if (tri->EdgeFlags[c] & TRIFLAGS_EDGE) {
         /*

            Texture u is determined according to world position so that
            edges that should appear to meet up do.

          */
         x = tri->Edges[c]->Pos[0] - ScrBounder->TL.Pos[0];
         y = tri->Edges[c]->Pos[1] - ScrBounder->TL.Pos[1];
         w = tri->EdgeFlags[c] & TRIFLAGS_WIDTH;
         set_v(PolyEdges + 0, x, y - w, tri->Edges[c]->Pos[0], 0);
         set_v(PolyEdges + 3, x, y + w, tri->Edges[c]->Pos[0], al_get_bitmap_height(tri->Material->Edge));
         x = tri->Edges[c2]->Pos[0] - ScrBounder->TL.Pos[0] + 1;
         y = tri->Edges[c2]->Pos[1] - ScrBounder->TL.Pos[1];
         w = tri->EdgeFlags[c2] & TRIFLAGS_WIDTH;
         set_v(PolyEdges + 1, x, y - w, tri->Edges[c2]->Pos[0], 0);
         set_v(PolyEdges + 2, x, y + w, tri->Edges[c2]->Pos[0], al_get_bitmap_height(tri->Material->Edge));

         al_draw_prim(PolyEdges, NULL, tri->Material->Edge, 0, 4, A5O_PRIM_TRIANGLE_FAN);
      }
   }
}

static void setuv(A5O_VERTEX *v, struct BoundingBox *ScrBounder)
{
   v->u = v->x + ScrBounder->TL.Pos[0];
   v->v = v->y + ScrBounder->TL.Pos[1];
   
   v->z = 0;
   v->color = al_map_rgb_f(1, 1, 1);
}

/*

	DrawTriangle is a drawing function that draws a triangle with a textured
	fill

*/
static void DrawTriangle(struct Triangle *tri,
                  struct BoundingBox *ScrBounder)
{
   A5O_VERTEX v[3];

   /* quick bounding box check - if the triangle isn't on screen then we can
      forget about it */
   if (Separated(&tri->Bounder, ScrBounder))
      return;

    v[0].x = tri->Edges[0]->Pos[0] - ScrBounder->TL.Pos[0];
    v[0].y = tri->Edges[0]->Pos[1] - ScrBounder->TL.Pos[1];
    setuv(&v[0], ScrBounder);
    v[1].x = tri->Edges[1]->Pos[0] - ScrBounder->TL.Pos[0];
    v[1].y = tri->Edges[1]->Pos[1] - ScrBounder->TL.Pos[1];
    setuv(&v[1], ScrBounder);
    v[2].x = tri->Edges[2]->Pos[0] - ScrBounder->TL.Pos[0];
    v[2].y = tri->Edges[2]->Pos[1] - ScrBounder->TL.Pos[1];
    setuv(&v[2], ScrBounder);

    al_draw_prim(v, NULL, tri->Material->Fill, 0, 3, A5O_PRIM_TRIANGLE_STRIP);
}

/*

	DrawObject is a drawing function that draws an object (!)

*/
static void DrawObject(struct Object *obj,
                struct BoundingBox *ScrBounder)
{
   /* quick bounding box check - if the triangle isn't on screen then we can
      forget about it */
   if (Separated(&obj->Bounder, ScrBounder))
      return;

   if (obj->Flags & OBJFLAGS_VISIBLE)
      al_draw_scaled_rotated_bitmap(obj->ObjType->Image, 0, 0,
                           (int)(0.5f *
                                 (obj->Bounder.TL.Pos[0] +
                                  obj->Bounder.BR.Pos[0]) -
                                 ScrBounder->TL.Pos[0]) -
                           (al_get_bitmap_width(obj->ObjType->Image) >> 1),
                           (int)(0.5f *
                                 (obj->Bounder.TL.Pos[1] +
                                  obj->Bounder.BR.Pos[1]) -
                                 ScrBounder->TL.Pos[1]) -
                           (al_get_bitmap_height(obj->ObjType->Image) >> 1),
                           1, 1,
                           obj->Angle, 0);
}

/*

	DrawTriangleTree does the 'hard' work of actually drawing the contents of a
	quad tree. If you've understood how the quad tree is made up and works from
	all the other comments in this file then it is very straightforward and easy
	to follow.

	The only point of interest here is 'framec' - an integer that identifies the
	current frame.

	The problem that necessitates it is that a single screen may cover multiple
	leaf nodes, and that some triangles may be present in more than one of those
	nodes. We don't want to expend energy drawing those triangles twice.

	To resolve this, every triangle keeps a note of the frame in which it was
	last drawn. If that frame is not this frame then it is drawn and that note
	is updated.

	The 'clever' thing about this is that it kills the overdraw without any per
	frame seeding of triangles. The disadvantage is that any triangle which
	doesn't appear on screen for exactly as long as it takes the framec integer
	to overflow then does will not be drawn for one frame.

	Assuming 100 fps and a 32bit CPU, this bug can in the unlikely situation
	that it does occur, only happen after approximately 1.36 years of
	continuous gameplay. It is therefore submitted that it shouldn't be of high
	concern!

	Calculations:
	
		2^32 = 4,294,967,296
		/100 = 4,294,967.296 seconds
		/60 = 715,827.883 minutes
		/60 = 11,930.465 hours
		/24 = 497.103 days
		/365.25 = 1.36 years

*/
static void GetQuadTreeVisibilityList(struct QuadTreeNode *TriTree,
                               struct Level *Lvl,
                               struct BoundingBox *ScrBounder)
{
   int c;

   /*

      if the view window doesn't overlap this node then do nothing

    */
   if (Separated(&TriTree->Bounder, ScrBounder))
      return;

   /*

      if this node has children, consider them instead

    */
   if (TriTree->Children) {
      c = 4;
      while (c--)
         GetQuadTreeVisibilityList(&TriTree->Children[c], Lvl, ScrBounder);
   } else {
      /*

         otherwise, add to draw list

       */
      TriTree->Next = Lvl->VisibleList;
      Lvl->VisibleList = TriTree;
   }
}

static void DrawQuadTreePart(struct Level *Lvl,
                      struct BoundingBox *ScrBounder,
                      unsigned int framec, int PostContents)
{
   struct Container *Thing, *TriStart;
   struct QuadTreeNode *Visible;

   /* go through each node drawing background details - objects first, then polygons, finally edges */
   Visible = Lvl->VisibleList;
   while (Visible) {
      /*

         objects

       */
      Thing = PostContents ? Visible->PostContents : Visible->Contents;
      while (Thing && Thing->Type == OBJECT) {
         if (Thing->Content.O->LastFrame != framec) {
            DrawObject(Thing->Content.O, ScrBounder);
            Thing->Content.O->LastFrame = framec;
         }
         Thing = Thing->Next;
      }

      TriStart = Thing;
      while (Thing) {
         if (Thing->Content.T->LastFrame != framec)
            DrawTriangle(Thing->Content.T, ScrBounder);
         Thing = Thing->Next;
      }

      /*

         and add edges that haven't already been drawn this frame, this time
         updating the note of the last frame in which this triangle was
         drawn

       */
      Thing = TriStart;
      while (Thing) {
         if (Thing->Content.T->LastFrame != framec) {
            DrawTriEdge(Thing->Content.T, ScrBounder);
            Thing->Content.T->LastFrame = framec;
         }
         Thing = Thing->Next;
      }

      Visible = Visible->Next;
   }
}

void BeginQuadTreeDraw(struct Level *Lvl,
                       struct QuadTreeNode *TriTree,
                       struct BoundingBox *ScrBounder, unsigned int framec)
{
   /* compile list of visible nodes */
   Lvl->VisibleList = NULL;
   GetQuadTreeVisibilityList(TriTree, Lvl, ScrBounder);
   DrawQuadTreePart(Lvl, ScrBounder, framec, false);
}

void EndQuadTreeDraw(struct Level *Lvl,
                     struct BoundingBox *ScrBounder, unsigned int framec)
{
   DrawQuadTreePart(Lvl, ScrBounder, framec, true);
}

/*

	FUNCTIONS FOR ADDING CONTENT

*/
#define MAX_COLL		200
#define MAX_DISP		50

/*

	AddTriangle and AddEdge are both very similar indeed and just package the
	new triangle or edge into a 'Container' so that the AddContent function
	knows how to deal with them in a unified manner.

*/

void AddTriangle(struct Level *level, struct Triangle *NewTri)
{
   struct Container Cont;

   Cont.Content.T = NewTri;
   Cont.Type = TRIANGLE;
   AddContent(&level->DisplayTree, &Cont, MAX_DISP);
}

void AddEdge(struct Level *level, struct Edge *NewEdge)
{
   struct Container Cont;

   Cont.Content.E = NewEdge;
   Cont.Type = EDGE;
   AddContent(&level->CollisionTree, &Cont, MAX_COLL);
}

void AddObject(struct Level *level, struct Object *NewObject, int DisplayTree)
{
   struct Container Cont;

   Cont.Content.O = NewObject;
   Cont.Type = OBJECT;

   if (DisplayTree) {
      if (NewObject->Flags & OBJFLAGS_VISIBLE)
         AddContent(&level->DisplayTree, &Cont, MAX_DISP);
   } else {
      if (NewObject->Flags & FLAGS_COLLIDABLE)
         AddContent(&level->CollisionTree, &Cont, MAX_COLL);
   }
}

/*

	OrderTree sorts the items stored within Tree so that their lists
	consist of all their non-objects (i.e. triangles or edges) first
	followed by all of their objects.

*/
void SplitTree(struct QuadTreeNode *Tree)
{
   struct Container *P, *PNext;
   int c = 4;

   if (Tree->Children) {
      while (c--)
         SplitTree(&Tree->Children[c]);
   } else if (Tree->Contents) {
      P = Tree->Contents;
      Tree->Contents = Tree->PostContents = NULL;

      while (P) {
         PNext = P->Next;

         if (P->Type == OBJECT) {
            if (P->Content.O->Flags & FLAGS_FOREGROUND) {
               P->Next = Tree->PostContents;
               Tree->PostContents = P;
            } else {
               P->Next = Tree->Contents;
               Tree->Contents = P;
            }
         } else {
            if ((P->Content.T->EdgeFlags[0] | P->Content.T->
                 EdgeFlags[1] | P->Content.T->EdgeFlags[2])
                & FLAGS_FOREGROUND) {
               P->Next = Tree->PostContents;
               Tree->PostContents = P;
            } else {
               P->Next = Tree->Contents;
               Tree->Contents = P;
            }
         }

         P = PNext;
      }
   }
}

void OrderTree(struct QuadTreeNode *Tree, int PostTree)
{
   struct Container *Objects, *NonObjects, *P, *PNext, **ITree;
   int c = 4;

   if (Tree->Children) {
      while (c--)
         OrderTree(&Tree->Children[c], PostTree);
   } else {
      ITree = PostTree ? &Tree->PostContents : &Tree->Contents;

      if (*ITree) {
         /* separate object and non-oject lists */
         Objects = NonObjects = NULL;
         P = *ITree;
         while (P) {
            PNext = P->Next;

            if (P->Type == OBJECT) {
               P->Next = Objects;
               Objects = P;
            } else {
               P->Next = NonObjects;
               NonObjects = P;
            }

            P = PNext;
         }

         /* now reintegrate lists - objects then non-objects */
         *ITree = Objects;
         while (*ITree)
            ITree = &(*ITree)->Next;
         *ITree = NonObjects;
      }
   }
}
