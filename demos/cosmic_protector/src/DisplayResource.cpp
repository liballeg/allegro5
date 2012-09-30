#include "cosmic_protector.hpp"

bool useFullScreenMode = false;

void DisplayResource::destroy(void)
{
   if (!display)
      return;
   al_destroy_event_queue(events);
   al_destroy_display(display);
   display = 0;
}

bool DisplayResource::load(void)
{
   int flags = useFullScreenMode ? ALLEGRO_FULLSCREEN : ALLEGRO_WINDOWED;

   al_set_new_display_flags(flags);
   display = al_create_display(BB_W, BB_H);
   if (!display)
       return false;
   events = al_create_event_queue();
   al_register_event_source(events, al_get_display_event_source(display));
   return true;
}

void* DisplayResource::get(void)
{
   return display;
}

ALLEGRO_EVENT_QUEUE *DisplayResource::getEventQueue(void)
{
   return events;
}

DisplayResource::DisplayResource(void) :
   display(0)
{
}

