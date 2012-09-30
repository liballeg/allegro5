#ifndef DISPLAYRESOURCE_HPP
#define DISPLAYRESOURCE_HPP

#include "cosmic_protector.hpp"

class DisplayResource : public Resource {
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   ALLEGRO_EVENT_QUEUE *getEventQueue(void);
   DisplayResource(void);
private:
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *events;
};

extern bool useFullScreenMode;

#endif // DISPLAYRESOURCE_HPP

