#ifndef __DEMO_ANIM_H__
#define __DEMO_ANIM_H__

#include "global.h"

struct Animation {
   ALLEGRO_BITMAP *Animation[3], *Still, *Slow, *Medium, *Fast, *CBitmap;
   ALLEGRO_SAMPLE_INSTANCE *SkateVoice;
   double TimeCount;
};

extern ALLEGRO_BITMAP *GetCurrentBitmap(struct Animation *);
extern void AdvanceAnimation(struct Animation *, double Distance,
                             int OnPlatform);
extern struct Animation *SeedPlayerAnimation(void);
extern void FreePlayerAnimation(struct Animation *Anim);
extern void PauseAnimation(struct Animation *Anim);
extern void UnpauseAnimation(struct Animation *Anim);

#endif
