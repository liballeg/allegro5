/*

   Level.c contains the code for loading levels (which are in an ASCII format
   and parsed with a 'reasonable' degree of intelligence) and preprocessing
   them for game use.

   It can also produce a condensed version of the current level state
   (suitable for saving and loading of games, used elsewhere to retain state
   when display resolution is changed) and includes the main hook for level
   drawing - although this is passed on to quadtree.c as determining visibility
   is really a quadtree function.

   Note that the graphical parts of the level are scaled for the current
   display resolution but the collision / physics parts are never scaled. This
   is so that game behaviour is identical regardless of the display resolution
   and so that if the user changes resolution during a game they don't break
   their game state. The rounding error adjustment algorithms (see Physics.c)
   would not be very likely to survive a scaling of velocity and position and
   the player would likely fall through any platform they may be resting on.

*/

#include <allegro5/allegro.h>
#include <math.h>
#include <string.h>
#include "defines.h"
#include "game.h"
#include "level.h"
#include "level_alloc.h"
#include "level_file.h"
#include "token.h"

/*

   Drawing stuff first. Framec is a count of the current frame number, which
   is a required parameter to the drawing stuff in QuadTree.c - see that file
   for comments on that.

   DispBox is the rectangle that the user expects to be displayed. Note that
   (x, y) are parsed in the same units as the player moves so are scaled here
   into screen co-ordinates.

*/

int framec = 0;
struct BoundingBox DispBox;

void DrawLevelBackground(struct Level *lev, double *pos)
{
   float w, h;
   if (!lev)
      return;
   h = 480.0;
   w = screen_width * 480 / screen_height;
   framec++;
   DispBox.TL.Pos[0] = pos[0] - w / 2;
   DispBox.TL.Pos[1] = pos[1] - h / 2;
   DispBox.BR.Pos[0] = DispBox.TL.Pos[0] + w;
   DispBox.BR.Pos[1] = DispBox.TL.Pos[1] + h;

   BeginQuadTreeDraw(lev, &lev->DisplayTree, &DispBox, framec);
}

void DrawLevelForeground(struct Level *lev)
{
   if (!lev)
      return;
   EndQuadTreeDraw(lev, &DispBox, framec);
}


ALLEGRO_BITMAP *ObtainBitmap(const char *name)
{
   DATA_ENTRY *data = demo_data;
   while (data && data->name) {
      if (!strcmp(data->type, "bitmap") &&
         !strcmp(data->name, name)) return data->dat;
      data++;
   }
   return NULL;
}

ALLEGRO_SAMPLE *ObtainSample(const char *name)
{
   DATA_ENTRY *data = demo_data;
   while (data && data->name) {
      if (!strcmp(data->type, "sample") &&
         !strcmp(data->name, name)) return data->dat;
      data++;
   }
   return NULL;
}

/*

   A couple of very standard functions - min(a,b) returns the minimum of a and
   b, max(a,b) returns the maximum of a and b. Both have all the usual
   disadvantages of preprocessor macros (e.g. consider what will happen with
   min(a, b++)), but for this code it doesn't matter in the slightest.

*/

#undef min
#undef max
#define min(a, b)        (((a) < (b)) ? (a) : (b))
#define max(a, b)        (((a) > (b)) ? (a) : (b))

/*

   The FreeLevel function and associated macros. Just runs through all the data
   structures freeing anything that may have been allocated. Not really worthy
   of extensive comment.

   This function does the freeing for all level related struct types - there is
   no set of C++ equivalent deconstructors

*/
#define FreeList(name, type, sp) \
        while(lvl->name)\
        {\
                Next = (void *)lvl->name->Next;\
                sp(lvl->name);\
                free(lvl->name);\
                lvl->name = (struct type *)Next;\
        }

#define NoAction(v)

void FreeLevel(struct Level *lvl)
{
   void *Next;

   if (!lvl)
      return;

   /* easy things first */
   FreeQuadTree(&lvl->DisplayTree);
   FreeQuadTree(&lvl->CollisionTree);

   /* free the resource lists */
   FreeList(AllTris, Triangle, NoAction);
   FreeList(AllEdges, Edge, NoAction);

   FreeList(AllMats, Material, NoAction);
   FreeList(AllObjectTypes, ObjectType, NoAction);

   FreeList(AllVerts, Vertex, NoAction);
   FreeList(AllObjects, Object, NoAction);

   /* finally, free the level structure itself */
   FreeState(lvl->InitialState);
   free(lvl);
}

