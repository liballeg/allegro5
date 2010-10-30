#ifndef ASTEROID_HPP
#define ASTEROID_HPP

class Asteroid : public Entity
{
public:
   bool logic(int step);
   void render(int offx, int offy);

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
