#ifndef POWERUP_HPP
#define POWERUP_HPP

const int POWERUP_LIFE = 0;
const int POWERUP_WEAPON = 1;

class PowerUp : public Entity
{
public:
   const float SPIN_SPEED;

   bool logic(int step);
   void render(int offx, int offy);

   PowerUp(float x, float y, int type);
private:
   int type;
   ALLEGRO_BITMAP *bitmap;
   float angle;
   float da;
};

#endif // POWERUP_HPP

