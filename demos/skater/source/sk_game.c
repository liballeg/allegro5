#include <allegro.h>
#include <math.h>
#include "../include/defines.h"
#include "../include/game.h"
#include "../include/global.h"
#include "../include/menus.h"
#include "../include/music.h"
#include "../include/physics.h"
#include "../include/virtctl.h"

/*

	All variables relevant to player position and level / level status. Only
	thing of note is that PlayerPos is a 3 element array - it contains
	player position (in elements 0 and 1) and rotation (in element 2)

*/
int KeyFlags;
int RequiredObjectsLeft, TotalObjectsLeft;
float PlayerPos[3], PlayerVec[2];
float ScrollPos[2];
struct Animation *PlayerAnim = NULL;
struct Level *Lvl = NULL;
struct LevelState *LvlState = NULL;
float LeftWindow = -120, RightWindow = 120;
float Pusher = 0;

#define PLAYER_STRENGTH 0.14f

/*

	ID things, for GAMESTATE purposes. Note that two menu entries head this way -
	"new game" and "continue game", so we have to IDs
*/
static int _newid = DEMO_STATE_NEW_GAME;
static int newid(void)
{
   return _newid;
}

static int _continueid = DEMO_STATE_CONTINUE_GAME;
static int continueid(void)
{
   return _continueid;
}

int CurrentID;

/* background stuff */
BITMAP *water;
int WaterVoice = -1, WaterVoice2 = -1;
SAMPLE *WaveNoise = NULL;

BITMAP *cloud;
fixed *TanTable = NULL;
int CalibRes = 0;

fixed TexX;
float WaveY;

DATAFILE *game_audio;

struct {
   fixed x, y;
} Clouds[8];
int CloudX;

fixed Scale;

char *load_game_resources(void)
{
   int c;
   char AudioPath[DEMO_PATH_LENGTH];

   get_executable_name(AudioPath, DEMO_PATH_LENGTH);
   replace_filename(AudioPath, AudioPath, "demo.dat#audio.dat",
                    DEMO_PATH_LENGTH);
   game_audio = load_datafile(AudioPath);

   Scale = itofix(SCREEN_H) / 480;
   Lvl = LoadLevel("level.dat", 15);

   if (!Lvl)
      return GetLevelError();

   ReturnState(Lvl, LvlState);

   PlayerAnim = SeedPlayerAnimation();
   cloud = ObtainBitmap("cloud");
   water = ObtainBitmap("water");

   c = SCREEN_H << 2;
   TanTable = (fixed *) malloc(sizeof(fixed) * c);
   while (c--)
      TanTable[c] =
         ftofix(tan
                ((3.141592654f / 2.0f -
                  atan2(0.75f *
                        (((float)(c * 2) / (float)(SCREEN_H << 2)) - 1), 1)
                 )));

   WaveNoise = ObtainSample("wave");

   return NULL;
}

void unload_game_resources(void)
{
   if (game_audio) {
      unload_datafile(game_audio);
      game_audio = NULL;
   }

   FreePlayerAnimation(PlayerAnim);
   PlayerAnim = NULL;

   if (cloud) {
      destroy_bitmap(cloud);
      cloud = NULL;
   }

   if (water) {
      destroy_bitmap(water);
      water = NULL;
   }

   FreeState(LvlState);
   LvlState = BorrowState(Lvl);
   FreeLevel(Lvl);
   Lvl = NULL;

   if (TanTable) {
      free(TanTable);
      TanTable = NULL;
   }
}

void DeInit(void)
{
   if (WaterVoice >= 0) {
      voice_stop(WaterVoice);
      deallocate_voice(WaterVoice);
      WaterVoice = -1;
   }
   if (WaterVoice2 >= 0) {
      voice_stop(WaterVoice2);
      deallocate_voice(WaterVoice2);
      WaterVoice2 = -1;
   }
   PauseAnimation(PlayerAnim);
}

