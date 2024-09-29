#include <allegro5/allegro.h>
#include <math.h>
#include "game.h"
#include "physics.h"

/*

	FixUp is called when the player has collided with 'something' and takes
	the current player's velocity and the vector normal of whatever has been
	hit as arguments.

	It's job is primarily to remove whatever proportion of the velocity is
	running into the surface. This means that the player's velocity ends
	up running at a right angle to the normal.

	Think of a perfectly unbouncy ball falling onto a horizontal floor.
	It's vertical velocity is instantly zero'd, but its horizontal velocity
	is unaffected.

	FixUp does this same thing for surfaces that aren't horizontal using
	the dot product.

	As a secondary function it returns the magnitude of force it has away,
	which can be used to pick a 'thud' sound effect and also for friction
	calculations

*/
static double FixUp(double *vec, double normalx, double normaly)
{
   double mul;

   if (KeyFlags & KEYFLAG_JUMPING)
      KeyFlags &= ~(KEYFLAG_JUMPING | KEYFLAG_JUMP | KEYFLAG_JUMP_ISSUED);

   mul = vec[0] * normalx + vec[1] * normaly;
   vec[0] -= mul * normalx;
   vec[1] -= mul * normaly;
   return mul;
}

/*

	SetAngle takes a surface normal, player angle and JustDoIt flag and
	decides how the player's interaction with that surface affects his
	angle.

	If JustDoIt is set then the function just sets the player to be at
	the angle of the normal, no questions asked.

	Otherwise the players angle is set if either:

		- the angle of the normal is close to vertical
		- the angle of the normal is close to the player angle
		
	Note that the arguments to atan2 are adjusted so that we're thinking
	in Allegro style angle measurements (i.e. 0 degrees = straight up,
	increases go clockwise) rather than the usual mathematics meaning
	(i.e. 0 degrees = right, increases go anticlockwise)

*/
static void SetAngle(double normalx, double normaly, double *a, int JustDoIt)
{
   double NewAng = atan2(normalx, -normaly);

   if (JustDoIt || ((NewAng < (A5O_PI * 0.25f)) && (NewAng >= 0)) ||
       ((NewAng > (-A5O_PI * 0.25f)) && (NewAng <= 0)) ||
       fabs(NewAng - *a) < (A5O_PI * 0.25f)
      )
      *a = NewAng;
}

/*

	DoFriction applies surface friction along an edge, taking the player's
	intended direction of travel into account

	First of all it breaks movement into 'ForwardSpeed' and 'UpSpeed', both
	relative to the edge - i.e. forward speed is motion parallel to the edge,
	upward speed is motion perpendicular

	Then the two are adjusted according to surface friction and player input,
	and finally put back together to reform the complete player velocity

*/
static void DoFriction(double r, struct Edge *E, double *vec)
{
   /* calculate how quickly we're currently moving parallel and perpendicular to this edge */
   double ForwardSpeed =
      (vec[0] * E->b - vec[1] * E->a) / E->Material->Friction,
      UpSpeed = vec[0] * E->a + vec[1] * E->b;
   double FricLevel;

   (void)r;

   /* apply adjustments based on user controls */
   if (KeyFlags & KEYFLAG_LEFT) {
      ForwardSpeed += Pusher;
      KeyFlags |= KEYFLAG_FLIP;
   }
   if (KeyFlags & KEYFLAG_RIGHT) {
      ForwardSpeed -= Pusher;
      KeyFlags &= ~KEYFLAG_FLIP;
   }

   /* apply friction as necessary */
   FricLevel = 0.05f * E->Material->Friction;
   if (ForwardSpeed > 0) {
      if (ForwardSpeed < FricLevel)
         ForwardSpeed = 0;
      else
         ForwardSpeed -= FricLevel;
   } else {
      if (ForwardSpeed > -FricLevel)
         ForwardSpeed = 0;
      else
         ForwardSpeed += FricLevel;
   }

   /* add jump if requested */
   if ((KeyFlags & (KEYFLAG_JUMP | KEYFLAG_JUMP_ISSUED)) == KEYFLAG_JUMP) {
      UpSpeed += 5.0f;
      KeyFlags |= KEYFLAG_JUMP_ISSUED | KEYFLAG_JUMPING;
   }

   /* put velocity back together */
   vec[0] = UpSpeed * E->a + ForwardSpeed * E->Material->Friction * E->b;
   vec[1] = UpSpeed * E->b - ForwardSpeed * E->Material->Friction * E->a;
}

