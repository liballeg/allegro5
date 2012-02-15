#include "a5teroids.hpp"

ALLEGRO_VOICE *voice;
ALLEGRO_MIXER *mixer;

#ifdef USE_JOYPAD
#if defined ALLEGRO_IPHONE || defined ALLEGRO_MACOSX
#include "joypad_c.h"
#endif
#endif

#ifdef ALLEGRO_IPHONE
#include "a5teroids_objc.h"
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
      al_rest(0.010);
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

   ALLEGRO_AUDIO_STREAM *title_music = (ALLEGRO_AUDIO_STREAM *)rm.getData(RES_TITLE_MUSIC);
   ALLEGRO_AUDIO_STREAM *game_music = (ALLEGRO_AUDIO_STREAM *)rm.getData(RES_GAME_MUSIC);

   for (;;) {
      if (title_music) {
         al_set_audio_stream_playing(title_music, true);
      }

#ifdef USE_JOYPAD
#if defined ALLEGRO_IPHONE || defined ALLEGRO_MACOSX
      joypad_find();
#endif
#endif

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

#ifdef USE_JOYPAD
#if defined ALLEGRO_IPHONE || defined ALLEGRO_MACOSX
      joypad_stop_finding();
#endif
#endif
      
      ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)rm.getData(RES_DISPLAY);
      int o = al_get_display_orientation(display);
      al_change_display_option(display, ALLEGRO_SUPPORTED_ORIENTATIONS, o);

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

      al_change_display_option(display, ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE);

   }

   return 0;
}

#ifdef ALLEGRO_IPHONE
void switch_game_out(bool halt)
{
   if (!isMultitaskingSupported()) {
      exit(0);
   }
   if (!switched_out)
	sb_stop();
   if (halt) {
      ResourceManager& rm = ResourceManager::getInstance();
      ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)rm.getData(RES_DISPLAY);
      al_acknowledge_drawing_halt(display);
   }
   switched_out = true;
}

void switch_game_in(void)
{
   if (switched_out)
      sb_start();
   switched_out = false;
}
#endif

