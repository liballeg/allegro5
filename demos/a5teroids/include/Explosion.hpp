#ifndef EXPLOSION_HPP
#define EXPLOSION_HPP

class Explosion : public Entity
{
public:
   static const int NUM_FRAMES = 5;
   static const int FRAME_TIME = 100;

   bool logic(int step);
   void render(int offx, int offy);

   Explosion(float x, float y, bool big);
private:
   int frameCount;
   int currFrame;
   bool big;
};

#endif // EXPLOSION_HPP

