/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to play MIDI files.
 */


#include "allegro.h"



int main(int argc, char *argv[])
{
   MIDI *the_music;

   allegro_init();

   if (argc != 2) {
      allegro_message("Usage: 'exmidi filename.mid'\n");
      return 1;
   }

   install_keyboard(); 
   install_timer();

   /* install a MIDI sound driver */
   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, argv[0]) != 0) {
      allegro_message("Error initialising sound system\n%s\n", allegro_error);
      return 1;
   }

   /* read in the MIDI file */
   the_music = load_midi(argv[1]);
   if (!the_music) {
      allegro_message("Error reading MIDI file '%s'\n", argv[1]);
      return 1;
   }

   set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);

   textprintf_centre(screen, font, SCREEN_W/2, SCREEN_H/3, 255, "Driver: %s", midi_driver->name);
   textprintf_centre(screen, font, SCREEN_W/2, SCREEN_H/2, 255, "Playing %s", argv[1]);

   /* start up the MIDI file */
   play_midi(the_music, TRUE);

   /* wait for a keypress */
   readkey();

   /* destroy the MIDI file */
   destroy_midi(the_music);

   return 0;
}

END_OF_MAIN();
