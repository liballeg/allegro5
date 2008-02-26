#include "a5teroids.hpp"

bool Player::logic(int step)
{
   if (!isDestructable && invincibleCount > 0) {
      invincibleCount -= step;
      if (invincibleCount <= 0) {
         isDestructable = true;
         if (lives <= 0)
            return false;
      }
   }

   if (lives <= 0) return true;

   ResourceManager& rm = ResourceManager::getInstance();
   Input *input = (Input *)rm.getData(RES_INPUT);

   if (input->lr() < 0.0f) {
      angle += 0.005f * step;
   }
   else if (input->lr() > 0.0f) {
      angle -= 0.005f * step;
   }

   if (input->ud() < 0.0f) {
      dx += ACCEL * step * cos(-angle);
      if (dx > MAX_SPEED)
         dx = MAX_SPEED;
      else if (dx < MIN_SPEED)
         dx = MIN_SPEED;
      dy += ACCEL * step * sin(-angle);
      if (dy > MAX_SPEED)
         dy = MAX_SPEED;
      else if (dy < MIN_SPEED)
         dy = MIN_SPEED;
      draw_trail = true;
   }
   else {
      if (dx > 0)
         dx -= DECCEL * step;
      else if (dx < 0)
         dx += DECCEL * step;
      if (dx > -0.1f && dx < 0.1f)
         dx = 0;
      if (dy > 0)
         dy -= DECCEL * step;
      else if (dy < 0)
         dy += DECCEL * step;
      if (dy > -0.1f && dy < 0.1f)
         dy = 0;
      draw_trail = false;
   }

   int shotRate;
   switch (weapon) {
      case WEAPON_SMALL:
         shotRate = 300;
         break;
      case WEAPON_LARGE:
         shotRate = 250;
         break;
      default:
         shotRate = INT_MAX;
         break;
   }

   if ((lastShot+shotRate) < (al_current_time() * 1000) && input->b1()) {
      lastShot = (int) (al_current_time() * 1000);
      float realAngle = -angle;
      float bx = x + radius * cos(realAngle);
      float by = y + radius * sin(realAngle);
      Bullet *b = 0;
      int resourceID = RES_FIRESMALL;
      switch (weapon) {
         case WEAPON_SMALL:
            b = new SmallBullet(bx, by, angle, this);
            resourceID = RES_FIRESMALL;
            break;
         case WEAPON_LARGE:
            b = new LargeBullet(bx, by, angle, this);
            resourceID = RES_FIRELARGE;
            break;
      }
      if (b) {
         my_play_sample(resourceID);
         new_entities.push_back(b);
      }
   }

   if (input->cheat()) {
      al_rest(0.250);
      std::list<Entity *>::iterator it;
      for (it = entities.begin(); it != entities.end(); it++) {
         Entity *e = *it;
         delete e;
      }
      entities.clear();
   }

   Entity::wrap();

   if (!Entity::logic(step))
      return false;

   return true;
}

void Player::render(void)
{
   ResourceManager& rm = ResourceManager::getInstance();

   if (lives <= 0) {
      int w = al_get_bitmap_width(highscoreBitmap);
      int h = al_get_bitmap_height(highscoreBitmap);
      al_draw_rotated_bitmap(highscoreBitmap, w/2, h/2, BB_W/2, BB_H/2,
         0.0f, 0);
      return;
   }

   if (!isDestructable) {
      al_draw_rotated_bitmap(trans_bitmap, radius, radius, x, y,
         angle-(M_PI/2.0f), 0);
   }
   else {
      al_draw_rotated_bitmap(bitmap, radius, radius, x, y,
         angle-(M_PI/2.0f), 0);
   }
   if (draw_trail) {
      int tw = al_get_bitmap_width(trail_bitmap);
      float tx = x - radius * cos(-angle);
      float ty = y - radius * sin(-angle);
      al_draw_rotated_bitmap(trail_bitmap, tw/2, 0,
         tx, ty, angle-(M_PI/2.0f), 0);
   }

   al_draw_bitmap(icon, 2, 2, 0);

   small_font = (A5FONT_FONT *)rm.getData(RES_SMALLFONT);

   a5font_textprintf(small_font, 20, 2, "x%d", lives);

   a5font_textprintf(small_font, 2, 18, "%d", score);
}

bool Player::hit(int damage)
{
   Entity::hit(damage);
   die();
   return true;
}

Player::Player() :
   weapon(WEAPON_SMALL),
   lastShot(0),
   score(0)
{
}

