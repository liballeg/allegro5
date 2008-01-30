#include "a5teroids.hpp"

bool Entity::logic(int step)
{
   if (hp <= 0) {
      spawn();
      ResourceManager& rm = ResourceManager::getInstance();
      Player *p = (Player *)rm.getData(RES_PLAYER);
      p->addScore(points);
      return false;
   }
   
   if (hilightCount > 0) {
      hilightCount -= step;
   }

   return true;
}

float Entity::getX(void)
{
   return x;
}

float Entity::getY(void)
{
   return y;
}

float Entity::getRadius(void)
{
   return radius;
}

bool Entity::getDestructable(void)
{
   return isDestructable;
}

bool Entity::isHighlighted(void)
{
   return hilightCount > 0;
}

bool Entity::isUFO(void)
{
   return ufo;
}

void Entity::setPowerUp(int type)
{
   powerup = type;
}

void Entity::wrap(void)
{
   x += dx;
   if (x < -radius && dx < 0.0f)
      x = BB_W+radius;
   else if (x > (BB_W+radius) && dx > 0.0f)
      x = -radius;
   y += dy;
   if (y < -radius && dy < 0.0f)
      y = BB_H+radius;
   else if (y > (BB_H+radius) && dy > 0.0f)
      y = -radius;
}

Entity *Entity::checkCollisions(std::list<Entity *>& e)
{
   std::list<Entity *>::iterator it;

   for (it = e.begin(); it != e.end(); it++) {
      Entity *entity = *it;
      if (entity == this || !entity->getDestructable())
         continue;
      float ex = entity->getX();
      float ey = entity->getY();
      float er = entity->getRadius();
      if (checkCircleCollision(ex, ey, er, x, y, radius))
         return entity;
   }

   return 0;
}

Entity *Entity::getPlayerCollision(void)
{
   std::list<Entity *> e;

   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);

   e.push_back(player);

   Entity *ret = checkCollisions(e);
   e.clear();
   return ret;
}

Entity *Entity::getEntityCollision(void)
{
   return checkCollisions(entities);
}

Entity *Entity::getAllCollision(void)
{
   Entity *e = getEntityCollision();
   if (e) return e;
   return getPlayerCollision();
}

// Returns true if dead
bool Entity::hit(int damage)
{
   hp-=damage;

   hilightCount = 500;

   bool ret;

   if (hp <= 0) {
      ret = true;
      explode();
   }
   else
      ret = false;

   return ret;
}

void Entity::explode(void)
{
   bool big;
   if (radius >= 32) big = true;
   else big = false;
   Explosion *e = new Explosion(x, y, big);
   new_entities.push_back(e);
   if (big) my_play_sample(RES_BIGEXPLOSION);
   else my_play_sample(RES_SMALLEXPLOSION);
}

void Entity::spawn(void)
{
   if (powerup >= 0) {
      PowerUp *p = new PowerUp(x, y, powerup);
      new_entities.push_back(p);
   }
}

Entity::Entity() :
   isDestructable(true),
   hp(1),
   powerup(-1),
   hilightCount(0),
   points(0),
   ufo(false)
{
}

