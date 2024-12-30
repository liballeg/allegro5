#include "cosmic_protector.hpp"

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
   y += dy;
   if (x < 0)
      x += BB_W;
   if (x >= BB_W)
      x -= BB_W;
   if (y < 0)
      y += BB_H;
   if (y >= BB_H)
      y -= BB_H;
}

void Entity::render_four(ALLEGRO_COLOR tint)
{
   int ox = 0;
   render(0, 0, tint);
   if (x > BB_W / 2) {
      ox = -BB_W;
      render(ox, 0, tint);
   }
   else {
      ox = BB_W;
      render(ox, 0, tint);
   }
   if (y > BB_H / 2) {
      render(0, -BB_H, tint);
      render(ox, -BB_H, tint);
   }
   else {
      render(0, BB_H, tint);
      render(ox, BB_H, tint);
   }
}

void Entity::render(int x, int y, ALLEGRO_COLOR c)
{
   (void)c; // To use c must override this in a sub-class.
   render(x, y);
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
   if (e)
      return e;
   return getPlayerCollision();
}

// Returns true if dead
bool Entity::hit(int damage)
{
   hp -= damage;

   hilightCount = 500;

   if (hp <= 0) {
      explode();
      return true;
   }

   return false;
}

void Entity::explode(void)
{
   bool big;
   if (radius >= 32)
      big = true;
   else
      big = false;
   Explosion *e = new Explosion(x, y, big);
   new_entities.push_back(e);
   if (big)
      my_play_sample(RES_BIGEXPLOSION);
   else
      my_play_sample(RES_SMALLEXPLOSION);
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