void GenericInit(void)
{
   int c;

   set_palette(demo_data[DEMO_GAME_PALETTE].dat);
   CloudX = 0;
   Clouds[0].x = Clouds[0].y = 0;
   for (c = 1; c < 8; c++) {
      Clouds[c].x =
         Clouds[c - 1].x + itofix(cloud->w + 8) +
         itofix(rand() / (RAND_MAX / 32));
      Clouds[c].y = rand() / (RAND_MAX / (SCREEN_H >> 1));
   }

   WaterVoice = allocate_voice(WaveNoise);
   voice_set_playmode(WaterVoice, PLAYMODE_BIDIR | PLAYMODE_LOOP);
   voice_set_volume(WaterVoice, 48);
   voice_start(WaterVoice);

   WaterVoice2 = allocate_voice(WaveNoise);
   voice_set_playmode(WaterVoice2,
                      PLAYMODE_BIDIR | PLAYMODE_LOOP | PLAYMODE_BACKWARD);
   voice_set_volume(WaterVoice2, 64);
   voice_start(WaterVoice2);

   play_music(DEMO_MIDI_INGAME, TRUE);
   UnpauseAnimation(PlayerAnim);
}

void ContinueInit(void)
{
   GenericInit();
   CurrentID = continueid();
}

void GameInit(void)
{
   KeyFlags = 0;
   enable_continue_game();

   SetInitialState(Lvl);
   ScrollPos[0] = PlayerPos[0] = Lvl->PlayerStartPos[0];
   ScrollPos[1] = PlayerPos[1] = Lvl->PlayerStartPos[1];
   PlayerVec[0] = PlayerVec[1] = 0;
   RequiredObjectsLeft = Lvl->ObjectsRequired;
   TotalObjectsLeft = Lvl->TotalObjects;

   GenericInit();
   CurrentID = newid();
}

void DrawClouds(BITMAP *buffer)
{
   int c = 8;
   fixed x1, y1, x2, y2, x;

   while (c--) {
      x = Clouds[c].x + CloudX;

      x1 = fixmul(x - (fixsin(x) << 2), Scale);
      x2 = fixmul(x + itofix(cloud->w) + (fixsin(x) << 2), Scale);
      y1 = itofix(Clouds[c].y) - fixmul(fixcos(x) << 2, Scale);
      y2 =
         itofix(Clouds[c].y) + fixmul(itofix(cloud->h),
                                    Scale) + fixmul(fixcos(x) << 2, Scale);

      if (x2 < 0) {
         Clouds[c].x =
            Clouds[(c - 1) & 7].x + itofix(cloud->w + 8) +
            itofix(rand() / (RAND_MAX / 32));
         Clouds[c].y = rand() / (RAND_MAX / (SCREEN_H >> 1));
      } else
         stretch_blit(cloud, buffer, 0, 0, cloud->w, cloud->h,
                      fixtoi(x1), fixtoi(y1), fixtoi(x2 - x1),
                      fixtoi(y2 - y1));
   }
}

