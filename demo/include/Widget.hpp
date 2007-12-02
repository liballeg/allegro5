#ifndef WIDGET_HPP
#define WIDGET_HPP

class Widget
{
public:
   // Returns false to destroy the gui
   virtual bool activate(void) = 0;
   virtual void left(void) = 0;
   virtual void right(void) = 0;

   virtual void render(bool selected) = 0;

   virtual ~Widget() {};
protected:
};

#endif // WIDGET_HPP

