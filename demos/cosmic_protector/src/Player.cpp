#include "cosmic_protector.hpp"

const float Player::MAX_SPEED = 10.0f;
const float Player::MIN_SPEED = -10.0f;
const float Player::ACCEL = 0.006f;
const float Player::DECCEL = 0.001f;

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

   if (lives <= 0)
      return true;

   ResourceManager& rm = ResourceManager::getInstance();
   Input *input = (Input *)rm.getData(RES_INPUT);

   if (input->lr() < 0.0f) {
      angle -= 0.005f * step;
   }
   else if (input->lr() > 0.0f) {
      angle += 0.005f * step;
   }

   if (input->ud() < 0.0f) {
      dx += ACCEL * step * cos(angle);
      if (dx > MAX_SPEED)
         dx = MAX_SPEED;
      else if (dx < MIN_SPEED)
         dx = MIN_SPEED;
      dy += ACCEL * step * sin(angle);
      if (dy > MAX_SPEED)
         dy = MAX_SPEED;
      else if (dy < MIN_SPEED)
         dy = MIN_SPEED;
      draw_trail = true;
   }
   else if (input->ud() > 0.0f) {
      if (dx > 0)
         dx -= ACCEL * step;
      else if (dx < 0)
         dx += ACCEL * step;
      if (dx > -0.1f && dx < 0.1f)
         dx = 0;
      if (dy > 0)
         dy -= ACCEL * step;
      else if (dy < 0)
         dy += ACCEL * step;
      if (dy > -0.1f && dy < 0.1f)
         dy = 0;
      draw_trail = false;
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

   int now = (int) (al_get_time() * 1000.0);
   if ((lastShot+shotRate) < now && input->b1()) {
      lastShot = now;
      float realAngle = angle;
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

void Player::render_extra(void)
{
   ResourceManager& rm = ResourceManager::getInstance();

   if (lives <= 0) {
      int w = al_get_bitmap_width(highscoreBitmap);
      int h = al_get_bitmap_height(highscoreBitmap);
      al_draw_bitmap(highscoreBitmap, (BB_W-w)/2, (BB_H-h)/2, 0);
      return;
   }

   ALLEGRO_STATE st;
   al_store_state(&st, ALLEGRO_STATE_BLENDER);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
   al_draw_bitmap(icon, 1, 2, 0);
   al_restore_state(&st);
   ALLEGRO_FONT *small_font = (ALLEGRO_FONT *)rm.getData(RES_SMALLFONT);
   al_draw_textf(small_font, al_map_rgb(255, 255, 255), 20, 2, 0, "x%d", lives);
   al_draw_textf(small_font, al_map_rgb(255, 255, 255), 2, 18, 0, "%d", score);
}

void Player::render(int offx, int offy, ALLEGRO_COLOR tint)
{
   if (lives <= 0)
      return;

   int rx = (int)(offx + x), ry = (int)(offy + y);

   if (!isDestructable) {
      al_draw_tinted_rotated_bitmap(trans_bitmap, tint,
         draw_radius, draw_radius, rx, ry,
         angle+(ALLEGRO_PI/2.0f), 0);
   }
   else {
      al_draw_tinted_rotated_bitmap(bitmap,
         tint, draw_radius, draw_radius, rx, ry,
         angle+(ALLEGRO_PI/2.0f), 0);
   }
   if (draw_trail) {
      int tw = al_get_bitmap_width(trail_bitmap);
      int th = al_get_bitmap_height(trail_bitmap);
      float ca = (ALLEGRO_PI*2)+angle;
      float a = ca + ((210.0f / 180.0f) * ALLEGRO_PI);
      float tx = rx + 42.0f * cos(a);
      float ty = ry + 42.0f * sin(a);
      al_draw_tinted_rotated_bitmap(trail_bitmap, tint, tw, th/2,
         tx, ty, a, 0);
      a = ca + ((150.0f / 180.0f) * ALLEGRO_PI);
      tx = rx + 42.0f * cos(a);
      ty = ry + 42.0f * sin(a);
      al_draw_tinted_rotated_bitmap(trail_bitmap, tint, tw, th/2,
         tx, ty, a, 0);
   }
}

void Player::render(int offx, int offy)
{
   render(offx, offy, al_map_rgb(255, 255, 255));
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
   score(0),
   bitmap(0),
   trans_bitmap(0),
   trail_bitmap(0),
   icon(0),
   highscoreBitmap(0)
{
}

Player::~Player()
{
}

void Player::destroy(void)
{
   al_destroy_bitmap(bitmap);
   al_destroy_bitmap(trans_bitmap);
   al_destroy_bitmap(trail_bitmap);
   al_destroy_bitmap(icon);
   al_destroy_bitmap(highscoreBitmap);
   bitmap = 0;
   trans_bitmap = 0;
   trail_bitmap = 0;
   icon = 0;
   highscoreBitmap = 0;
}

bool Player::load(void)
{
   ALLEGRO_STATE state;
   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP | ALLEGRO_STATE_BLENDER);

   bitmap = al_load_bitmap(getResource("gfx/ship.png"));
   if (!bitmap) {
      debug_message("Error loading %s\n", getResource("gfx/ship.png"));
      return false;
   }

   trans_bitmap = al_create_bitmap(al_get_bitmap_width(bitmap),
      al_get_bitmap_height(bitmap));
   if (!trans_bitmap) {
      debug_message("Error loading %s\n", getResource("gfx/ship_trans.png"));
      al_destroy_bitmap(bitmap);
      return false;
   }

   /* Make a translucent copy of the ship */
   al_set_target_bitmap(trans_bitmap);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_draw_tinted_bitmap(bitmap, al_map_rgba(255, 255, 255, 160),
      0, 0, 0);
   al_restore_state(&state);

   trail_bitmap = al_load_bitmap(getResource("gfx/trail.png"));
   if (!trail_bitmap) {
      debug_message("Error loading %s\n", getResource("gfx/trail.png"));
      al_destroy_bitmap(bitmap);
      al_destroy_bitmap(trans_bitmap);
      return false;
   }

   icon = al_load_bitmap(getResource("gfx/ship_icon.tga"));
   if (!icon) {
      debug_message("Error loading %s\n", getResource("gfx/ship_icon.tga"));
      al_destroy_bitmap(bitmap);
      al_destroy_bitmap(trans_bitmap);
      al_destroy_bitmap(trail_bitmap);
      return false;
   }

   highscoreBitmap = al_create_bitmap(300, 200);
   al_set_target_bitmap(highscoreBitmap);
   al_clear_to_color(al_map_rgba(0, 0, 0, 0));

   al_restore_state(&state);

   draw_radius = al_get_bitmap_width(bitmap)/2;
   radius = draw_radius / 2;

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
   angle = -ALLEGRO_PI/2;
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
      invincibleCount = 8000;
      ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
      al_set_target_bitmap(highscoreBitmap);
      int w = al_get_bitmap_width(highscoreBitmap);
      int h = al_get_bitmap_height(highscoreBitmap);
      ResourceManager& rm = ResourceManager::getInstance();
      ALLEGRO_FONT *large_font = (ALLEGRO_FONT *)rm.getData(RES_LARGEFONT);
      ALLEGRO_FONT *small_font = (ALLEGRO_FONT *)rm.getData(RES_SMALLFONT);
      al_draw_textf(large_font, al_map_rgb(255, 255, 255), w/2, h/2-16, ALLEGRO_ALIGN_CENTRE, "GAME OVER");
      al_draw_textf(small_font, al_map_rgb(255, 255, 255), w/2, h/2+16, ALLEGRO_ALIGN_CENTRE, "%d Points", score);
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

float Player::getAngle(void)
{
   return angle;
}

void Player::getSpeed(float *dx, float *dy)
{
   *dx = this->dx;
   *dy = this->dy;
}
