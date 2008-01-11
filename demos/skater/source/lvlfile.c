#include <allegro.h>
#ifdef DEMO_USE_ALLEGRO_GL
#include <alleggl.h>
#endif
#include <string.h>
#include <math.h>
#include "../include/lvlalloc.h"
#include "../include/lvlfile.h"
#include "../include/token.h"

/*

	Routines related to the initial reading of level data - heavily connected to
	tkeniser.c. Defines the grammar that level files should use.

	Also includes a simple function for obtaining the equation of a line and some
	other bits to do with data initialisation that flows straight from level data

*/

/*
	Converts a bitmap using bright pink for transparency to a bitmap using alpha
	channel.
*/
#ifdef DEMO_USE_ALLEGRO_GL
void ConvertFromMaskedToTrans(BITMAP *From, BITMAP *To)
{
   int x, y;

   for (y = 0; y < From->h; y++) {
     for (x = 0; x < From->w; x++) {
        int col = getpixel(From, x, y);
        /* If the pixel matches mask color (bright pink) write it with alpha value 0
           (fully transparent), otherwise write at with alpha value 255 (fully
           opaque). */
        if (col == bitmap_mask_color(From))
           putpixel(To, x, y, makeacol32(getr(col), getg(col), getb(col), 0));
        else
           putpixel(To, x, y, makeacol32(getr(col), getg(col), getb(col), 255));
      }
   }
}
#endif