/* 01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
static int CalcIntersection(struct Edge *e1, struct Edge *e2, double *Inter,
                     int radius)
{
   double d1, d2;

   d1 =
      (e1->EndPoints[0]->Pos[0] + radius * e1->a) * e2->a +
      (e1->EndPoints[0]->Pos[1] + radius * e1->b) * e2->b + e2->c;
   d2 =
      (e1->EndPoints[1]->Pos[0] + radius * e1->a) * e2->a +
      (e1->EndPoints[1]->Pos[1] + radius * e1->b) * e2->b + e2->c;

   if (!(d1 - d2))
      return false;

   d1 /= (d1 - d2);
   Inter[0] =
      e1->EndPoints[0]->Pos[0] + radius * e1->a +
      d1 * (e1->EndPoints[1]->Pos[0] - e1->EndPoints[0]->Pos[0]);
   Inter[1] =
      e1->EndPoints[0]->Pos[1] + radius * e1->b +
      d1 * (e1->EndPoints[1]->Pos[1] - e1->EndPoints[0]->Pos[1]);

   return true;
}

static void FixVerts(struct Level *NewLev, int radius)
{
   struct Vertex *v = NewLev->AllVerts;

   while (v) {
      if (v->Edges[1]) {
         /* position normal at intersection of two connected edges */
         if (!CalcIntersection(v->Edges[0], v->Edges[1], v->Normal, radius)) {
            v->Normal[0] = v->Pos[0] + v->Edges[0]->a * radius;
            v->Normal[1] = v->Pos[1] + v->Edges[0]->b * radius;
         }
      } else if (v->Edges[0]) {
         /* position normal as a straight projection from v->Edges[0] */
         double Direction = -1.0f;

         if (v->Edges[0]->EndPoints[0] == v)
            Direction = 1.0f;

         v->Normal[0] =
            v->Pos[0] + v->Edges[0]->a * radius +
            radius * Direction * v->Edges[0]->b;
         v->Normal[1] =
            v->Pos[1] + v->Edges[0]->b * radius -
            radius * Direction * v->Edges[0]->a;
      }

      v = v->Next;
   }
}

static int FixEdges(struct Level *NewLev, int radius)
{
   struct Edge **e = &NewLev->AllEdges;
   int NotFinished = false, Failed;
   struct Edge OldEdge, *EdgePtr;

   (void)radius;

   while (*e) {
      /* check whether edge is now the wrong way around, and if so set NotFinished */
      OldEdge = **e;
      Failed =
         GetNormal(*e, (*e)->EndPoints[0]->Normal,
                   (*e)->EndPoints[1]->Normal);

      if (Failed || (((*e)->a * OldEdge.a + (*e)->b * OldEdge.b) < 0)) {
         NotFinished = true;

         /* fix edge pointers */
         EdgePtr =
            ((*e)->EndPoints[1]->Edges[0] ==
             (*e)) ? (*e)->EndPoints[1]->Edges[1] : (*e)->EndPoints[1]->
            Edges[0];

         if ((*e)->EndPoints[0]->Edges[0] == (*e)) {
            if (!EdgePtr) {
               (*e)->EndPoints[0]->Edges[0] = (*e)->EndPoints[0]->Edges[1];
               (*e)->EndPoints[0]->Edges[1] = NULL;
            } else
               (*e)->EndPoints[0]->Edges[0] = EdgePtr;
         } else
            (*e)->EndPoints[0]->Edges[1] = EdgePtr;

         if (EdgePtr->EndPoints[0] == (*e)->EndPoints[1])
            EdgePtr->EndPoints[0] = (*e)->EndPoints[0];
         else
            EdgePtr->EndPoints[1] = (*e)->EndPoints[0];

         (*e)->EndPoints[1]->Edges[0] = (*e)->EndPoints[1]->Edges[1] = NULL;

         /* unlink & free */
         EdgePtr = *e;
         (*e) = (*e)->Next;
         free((void *)EdgePtr);

         if (!(*e))
            break;
      }

      e = &((*e)->Next);
   }

   return NotFinished;
}

