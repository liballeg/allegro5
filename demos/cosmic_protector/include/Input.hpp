#ifndef INPUT_HPP
#define INPUT_HPP

#include "cosmic_protector.hpp"

class Input : public Resource
{
public:
   Input();
   ~Input();

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
   ALLEGRO_KEYBOARD_STATE kbdstate;
   ALLEGRO_JOYSTICK_STATE joystate;
   ALLEGRO_JOYSTICK *joystick;
};

#endif // INPUT_HPP
