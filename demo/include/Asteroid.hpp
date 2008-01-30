#ifndef ASTEROID_HPP
#define ASTEROID_HPP

class Asteroid : public Enemy
{
public:
   bool logic(int step);
   void render(void);

   //bool init(lua_State *luaState);
   void init(float x, float y, float speed_x, float speed_y, float da);

   Asteroid(float radius, int bitmapID);
   ~Asteroid();
protected:
   float da;
   float angle;
   float speed_x;
   float speed_y;
   ALLEGRO_BITMAP *bitmap;
};

#endif // ASTEROID_HPP
