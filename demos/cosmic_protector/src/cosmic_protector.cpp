#include "cosmic_protector.hpp"

ALLEGRO_VOICE *voice;
ALLEGRO_MIXER *mixer;

static int check_arg(int argc, char **argv, const char *arg)
{
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], arg))
         return true;
   }
   return false;
}

void game_loop()
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

      if (do_menu() != 0) {
         break;
      }

      if (title_music) {
         al_drain_audio_stream(title_music);
         al_rewind_audio_stream(title_music);
      }

      if (game_music) {
         al_set_audio_stream_playing(game_music, true);
      }

      player->load();
      game_loop();
      player->destroy();

      if (game_music) {
         al_drain_audio_stream(game_music);
         al_rewind_audio_stream(game_music);
      }
   }

   done();

   return 0;
}

