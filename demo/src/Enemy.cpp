#include "a5teroids.hpp"

Enemy::~Enemy()
{
}

/*
void createEnemy(char *id, lua_State *luaState)
{
   Enemy *enemy;

   if (!strcmp(id, "la")) {
      enemy = new LargeAsteroid();
   }
   else if (!strcmp(id, "ma")) {
      enemy = new MediumAsteroid();
   }
   else if (!strcmp(id, "ufo")) {
      enemy = new UFO();
   }
   else { // Small Asteroid default
      enemy = new SmallAsteroid();
   }

   enemy->init(luaState);
   entities.push_back(enemy);

   lua_pushstring(luaState, "powerup");
   lua_gettable(luaState, -2);
   if (lua_isnumber(luaState, -1)) {
      int poweruptype = (int)lua_tonumber(luaState, -1);
      enemy->setPowerUp(poweruptype);
   }
   lua_pop(luaState, 1);
}
*/

