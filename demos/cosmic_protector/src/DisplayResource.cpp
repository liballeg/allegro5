#include "cosmic_protector.hpp"

bool useFullScreenMode = false;

#ifdef A5O_IPHONE
int BB_W;
int BB_H;
#else
int BB_W = 800;
int BB_H = 600	;
#endif

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
#ifdef A5O_IPHONE
   int flags = A5O_FULLSCREEN_WINDOW;
   al_set_new_display_option(A5O_SUPPORTED_ORIENTATIONS, A5O_DISPLAY_ORIENTATION_LANDSCAPE, A5O_REQUIRE);
#else
   int flags = useFullScreenMode ? A5O_FULLSCREEN : A5O_WINDOWED;
#endif
   al_set_new_display_flags(flags);
   display = al_create_display(BB_W, BB_H);
   if (!display)
       return false;

#ifndef A5O_IPHONE
   A5O_BITMAP *bmp = al_load_bitmap(getResource("gfx/icon48.png"));
   al_set_display_icon(display, bmp);
   al_destroy_bitmap(bmp);
#endif

   BB_W = al_get_display_width(display);
   BB_H = al_get_display_height(display);
   
#ifdef A5O_IPHONE
   if (BB_W < 960) {
	BB_W *= 2;
	BB_H *= 2;
	A5O_TRANSFORM t;
	al_identity_transform(&t);
	al_scale_transform(&t, 0.5, 0.5);
	al_use_transform(&t);
   }
#endif
      
   events = al_create_event_queue();
   al_register_event_source(events, al_get_display_event_source(display));

   return true;
}

void* DisplayResource::get(void)
{
   return display;
}

A5O_EVENT_QUEUE *DisplayResource::getEventQueue(void)
{
   return events;
}

DisplayResource::DisplayResource(void) :
   display(0)
{
}

