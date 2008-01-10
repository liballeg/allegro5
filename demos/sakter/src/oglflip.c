#ifdef SKATER_USE_ALLEGRO_GL

#include <allegro.h>
#include <alleggl.h>
#include "defines.h"
#include "oglflip.h"

/*****************************************************************************
 * OpenGL screen update module                                               *
 *****************************************************************************/

static void destroy(void)
{
}


static int create(void)
{
   return DEMO_OK;
}


static void draw(void)
{
   allegro_gl_flip();
}


static BITMAP *get_canvas(void)
{
   return screen;
}


void select_ogl_flipping(DEMO_SCREEN_UPDATE_DRIVER * driver)
{
   driver->create = create;
   driver->destroy = destroy;
   driver->draw = draw;
   driver->get_canvas = get_canvas;
}

#endif //SKATER_USE_ALLEGRO_GL
