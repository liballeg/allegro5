#include "cosmic_protector.hpp"

void SmallBullet::render(int offx, int offy)
{
   al_draw_rotated_bitmap(bitmap, radius, radius, offx + x, offy + y, 0.0f, 0);
}

SmallBullet::SmallBullet(float x, float y, float angle, Entity *shooter) :
   Bullet(x, y, 4, 0.5f, angle, 600, 1, RES_SMALLBULLET, shooter)
{
}

