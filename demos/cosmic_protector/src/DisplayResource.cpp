#include "cosmic_protector.hpp"

bool useFullScreenMode = false;

#ifdef ALLEGRO_IPHONE
int BB_W;
int BB_H;
#else
int BB_W = 800;
int BB_H = 600        ;
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
#ifdef ALLEGRO_IPHONE
   int flags = ALLEGRO_FULLSCREEN_WINDOW;
   al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE, ALLEGRO_REQUIRE);
#else
   int flags = useFullScreenMode ? ALLEGRO_FULLSCREEN : ALLEGRO_WINDOWED;
#endif
   al_set_new_display_flags(flags);
   display = al_create_display(BB_W, BB_H);
   if (!display)
       return false;

#ifndef ALLEGRO_IPHONE
   ALLEGRO_BITMAP *bmp = al_load_bitmap(getResource("gfx/icon48.png"));
   al_set_display_icon(display, bmp);
   al_destroy_bitmap(bmp);
#endif

   BB_W = al_get_display_width(display);
   BB_H = al_get_display_height(display);

#ifdef ALLEGRO_IPHONE
   if (BB_W < 960) {
        BB_W *= 2;
        BB_H *= 2;
        ALLEGRO_TRANSFORM t;
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

ALLEGRO_EVENT_QUEUE *DisplayResource::getEventQueue(void)
{
   return events;
}

DisplayResource::DisplayResource(void) :
   display(0)
{
}

