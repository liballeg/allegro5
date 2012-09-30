#ifndef PLAYER_HPP
#define PLAYER_HPP

class Player : public Entity, public Resource {
public:
   static const float MAX_SPEED;
   static const float MIN_SPEED;
   static const float ACCEL;
   static const float DECCEL;

   bool logic(int step);
   void render_extra(void);
   void render(int offx, int offy);
   void render(int offx, int offy, ALLEGRO_COLOR tint);
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
   float draw_radius;
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
   ALLEGRO_BITMAP *highscoreBitmap;
};

#endif // PLAYER_HPP
