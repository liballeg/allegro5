#include "a5teroids.hpp"

ALLEGRO_VOICE *voice;
ALLEGRO_MIXER *mixer;

int check_arg(int argc, char **argv, const std::string& arg)
{
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], arg.c_str()))
         return i;
   }
   return -1;
}

int main(int argc, char **argv)
{
   try {

   if (check_arg(argc, argv, "-fullscreen") > 0)
      useFullScreenMode = true;

   bool won = false;

   if (!init()) {
      debug_message("Error in initialization.\n");
      return 1;
   }


   Wave& w = Wave::getInstance();

   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);

   ALLEGRO_AUDIO_STREAM *title_music = (ALLEGRO_AUDIO_STREAM *)rm.getData(RES_TITLE_MUSIC);
   ALLEGRO_AUDIO_STREAM *game_music = (ALLEGRO_AUDIO_STREAM *)rm.getData(RES_GAME_MUSIC);

   if (game_music) {
      al_set_audio_stream_playmode(title_music, ALLEGRO_PLAYMODE_LOOP);
      al_set_audio_stream_playmode(game_music, ALLEGRO_PLAYMODE_LOOP);
   }

   for (;;) {
      player->load();

      if (title_music) {
         al_set_audio_stream_playing(title_music, true);
      }

      int choice = do_menu();
      if (choice != 0) {
         if (joystick)
            al_release_joystick(joystick);
         break;
      }

      if (title_music) {
         al_drain_audio_stream(title_music);
         al_rewind_audio_stream(title_music);
      }

      lastUFO = -1;
      canUFO = true;

      w.init();
      if (game_music) {
         al_set_audio_stream_playing(game_music, true);
      }
      al_rest(0.200);

      int step = 0;
      long start = (long) (al_get_time() * 1000);

      for (;;) {
         if (entities.size() <= 0) {
            if (!w.next()) {
               won = true;
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

      if (game_music) {
         al_drain_audio_stream(game_music);
         al_rewind_audio_stream(game_music);
      }

      std::list<Entity *>::iterator it;
      for (it = entities.begin(); it != entities.end(); it++) {
         Entity *e = *it;
         delete e;
      }
      entities.clear();

      if (won) {
         // FIXME: show end screen
      }

      player->destroy();
   }

   done();

   }
   catch (Error e) {
      debug_message("An error occurred: %s\n", e.getMessage().c_str());
   }

   return 0;
}

