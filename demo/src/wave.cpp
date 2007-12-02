#include "a5teroids.hpp"

Wave* Wave::wave = 0;

Wave& Wave::getInstance(void)
{
   if (!wave) {
      wave = new Wave();
   }
   return *wave;
}

void Wave::init(void)
{
   /*
   script.destroy();
   script.create(false);
   script.source(getResource("scripts/waves.%s", SCRIPT_EXT));
   luaState = script.getLuaState();
   waveNum = 0;
   beginWave();
   */
   rippleNum = 0;
}

bool Wave::beginWave(void)
{
   /*
   waveNum++;

   char name[100];
   sprintf(name, "wave%d", waveNum);
   
   lua_getglobal(luaState, name);
   if (!lua_istable(luaState, -1)) {
      lua_pop(luaState, 1);
      return false;
   }

   rippleNum = 0;

   showWave(waveNum);

   return true;
   */
   return true;
}

bool Wave::next(void)
{
   rippleNum++;

   /*
   lua_pushnumber(luaState, rippleNum);
   lua_gettable(luaState, -2);

   if (!lua_istable(luaState, -1)) {
      lua_pop(luaState, 2);
      if (!beginWave())
         return false;
      return next();
   }

   int enemyNum = 1;

   for (;; enemyNum++) {
      lua_pushnumber(luaState, enemyNum);
      lua_gettable(luaState, -2);
      if (!lua_istable(luaState, -1)) {
         lua_pop(luaState, 2);
         break;
      }
      lua_pushstring(luaState, "id");
      lua_gettable(luaState, -2);
      char id[100];
      strcpy(id, lua_tostring(luaState, -1));
      lua_pop(luaState, 1);
      createEnemy(id, luaState);
      lua_pop(luaState, 1);
   }

   dumpLuaStack(luaState);
   */

   // added

   showWave(rippleNum);

   for (int i = 0; i < rippleNum+1; i++) {
      float x, y, dx, dy, da;
      if (rand() % 2) {
         x = -randf(32.0f, 100.0f);
      }
      else {
         x = BB_W+randf(32.0f, 100.0f);
      }
      if (rand() % 2) {
         y = -randf(32.0f, 70.0f);
      }
      else {
         y = BB_H+randf(32.0f, 70.0f);
      }
      dx = randf(0.06f, 0.12f);
      dy = randf(0.04f, 0.08f);
      da = randf(0.001f, 0.005f);
      if (rand() % 2) dx = -dx;
      if (rand() % 2) dy = -dy;
      if (rand() % 2) da = -da;
      LargeAsteroid *la = new LargeAsteroid(x, y, dx, dy, da);
      if ((rand() % 5) == 0) {
         la->setPowerUp(rand() % 2);
      }
      entities.push_back(la);
   }

   return true;
}

Wave::Wave(void)
{
}

Wave::~Wave(void)
{
}

