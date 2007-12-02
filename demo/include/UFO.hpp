#ifndef UFO_HPP
#define UFO_HPP

class UFO : public Enemy
{
public:
   const int SHOT_SPEED;
   const int ANIMATION_SPEED;

   bool logic(int step);
   void render(void);

   //bool init(lua_State *luaState);

//   UFO();
   UFO(float x, float y, float dx, float dy);
   ~UFO();
protected:
   ALLEGRO_BITMAP *bitmaps[3];
   int nextShot;
   int bitmapFrame;
   int bitmapFrameCount;
   float speed_x;
   float speed_y;
};

#endif // UFO_HPP
