#ifndef ENTITY_HPP
#define ENTITY_HPP

const int ENTITY_LARGE_ASTEROID = 0;
const int ENTITY_SMALL_ASTEROID = 1;

class Entity {
public:
   virtual bool logic(int step);
   virtual void render(void) = 0;

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
