#include "a5teroids.hpp"

int do_gui(std::vector<Widget *>& widgets, unsigned int selected)
{
   ResourceManager& rm = ResourceManager::getInstance();
   ALLEGRO_BITMAP *bg = (ALLEGRO_BITMAP *)rm.getData(RES_BACKGROUND);
   Input *input = (Input *)rm.getData(RES_INPUT);
   ALLEGRO_BITMAP *logo = (ALLEGRO_BITMAP *)rm.getData(RES_LOGO);
   int lw = al_get_bitmap_width(logo);
   int lh = al_get_bitmap_height(logo);
   ALLEGRO_FONT *myfont = (ALLEGRO_FONT *)rm.getData(RES_SMALLFONT);

   for (;;) {
      input->poll();
      float ud = input->ud();
      if (ud < 0 && selected) {
         selected--;
         my_play_sample(RES_FIRELARGE);
         al_rest(0.200);
      }
      else if (ud > 0 && selected < (widgets.size()-1)) {
         selected++;
         my_play_sample(RES_FIRELARGE);
         al_rest(0.200);
      }
      if (input->b1()) {
         if (!widgets[selected]->activate())
            return selected;
      }
      if (input->esc())
         return -1;
      al_draw_scaled_bitmap(bg, 0, 0, 
         al_get_bitmap_width(bg),
         al_get_bitmap_height(bg),
         0, 0, BB_W, BB_H,
         0);
      al_draw_rotated_bitmap(logo, lw/2, lh/2, BB_W/2, BB_H/4, 0.0f, 0);

      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
         al_map_rgb(255, 255, 0));
      al_draw_textf(myfont, BB_W/2, BB_H/2, ALLEGRO_ALIGN_CENTRE, "z/y to start");
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
         al_map_rgb(255, 255, 255));

      for (unsigned int i = 0; i < widgets.size(); i++) {
         widgets[i]->render(i == selected);
      }
      al_flip_display();
      al_rest(0.010);
   }
}

int do_menu(void)
{
   ButtonWidget *play = new ButtonWidget(BB_W/2, BB_H/4*3-16, true, "PLAY");
   ButtonWidget *end = new ButtonWidget(BB_W/2, BB_H/4*3+16, true, "EXIT");
   std::vector<Widget *> widgets;
   widgets.push_back(play);
   widgets.push_back(end);

   int ret = do_gui(widgets, 0);

   delete play;
   delete end;
   widgets.clear();

   return ret;
}

