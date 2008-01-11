#include <allegro.h>
#include <math.h>
#include "../include/anim.h"
#include "../include/game.h"
#include "../include/level.h"


float LastSpeedStore = 0, LastSpeed;
int OnLand;

BITMAP *GetCurrentBitmap(struct Animation *Anim)
{
   LastSpeedStore = LastSpeed;
   if (Anim->SkateVoice >= 0) {
      if (LastSpeed >= 1.0f && OnLand)
         voice_set_volume(Anim->SkateVoice, 256 - (256 / LastSpeed));
      else
         voice_set_volume(Anim->SkateVoice, 0);
   }
   return Anim->CBitmap;
}

void AdvanceAnimation(struct Animation *Anim, float Distance, int OnPlatform)
{
   Anim->TimeCount += Distance;
   OnLand = OnPlatform;

   if (!OnPlatform) {
      Anim->CBitmap = Anim->Fast;
   } else {
      /* obtain speed */
      LastSpeed = Distance = fabs(Distance);

      Anim->CBitmap = Anim->Fast;
      if ((Distance < 12.0f) && (LastSpeedStore < 12.0f))
         Anim->CBitmap = Anim->Medium;
      if ((Distance < 5.0f) && (LastSpeedStore < 5.0f))
         Anim->CBitmap =
            (KeyFlags & (KEYFLAG_LEFT | KEYFLAG_RIGHT)) ? Anim->
            Animation[((unsigned int)Anim->TimeCount) % 3] : Anim->Slow;
   }
}

struct Animation *SeedPlayerAnimation(void)
{
   struct Animation *Anim =
      (struct Animation *)malloc(sizeof(struct Animation));
   SAMPLE *Sound;

   Anim->Animation[0] = ObtainBitmap("skater2");
   Anim->Animation[1] = ObtainBitmap("skater3");
   Anim->Animation[2] = ObtainBitmap("skater4");

   Anim->CBitmap = Anim->Still = ObtainBitmap("skater1");
   Anim->Slow = ObtainBitmap("skateslow");
   Anim->Medium = ObtainBitmap("skatemed");
   Anim->Fast = ObtainBitmap("skatefast");

   if ((Sound = ObtainSample("skating"))) {
      Anim->SkateVoice = allocate_voice(Sound);
      voice_set_playmode(Anim->SkateVoice, PLAYMODE_BIDIR | PLAYMODE_LOOP);
   } else
      Anim->SkateVoice = -1;

   return Anim;
}

void FreePlayerAnimation(struct Animation *Anim)
{
   int c;

   if (Anim) {
      c = 3;
      while (c--)
         destroy_bitmap(Anim->Animation[c]);
      destroy_bitmap(Anim->Still);
      destroy_bitmap(Anim->Slow);
      destroy_bitmap(Anim->Medium);
      destroy_bitmap(Anim->Fast);

      if (Anim->SkateVoice >= 0) {
         voice_stop(Anim->SkateVoice);
         release_voice(Anim->SkateVoice);
      }

      free(Anim);
   }
}

void PauseAnimation(struct Animation *Anim)
{
   if (Anim->SkateVoice >= 0)
      voice_stop(Anim->SkateVoice);
}

void UnpauseAnimation(struct Animation *Anim)
{
   if (Anim->SkateVoice >= 0)
      voice_start(Anim->SkateVoice);
}

/*
BITMAP *GetCurrentBitmap(struct Animation *Anim)
{
	return Anim->bmps[((int)Anim->TimeCount)&3];
}

void AdvanceAnimation(struct Animation *Anim, float Distance, int OnPlatform, int Forward)
{
	Anim->TimeCount += Distance*0.125f;
}

struct Animation *SeedPlayerAnimation(void)
{
	struct Animation *Anim = (struct Animation *)malloc(sizeof(struct Animation));
	Anim->bmps = (BITMAP **)malloc(sizeof(BITMAP *)*4);
	Anim->bmps[0] = load_bitmap("man-frame1.bmp", NULL);
	Anim->bmps[1] = load_bitmap("man-frame2.bmp", NULL);
	Anim->bmps[2] = load_bitmap("man-frame3.bmp", NULL);
	Anim->bmps[3] = load_bitmap("man-frame4.bmp", NULL);
	return Anim;
}

void FreePlayerAnimation(struct Animation *Anim)
{
	if(Anim)
	{
		if(Anim->bmps)
		{
			destroy_bitmap(Anim->bmps[0]);
			destroy_bitmap(Anim->bmps[1]);
			destroy_bitmap(Anim->bmps[2]);
			destroy_bitmap(Anim->bmps[3]);
			free((void *)Anim->bmps);
		}
		free(Anim);
	}
}
*/
