#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include "defines.h"
#include "game.h"
#include "global.h"
#include "menus.h"
#include "music.h"
#include "physics.h"
#include "vcontroller.h"

/*

	All variables relevant to player position and level / level status. Only
	thing of note is that PlayerPos is a 3 element array - it contains
	player position (in elements 0 and 1) and rotation (in element 2)

*/
int KeyFlags;
int RequiredObjectsLeft, TotalObjectsLeft;
double PlayerPos[3], PlayerVec[2];
double ScrollPos[2];
struct Animation *PlayerAnim = NULL;
struct Level *Lvl = NULL;
struct LevelState *LvlState = NULL;
double LeftWindow = -120, RightWindow = 120;
double Pusher = 0;
static A5O_COLOR cloud_color;

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
A5O_BITMAP *water;
A5O_SAMPLE_INSTANCE *WaterVoice, *WaterVoice2;
A5O_SAMPLE *WaveNoise = NULL;

A5O_BITMAP *cloud;
al_fixed *TanTable = NULL;
int CalibRes = 0;

double TexX;
double WaveY;

struct {
   double x, y;
} Clouds[8];
double CloudX;

char *load_game_resources(const char *data_path)
{
   int c;
   A5O_PATH *path;

   printf("load_game_resources\n");
   path = al_create_path_for_directory(data_path);
   al_set_path_filename(path, "level.txt");
   Lvl = LoadLevel(al_path_cstr(path, '/'), 15);
   al_destroy_path(path);

   if (!Lvl)
      return GetLevelError();

   ReturnState(Lvl, LvlState);

   PlayerAnim = SeedPlayerAnimation();
   cloud = ObtainBitmap("cloud");
   water = ObtainBitmap("water");

   c = 480 << 2;
   TanTable = (al_fixed *) malloc(sizeof(al_fixed) * c);
   while (c--)
      TanTable[c] =
         al_ftofix(tan
                ((3.141592654f / 2.0f -
                  atan2(0.75f *
                        (((float)(c * 2) / (float)(480 << 2)) - 1), 1)
                 )));

   WaveNoise = ObtainSample("wave");
   
   cloud_color = al_get_pixel(cloud, 0, 0);

   return NULL;
}

void unload_game_resources(void)
{
   printf("unload_game_resources\n");
   FreePlayerAnimation(PlayerAnim);
   PlayerAnim = NULL;

   FreeState(LvlState);
   LvlState = BorrowState(Lvl);
   FreeLevel(Lvl);
   Lvl = NULL;

   if (TanTable) {
      free(TanTable);
      TanTable = NULL;
   }
}

static void DeInit(void)
{
   if (WaterVoice) {
      al_stop_sample_instance(WaterVoice);
      al_destroy_sample_instance(WaterVoice);
      WaterVoice = NULL;
   }
   if (WaterVoice2) {
      al_stop_sample_instance(WaterVoice2);
      al_destroy_sample_instance(WaterVoice2);
      WaterVoice2 = NULL;
   }
   PauseAnimation(PlayerAnim);
}

static void GenericInit(void)
{
   int c;

   CloudX = 0;
   Clouds[0].x = Clouds[0].y = 0;
   for (c = 1; c < 8; c++) {
      Clouds[c].x =
         Clouds[c - 1].x + al_get_bitmap_width(cloud) + 8 +
            rand() * (screen_width * 480 / screen_height / 8) / RAND_MAX;
      Clouds[c].y = rand() * 0.5 * 480 / RAND_MAX;
   }

   WaterVoice = al_create_sample_instance(WaveNoise);
   al_set_sample_instance_playmode(WaterVoice, A5O_PLAYMODE_BIDIR);
   al_set_sample_instance_gain(WaterVoice, 0.5);
   al_attach_sample_instance_to_mixer(WaterVoice, al_get_default_mixer());
   al_play_sample_instance(WaterVoice);

   WaterVoice2 = al_create_sample_instance(WaveNoise);
   al_set_sample_instance_playmode(WaterVoice2,
                      A5O_PLAYMODE_BIDIR);
   al_set_sample_instance_gain(WaterVoice2, 0.25);
   al_attach_sample_instance_to_mixer(WaterVoice2, al_get_default_mixer());
   al_play_sample_instance(WaterVoice2);

   play_music(DEMO_MIDI_INGAME, true);
   UnpauseAnimation(PlayerAnim);
}

