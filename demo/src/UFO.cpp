#include "a5teroids.hpp"

/*
bool UFO::init(lua_State *luaState)
{
   x = getNumberFromTable(luaState, "x");
   y = getNumberFromTable(luaState, "y");
   dx = getNumberFromTable(luaState, "dx");
   dy = getNumberFromTable(luaState, "dy");
}
*/

bool UFO::logic(int step)
{
   Player *p = (Player *)getPlayerCollision();
   if (p) {
      explode();
      p->hit(1);
      p->die();
      my_play_sample(RES_COLLISION);
      return false;
   }

   if ((al_current_time() * 1000) > nextShot) {
      nextShot = (int)(al_current_time() * 1000) + SHOT_SPEED;
      ResourceManager& rm = ResourceManager::getInstance();
      Player *p = (Player *)rm.getData(RES_PLAYER);
      float px = p->getX();
      float py = p->getY();
      float shot_angle = atan2(x-px, y-py) + ALLEGRO_PI/2.0f;
      LargeSlowBullet *b = new LargeSlowBullet(x, y, shot_angle, this);
      new_entities.push_back(b);
      my_play_sample(RES_FIRELARGE);
   }

   bitmapFrameCount -= step;
   if (bitmapFrameCount <= 0) {
      bitmapFrameCount = ANIMATION_SPEED;
      bitmapFrame++;
      bitmapFrame %= 3; // loop
   }
   
   dx = speed_x * step;
   dy = speed_y * step;

   Entity::wrap();

   if (!Entity::logic(step))
      return false;

   return true;
}

void UFO::render(int offx, int offy)
{
   al_draw_rotated_bitmap(bitmaps[bitmapFrame], radius, radius, offx + x,
      offy + y, 0.0f, 0);
}

// added x, y, dx, dy
UFO::UFO(float x, float y, float speed_x, float speed_y) :
   SHOT_SPEED(3000),
   ANIMATION_SPEED(150)
{
   this->x = x;
   this->y = y;
   this->speed_x = speed_x;
   this->speed_y = speed_y;

   radius = 32;
   hp = 8;
   points = 500;
   ufo = true;

   nextShot = (int)(al_current_time() * 1000.0) + SHOT_SPEED;

   ResourceManager& rm = ResourceManager::getInstance();
   bitmaps[0] = (ALLEGRO_BITMAP *)rm.getData(RES_UFO0);
   bitmaps[1] = (ALLEGRO_BITMAP *)rm.getData(RES_UFO1);
   bitmaps[2] = (ALLEGRO_BITMAP *)rm.getData(RES_UFO2);
   bitmapFrame = 0;
   bitmapFrameCount = ANIMATION_SPEED;
}

UFO::~UFO()
{
}

