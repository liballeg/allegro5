#ifndef LARGEBULLET_HPP
#define LARGEBULLET_HPP

class LargeBullet : public Bullet
{
public:
   void render(int offx, int offy);

   LargeBullet(float x, float y, float angle, Entity *shooter);
};

#endif // LARGEBULLET_HPP

