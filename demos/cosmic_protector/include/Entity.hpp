#ifndef ENTITY_HPP
#define ENTITY_HPP

const int ENTITY_LARGE_ASTEROID = 0;
const int ENTITY_SMALL_ASTEROID = 1;

class Entity {
public:
   virtual bool logic(int step);
   virtual void render(int offx = 0, int offy = 0) = 0;
   virtual void render(int x, int y, A5O_COLOR c);

   virtual void spawn(void);
   virtual bool hit(int damage);

   float getX(void);
   float getY(void);
   float getRadius(void);
   bool getDestructable(void);
   int getDamage(void);
   bool isHighlighted(void);
   bool isUFO(void);

   void setPowerUp(int type);

   void wrap(void);
   Entity *getPlayerCollision(void);
   Entity *getEntityCollision(void);
   Entity *getAllCollision(void);
   void explode(void);
   
   void render_four(A5O_COLOR tint);

   Entity(void);
   virtual ~Entity(void) {};
protected:
   Entity *checkCollisions(std::list<Entity *>& e);

   float x;
   float y;
   float radius;
   float dx;
   float dy;
   bool isDestructable;
   int hp;
   int powerup;
   int hilightCount;
   int points;
   bool ufo;
};

#endif // ENTITY_HPP
