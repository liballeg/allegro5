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
   A5FONT_FONT *myfont;
   ResourceManager& rm = ResourceManager::getInstance();

   if (center) {
      if (selected) {
         myfont = (A5FONT_FONT *)rm.getData(RES_LARGEFONT);
      }
      else {
         myfont = (A5FONT_FONT *)rm.getData(RES_SMALLFONT);
      }
      a5font_textprintf_centre(myfont, x, y, text);
   }
}

ButtonWidget::ButtonWidget(int x, int y, bool center, const char *text) :
   x(x),
   y(y),
   center(center),
   text(text)
{
}

