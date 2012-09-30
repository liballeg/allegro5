#include "cosmic_protector.hpp"

Wave::Wave() :
   rippleNum(0)
{
}

bool Wave::next(void)
{
   rippleNum++;

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

