#ifndef LARGEASTEROID_HPP
#define LARGEASTEROID_HPP

class LargeAsteroid : public Asteroid
{
public:
   void spawn(void);

   LargeAsteroid();
   LargeAsteroid(float x, float y, float speed_x, float speed_y, float da);
   ~LargeAsteroid();
};

#endif // LARGEASTEROID_HPP

