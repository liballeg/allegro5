#ifndef ALLEGRO_INTERNAL_DISPLAY_NEW_H
#define ALLEGRO_INTERNAL_DISPLAY_NEW_H

typedef struct AL_DISPLAY_INTERFACE AL_DISPLAY_INTERFACE;

#include "../display_new.h"

struct AL_DISPLAY_INTERFACE
{
   AL_DISPLAY *(*create_display)(int w, int h, int flags);
   void (*make_display_current)(AL_DISPLAY *d);
   void (*clear)(AL_DISPLAY *d, AL_COLOR color);
   void (*line)(AL_DISPLAY *d, float fx, float fy, float tx, float ty,
      AL_COLOR color);
   void (*filled_rectangle)(AL_DISPLAY *d, float fx, float fy, float tx,
    float ty, AL_COLOR color);
   void (*flip)(AL_DISPLAY *d);
   void (*acknowledge_resize)(AL_DISPLAY *d);

};

struct AL_DISPLAY
{
   /* Must be first, so the display can be used as event source. */
   AL_EVENT_SOURCE es; 
   AL_DISPLAY_INTERFACE *vt;
   int w, h;
};

AL_DISPLAY_INTERFACE *_al_display_xdummy_driver(void);

#endif
