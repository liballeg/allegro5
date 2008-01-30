#ifndef PLAYER_HPP
#define PLAYER_HPP

class Player : public Entity, public Resource {
public:
   static const float MAX_SPEED = 10.0f;
   static const float MIN_SPEED = -10.0f;
   static const float ACCEL = 0.006f;
   static const float DECCEL = 0.001f;

   bool logic(int step);
   void render(void);
   bool hit(int damage);

   void destroy(void);
   bool load(void);
   void* get(void);

   void newGame(void);
   void reset(void);
   void die(void);
   void givePowerUp(int type);

   void addScore(int points);
   int getScore(void);

   Player();
   ~Player();
private:
   float angle;
   bool draw_trail;
   int weapon;
   int lastShot;
   int lives;
   int invincibleCount;
   int score;

   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP *trans_bitmap;
   ALLEGRO_BITMAP *trail_bitmap;
   ALLEGRO_BITMAP *icon;
   A5FONT_FONT *small_font;
   ALLEGRO_BITMAP *highscoreBitmap;
};

#endif // PLAYER_HPP
