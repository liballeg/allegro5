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


   SampleResource *game_res = new SampleResource(getResource("sfx/game_music.ogg"));
   SampleResource *title_res = new SampleResource(getResource("sfx/title_music.ogg"));
   game_res->load();
   title_res->load();


   Wave& w = Wave::getInstance();

   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);

   ALLEGRO_SAMPLE *title_music = (ALLEGRO_SAMPLE *)title_res->get();
   ALLEGRO_SAMPLE *game_music = (ALLEGRO_SAMPLE *)game_res->get();

   al_sample_set_enum(title_music, ALLEGRO_AUDIOPROP_LOOPMODE, ALLEGRO_PLAYMODE_ONEDIR);
   al_sample_set_enum(game_music, ALLEGRO_AUDIOPROP_LOOPMODE, ALLEGRO_PLAYMODE_ONEDIR);

   for (;;) {
      player->load();

      al_rest(0.500);
   
      al_sample_play(title_music);

      int choice = do_menu();
      if (choice != 0) {
         if (joystick)
            al_release_joystick(joystick);
         break;
      }
      al_sample_stop(title_music);

      al_rest(0.250);
      lastUFO = -1;
      canUFO = true;

      w.init();
      al_sample_play(game_music);

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

      al_sample_stop(game_music);

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

