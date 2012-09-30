#include "cosmic_protector.hpp"

bool Explosion::logic(int step)
{
   frameCount -= step;
   if (frameCount <= 0) {
      currFrame++;
      frameCount = FRAME_TIME;
      if (currFrame >= NUM_FRAMES)
         return false;
   }

   return true;
}

void Explosion::render(int offx, int offy)
{
   ResourceManager& rm = ResourceManager::getInstance();
   ALLEGRO_BITMAP *bitmap;
   int bitmapIndex;

   if (big)
      bitmapIndex = RES_LARGEEXPLOSION0 + currFrame;
   else
      bitmapIndex = RES_SMALLEXPLOSION0 + currFrame;

   bitmap = (ALLEGRO_BITMAP *)rm.getData(bitmapIndex);

   al_draw_rotated_bitmap(bitmap, radius, radius, offx + x, offy + y, 0, 0);
}

Explosion::Explosion(float x, float y, bool big)
{
   this->x = x;
   this->y = y;
   dx = 0.0f;
   dy = 0.0f;
   radius = (big) ? 32 : 12;
   isDestructable = false;
   hp = 1;

   frameCount = FRAME_TIME;
   currFrame = 0;
   this->big = big;
}

