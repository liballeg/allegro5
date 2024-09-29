#ifndef INPUT_HPP
#define INPUT_HPP

#include "cosmic_protector.hpp"

class Input : public Resource
{
public:
   Input();
   ~Input();

#ifdef A5O_IPHONE
   void draw(void);
   bool button_pressed(int x, int y, int w, int h, bool check_if_controls_at_top = true);
#endif

   void poll(void);
   float lr(void);
   float ud(void);
   bool esc(void);
   bool b1(void);
   bool cheat(void);

   void destroy(void);
   bool load(void);
   void* get(void);
private:
   A5O_KEYBOARD_STATE kbdstate;
   A5O_JOYSTICK_STATE joystate;
   A5O_JOYSTICK *joystick;

   A5O_EVENT_QUEUE *input_queue;

   bool joypad_u, joypad_d, joypad_l, joypad_r, joypad_b, joypad_esc;

#ifdef A5O_IPHONE
   struct Touch {
      int id;
      int x;
      int y;
   };
   std::vector<Touch> touches;
   bool controls_at_top;
   int size;
   float joyaxis0, joyaxis1, joyaxis2;
#endif
};

#endif // INPUT_HPP
