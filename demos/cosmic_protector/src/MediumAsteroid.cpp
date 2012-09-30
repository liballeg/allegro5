#include "cosmic_protector.hpp"

void MediumAsteroid::spawn(void)
{
   // Break into small fragments
   for (int i = 0; i < 3; i++) {
      float dx = randf(0.06f, 0.12f);
      float dy = randf(0.04f, 0.08f);
      float da = randf(0.001, 0.005);
      if (rand() % 2) dx = -dx;
      if (rand() % 2) dy = -dy;
      if (rand() % 2) da = -da;
      SmallAsteroid *sa = new SmallAsteroid();
      sa->init(x, y, dx, dy, da);
      new_entities.push_back(sa);
   }

   Entity::spawn();
}

MediumAsteroid::MediumAsteroid() :
   Asteroid(20, RES_MEDIUMASTEROID)
{
   hp = 4;
   points = 50;
}

MediumAsteroid::~MediumAsteroid()
{
}

