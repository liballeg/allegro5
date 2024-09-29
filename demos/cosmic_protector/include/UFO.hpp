#ifndef UFO_HPP
#define UFO_HPP

class UFO : public Entity
{
public:
   const int SHOT_SPEED;
   const int ANIMATION_SPEED;

   bool logic(int step);
   void render(int offx, int offy);
   void render(int offx, int offy, A5O_COLOR tint);

   UFO(float x, float y, float dx, float dy);
   ~UFO();
protected:
   A5O_BITMAP *bitmaps[3];
   int nextShot;
   int bitmapFrame;
   int bitmapFrameCount;
   float speed_x;
   float speed_y;
};

#endif // UFO_HPP