/*

   AddEdges calculates the axis aligned bounding boxes of all
   edges and inserts them into the collision tree

*/
static void AddEdges(struct Level *NewLev)
{
   struct Edge *e = NewLev->AllEdges;

   while (e) {
      /* calculate AABB for this edge */
      e->Bounder.TL.Pos[0] =
         min(e->EndPoints[0]->Normal[0], e->EndPoints[1]->Normal[0]) - 0.05f;
      e->Bounder.TL.Pos[1] =
         min(e->EndPoints[0]->Normal[1], e->EndPoints[1]->Normal[1]) - 0.05f;

      e->Bounder.BR.Pos[0] =
         max(e->EndPoints[0]->Normal[0], e->EndPoints[1]->Normal[0]) + 0.05f;
      e->Bounder.BR.Pos[1] =
         max(e->EndPoints[0]->Normal[1], e->EndPoints[1]->Normal[1]) + 0.05f;

      /* insert into quadtree */
      AddEdge(NewLev, e);

      e = e->Next;
   }
}

/*

   ScaleAndAddObjects calculates a collision box for the
   object, adds it to the collision tree then scales the
   box for the current display resolution before adding to
   the display tree

*/
static void ScaleAndAddObjects(struct Level *Lvl)
{
   struct Object *O = Lvl->AllObjects;
   //double Scaler = (float)screen_height / 480.0f;

   while (O) {
      /* generate bounding box for collisions (i.e. no scaling yet) */
      O->Bounder.BR.Pos[0] = O->Pos[0] + O->ObjType->Radius;
      O->Bounder.BR.Pos[1] = O->Pos[1] + O->ObjType->Radius;
      O->Bounder.TL.Pos[0] = O->Pos[0] - O->ObjType->Radius;
      O->Bounder.TL.Pos[1] = O->Pos[1] - O->ObjType->Radius;

      /* insert into collisions tree */
      AddObject(Lvl, O, false);

      /* scale bounding box to get visual position */
      /*O->Bounder.BR.Pos[0] *= Scaler;
      O->Bounder.BR.Pos[1] *= Scaler;
      O->Bounder.TL.Pos[0] *= Scaler;
      O->Bounder.TL.Pos[1] *= Scaler;*/

      /* insert into display tree */
      AddObject(Lvl, O, true);

      /* move along linked lists */
      O = O->Next;
   }
}

/*

   AddTriangles takes the now fully processed list of world
   triangles, calculates their bounding boxes (allowing for any
   edge trim) and inserts them into the display tree

*/
static void AddTriangles(struct Level *Lvl)
{
   struct Triangle *Tri = Lvl->AllTris;

   while (Tri) {
      int c = 3;

      /* determine axis aligned bounding box */
      Tri->Bounder.TL.Pos[0] = Tri->Bounder.BR.Pos[0] = Tri->Edges[0]->Pos[0];
      Tri->Bounder.TL.Pos[1] = Tri->Bounder.BR.Pos[1] = Tri->Edges[0]->Pos[1];

      while (c--) {
         int lc = c - 1;

         if (lc < 0)
            lc = 2;

         /* check if this x expands the bounding box either leftward or rightward */
         if (Tri->Edges[c]->Pos[0] < Tri->Bounder.TL.Pos[0])
            Tri->Bounder.TL.Pos[0] = Tri->Edges[c]->Pos[0];
         if (Tri->Edges[c]->Pos[0] > Tri->Bounder.BR.Pos[0])
            Tri->Bounder.BR.Pos[0] = Tri->Edges[c]->Pos[0];

         /* check y */
         if ((Tri->EdgeFlags[c] & TRIFLAGS_EDGE)
             || (Tri->EdgeFlags[lc] & TRIFLAGS_EDGE)) {
            if (Tri->Edges[c]->Pos[1] -
                (Tri->EdgeFlags[c] & TRIFLAGS_WIDTH) < Tri->Bounder.TL.Pos[1])
               Tri->Bounder.TL.Pos[1] =
                  Tri->Edges[c]->Pos[1] -
                  (Tri->EdgeFlags[c] & TRIFLAGS_WIDTH);
            if (Tri->Edges[c]->Pos[1] + (Tri->EdgeFlags[c] & TRIFLAGS_WIDTH) >
                Tri->Bounder.BR.Pos[1])
               Tri->Bounder.BR.Pos[1] =
                  Tri->Edges[c]->Pos[1] +
                  (Tri->EdgeFlags[c] & TRIFLAGS_WIDTH);
         } else {
            if (Tri->Edges[c]->Pos[1] < Tri->Bounder.TL.Pos[1])
               Tri->Bounder.TL.Pos[1] = Tri->Edges[c]->Pos[1];
            if (Tri->Edges[c]->Pos[1] > Tri->Bounder.BR.Pos[1])
               Tri->Bounder.BR.Pos[1] = Tri->Edges[c]->Pos[1];
         }
      }

      /* insert into tree */
      AddTriangle(Lvl, Tri);

      /* move along linked list */
      Tri = Tri->Next;
   }
}

