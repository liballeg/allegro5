#ifndef SMALLBULLET_HPP
#define SMALLBULLET_HPP

class SmallBullet : public Bullet
{
public:
   void render(int offx, int offy);

   SmallBullet(float x, float y, float angle, Entity *shooter);
};

#endif // SMALLBULLET_HPP

