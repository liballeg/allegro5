/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    Plays MIDI files in the background of a system shell. A dodgy
 *    trick, and not really useful for anything, but I think it is
 *    pretty cool!
 */

#include <stdio.h>

#include "allegro.h"

#if (defined ALLEGRO_DJGPP) && (!defined SCAN_DEPEND)
   #include <dos.h>
   #include <sys/exceptn.h>
#endif



int main(int argc, char *argv[])
{
   MIDI *the_music;
   char msg[8192];

   if (allegro_init() != 0)
      return 1;

   if (argc != 2) {
      allegro_message("Usage: 'exdodgy filename.mid'\n");
      return 1;
   }

   #ifdef ALLEGRO_DJGPP
      __djgpp_set_ctrl_c(0);
      setcbrk(0);
   #endif

   install_timer();

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, argv[0]) != 0) {
      allegro_message("Error initialising sound system\n%s\n", allegro_error);
      return 1;
   }

   the_music = load_midi(argv[1]);
   if (!the_music) {
      allegro_message("Error reading MIDI file '%s'\n", argv[1]);
      return 1;
   }

   ustrcpy(msg, "\nMusic driver: ");
   ustrcat(msg, midi_driver->name);
   ustrcat(msg, "\nPlaying ");
   ustrcat(msg, argv[1]);
   ustrcat(msg, "\n\n");

   #ifdef ALLEGRO_DOS

      /* messages for DOS users */
      if (os_multitasking) {
	 ustrcat(msg, "I seem to be running under a multitasking environment. This means that I\n");
	 ustrcat(msg, "may not work properly, and even if I do, the sound will probably cut out\n");
	 ustrcat(msg, "whenever you exit from a child program. Please run me under DOS instead!\n\n");
      }

      ustrcat(msg, "You must not run any programs that trap hardware interrupts\n");
      ustrcat(msg, "or access the soundcard from this command prompt!\n\n");

   #else

      /* messages for other platforms */
      ustrcat(msg, "This program is _really_ nasty. It works under DOS, but you aren't using DOS\n");
      ustrcat(msg, "so that won't help you very much :-) Don't blame me if it locks up or does\n");
      ustrcat(msg, "other bad things, because I'm telling you now, that is likely to happen...\n\n");

   #endif

   ustrcat(msg, "Type \"exit\" to quit from the subshell.\n");

   #ifdef ALLEGRO_CONSOLE_OK
      ustrcat(msg, "\n----------------------------------------------------------------\n\n");
   #endif

   allegro_message(msg);

   play_midi(the_music, TRUE);

   if (getenv("SHELL"))
      system(getenv("SHELL"));
   else if (getenv("COMSPEC"))
      system(getenv("COMSPEC"));
   else
      system(NULL);

   remove_sound();
   destroy_midi(the_music);

   #ifdef ALLEGRO_CONSOLE_OK
      ustrcpy(msg, "\n----------------------------------------------------------------\n\n");
   #else
      msg[0] = 0;
   #endif

   ustrcat(msg, "Shutting down the music player...\n");
   ustrcat(msg, "Goodbye!\n");

   allegro_message(msg);

   return 0;
}

END_OF_MAIN()
