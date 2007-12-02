#include "a5teroids.hpp"

void LargeBullet::render(void)
{
   al_draw_rotated_bitmap(bitmap, radius, radius, x, y, angle-(M_PI/2), 0);
}

LargeBullet::LargeBullet(float x, float y, float angle, Entity *shooter) :
   Bullet(x, y, 6, 0.6f, angle, 600, 2, RES_LARGEBULLET, shooter)
{
}

