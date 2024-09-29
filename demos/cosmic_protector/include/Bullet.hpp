#ifndef BULLET_HPP
#define BULLET_HPP

class Bullet : public Entity
{
public:
   bool logic(int step);

   Bullet(float x, float y, float radius, float speed, float angle,
      int lifetime, int damage, int bitmapID, Entity *shooter);
   ~Bullet(void);
protected:
   A5O_BITMAP *bitmap;
   float speed;
   float angle;
   int lifetime;
   float cosa;
   float sina;
   Entity *shooter;
   int damage;
   bool playerOnly;
};

#endif // BULLET_HPP

