#include "cosmic_protector.hpp"

bool Bullet::logic(int step)
{
   lifetime -= step;
   if (lifetime <= 0)
      return false;

   dx = speed * cosa * step;
   dy = speed * sina * step;

   Entity *c;
   if (playerOnly)
      c = getPlayerCollision();
   else
      c = getAllCollision();
   if (c && c != shooter) {
      c->hit(damage);
      my_play_sample(RES_COLLISION);
      return false;
   }

   Entity::wrap();

   if (!Entity::logic(step))
      return false;

   return true;
}

Bullet::Bullet(float x, float y, float radius, float speed, float angle,
   int lifetime, int damage, int bitmapID, Entity *shooter) :
   playerOnly(false)
{
   this->x = x;
   this->y = y;
   this->radius = radius;
   this->speed = speed;
   this->angle = angle;
   this->lifetime = lifetime;
   this->shooter = shooter;
   this->damage = damage;

   cosa = cos(angle);
   sina = sin(angle);

   ResourceManager& rm = ResourceManager::getInstance();
   bitmap = (A5O_BITMAP *)rm.getData(bitmapID);

   isDestructable = false;
}

Bullet::~Bullet(void)
{
}