static void ContinueInit(void)
{
   GenericInit();
   CurrentID = continueid();
}

static void GameInit(void)
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

static void DrawClouds(void)
{
   int c = 8;
   double x1, y1, x2, y2, x;

   while (c--) {
      x = Clouds[c].x + CloudX;

      x1 = (x - sin(x) * 1.4);
      x2 = (x + al_get_bitmap_width(cloud) + sin(x) * 1.4);
      y1 = (Clouds[c].y - cos(x) * 1.4);
      y2 = (Clouds[c].y + al_get_bitmap_height(cloud) + cos(x) * 1.4);

      if (x2 < 0) {
         Clouds[c].x =
            Clouds[(c - 1) & 7].x + al_get_bitmap_width(cloud) + 8 +
            (rand() * 32.0 / RAND_MAX);
         Clouds[c].y = rand() * 0.5 * screen_height / RAND_MAX;
      }
      else {
         al_draw_scaled_bitmap(cloud, 0, 0, al_get_bitmap_width(cloud),
            al_get_bitmap_height(cloud),
            x1, y1, x2 - x1, y2 - y1, 0);
      }
   }
}

static void set_v(A5O_VERTEX *vt, double x, double y, double u, double v)
{
   vt->x = x;
   vt->y = y;
   vt->z = 0;
   vt->u = u;
   vt->v = v;
   vt->color = al_map_rgb_f(1, 1, 1);
}

static void GameDraw(void)
{
   A5O_BITMAP *ch;
   double depth;
   A5O_VERTEX Points[4];
   A5O_TRANSFORM transform;
   int chw, chh;
   float w;
   
   al_identity_transform(&transform);
   al_scale_transform(&transform, screen_height / 480.0, screen_height / 480.0);
   al_use_transform(&transform);

   w = screen_width * 480.0 / screen_height;

   /* draw background */
   {
      int c = 480 * 4 - 1;
      double y2, lowy = 480, y1, u;
      double index;

      while (c > 480 * 2 + 15) {

         depth = al_fixtof(al_fixmul(TanTable[c], (int)((ScrollPos[1] - 240) * 4096.0f)));

         if (depth > -261 && depth < -5.0f) {
            int d = ((int)(depth * 65536)) & (256 * 65536 - 1);
            y1 = lowy;
            index = (d / 65536.0 - 1.0) * A5O_PI * 2 * 8 / 128.0f;

            y2 = c / 4.0 - 125.0f * sin(index + WaveY) / depth;

            if (y2 < lowy) {
               lowy = ceil(y2);

               u = 64 + ScrollPos[0] / 8 + TexX;
               set_v(Points + 0, 0, y2, u + depth * 4, depth + 5);
               set_v(Points + 1, w, y2, u - depth * 4, depth + 5);
               set_v(Points + 2, w, y1, u - depth * 4, depth + 5);
               set_v(Points + 3, 0, y1, u + depth * 4, depth + 5);

               al_draw_prim(Points, NULL, water, 0, 4, A5O_PRIM_TRIANGLE_FAN);
            }
         }
         c--;
      }

      al_set_clipping_rectangle(0, 0, screen_width, lowy * screen_height / 480 + 1);
      al_clear_to_color(cloud_color);
      DrawClouds();
      al_set_clipping_rectangle(0, 0, screen_width, screen_height);
   }

   /* draw interactable parts of level */
   DrawLevelBackground(Lvl, ScrollPos);

#ifdef DEBUG_EDGES
   solid_mode();
   struct Edge *E = Lvl->AllEdges;

   while (E) {
      line(buffer,
           E->EndPoints[0]->Normal[0] - x + (screen_width >> 1),
           E->EndPoints[0]->Normal[1] - y + (screen_height >> 1),
           E->EndPoints[1]->Normal[0] - x + (screen_width >> 1),
           E->EndPoints[1]->Normal[1] - y + (screen_height >> 1), al_map_rgb(0, 0,
                                                                     0));
      E = E->Next;
   }
#endif

#ifdef DEBUG_OBJECTS
   struct Object *O = Lvl->AllObjects;

   while (O) {
      rect(buffer,
           O->Bounder.TL.Pos[0] - x + (screen_width >> 1),
           O->Bounder.TL.Pos[1] - y + (screen_height >> 1),
           O->Bounder.BR.Pos[0] - x + (screen_width >> 1),
           O->Bounder.BR.Pos[1] - y + (screen_height >> 1), al_map_rgb(0, 0, 0));
      circle(buffer, O->Pos[0] - x + (screen_width >> 1),
             O->Pos[1] - y + (screen_height >> 1), O->ObjType->Radius, al_map_rgb(0,
                                                                          0,
                                                                          0));
      O = O->Next;
   }
#endif

   /* add player sprite */
   ch = GetCurrentBitmap(PlayerAnim);
   chw = al_get_bitmap_width(ch);
   chh = al_get_bitmap_height(ch);

   al_draw_scaled_rotated_bitmap(ch, chw / 2.0, chh / 2.0,
      (PlayerPos[0] - ScrollPos[0]) + w / 2,
      (PlayerPos[1] - ScrollPos[1]) + 480.0 / 2,
      1, 1, PlayerPos[2],
      KeyFlags & KEYFLAG_FLIP ? A5O_FLIP_HORIZONTAL : 0);

   DrawLevelForeground(Lvl);
   
   al_identity_transform(&transform);
   al_use_transform(&transform);

   /* add status */
   demo_textprintf(demo_font,
                 screen_width - al_get_text_width(demo_font,
                                        "Items Required: 1000"), 8,
                 al_map_rgb(255, 255, 255), "Items Required: %d",
                 RequiredObjectsLeft);
   demo_textprintf(demo_font,
                 screen_width - al_get_text_width(demo_font,
                                        "Items Remaining: 1000"),
                 8 + al_get_font_line_height(demo_font), al_map_rgb(255, 255, 255),
                 "Items Remaining: %d", TotalObjectsLeft);
}

