#include "a5teroids.hpp"

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
   /*
   Configuration& cfg = Configuration::getInstance();
   bool fullscreen = cfg.isFullscreen();
   */
   // added
   bool fullscreen = useFullScreenMode;
   int flags = fullscreen ? ALLEGRO_FULLSCREEN : ALLEGRO_WINDOWED;

   al_set_new_display_flags(flags);
   display = al_create_display(640, 480);
   events = al_create_event_queue();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display);
   return display != 0;
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

