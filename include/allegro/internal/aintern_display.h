#ifndef ALLEGRO_INTERNAL_DISPLAY_NEW_H
#define ALLEGRO_INTERNAL_DISPLAY_NEW_H

#include "allegro/display_new.h"
#include "allegro/bitmap_new.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_events.h"

typedef struct AL_DISPLAY_INTERFACE AL_DISPLAY_INTERFACE;

struct AL_DISPLAY_INTERFACE
{
   int id;
   AL_DISPLAY *(*create_display)(int w, int h, int flags);
   void (*make_display_current)(AL_DISPLAY *d);
   void (*clear)(AL_DISPLAY *d, AL_COLOR color);
   void (*line)(AL_DISPLAY *d, float fx, float fy, float tx, float ty,
      AL_COLOR color);
   void (*filled_rectangle)(AL_DISPLAY *d, float fx, float fy, float tx,
    float ty, AL_COLOR color);
   void (*flip)(AL_DISPLAY *d);
   void (*acknowledge_resize)(AL_DISPLAY *d);

   AL_BITMAP *(*create_bitmap)(AL_DISPLAY *d, int w, int h, int flags);
};

struct AL_DISPLAY
{
   /* Must be first, so the display can be used as event source. */
   struct AL_EVENT_SOURCE es; 
   AL_DISPLAY_INTERFACE *vt;
   int flags;
   int w, h;
};

AL_DISPLAY *_al_current_display;
//AL_DISPLAY_INTERFACE *_al_display_xdummy_driver(void);
AL_DISPLAY_INTERFACE *_al_display_d3ddummy_driver(void);

#if defined ALLEGRO_D3D
	#include "allegro/platform/ald3d.h"
#endif

#endif
