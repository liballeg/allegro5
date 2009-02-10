/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *   Additional display settings (like multisample).
 *
 *   Original code from AllegroGL.
 *
 *   Heavily modified by Elias Pschernig.
 *
 *   See readme.txt for copyright information.
 */
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_display.h"



/* Function: al_set_display_option */
void al_set_display_option(int option, int value, int importance)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = _al_get_new_display_settings();
   if (!extras) {
      extras = _AL_MALLOC(sizeof *extras);
      memset(extras, 0, sizeof *extras);
      _al_set_new_display_settings(extras);
   }
   switch (importance) {
      case ALLEGRO_REQUIRE:
         extras->required |= 1 << option;
         extras->suggested &= ~(1 << option);
         break;
      case ALLEGRO_SUGGEST:
         extras->suggested |= 1 << option;
         extras->required &= ~(1 << option);
         break;
      case ALLEGRO_DONTCARE:
         extras->required &= ~(1 << option);
         extras->suggested &= ~(1 << option);
         break;
   }
   extras->settings[option] = value;
}



/* Function: al_get_display_option */
int al_get_display_option(int option, int *importance)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = _al_get_new_display_settings();
   if (!extras) {
      if (*importance) *importance = ALLEGRO_DONTCARE;
      return 0;
   }
   if (extras->required & (1 << option)) {
      if (*importance) *importance = ALLEGRO_REQUIRE;
      return extras->settings[option];
   }
   if (extras->suggested & (1 << option)) {
      if (*importance) *importance = ALLEGRO_SUGGEST;
      return extras->settings[option];
   }
   if (*importance) *importance = ALLEGRO_DONTCARE;
   return 0;
}



/* Function: al_clear_display_options */
void al_clear_display_options(void)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = _al_get_new_display_settings();
   if (extras) _AL_FREE(extras);
   _al_set_new_display_settings(NULL);
}
