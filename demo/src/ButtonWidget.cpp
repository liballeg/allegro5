#include "a5teroids.hpp"

bool ButtonWidget::activate(void)
{
   return false;
}

void ButtonWidget::left(void)
{
}

void ButtonWidget::right(void)
{
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
      al_draw_textf(myfont, x, y, ALLEGRO_CENTER, "%s", text);
   }
}

ButtonWidget::ButtonWidget(int x, int y, bool center, const char *text) :
   x(x),
   y(y),
   center(center),
   text(text)
{
}

