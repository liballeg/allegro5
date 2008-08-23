#include "a5teroids.hpp"

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

   ALLEGRO_SAMPLE *game_music = al_load_sample(getResource("sfx/game_music.wav"));
   if (!game_music) {
      printf("Failed to load %s\n", getResource("sfx/game_music.wav"));
      return 1;
   }
   ALLEGRO_SAMPLE *title_music = al_load_sample(getResource("sfx/title_music.wav"));
   if (!title_music) {
      printf("Failed to load %s\n", getResource("sfx/title_music.wav"));
      return 1;
   }

   Wave& w = Wave::getInstance();

   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);

   ALLEGRO_VOICE *title_voice = al_voice_create(title_music);
   ALLEGRO_VOICE *game_voice = al_voice_create(game_music);
   // al_voice_set_loop_mode(title_voice, ALLEGRO_AUDIO_ONE_DIR);
   // al_voice_set_loop_mode(game_voice, ALLEGRO_AUDIO_ONE_DIR);
   for (;;) {
      player->load();

      al_rest(0.500);

      al_voice_start(title_voice);
      
      int choice = do_menu();
      if (choice != 0) {
         if (joystick)
            al_release_joystick(joystick);
         break;
      }
      al_voice_stop(title_voice);

      al_rest(0.250);
      lastUFO = -1;
      canUFO = true;

      w.init();
      al_voice_start(game_voice);

      int step = 0;
      long start = (long) (al_current_time() * 1000);

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
         long end = (long) (al_current_time() * 1000);
         step = end - start;
         start = end;
      }

      std::list<Entity *>::iterator it;
      for (it = entities.begin(); it != entities.end(); it++) {
         Entity *e = *it;
         delete e;
      }
      entities.clear();

      al_voice_stop(game_voice);

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
END_OF_MAIN()

