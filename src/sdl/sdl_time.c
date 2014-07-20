#include "SDL.h"

#include "allegro5/altime.h"
#include "allegro5/platform/allegro_sdl_thread.h"
#include "allegro5/debug.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

/* Function: al_get_time
 */
double al_get_time(void)
{
   return 1.0 * SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
}



/* Function: al_rest
 */
void al_rest(double seconds)
{
   SDL_Delay(seconds * 1000);
}



/* Function: al_init_timeout
 */
void al_init_timeout(ALLEGRO_TIMEOUT *timeout, double seconds)
{
   ALLEGRO_TIMEOUT_SDL *timeout_sdl = (void *)timeout;
   timeout_sdl->ms = seconds * 1000;
}

/* vim: set sts=3 sw=3 et */
