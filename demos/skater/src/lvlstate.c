#include <allegro.h>
#include <string.h>
#include "../include/level.h"

struct LevelState *BorrowState(struct Level *NewLev)
{
   struct LevelState *St =
      (struct LevelState *)malloc(sizeof(struct LevelState));
   struct Object *O;
   int ObjCount;

   if (!NewLev)
      return NULL;

   St->Length = (NewLev->TotalObjects + 31) >> 5;
   St->Data = (uint32_t *) malloc(sizeof(uint32_t) * St->Length);
   memset(St->Data, 0, sizeof(uint32_t) * St->Length);

   ObjCount = 0;
   O = NewLev->AllObjects;
   while (O) {
      if (O->Flags & OBJFLAGS_VISIBLE)
         St->Data[ObjCount >> 5] |= 1 << (ObjCount & 31);
      O = O->Next;
      ObjCount++;
   }

   St->DoorOpen = (NewLev->Door.Image == NewLev->DoorOpen);

   return St;
}

void ReturnState(struct Level *NewLev, struct LevelState *State)
{
   struct Object *O;
   int ObjCount;

   if (!State || !NewLev)
      return;

   ObjCount = 0;
   O = NewLev->AllObjects;
   while (O) {
      if (State->Data[ObjCount >> 5] & (1 << (ObjCount & 31)))
         O->Flags |= OBJFLAGS_VISIBLE;
      else
         O->Flags &= ~OBJFLAGS_VISIBLE;
      O = O->Next;
      ObjCount++;
   }

   if (State->DoorOpen)
      SetDoorOpen(NewLev);
}

void FreeState(struct LevelState *State)
{
   if (!State)
      return;

   free((void *)State->Data);
   free(State);
}

void SetInitialState(struct Level *lvl)
{
   ReturnState(lvl, lvl->InitialState);
}

void SetDoorOpen(struct Level *Lvl)
{
   Lvl->Door.Image = Lvl->DoorOpen;
}
