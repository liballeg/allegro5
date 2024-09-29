/*
 *    Example program for the Allegro library, by Beoran.
 *
 *    This program tests haptic effects.
 */

#define A5O_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"


static void test_haptic_joystick(A5O_JOYSTICK *joy)
{
   A5O_HAPTIC_EFFECT_ID id;
   A5O_HAPTIC *haptic;
   A5O_HAPTIC_EFFECT effect;
   const double intensity = 1.0;
   const double duration = 1.0;

   haptic = al_get_haptic_from_joystick(joy);

   if (!haptic) {
      log_printf("Could not get haptic device from joystick!");
      return;
   }

   log_printf("Can play back %d haptic effects.\n",
      al_get_max_haptic_effects(haptic));

   log_printf("Set gain to 0.8: %d.\n",
      al_set_haptic_gain(haptic, 0.8));

   log_printf("Get gain: %lf.\n",
      al_get_haptic_gain(haptic));

   log_printf("Capabilities: %d.\n",
      al_get_haptic_capabilities(haptic));

   memset(&effect, 0, sizeof(effect));
   effect.type = A5O_HAPTIC_RUMBLE;
   effect.data.rumble.strong_magnitude = intensity;
   effect.data.rumble.weak_magnitude = intensity;
   effect.replay.delay = 0.1;
   effect.replay.length = duration;

   log_printf("Upload effect: %d.\n",
      al_upload_haptic_effect(haptic, &effect, &id));

   log_printf("Playing effect: %d.\n",
      al_play_haptic_effect(&id, 3));

   do {
      al_rest(0.1);
   } while (al_is_haptic_effect_playing(&id));

   log_printf("Set gain to 0.4: %d.\n",
      al_set_haptic_gain(haptic, 0.4));

   log_printf("Get gain: %lf.\n",
      al_get_haptic_gain(haptic));

   log_printf("Playing effect again: %d.\n",
      al_play_haptic_effect(&id, 5));

   al_rest(2.0);

   log_printf("Stopping effect: %d.\n",
      al_stop_haptic_effect(&id));

   do {
      al_rest(0.1);
   } while (al_is_haptic_effect_playing(&id));

   log_printf("Release effect: %d.\n",
      al_release_haptic_effect(&id));

   log_printf("Release haptic: %d.\n",
      al_release_haptic(haptic));
}


int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   int num_joysticks;
   int i;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("al_create_display failed\n");
   }
   if (!al_install_joystick()) {
      abort_example("al_install_joystick failed\n");
   }
   if (!al_install_haptic()) {
      abort_example("al_install_haptic failed\n");
   }

   open_log();

   num_joysticks = al_get_num_joysticks();
   log_printf("Found %d joysticks.\n", num_joysticks);
   
   for (i = 0; i < num_joysticks; i++) {
      A5O_JOYSTICK *joy = al_get_joystick(i);
      if (!joy) {
         continue;
      }

      if (al_is_joystick_haptic(joy)) {
         log_printf("Joystick %s supports force feedback.\n",
            al_get_joystick_name(joy));
         test_haptic_joystick(joy);
      }
      else {
         log_printf("Joystick %s does not support force feedback.\n",
            al_get_joystick_name(joy));
      }

      al_release_joystick(joy);
   }

   log_printf("\nAll done!\n");
   close_log(true);

   return 0;
}

/* vim: set ts=8 sts=3 sw=3 et: */
