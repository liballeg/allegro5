#include "cosmic_protector.hpp"

bool checkCircleCollision(float x1, float y1, float r1,
   float x2, float y2, float r2)
{
   float dx = x1-x2;
   float dy = y1-y2;
   float dist = sqrt(dx*dx + dy*dy);
   return dist < (r1+r2);
}

