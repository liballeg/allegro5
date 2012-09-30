#ifndef WIDGET_HPP
#define WIDGET_HPP

class Widget
{
public:
   // Returns false to destroy the gui
   virtual bool activate(void) = 0;
   virtual void render(bool selected) = 0;
   virtual ~Widget() {};
};

#endif // WIDGET_HPP

