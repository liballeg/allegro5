/* This example lists the available audio devices.
 */
#define A5O_UNSTABLE

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"

#include "common.c"

int main(int argc, char **argv)
{
   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }

   open_log();

   int count = al_get_num_audio_output_devices();
   if (count < 0) {
      log_printf("Platform not supported.\n");
      goto done;
   }

   for (int i = 0; i < count; i++) {
      const A5O_AUDIO_DEVICE* device = al_get_audio_output_device(i);
      log_printf("%s\n", al_get_audio_device_name(device));
   }

done:
   close_log(true);
}