void GameDraw(BITMAP *buffer)
{
   BITMAP *ch;
   fixed depth;
   V3D Points[4];

   /* draw background */
   {
      int c = (SCREEN_H << 2) - 1;
      fixed y2, lowy = itofix(SCREEN_H), y1;
      float index;

      Points[0].x = Points[3].x = 0;
      Points[1].x = Points[2].x = itofix(SCREEN_W);

      while (c > (SCREEN_H << 1) + 15) {
         depth = fixmul(TanTable[c], (int)((ScrollPos[1] - 240) * 4096.0f));
         if (depth > itofix(-261) && depth < ftofix(-5.0f)) {
            y1 = lowy;
            index =
               (fixtof((depth) & (itofix(256) - 1)) -
                1.0f) * AL_PI * 2 * 8 / 128.0f;

            y2 =
               itofix(c >> 2) - fixdiv(ftofix(125.0f * sin(index + WaveY)),
                                     depth);

            if (y2 < lowy) {
               lowy = y2;

               Points[2].y = Points[3].y = y1;
               Points[0].y = Points[1].y = y2;

               Points[0].v = Points[1].v = Points[2].v = Points[3].v =
                  depth + itofix(5);

               Points[0].u = Points[3].u =
                  itofix(64) + (depth << 2) + (int)(ScrollPos[0] * 8192.0f) +
                  TexX;
               Points[1].u = Points[2].u =
                  itofix(64) - (depth << 2) + (int)(ScrollPos[0] * 8192.0f) +
                  TexX;

               quad3d(buffer, POLYTYPE_ATEX, water, &Points[0],
                      &Points[1], &Points[2], &Points[3]);
            }
         }
         c--;
      }

      set_clip_rect(buffer, 0, 0, buffer->w, fixtoi(lowy));
      clear_to_color(buffer, getpixel(cloud, 0, 0));
      DrawClouds(buffer);
      set_clip_rect(buffer, 0, 0, buffer->w, buffer->h);
   }

   /* draw interactable parts of level */
   DrawLevelBackground(buffer, Lvl, ScrollPos);

#ifdef DEBUG_EDGES
   solid_mode();
   struct Edge *E = Lvl->AllEdges;

   while (E) {
      line(buffer,
           E->EndPoints[0]->Normal[0] - x + (SCREEN_W >> 1),
           E->EndPoints[0]->Normal[1] - y + (SCREEN_H >> 1),
           E->EndPoints[1]->Normal[0] - x + (SCREEN_W >> 1),
           E->EndPoints[1]->Normal[1] - y + (SCREEN_H >> 1), makecol(0, 0,
                                                                     0));
      E = E->Next;
   }
#endif

#ifdef DEBUG_OBJECTS
   struct Object *O = Lvl->AllObjects;

   while (O) {
      rect(buffer,
           O->Bounder.TL.Pos[0] - x + (SCREEN_W >> 1),
           O->Bounder.TL.Pos[1] - y + (SCREEN_H >> 1),
           O->Bounder.BR.Pos[0] - x + (SCREEN_W >> 1),
           O->Bounder.BR.Pos[1] - y + (SCREEN_H >> 1), makecol(0, 0, 0));
      circle(buffer, O->Pos[0] - x + (SCREEN_W >> 1),
             O->Pos[1] - y + (SCREEN_H >> 1), O->ObjType->Radius, makecol(0,
                                                                          0,
                                                                          0));
      O = O->Next;
   }
#endif

   /* add player sprite */
   ch = GetCurrentBitmap(PlayerAnim);
   if (KeyFlags & KEYFLAG_FLIP)
      rotate_scaled_sprite_v_flip(buffer, ch,
                                  fixtoi(fixmul
                                         (ftofix
                                          (PlayerPos[0] -
                                           (ch->w >> 1) -
                                           ScrollPos[0]),
                                          Scale)) + (SCREEN_W >> 1),
                                  fixtoi(fixmul
                                         (ftofix
                                          (PlayerPos[1] -
                                           (ch->h >> 1) -
                                           ScrollPos[1]),
                                          Scale)) + (SCREEN_H >> 1),
                                  ftofix(PlayerPos[2] * 128.0f /
                                         AL_PI) + itofix(128), Scale);
   else
      rotate_scaled_sprite(buffer, ch,
                           fixtoi(fixmul
                                  (ftofix
                                   (PlayerPos[0] - (ch->w >> 1) -
                                    ScrollPos[0]),
                                   Scale)) + (SCREEN_W >> 1),
                           fixtoi(fixmul
                                  (ftofix
                                   (PlayerPos[1] - (ch->h >> 1) -
                                    ScrollPos[1]),
                                   Scale)) + (SCREEN_H >> 1),
                           ftofix(PlayerPos[2] * 128.0f / AL_PI), Scale);

   DrawLevelForeground(buffer, Lvl);

   /* add status */
   demo_textprintf(buffer, demo_font,
                 SCREEN_W - text_length(demo_font,
                                        "Items Required: 1000"), 8,
                 makecol(255, 255, 255), -1, "Items Required: %d",
                 RequiredObjectsLeft);
   demo_textprintf(buffer, demo_font,
                 SCREEN_W - text_length(demo_font,
                                        "Items Remaining: 1000"),
                 8 + text_height(demo_font), makecol(255, 255, 255),
                 -1, "Items Remaining: %d", TotalObjectsLeft);
}

