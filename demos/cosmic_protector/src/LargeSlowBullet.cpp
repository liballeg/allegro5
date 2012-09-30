#include "cosmic_protector.hpp"

LargeSlowBullet::LargeSlowBullet(float x, float y, float angle, Entity *shooter) :
   LargeBullet(x, y, angle, shooter)
{
   speed = 0.20f;
   lifetime += 1000;
   playerOnly = true;
}

