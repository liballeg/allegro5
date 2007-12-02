#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "a5teroids.hpp"

class Enemy : public Entity
{
public:
   //virtual bool init(lua_State *luaState) = 0;

   virtual ~Enemy();
};

//void createEnemy(char *id, lua_State *luaState);

#endif // ENEMY_HPP
