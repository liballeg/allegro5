#include "cosmic_protector.hpp"
#include "joypad_c.h"

A5O_VOICE *voice;
A5O_MIXER *mixer;

#ifdef A5O_IPHONE
#include "cosmic_protector_objc.h"
#endif

bool switched_out;

static int check_arg(int argc, char **argv, const char *arg)
{
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], arg))
         return true;
   }
   return false;
}

static void game_loop()
{
   lastUFO = -1;
   canUFO = true;

   Wave wave;

   int step = 0;
   long start = (long) (al_get_time() * 1000);

   for (;;) {
      if (entities.size() <= 0) {
         if (!wave.next()) {
            // Won.
            break;
         }
      }

      if (!logic(step))
         break;
      render(step);
      al_rest(1.0/60.0);
      long end = (long) (al_get_time() * 1000);
      step = end - start;
      start = end;
      
      if (step > 50) step = 50;
   }

   std::list<Entity *>::iterator it;
   for (it = entities.begin(); it != entities.end(); it++) {
      Entity *e = *it;
      delete e;
   }
   entities.clear();
}

int main(int argc, char **argv)
{
   if (check_arg(argc, argv, "-fullscreen"))
      useFullScreenMode = true;
   if (!init()) {
      debug_message("Error in initialization.\n");
      return 1;
   }

   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);

   A5O_AUDIO_STREAM *title_music = (A5O_AUDIO_STREAM *)rm.getData(RES_TITLE_MUSIC);
   A5O_AUDIO_STREAM *game_music = (A5O_AUDIO_STREAM *)rm.getData(RES_GAME_MUSIC);

   for (;;) {
      if (title_music) {
         al_set_audio_stream_playing(title_music, true);
      }

      joypad_find();

      while (true) {
         int result = do_menu();
         al_rest(0.250);
         if (result == 1) {
            do_highscores(INT_MIN);
	    continue;
         }
         else if (result == 2 || result == -1) {
            done();
	    return 0;
         }
         break;
      }

      if (title_music) {
         al_drain_audio_stream(title_music);
         al_rewind_audio_stream(title_music);
      }

      joypad_stop_finding();

      A5O_DISPLAY *display = (A5O_DISPLAY *)rm.getData(RES_DISPLAY);
      int o = al_get_display_orientation(display);
      al_set_display_option(display, A5O_SUPPORTED_ORIENTATIONS, o);

      if (game_music) {
         al_set_audio_stream_playing(game_music, true);
      }

      player->load();
      game_loop();
      do_highscores(player->getScore());
      player->destroy();

      if (game_music) {
         al_drain_audio_stream(game_music);
         al_rewind_audio_stream(game_music);
      }

      al_set_display_option(display, A5O_SUPPORTED_ORIENTATIONS, A5O_DISPLAY_ORIENTATION_LANDSCAPE);

   }

   return 0;
}

#ifdef A5O_IPHONE
void switch_game_out(bool halt)
{
   if (!isMultitaskingSupported()) {
      exit(0);
   }
   if (halt) {
      ResourceManager& rm = ResourceManager::getInstance();
      A5O_DISPLAY *display = (A5O_DISPLAY *)rm.getData(RES_DISPLAY);
      al_acknowledge_drawing_halt(display);
   }
   switched_out = true;
}

void switch_game_in(void)
{
   switched_out = false;
}
#endif

