#ifndef __DEMO_ANIM_H__
#define __DEMO_ANIM_H__

#include <allegro.h>

struct Animation {
   BITMAP *Animation[3], *Still, *Slow, *Medium, *Fast, *CBitmap;
   int SkateVoice;
   float TimeCount;
};

extern BITMAP *GetCurrentBitmap(struct Animation *);
extern void AdvanceAnimation(struct Animation *, float Distance,
			     int OnPlatform);
extern struct Animation *SeedPlayerAnimation(void);
extern void FreePlayerAnimation(struct Animation *Anim);
extern void PauseAnimation(struct Animation *Anim);
extern void UnpauseAnimation(struct Animation *Anim);

#endif
