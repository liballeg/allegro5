#include <allegro5/allegro.h>
#include <math.h>
#include "anim.h"
#include "game.h"
#include "level.h"


double LastSpeedStore = 0, LastSpeed;
int OnLand;

ALLEGRO_BITMAP *GetCurrentBitmap(struct Animation *Anim)
{
   LastSpeedStore = LastSpeed;
   if (Anim->SkateVoice) {
      if (LastSpeed >= 1.0f && OnLand)
         al_set_sample_instance_gain(Anim->SkateVoice, 1.0 - (1.0 / LastSpeed));
      else
         al_set_sample_instance_gain(Anim->SkateVoice, 0);
   }
   return Anim->CBitmap;
}

void AdvanceAnimation(struct Animation *Anim, double Distance, int OnPlatform)
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
   ALLEGRO_SAMPLE *Sound;

   Anim->Animation[0] = ObtainBitmap("skater2");
   Anim->Animation[1] = ObtainBitmap("skater3");
   Anim->Animation[2] = ObtainBitmap("skater4");

   Anim->CBitmap = Anim->Still = ObtainBitmap("skater1");
   Anim->Slow = ObtainBitmap("skateslow");
   Anim->Medium = ObtainBitmap("skatemed");
   Anim->Fast = ObtainBitmap("skatefast");

   if ((Sound = ObtainSample("skating"))) {
      Anim->SkateVoice = al_create_sample_instance(Sound);
      al_set_sample_instance_playmode(Anim->SkateVoice, ALLEGRO_PLAYMODE_BIDIR);
   } else
      Anim->SkateVoice = NULL;

   return Anim;
}

void FreePlayerAnimation(struct Animation *Anim)
{
   if (Anim) {

      if (Anim->SkateVoice) {
         al_stop_sample_instance(Anim->SkateVoice);
         al_destroy_sample_instance(Anim->SkateVoice);
      }

      free(Anim);
   }
}

void PauseAnimation(struct Animation *Anim)
{
   if (Anim->SkateVoice)
      al_stop_sample_instance(Anim->SkateVoice);
}

void UnpauseAnimation(struct Animation *Anim)
{
   if (Anim->SkateVoice) {
      al_attach_sample_instance_to_mixer(Anim->SkateVoice, al_get_default_mixer());
      al_play_sample_instance(Anim->SkateVoice);
   }
}

/*
ALLEGRO_BITMAP *GetCurrentBitmap(struct Animation *Anim)
{
   return Anim->bmps[((int)Anim->TimeCount)&3];
}

void AdvanceAnimation(struct Animation *Anim, double Distance, int OnPlatform, int Forward)
{
   Anim->TimeCount += Distance*0.125f;
}

struct Animation *SeedPlayerAnimation(void)
{
   struct Animation *Anim = (struct Animation *)malloc(sizeof(struct Animation));
   Anim->bmps = (ALLEGRO_BITMAP **)malloc(sizeof(ALLEGRO_BITMAP *)*4);
   Anim->bmps[0] = load_bitmap("man-frame1.bmp", NULL);
   Anim->bmps[1] = load_bitmap("man-frame2.bmp", NULL);
   Anim->bmps[2] = load_bitmap("man-frame3.bmp", NULL);
   Anim->bmps[3] = load_bitmap("man-frame4.bmp", NULL);
   return Anim;
}

void FreePlayerAnimation(struct Animation *Anim)
{
   if(Anim) {
      if(Anim->bmps) {
         al_destroy_bitmap(Anim->bmps[0]);
         al_destroy_bitmap(Anim->bmps[1]);
         al_destroy_bitmap(Anim->bmps[2]);
         al_destroy_bitmap(Anim->bmps[3]);
         free((void *)Anim->bmps);
      }
      free(Anim);
   }
}
*/