/*

	RunPhysics is the centrepiece of the game simulation and is perhaps
	misnamed in that it runs physics and game logic generally.

	The basic structure is:

		apply gravity and air resistance;

		while(some simulation time remains)
		{
			find first thing the player hits during the simulation time

			run time forward until that hit occurs

			adjust player velocity according to whatever he has hit
		}

	Although some additional work needs to be done
*/
#if 0 /* unused */
struct QuadTreeNode *DoContinuousPhysics(struct Level *lvl, struct QuadTreeNode
                                         *CollTree, double *pos,
                                         double *vec, double TimeToGo,
                                         struct Animation *PAnim)
{
   double MoveVec[2];
   double CollTime, NCollTime;
   double End[2], ColPoint[2];
   void *CollPtr;
   double d1, d2;
   struct Edge *E;
   double r;
   int Contact;
   struct Container *EPtr, *FirstEdge;

   /* save a small amount of time by finding the first edge here */
   CollTree = GetCollisionNode(lvl, pos, vec);
   EPtr = CollTree->Contents;
   while (EPtr && EPtr->Type != EDGE)
      EPtr = EPtr->Next;
   FirstEdge = EPtr;

   /* do collisions and reactions */
   do {
      Contact = false;

      /* rounding error fixup */
      EPtr = FirstEdge;
      while (EPtr) {
         /* simple line test */
         d1 =
            EPtr->Content.E->a * pos[0] +
            EPtr->Content.E->b * pos[1] + EPtr->Content.E->c;

         if (d1 >= (-0.5f) && d1 <= 0.05f &&
             pos[0] >= EPtr->Content.E->Bounder.TL.Pos[0]
             && pos[0] <= EPtr->Content.E->Bounder.BR.Pos[0]
             && pos[1] >= EPtr->Content.E->Bounder.TL.Pos[1]
             && pos[1] <= EPtr->Content.E->Bounder.BR.Pos[1]
            ) {
            pos[0] += EPtr->Content.E->a * (0.05f - d1);
            pos[1] += EPtr->Content.E->b * (0.05f - d1);
            Contact = true;
         }

         EPtr = EPtr->Next;
      }

      MoveVec[0] = TimeToGo * vec[0];
      MoveVec[1] = TimeToGo * vec[1];

      CollTime = TimeToGo + 1.0f;
      End[0] = pos[0] + MoveVec[0];
      End[1] = pos[1] + MoveVec[1];
      CollPtr = NULL;

      /* search for collisions */

      /* test 1: do we hit the edge of this collision tree node? */
      if (End[0] > CollTree->Bounder.BR.Pos[0]) {
         NCollTime = (End[0] - CollTree->Bounder.BR.Pos[0]) / MoveVec[0];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      if (End[0] < CollTree->Bounder.TL.Pos[0]) {
         NCollTime = (End[0] - CollTree->Bounder.TL.Pos[0]) / MoveVec[0];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      if (End[1] > CollTree->Bounder.BR.Pos[1]) {
         NCollTime = (End[1] - CollTree->Bounder.BR.Pos[1]) / MoveVec[1];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      if (End[1] < CollTree->Bounder.TL.Pos[1]) {
         NCollTime = (End[1] - CollTree->Bounder.TL.Pos[1]) / MoveVec[1];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      /* test 2: do we hit any of the edges contained in the tree? */
      EPtr = FirstEdge;
      while (EPtr) {
         /* simple line test */
         d1 =
            EPtr->Content.E->a * pos[0] +
            EPtr->Content.E->b * pos[1] + EPtr->Content.E->c;
         d2 =
            EPtr->Content.E->a * End[0] +
            EPtr->Content.E->b * End[1] + EPtr->Content.E->c;
         if ((d1 >= (-0.05f)) && (d2 < 0)) {
            NCollTime = d1 / (d1 - d2);

            ColPoint[0] = pos[0] + NCollTime * MoveVec[0];
            ColPoint[1] = pos[1] + NCollTime * MoveVec[1];
            if (ColPoint[0] >= EPtr->Content.E->Bounder.TL.Pos[0]
                && ColPoint[0] <= EPtr->Content.E->Bounder.BR.Pos[0]
                && ColPoint[1] >= EPtr->Content.E->Bounder.TL.Pos[1]
                && ColPoint[1] <= EPtr->Content.E->Bounder.BR.Pos[1]
               ) {
               CollTime = NCollTime;
               CollPtr = (void *)EPtr;
            }
         }

         /* move to next edge */
         EPtr = EPtr->Next;
      }

      /* advance - apply motion and resulting friction here! */
      pos[0] += MoveVec[0] * CollTime;
      pos[1] += MoveVec[1] * CollTime;
      if (!Contact)
         AdvanceAnimation(PAnim, 0, false);

      /* fix up */
      if (CollPtr) {
         if (CollPtr == (void *)CollTree) {
            CollTree = GetCollisionNode(lvl, pos, vec);

            /* find new first edge */
            EPtr = CollTree->Contents;
            while (EPtr && EPtr->Type != EDGE)
               EPtr = EPtr->Next;
            FirstEdge = EPtr;
         } else {
            /* edge collision */
            E = ((struct Container *)CollPtr)->Content.E;
            r = FixUp(vec, E->a, E->b);

            SetAngle(E->a, E->b, &pos[2],
                     (fabs(vec[0] * E->b - vec[1] * E->a) > 0.5f));
            AdvanceAnimation(PAnim,
                             (vec[0] * E->b -
                              vec[1] * E->a) *
                             ((KeyFlags & KEYFLAG_FLIP) ? 1.0f : -1.0f),
                             true);

            /* apply friction */
            DoFriction(r, E, vec);
         }
      } else if (!Contact) {
         /* check if currently in contact with any surface. If not then do empty space movement */
         if (KeyFlags & KEYFLAG_LEFT) {
            vec[0] -= 0.05f;
            KeyFlags |= KEYFLAG_FLIP;
         }
         if (KeyFlags & KEYFLAG_RIGHT) {
            vec[0] += 0.05f;
            KeyFlags &= ~KEYFLAG_FLIP;
         }
         if (pos[2] > 0) {
            pos[2] -= TimeToGo * CollTime * 0.03f;
            if (pos[2] < 0)
               pos[2] = 0;
         }
         if (pos[2] < 0) {
            pos[2] += TimeToGo * CollTime * 0.03f;
            if (pos[2] > 0)
               pos[2] = 0;
         }
      }

      /* reduce time & continue */
      TimeToGo -= TimeToGo * CollTime;
   }
   while (TimeToGo > 0.01f);

   return CollTree;
}
#endif

/*#define TIME_STEP	0.6f
struct QuadTreeNode *RunPhysics(struct Level *lvl, double *pos, double *vec, double TimeToGo, struct Animation *PAnim)
{
	struct QuadTreeNode *CollTree = GetCollisionNode(lvl, pos, vec);
	static double TimeAccumulator = 0;
	double Step;

	TimeAccumulator += TimeToGo;

	Step = fmod(TimeAccumulator, TIME_STEP);
	if(Step >= 0.01f)
	{
		TimeAccumulator -= Step;
		CollTree = DoContinuousPhysics(lvl, CollTree, pos, vec, Step, PAnim);
	}

	while(TimeAccumulator > 0)
	{

			vec[1] += 0.1f;

			vec[0] *= 0.997f;
			vec[1] *= 0.997f;

		CollTree = DoContinuousPhysics(lvl, CollTree, pos, vec, TimeToGo, PAnim);
		TimeAccumulator -= TIME_STEP;
	}

	return CollTree;
}*/
struct QuadTreeNode *RunPhysics(struct Level *lvl, double *pos,
                                double *vec, double TimeToGo,
                                struct Animation *PAnim)
{
   double MoveVec[2];
   struct QuadTreeNode *CollTree = GetCollisionNode(lvl, pos, vec);
   double CollTime, NCollTime;
   double End[2], ColPoint[2];
   void *CollPtr;
   double d1, d2;
   struct Edge *E;
   double r;
   int Contact;
   struct Container *EPtr, *FirstEdge;

   /* Step 1: apply gravity */
   vec[1] += 0.1f;

   /* Step 2: apply atmoshperic resistance */
   vec[0] *= 0.997f;
   vec[1] *= 0.997f;

   EPtr = CollTree->Contents;
   while (EPtr && EPtr->Type != EDGE)
      EPtr = EPtr->Next;
   FirstEdge = EPtr;

   /* Step 2: do collisions and reactions */
   do {
      Contact = false;

      /* rounding error fixup */
      EPtr = FirstEdge;
      while (EPtr) {
         /* simple line test */
         d1 =
            EPtr->Content.E->a * pos[0] +
            EPtr->Content.E->b * pos[1] + EPtr->Content.E->c;

         if (d1 >= (-0.5f) && d1 <= 0.05f &&
             pos[0] >= EPtr->Content.E->Bounder.TL.Pos[0]
             && pos[0] <= EPtr->Content.E->Bounder.BR.Pos[0]
             && pos[1] >= EPtr->Content.E->Bounder.TL.Pos[1]
             && pos[1] <= EPtr->Content.E->Bounder.BR.Pos[1]
            ) {
            pos[0] += EPtr->Content.E->a * (0.05f - d1);
            pos[1] += EPtr->Content.E->b * (0.05f - d1);
            Contact = true;
         }

         EPtr = EPtr->Next;
      }

      MoveVec[0] = TimeToGo * vec[0];
      MoveVec[1] = TimeToGo * vec[1];

      CollTime = 1.0f;
      End[0] = pos[0] + MoveVec[0];
      End[1] = pos[1] + MoveVec[1];
      CollPtr = NULL;

      /* search for collisions here */

      /* test 1: do we hit the edge of this collision tree node? */
      if (End[0] > CollTree->Bounder.BR.Pos[0]) {
         NCollTime = (End[0] - CollTree->Bounder.BR.Pos[0]) / MoveVec[0];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      if (End[0] < CollTree->Bounder.TL.Pos[0]) {
         NCollTime = (End[0] - CollTree->Bounder.TL.Pos[0]) / MoveVec[0];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      if (End[1] > CollTree->Bounder.BR.Pos[1]) {
         NCollTime = (End[1] - CollTree->Bounder.BR.Pos[1]) / MoveVec[1];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      if (End[1] < CollTree->Bounder.TL.Pos[1]) {
         NCollTime = (End[1] - CollTree->Bounder.TL.Pos[1]) / MoveVec[1];
         if (NCollTime < CollTime) {
            CollTime = NCollTime;
            CollPtr = (void *)CollTree;
         }
      }

      /* test 3: do we hit any of the edges contained in the tree? */
      EPtr = FirstEdge;
      while (EPtr) {
         /* simple line test */
         d1 =
            EPtr->Content.E->a * pos[0] +
            EPtr->Content.E->b * pos[1] + EPtr->Content.E->c;
         d2 =
            EPtr->Content.E->a * End[0] +
            EPtr->Content.E->b * End[1] + EPtr->Content.E->c;
         if ((d1 >= (-0.05f)) && (d2 < 0)) {
            NCollTime = d1 / (d1 - d2);

            ColPoint[0] = pos[0] + NCollTime * MoveVec[0];
            ColPoint[1] = pos[1] + NCollTime * MoveVec[1];
            if (ColPoint[0] >= EPtr->Content.E->Bounder.TL.Pos[0]
                && ColPoint[0] <= EPtr->Content.E->Bounder.BR.Pos[0]
                && ColPoint[1] >= EPtr->Content.E->Bounder.TL.Pos[1]
                && ColPoint[1] <= EPtr->Content.E->Bounder.BR.Pos[1]
               ) {
               CollTime = NCollTime;
               CollPtr = (void *)EPtr;
            }
         }

         /* move to next edge */
         EPtr = EPtr->Next;
      }

      /* advance - apply motion and resulting friction here! */
      pos[0] += MoveVec[0] * CollTime;
      pos[1] += MoveVec[1] * CollTime;
      if (!Contact)
         AdvanceAnimation(PAnim, 0, false);

      /* fix up */
      if (CollPtr) {
         if (CollPtr == (void *)CollTree) {
            CollTree = GetCollisionNode(lvl, pos, vec);
            EPtr = CollTree->Contents;
            while (EPtr && EPtr->Type != EDGE)
               EPtr = EPtr->Next;
            FirstEdge = EPtr;
         } else {
            /* edge collision */
            E = ((struct Container *)CollPtr)->Content.E;
            r = FixUp(vec, E->a, E->b);

            SetAngle(E->a, E->b, &pos[2],
                     (fabs(vec[0] * E->b - vec[1] * E->a) > 0.5f));
            AdvanceAnimation(PAnim,
                             (vec[0] * E->b -
                              vec[1] * E->a) *
                             ((KeyFlags & KEYFLAG_FLIP) ? 1.0f : -1.0f),
                             true);

            /* apply friction */
            DoFriction(r, E, vec);
         }
      } else if (!Contact) {
         /* check if currently in contact with any surface. If not then do empty space movement */
         if (KeyFlags & KEYFLAG_LEFT) {
            vec[0] -= 0.05f;
            KeyFlags |= KEYFLAG_FLIP;
         }
         if (KeyFlags & KEYFLAG_RIGHT) {
            vec[0] += 0.05f;
            KeyFlags &= ~KEYFLAG_FLIP;
         }
         if (pos[2] > 0) {
            pos[2] -= TimeToGo * CollTime * 0.03f;
            if (pos[2] < 0)
               pos[2] = 0;
         }
         if (pos[2] < 0) {
            pos[2] += TimeToGo * CollTime * 0.03f;
            if (pos[2] > 0)
               pos[2] = 0;
         }
      }

      /* reduce time & continue */
      TimeToGo -= TimeToGo * CollTime;
   }
   while (TimeToGo > 0.01f);

   return CollTree;
}
