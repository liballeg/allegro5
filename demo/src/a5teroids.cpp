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

   SAMPLE *game_music = load_sample(getResource("sfx/game_music.wav"));
   SAMPLE *title_music = load_sample(getResource("sfx/title_music.wav"));

   if (!game_music || !title_music) {
   	printf("Error loading music.\n");
	return 1;
   }

   Wave& w = Wave::getInstance();

   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);

   for (;;) {
      player->load();

      al_rest(500);

      play_sample(title_music, 255, 128, 1000, 1);
      int choice = do_menu();
      if (choice != 0) {
         if (joystick)
            al_release_joystick(joystick);
         break;
      }
      stop_sample(title_music);

      al_rest(250);
      lastUFO = -1;
      canUFO = true;

      w.init();
      play_sample(game_music, 255, 128, 1000, 1);

      int step = 0;
      long start = al_current_time();

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
         al_rest(10);
         long end = al_current_time();
         step = end - start;
         start = end;
      }

      std::list<Entity *>::iterator it;
      for (it = entities.begin(); it != entities.end(); it++) {
         Entity *e = *it;
         delete e;
      }
      entities.clear();

      stop_sample(game_music);

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