/*

	LoadMaterials loads the list of materials according to the following grammar:

		fillname -> string
		edgename -> string
		materal -> { fillname, edgename }
		material list -> { material* }

*/
void LoadMaterials(struct Level *NewLev)
{
   struct Material **NewMatPtr = &NewLev->AllMats;
#ifdef DEMO_USE_ALLEGRO_GL
   BITMAP *TmpEdge;
#endif

   ExpectToken(TK_OPENBRACE);
   while (1) {
      GetToken();
      switch (Token.Type) {
         case TK_CLOSEBRACE:
            return;
         case TK_OPENBRACE:
            {
               *NewMatPtr = NewMaterial();

               ExpectToken(TK_STRING);
               if (!((*NewMatPtr)->Fill = ObtainBitmap(Token.Text))) {
                  Error = 1;
                  uszprintf(ErrorText, sizeof(ErrorText),
                            "Could not load material fill %s at line %d",
                            Token.Text, Lines);
                  return;
               }

               ExpectToken(TK_COMMA);
               ExpectToken(TK_STRING);
#ifdef DEMO_USE_ALLEGRO_GL
               if (!(TmpEdge = ObtainBitmap(Token.Text))) {
#else
               if (!((*NewMatPtr)->Edge = ObtainBitmap(Token.Text))) {
#endif
                  Error = 1;
                  uszprintf(ErrorText, sizeof(ErrorText),
                            "Could not load material edge %s at line %d",
                            Token.Text, Lines);
                  return;
               }

#ifdef DEMO_USE_ALLEGRO_GL
            /* The "Edge" texture uses bright pink (also known as magic pink,
               makecol(255, 0, 255)) for transparency. Since AllegroGL doesn't
               support rendering of masked 3D polygons we have to convert the
               texture to a RGBA8 format which will use alpha channel for
               transparency.*/

            /* Tell AllegroGL to create 32-bit video bitmaps (RGBA8). */
               allegro_gl_set_video_bitmap_color_depth(32);
               (*NewMatPtr)->Edge = create_video_bitmap(TmpEdge->w, TmpEdge->h);
            /* Switch back to defualt video bitmaps color depth (screen color
               depth). */
               allegro_gl_set_video_bitmap_color_depth(-1);
               ConvertFromMaskedToTrans(TmpEdge, (*NewMatPtr)->Edge);
               destroy_bitmap(TmpEdge);
#endif

               ExpectToken(TK_COMMA);
               ExpectToken(TK_NUMBER);
               (*NewMatPtr)->Friction = Token.FQuantity;

               ExpectToken(TK_CLOSEBRACE);
               NewMatPtr = &(*NewMatPtr)->Next;
            }
            break;
         default:
            Error = 1;
            return;
      }
   }
}

/*

	LoadVertices loads the list of vertices according to the following grammar:

		xpos -> number
		ypos -> number
		vertex -> { xpos, ypos }
		vertex list -> { vertex* }

*/
void LoadVertices(struct Level *NewLev)
{
   struct Vertex **NewVertPtr = &NewLev->AllVerts;

   ExpectToken(TK_OPENBRACE);
   while (1) {
      GetToken();
      switch (Token.Type) {
         case TK_CLOSEBRACE:
            return;
         case TK_OPENBRACE:
            {
               *NewVertPtr = NewVertex();

               ExpectToken(TK_NUMBER);
               (*NewVertPtr)->Pos[0] = Token.FQuantity;

               ExpectToken(TK_COMMA);
               ExpectToken(TK_NUMBER);
               (*NewVertPtr)->Pos[1] = Token.FQuantity;

               ExpectToken(TK_CLOSEBRACE);
               NewVertPtr = &(*NewVertPtr)->Next;
            }
            break;
         default:
            Error = 1;
            return;
      }
   }
}

/*

	GetVert loads a single vertex reference as part of the LoadTriangles routine.
	Grammar is:

		edge height -> number
		reference -> number
		flags -> "edge" | "collidable" | "foreground"
		vertex reference -> { reference, edge height [, flags] }

*/
void GetVert(struct Level *NewLev, struct Triangle *t, int c)
{
   ExpectToken(TK_OPENBRACE);

   ExpectToken(TK_NUMBER);
   t->Edges[c] = NewLev->AllVerts;
   while (t->Edges[c] && Token.IQuantity--)
      t->Edges[c] = t->Edges[c]->Next;

   if (Token.IQuantity != -1) {
      Error = 1;
      uszprintf(ErrorText, sizeof(ErrorText),
                "Unknown vertex referenced at line %d", Lines);
      return;
   }
   ExpectToken(TK_COMMA);

   ExpectToken(TK_NUMBER);
   t->EdgeFlags[c] = (Token.IQuantity * SCREEN_H) / 480;

   GetToken();
   switch (Token.Type) {
      case TK_COMMA:
         {
            int Finished = FALSE;

            while (!Finished) {
               GetToken();
               switch (Token.Type) {
                  case TK_CLOSEBRACE:
                     Finished = TRUE;
                     break;
                  case TK_STRING:
                     if (!strcmp(Token.Text, "edge"))
                        t->EdgeFlags[c] |= TRIFLAGS_EDGE;
                     if (!strcmp(Token.Text, "collidable"))
                        t->EdgeFlags[c] |= FLAGS_COLLIDABLE;
                     if (!strcmp(Token.Text, "foreground"))
                        t->EdgeFlags[c] |= FLAGS_FOREGROUND;
                     break;
                  default:
                     Error = 1;
                     return;
               }
            }
         }
         break;
      case TK_CLOSEBRACE:
         break;
      default:
         Error = 1;
         return;
   }
}

/*

	GetNormal is a function that doesn't read anything from a file but calculates
	an edge normal for 'e' based on end points 'v1' and 'v2'

*/
int GetNormal(struct Edge *e, float *v1, float *v2)
{
   /* get line normal */
   float length;

   e->a = v2[1] - v1[1];
   e->b = -(v2[0] - v1[0]);

   /* make line normal unit length */
   length = sqrt(e->a * e->a + e->b * e->b);
   if (length < 1.0f)
      return TRUE;
   e->a /= length;
   e->b /= length;

   /* calculate distance of line from origin */
   e->c = -(v1[0] * e->a + v1[1] * e->b);
   return FALSE;
}

/*

	InitEdge intialises edges, which means calculating the 'expanded' edge equation
	(i.e. one moved away from the real edge by 'radius' units) and making a note at
	both end vertices of the edge they meet

*/
void InitEdge(struct Edge *e, int radius)
{
   /* get edge normal */
   GetNormal(e, e->EndPoints[0]->Pos, e->EndPoints[1]->Pos);

   /* calculate distance to line from origin */
   e->c -= radius * (e->a * e->a + e->b * e->b);

   /* link edge as necessary */
   if (!e->EndPoints[0]->Edges[0])
      e->EndPoints[0]->Edges[0] = e;
   else
      e->EndPoints[0]->Edges[1] = e;
   if (!e->EndPoints[1]->Edges[0])
      e->EndPoints[1]->Edges[0] = e;
   else
      e->EndPoints[1]->Edges[1] = e;
}

/*

	LoadTriangles loads a triangle list, using GetVert as required. Grammar is:

		vertex reference -> (see GetVert commentary)
		material reference -> number
		triangle ->	{ vertex reference, vertex reference, vertex reference,
					material reference }
		triangle list -> { triangle* }

*/
void LoadTriangles(struct Level *NewLev, int radius)
{
   struct Triangle *Tri;
   struct Edge *NextEdge;
   int c;

   ExpectToken(TK_OPENBRACE);
   while (1) {
      GetToken();
      switch (Token.Type) {
         case TK_CLOSEBRACE:
            return;
         case TK_OPENBRACE:
            {
               Tri = NewTriangle();

               /* read vertex pointers & edge flags */
               GetVert(NewLev, Tri, 0);
               ExpectToken(TK_COMMA);
               GetVert(NewLev, Tri, 1);
               ExpectToken(TK_COMMA);
               GetVert(NewLev, Tri, 2);
               ExpectToken(TK_COMMA);

               /* read material reference and store correct pointer */
               ExpectToken(TK_NUMBER);

               Tri->Material = NewLev->AllMats;
               while (Tri->Material && Token.IQuantity--)
                  Tri->Material = Tri->Material->Next;
               if (Token.IQuantity != -1) {
                  Error = 1;
                  uszprintf(ErrorText, sizeof(ErrorText),
                            "Unknown material referenced at line %d", Lines);
                  return;
               }

               /* expect end of this triangle */
               ExpectToken(TK_CLOSEBRACE);

               /* insert new triangle into total level list */
               Tri->Next = NewLev->AllTris;
               NewLev->AllTris = Tri;

               /* generate edges */
               c = 3;
               while (c--) {
                  if (Tri->EdgeFlags[c] & FLAGS_COLLIDABLE) {
                     NextEdge = NewLev->AllEdges;
                     NewLev->AllEdges = NewEdge();
                     NewLev->AllEdges->Material = Tri->Material;
                     NewLev->AllEdges->Next = NextEdge;

                     NewLev->AllEdges->EndPoints[0] = Tri->Edges[c];
                     NewLev->AllEdges->EndPoints[1] = Tri->Edges[(c + 1) % 3];

                     InitEdge(NewLev->AllEdges, radius);
                  }
               }
            }
            break;
         default:
            Error = 1;
            return;
      }
   }
}

/*

	LoadObjectTypes loads a list of object types. Grammar is:

		image name -> string
		collection noise -> string
		object type -> { image name, collection noise }
		object type list -> { object type* }

*/
void LoadObjectTypes(struct Level *NewLev, int radius)
{
   struct ObjectType **NewObjectPtr = &NewLev->AllObjectTypes;

   ExpectToken(TK_OPENBRACE);
   while (1) {
      GetToken();
      switch (Token.Type) {
         case TK_CLOSEBRACE:
            return;
         case TK_OPENBRACE:
            {
               *NewObjectPtr = NewObjectType();

               ExpectToken(TK_STRING);
               if (!((*NewObjectPtr)->Image = ObtainBitmap(Token.Text))) {
                  Error = 1;
                  uszprintf(ErrorText, sizeof(ErrorText),
                            "Could not load object image %s at line %d",
                            Token.Text, Lines);
                  return;
               }
               (*NewObjectPtr)->Radius =
                  ((*NewObjectPtr)->Image->w >
                   (*NewObjectPtr)->Image->h) ? (*NewObjectPtr)->
                  Image->w : (*NewObjectPtr)->Image->h;
               (*NewObjectPtr)->Radius += radius;

               ExpectToken(TK_COMMA);
               ExpectToken(TK_STRING);

               /* this doesn't generate an error as it is permissible to have objects that don't make a noise when collected */
               (*NewObjectPtr)->CollectNoise = ObtainSample(Token.Text);

               ExpectToken(TK_CLOSEBRACE);
               NewObjectPtr = &(*NewObjectPtr)->Next;
            }
            break;
         default:
            Error = 1;
            return;
      }
   }
}

/*

	LoadObjects loads a list of objects. Grammar is:

		object pos x -> number
		object pos y -> number
		object type -> number
		flags -> "collidable" | "foreground"
		object -> { object pos x, object pos y, object type [, flags] }
		object list -> { object* }

*/
void LoadObjects(struct Level *NewLev)
{
   struct Object *Obj;
   int Finished;

   ExpectToken(TK_OPENBRACE);
   while (1) {
      GetToken();
      switch (Token.Type) {
         case TK_CLOSEBRACE:
            return;
         case TK_OPENBRACE:
            {
               Obj = NewObject();
               NewLev->TotalObjects++;

               /* read location */
               ExpectToken(TK_NUMBER);
               Obj->Pos[0] = Token.FQuantity;
               ExpectToken(TK_COMMA);
               ExpectToken(TK_NUMBER);
               Obj->Pos[1] = Token.FQuantity;
               ExpectToken(TK_COMMA);

               /* read angle, convert into Allegro format */
               ExpectToken(TK_NUMBER);
               Obj->Angle = (Token.FQuantity * 128.0f) / AL_PI;
               ExpectToken(TK_COMMA);

               /* read object type, manipulate pointer */
               ExpectToken(TK_NUMBER);
               if (Token.IQuantity >= 0) {
                  Obj->ObjType = NewLev->AllObjectTypes;
                  while (Obj->ObjType && Token.IQuantity--)
                     Obj->ObjType = Obj->ObjType->Next;

                  if (Token.IQuantity != -1) {
                     Error = 1;
                     uszprintf(ErrorText, sizeof(ErrorText),
                               "Unknown object referenced at line %d", Lines);
                     return;
                  }
               } else {
                  /* this is the door - a hard coded type */
                  Obj->ObjType = &NewLev->Door;
                  Obj->Flags |= OBJFLAGS_DOOR;
                  NewLev->TotalObjects--;
               }

               /* parse any flags that may exist */
               GetToken();
               switch (Token.Type) {
                  case TK_COMMA:
                     {
                        Finished = FALSE;
                        while (!Finished) {
                           GetToken();
                           switch (Token.Type) {
                              case TK_CLOSEBRACE:
                                 Finished = TRUE;
                                 break;
                              case TK_STRING:
                                 if (!strcmp(Token.Text, "collidable"))
                                    Obj->Flags |= FLAGS_COLLIDABLE;
                                 if (!strcmp(Token.Text, "foreground"))
                                    Obj->Flags |= FLAGS_FOREGROUND;
                                 break;
                              default:
                                 Error = 1;
                                 return;
                           }
                        }
                     }
                     break;
                  case TK_CLOSEBRACE:
                     break;
                  default:
                     Error = 1;
                     return;
               }

               /* thread into list */
               Obj->Next = NewLev->AllObjects;
               NewLev->AllObjects = Obj;
            }
            break;
         default:
            Error = 1;
            return;
      }
   }
}

/*

	LoadStats loads some special variables. Grammar is:

		player start x -> number
		player start y -> number
		required number of objects -> number
		stats -> { player start x, player start y, required number of objects }

*/
void LoadStats(struct Level *NewLev)
{
   ExpectToken(TK_OPENBRACE);

   ExpectToken(TK_NUMBER);
   NewLev->PlayerStartPos[0] = Token.FQuantity;
   ExpectToken(TK_COMMA);
   ExpectToken(TK_NUMBER);
   NewLev->PlayerStartPos[1] = Token.FQuantity;
   ExpectToken(TK_COMMA);

   ExpectToken(TK_NUMBER);
   NewLev->ObjectsRequired = Token.IQuantity;
   ExpectToken(TK_CLOSEBRACE);
}
