#ifndef __LEVEL_H
#define __LEVEL_H

#include <allegro5/allegro.h>
#include "quadtree.h"

extern struct Level *LoadLevel(char const *name, int collradius);
extern void FreeLevel(struct Level *lvl);
extern char *GetLevelError(void);

extern void SetInitialState(struct Level *lvl);

extern void DrawLevelBackground(struct Level *lev,
				double *pos);
extern void DrawLevelForeground(struct Level *lev);

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

extern A5O_BITMAP *ObtainBitmap(const char *name);
extern A5O_SAMPLE *ObtainSample(const char *name);

#endif