/*

   GetLevelError returns a textual description of any error
   that has occurred. If no error has occurred it will
   slightly confusingly return "Unspecified error at line
   <line count for file>"

   Appends "Level load - " to the start of whatever text
   may already have been provided

*/
char *GetLevelError(void)
{
   char *LocTemp;

   if (!strlen(ErrorText))
      snprintf(ErrorText, sizeof(ErrorText), "Unspecified error at line %d",
                Lines);

   LocTemp = strdup(ErrorText);
   snprintf(ErrorText, sizeof(ErrorText), "Level load - %s", LocTemp);
   free((void *)LocTemp);
   return ErrorText;
}

/*

   LoadLevel is called by other parts of the program and returns
   either a complete level structure or NULL indicating error, in
   which case GetLevelError will return a textual description
   of the error.

   Parameters are 'name' - the file name of the level and 'radius'
   - the collision size of the player for collision tree building

*/
struct Level *LoadLevel(char const *name, int radius)
{
   ALLEGRO_FILE *file;
   struct Level *NewLev = NULL;

   ErrorText[0] = '\0';         /* set ErrorText to be a zero length string
                                   so that it will be obvious later if anything has set the error flag
                                   but not produced a verbose explanation of the error */
   Error = 0;                   /* reset error flag as no error has occurred yet */
   Lines = 1;                   /* first line is line 1 */

   /* attempt to open named level file */
   file = al_fopen(name, "r");
   input = file;

   if (!input) {
      snprintf(ErrorText, sizeof(ErrorText), "Unable to load %s", name);
      goto error;
   }

   /* allocate and initially set up new level structure */
   NewLev = NewLevel();

   /* load materials, vertices & triangles in that order */
   LoadMaterials(NewLev);
   if (Error) {
      goto error;
   }
   LoadVertices(NewLev);
   if (Error) {
      goto error;
   }
   LoadTriangles(NewLev, radius);
   if (Error) {
      goto error;
   }

   /* do a repeat 'fix' of vertices and fix of edges until we have
      no edge errors - see algorithm descriptions elsewhere in this
      file */
   do
      FixVerts(NewLev, radius);
   while (FixEdges(NewLev, radius));

   /* now that edges are al_fixed, add them to the collision tree */
   AddEdges(NewLev);

   /* load ordinary object types */
   LoadObjectTypes(NewLev, radius);
   if (Error) {
      goto error;
   }

   /* load special case object: door */
   if (!(NewLev->DoorOpen = ObtainBitmap("dooropen"))) {
      snprintf(ErrorText, sizeof(ErrorText),
                "Unable to obtain dooropen sprite");
      goto error;
   }
   if (!(NewLev->DoorShut = ObtainBitmap("doorshut"))) {
      snprintf(ErrorText, sizeof(ErrorText),
                "Unable to obtain doorshut sprite");
      goto error;
   }
   NewLev->Door.Image = NewLev->DoorShut;
   NewLev->Door.CollectNoise = ObtainSample("dooropen");
   NewLev->Door.Radius = 14 + radius;

   /* load objects */
   NewLev->TotalObjects = 0;
   LoadObjects(NewLev);
   if (Error) {
      goto error;
   }

   /* complete display tree additions */
   AddTriangles(NewLev);
   ScaleAndAddObjects(NewLev);

   /* order things for drawing */
   SplitTree(&NewLev->DisplayTree);
   OrderTree(&NewLev->DisplayTree, false);
   OrderTree(&NewLev->DisplayTree, true);
   OrderTree(&NewLev->CollisionTree, false);

   /* load static level stuff - player start pos, etc */
   LoadStats(NewLev);
   if (Error) {
      goto error;
   }

   /* make a copy of the initial state */
   NewLev->InitialState = BorrowState(NewLev);

   /* return level */
   return NewLev;

error:
   /* close input file */
   if (file)
      al_fclose(file);

   if (NewLev)
      FreeLevel(NewLev);

   return NULL;
}
