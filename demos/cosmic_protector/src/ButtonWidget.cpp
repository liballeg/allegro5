#include "cosmic_protector.hpp"

bool ButtonWidget::activate(void)
{
   return false;
}

void ButtonWidget::render(bool selected)
{
   ALLEGRO_FONT *myfont;
   ResourceManager& rm = ResourceManager::getInstance();

   if (center) {
      if (selected) {
         myfont = (ALLEGRO_FONT *)rm.getData(RES_LARGEFONT);
      }
      else {
         myfont = (ALLEGRO_FONT *)rm.getData(RES_SMALLFONT);
      }
      al_draw_textf(myfont, al_map_rgb(255, 255, 255), x, y, ALLEGRO_ALIGN_CENTRE, "%s", text);
   }
}

ButtonWidget::ButtonWidget(int x, int y, bool center, const char *text) :
   x(x),
   y(y),
   center(center),
   text(text)
{
}

