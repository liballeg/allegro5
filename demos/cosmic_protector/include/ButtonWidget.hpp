#ifndef BUTTONWIDGET_HPP
#define BUTTONWIDGET_HPP

class ButtonWidget : public Widget
{
public:
   bool activate(void);
   void render(bool selected);

   ButtonWidget(int x, int y, bool center, const char *text);
   ~ButtonWidget() {};
protected:
   int x;
   int y;
   bool center;
   const char *text;
};

#endif // BUTTONWIDGET_HPP

