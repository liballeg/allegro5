#include "cosmic_protector.hpp"

std::list<Entity *> entities;
std::list<Entity *> new_entities;

long lastUFO = -1;
bool canUFO = true;
const int MIN_UFO_TIME = 10000;
const int MAX_UFO_TIME = 50000;

bool logic(int step)
{
   const long now = (long) (al_get_time() * 1000.0);

   if (lastUFO < 0)
      lastUFO = now;

   if (canUFO && (now > (lastUFO+MIN_UFO_TIME))) {
      int r = rand() % (MAX_UFO_TIME-MIN_UFO_TIME);
      if (r <= step || (now > (lastUFO+MAX_UFO_TIME))) {
         canUFO = false;
         UFO *ufo;
         float x, y, dx, dy;
         if (rand() % 2) {
            x = -32;
            y = randf(32.0f, 75.0f);
            dx = 0.1f;
            dy = 0.0f;
         }
         else {
            x = BB_W+32;
            y = randf(BB_H-75, BB_H-32);
            dx = -0.1f;
            dy = 0.0f;
         }
         ufo = new UFO(x, y, dx, dy);
         entities.push_back(ufo);
      }
   }

   ResourceManager& rm = ResourceManager::getInstance();

   Player *player = (Player *)rm.getData(RES_PLAYER);

   Input *input = (Input *)rm.getData(RES_INPUT);
   input->poll();

   if (input->esc())
      return false;
   /* Catch close button presses */
   ALLEGRO_EVENT_QUEUE *events = ((DisplayResource *)rm.getResource(RES_DISPLAY))->getEventQueue();
   while (!al_is_event_queue_empty(events)) {
      ALLEGRO_EVENT event;
      al_get_next_event(events, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         return false;
   }

   std::list<Entity *>::iterator it = entities.begin();
   while (it != entities.end()) {
      Entity *e = *it;
      if (!e->logic(step)) {
         if (e->isUFO()) {
            lastUFO = now;
            canUFO = true;
         }
         delete e;
         it = entities.erase(it);
      }
      else
         it++;
   }

   for (it = new_entities.begin(); it != new_entities.end(); it++) {
      entities.push_back(*it);
   }
   new_entities.clear();

   if (!player->logic(step))
      return false;

   return true;
}

