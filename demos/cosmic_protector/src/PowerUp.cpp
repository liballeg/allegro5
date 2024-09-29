#include "cosmic_protector.hpp"

bool PowerUp::logic(int step)
{
   angle += da * step;

   x += dx;
   y += dy;

   if (x < -radius || x > BB_W+radius || y < -radius || y > BB_H+radius) {
      return false;
   }

   Player *p = (Player *)getPlayerCollision();
   if (p) {
      p->givePowerUp(type);
      my_play_sample(RES_POWERUP);
      return false;
   }

   if (!Entity::logic(step))
      return false;

   return true;
}

void PowerUp::render(int offx, int offy)
{
   al_draw_rotated_bitmap(bitmap, radius, radius, offx + x, offy + y, angle, 0);
}

PowerUp::PowerUp(float x, float y, int type) :
   SPIN_SPEED(0.002f)
{
   this->x = x;
   this->y = y;
   this->type = type;

   dx = randf(0.5f, 1.2f);
   dy = randf(0.5f, 1.2f);
   radius = 16;
   isDestructable = false;
   hp = 1;

   da = (rand() % 2) ? -SPIN_SPEED : SPIN_SPEED;

   ResourceManager& rm = ResourceManager::getInstance();
   
   switch (type) {
      case POWERUP_LIFE:
         bitmap = (A5O_BITMAP *)rm.getData(RES_LIFEPOWERUP);
         break;
      default:
         type = POWERUP_WEAPON;
         bitmap = (A5O_BITMAP *)rm.getData(RES_WEAPONPOWERUP);
         break;
   }
}

