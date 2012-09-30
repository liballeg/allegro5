#include "cosmic_protector.hpp"

void LargeBullet::render(int offx, int offy)
{
   al_draw_rotated_bitmap(bitmap, radius, radius, offx + x, offy + y,
      angle+(ALLEGRO_PI/2), 0);
}

LargeBullet::LargeBullet(float x, float y, float angle, Entity *shooter) :
   Bullet(x, y, 6, 0.6f, angle, 600, 2, RES_LARGEBULLET, shooter)
{
}

