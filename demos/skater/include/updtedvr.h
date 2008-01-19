#ifndef			__DEMO_UPDATE_DRIVER_H__
#define			__DEMO_UPDATE_DRIVER_H__

#include <allegro.h>

/*
   Structure that defines a screen update driver. Each actual driver
   consists of a number of functions that expose it to the user programmer
   and any number of internal static variables.
*/
typedef struct DEMO_SCREEN_UPDATE_DRIVER {
   /* Creates all the necessary internal variables such as video
      pages, memory buffers, etc. Must return an error code as
      defined in defines.h */
   int (*create) (void);

   /* Destroys all internal variables. */
   void (*destroy) (void);

   /* Flips the page, blits the backbuffer to the screen, etc. Whatever
      it takes to put the backbuffer to the screen. */
   void (*draw) (void);

   /* Returns a pointer to the backbuffer the user programmer can
      safely draw to. */
   BITMAP *(*get_canvas) (void);
} DEMO_SCREEN_UPDATE_DRIVER;


#endif				/* __DEMO_UPDATE_DRIVER_H__ */