int GameUpdate(void)
{
   struct QuadTreeNode *CollTree;
   struct Container *EPtr;

   if (RequiredObjectsLeft < 0)
      return DEMO_STATE_SUCCESS;

   TexX += ftofix(0.3f);
   WaveY -= 0.02f;
   CloudX -= ftofix(0.125f);

   /* scrolling */
   if ((PlayerPos[0] - ScrollPos[0]) < LeftWindow)
      ScrollPos[0] = PlayerPos[0] - LeftWindow;
   if ((PlayerPos[0] - ScrollPos[0]) > RightWindow)
      ScrollPos[0] = PlayerPos[0] - RightWindow;
   if ((PlayerPos[1] - ScrollPos[1]) < -80)
      ScrollPos[1] = PlayerPos[1] + 80;
   if ((PlayerPos[1] - ScrollPos[1]) > 80)
      ScrollPos[1] = PlayerPos[1] - 80;

   if ((KeyFlags & KEYFLAG_FLIP) ^ (((PlayerPos[2] < AL_PI * 0.5f)
                                     && (PlayerPos[2] >
                                         -AL_PI * 0.5f)) ? 0 : KEYFLAG_FLIP)
      ) {
      if (LeftWindow < 0)
         LeftWindow++;
      if (RightWindow < 120)
         RightWindow++;
   } else {
      if (LeftWindow > -120)
         LeftWindow--;
      if (RightWindow > 0)
         RightWindow--;
   }

   /* update controls */
   controller[controller_id]->poll(controller[controller_id]);
   if (controller[controller_id]->button[0]) {
      if (KeyFlags & KEYFLAG_LEFT) {
         Pusher += 0.005f;
         if (Pusher >= PLAYER_STRENGTH)
            Pusher = PLAYER_STRENGTH;
      } else {
         KeyFlags |= KEYFLAG_LEFT;
         Pusher = 0;
      }
   } else
      KeyFlags &= ~KEYFLAG_LEFT;

   if (controller[controller_id]->button[1]) {
      if (KeyFlags & KEYFLAG_RIGHT) {
         Pusher += 0.005f;
         if (Pusher >= PLAYER_STRENGTH)
            Pusher = PLAYER_STRENGTH;
      } else {
         KeyFlags |= KEYFLAG_RIGHT;
         Pusher = 0;
      }
   } else
      KeyFlags &= ~KEYFLAG_RIGHT;

   if (controller[controller_id]->button[2])
      KeyFlags |= KEYFLAG_JUMP;
   else {
      if ((KeyFlags & KEYFLAG_JUMPING) && PlayerVec[1] < -2.0f)
         PlayerVec[1] = -2.0f;
      KeyFlags &= ~(KEYFLAG_JUMP | KEYFLAG_JUMP_ISSUED);
   }

   /* run physics */
   CollTree = RunPhysics(Lvl, PlayerPos, PlayerVec, 0.6f, PlayerAnim);

   /* check whether any objects are collected */
   EPtr = CollTree->Contents;
   while (EPtr && EPtr->Type == OBJECT) {
      if (EPtr->Content.O->Flags & OBJFLAGS_VISIBLE) {
         float SqDistance, XDiff, YDiff;

         XDiff = PlayerPos[0] - EPtr->Content.O->Pos[0];
         YDiff = PlayerPos[1] - EPtr->Content.O->Pos[1];
         SqDistance = XDiff * XDiff + YDiff * YDiff;

         if (SqDistance <=
             EPtr->Content.O->ObjType->Radius *
             EPtr->Content.O->ObjType->Radius) {
            /* collision! */
            if (EPtr->Content.O->Flags & OBJFLAGS_DOOR) {
               if (!RequiredObjectsLeft) {
                  if (EPtr->Content.O->ObjType->CollectNoise)
                     play_sample(EPtr->Content.O->ObjType->
                                 CollectNoise, 255, 128, 1000, 0);
                  RequiredObjectsLeft = -1;
               }
            } else {
               EPtr->Content.O->Flags &= ~OBJFLAGS_VISIBLE;
               if (EPtr->Content.O->ObjType->CollectNoise)
                  play_sample(EPtr->Content.O->ObjType->CollectNoise,
                              255, 128, 1000, 0);

               if (RequiredObjectsLeft > 0) {
                  RequiredObjectsLeft--;
                  if (!RequiredObjectsLeft)
                     SetDoorOpen(Lvl);
               }
               TotalObjectsLeft--;
            }
         }
      }

      EPtr = EPtr->Next;
   }

   return key[KEY_ESC] ? DEMO_STATE_MAIN_MENU : CurrentID;
}

void destroy_game()
{
   FreeState(LvlState);
   LvlState = NULL;
}

/*

	GAMESTATE generation things - see comments in gmestate.h for more
	information

*/
void create_new_game(GAMESTATE * game)
{
   game->id = newid;
   game->init = GameInit;
   game->update = GameUpdate;
   game->draw = GameDraw;
   game->deinit = DeInit;
}

void create_continue_game(GAMESTATE * game)
{
   game->id = continueid;
   game->init = ContinueInit;
   game->update = GameUpdate;
   game->draw = GameDraw;
   game->deinit = DeInit;
}