static int GameUpdate(void)
{
   struct QuadTreeNode *CollTree;
   struct Container *EPtr;

   if (RequiredObjectsLeft < 0)
      return DEMO_STATE_SUCCESS;

   TexX += 0.3f;
   WaveY -= 0.02f;
   CloudX -= 0.125f;

   /* scrolling */
   if ((PlayerPos[0] - ScrollPos[0]) < LeftWindow)
      ScrollPos[0] = PlayerPos[0] - LeftWindow;
   if ((PlayerPos[0] - ScrollPos[0]) > RightWindow)
      ScrollPos[0] = PlayerPos[0] - RightWindow;
   if ((PlayerPos[1] - ScrollPos[1]) < -80)
      ScrollPos[1] = PlayerPos[1] + 80;
   if ((PlayerPos[1] - ScrollPos[1]) > 80)
      ScrollPos[1] = PlayerPos[1] - 80;

   if ((KeyFlags & KEYFLAG_FLIP) ^ (((PlayerPos[2] < A5O_PI * 0.5f)
                                     && (PlayerPos[2] >
                                         -A5O_PI * 0.5f)) ? 0 : KEYFLAG_FLIP)
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
         double SqDistance, XDiff, YDiff;

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
                     play_sound(EPtr->Content.O->ObjType->
                                 CollectNoise, 255, 128, 1000, 0);
                  RequiredObjectsLeft = -1;
               }
            } else {
               EPtr->Content.O->Flags &= ~OBJFLAGS_VISIBLE;
               if (EPtr->Content.O->ObjType->CollectNoise)
                  play_sound(EPtr->Content.O->ObjType->CollectNoise,
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

   return key_pressed(A5O_KEY_ESCAPE) ? DEMO_STATE_MAIN_MENU : CurrentID;
}

void destroy_game()
{
   FreeState(LvlState);
   LvlState = NULL;
}

/*

	GAMESTATE generation things - see comments in gamestate.h for more
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
