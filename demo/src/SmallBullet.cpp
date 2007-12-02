#include "a5teroids.hpp"

void SmallBullet::render(void)
{
   al_draw_rotated_bitmap(bitmap, radius, radius, x, y, 0.0f, 0);
}

SmallBullet::SmallBullet(float x, float y, float angle, Entity *shooter) :
   Bullet(x, y, 4, 0.5f, angle, 600, 1, RES_SMALLBULLET, shooter)
{
}

