#include "cosmic_protector.hpp"

#ifdef ALLEGRO_MSVC
/* "forcing value to bool 'true' or 'false' (performance warning)" */
#pragma warning( disable : 4800 )
#endif

Input::Input() :
   joystick(0)
{
   memset(&kbdstate, 0, sizeof(ALLEGRO_KEYBOARD_STATE));
   memset(&joystate, 0, sizeof(ALLEGRO_JOYSTICK_STATE));
}

Input::~Input()
{
}

void Input::poll(void)
{
   if (kb_installed)
      al_get_keyboard_state(&kbdstate);

   if (joystick)
      al_get_joystick_state(joystick, &joystate);
}

float Input::lr(void)
{
   if (al_key_down(&kbdstate, ALLEGRO_KEY_LEFT))
      return -1.0f;
   else if (al_key_down(&kbdstate, ALLEGRO_KEY_RIGHT))
      return 1.0f;
   else {
      float pos = joystate.stick[0].axis[0];
      return fabs(pos) > 0.1 ? pos : 0;
   }
}

float Input::ud(void)
{
   if (al_key_down(&kbdstate, ALLEGRO_KEY_UP))
      return -1.0f;
   else if (al_key_down(&kbdstate, ALLEGRO_KEY_DOWN))
      return 1.0f;
   else {
      float pos = joystate.stick[0].axis[1];
      return fabs(pos) > 0.1 ? pos : 0;
   }
}

bool Input::esc(void)
{
   if (al_key_down(&kbdstate, ALLEGRO_KEY_ESCAPE))
      return true;
   else
      return joystate.button[1];
}

bool Input::b1(void)
{
   if (al_key_down(&kbdstate, ALLEGRO_KEY_Z) || al_key_down(&kbdstate, ALLEGRO_KEY_Y))
      return true;
   else
      return joystate.button[0];
}

bool Input::cheat(void)
{
   if (al_key_down(&kbdstate, ALLEGRO_KEY_LSHIFT)
         && al_key_down(&kbdstate, ALLEGRO_KEY_EQUALS))
      return true;
   else
      return false;
}

void Input::destroy(void)
{
}

bool Input::load(void)
{
   if (!kb_installed)
      kb_installed = al_install_keyboard();
   if (!joy_installed)
      joy_installed = al_install_joystick();

   if (joy_installed && !joystick && al_get_num_joysticks()) {
      joystick = al_get_joystick(0);
   }
   if (kb_installed)
      debug_message("Keyboard driver installed.\n");
   if (joystick)
      debug_message("Joystick found.\n");
   return kb_installed || joystick;
}

void* Input::get(void)
{
   return this;
}

