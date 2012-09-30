#include "cosmic_protector.hpp"

void Asteroid::init(float x, float y, float speed_x, float speed_y, float da)
{
   this->x = x;
   this->y = y;
   this->speed_x = speed_x;
   this->speed_y = speed_y;
   this->da = da;
}

bool Asteroid::logic(int step)
{
   angle -= da * step;

   Player *p = (Player *)getPlayerCollision();
   if (p) {
      explode();
      p->hit(1);
      my_play_sample(RES_COLLISION);
      return false;
   }

   dx = speed_x * step;
   dy = speed_y * step;

   Entity::wrap();

   if (!Entity::logic(step))
      return false;

   return true;
}

void Asteroid::render(int offx, int offy)
{
   al_draw_rotated_bitmap(bitmap, radius, radius, offx + x, offy + y, angle, 0);
}

Asteroid::Asteroid(float radius, int bitmapID)
{
   this->radius = radius;

   angle = randf(0.0f, ALLEGRO_PI*2.0f);

   ResourceManager& rm = ResourceManager::getInstance();
   bitmap = (ALLEGRO_BITMAP *)rm.getData(bitmapID);
}

Asteroid::~Asteroid()
{
}

