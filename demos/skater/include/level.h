#ifndef __LEVEL_H
#define __LEVEL_H

#include <allegro.h>
#include "quadtree.h"

extern struct Level *LoadLevel(char *name, int collradius);
extern void FreeLevel(struct Level *lvl);
extern char *GetLevelError(void);

extern void SetInitialState(struct Level *lvl);

extern void DrawLevelBackground(BITMAP * target, struct Level *lev,
				float *pos);
extern void DrawLevelForeground(BITMAP * target, struct Level *lev);

/* level state struct, which stores the current state of the level - perfect for any future implementation
of loading/saving games */
struct LevelState {
   int Length;
   uint32_t *Data;
   int DoorOpen;
};
extern void SetDoorOpen(struct Level *Lvl);
extern struct LevelState *BorrowState(struct Level *);
extern void ReturnState(struct Level *, struct LevelState *);
extern void FreeState(struct LevelState *);

extern BITMAP *ObtainBitmap(const char *name);
extern SAMPLE *ObtainSample(const char *name);

#endif
