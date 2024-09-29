#ifndef DISPLAYRESOURCE_HPP
#define DISPLAYRESOURCE_HPP

#include "cosmic_protector.hpp"

class DisplayResource : public Resource {
public:
   void destroy(void);
   bool load(void);
   void* get(void);
   A5O_EVENT_QUEUE *getEventQueue(void);
   DisplayResource(void);
private:
   A5O_DISPLAY *display;
   A5O_EVENT_QUEUE *events;
};

extern bool useFullScreenMode;

#endif // DISPLAYRESOURCE_HPP