Player::~Player()
{
}
   
void Player::destroy(void)
{
   /*
   al_destroy_bitmap(bitmap);
   al_destroy_bitmap(trans_bitmap);
   al_destroy_bitmap(trail_bitmap);
   al_destroy_bitmap(icon);
   al_destroy_bitmap(highscoreBitmap);
   */
}

bool Player::load(void)
{
   bitmap = al_load_bitmap(getResource("gfx/ship.tga"));
   if (!bitmap) {
      debug_message("Error loading %s\n", getResource("gfx/ship.tga"));
      return false;
   }

   trans_bitmap = al_create_bitmap(al_get_bitmap_width(bitmap),
      al_get_bitmap_height(bitmap));
   if (!trans_bitmap) {
      debug_message("Error loading %s\n", getResource("gfx/ship_trans.tga"));
      al_destroy_bitmap(bitmap);
      return false;
   }
   /* Make a translucent copy of the ship */
   ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
   al_set_target_bitmap(trans_bitmap);
   int bitmap_format = al_get_bitmap_format(bitmap);
   int trans_format = al_get_bitmap_format(trans_bitmap);
   for (int py = 0; py < al_get_bitmap_height(bitmap); py++) {
      for (int px = 0; px < al_get_bitmap_height(bitmap); px++) {
         ALLEGRO_COLOR color = al_get_pixel(bitmap, px, py);
         unsigned char r, g, b, a;
         al_unmap_rgba(color, &r, &g, &b, &a);
         if (a != 0) {
            a = 160;
            color = al_map_rgba(r, g, b, a);
         }
         al_put_pixel(px, py, color);
      }
   }
   al_set_target_bitmap(old_target);

   trail_bitmap = al_load_bitmap(getResource("gfx/trail.tga"));
   if (!trail_bitmap) {
      debug_message("Error loading %s\n", getResource("gfx/trail.tga"));
      al_destroy_bitmap(bitmap);
      al_destroy_bitmap(trans_bitmap);
      return false;
   }

   icon = al_load_bitmap(getResource("gfx/ship_icon.tga"));
   if (!icon) {
      debug_message("Error loading %s\n", getResource("gfx/icon.tga"));
      al_destroy_bitmap(bitmap);
      al_destroy_bitmap(trans_bitmap);
      al_destroy_bitmap(trail_bitmap);
      return false;
   }

   highscoreBitmap = al_create_bitmap(300, 200);
   al_set_target_bitmap(highscoreBitmap);
   al_clear(al_map_rgba(0, 0, 0, 0));
   al_set_target_bitmap(old_target);

   radius = al_get_bitmap_width(bitmap)/2;

   newGame();
   reset();

   return true;
}

void* Player::get(void)
{
   return this;
}

void Player::newGame(void)
{
   lives = 5;
   hp = 1;
   score = 0;
   isDestructable = true;
}

void Player::reset(void)
{
   x = BB_W/2;
   y = BB_H/2;
   dx = 0;
   dy = 0;
   angle = M_PI/2;
   draw_trail = false;
   weapon = WEAPON_SMALL;
}

void Player::die(void)
{
   shake();
   reset();

   lives--;
   if (lives <= 0) {
      // game over
      isDestructable = false;
      invincibleCount = 20000;
      ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
      al_set_target_bitmap(highscoreBitmap);
      int w = al_get_bitmap_width(highscoreBitmap);
      int h = al_get_bitmap_height(highscoreBitmap);
      ResourceManager& rm = ResourceManager::getInstance();
      A5FONT_FONT *large_font = (A5FONT_FONT *)rm.getData(RES_LARGEFONT);
      A5FONT_FONT *small_font = (A5FONT_FONT *)rm.getData(RES_SMALLFONT);
      a5font_textprintf_centre(large_font, w/2, h/2-16, "GAME OVER");
      a5font_textprintf_centre(small_font, w/2, h/2+16, "%d Points", score);
      al_set_target_bitmap(old_target);
   }
   else {
      hp = 1;
      isDestructable = false;
      invincibleCount = 3000;
   }
}

void Player::givePowerUp(int type)
{
   switch (type) {
      case POWERUP_LIFE:
         lives++;
         break;
      case POWERUP_WEAPON:
         weapon = WEAPON_LARGE;
         break;
   }
}

void Player::addScore(int points)
{
   score += points;
}

int Player::getScore(void)
{
   return score;
}

