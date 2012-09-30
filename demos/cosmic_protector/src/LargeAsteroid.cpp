#include "cosmic_protector.hpp"

void LargeAsteroid::spawn(void)
{
   // Break into small fragments
   for (int i = 0; i < 2; i++) {
      float dx = randf(0.06f, 0.12f);
      float dy = randf(0.04f, 0.08f);
      float da = randf(0.001, 0.005);
      if (rand() % 2) dx = -dx;
      if (rand() % 2) dy = -dy;
      if (rand() % 2) da = -da;
      MediumAsteroid *ma = new MediumAsteroid();
      ma->init(x, y, dx, dy, da);
      new_entities.push_back(ma);
   }

   Entity::spawn();
}

LargeAsteroid::LargeAsteroid() :
   Asteroid(32, RES_LARGEASTEROID)
{
   hp = 6;
   points = 100;
}

LargeAsteroid::LargeAsteroid(float x, float y, float speed_x, float speed_y, float da) :
   Asteroid(32, RES_LARGEASTEROID)
{
   init(x, y, speed_x, speed_y, da);
   hp = 6;
   points = 100;
}

LargeAsteroid::~LargeAsteroid()
{
}

