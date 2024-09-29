#include "cosmic_protector.hpp"
#include "joypad_c.h"

#ifdef A5O_IPHONE
#include <allegro5/allegro_iphone.h>
#endif

#ifdef A5O_MSVC
/* "forcing value to bool 'true' or 'false' (performance warning)" */
#pragma warning( disable : 4800 )
#endif

Input::Input() :
   joystick(0)
{
   memset(&kbdstate, 0, sizeof(A5O_KEYBOARD_STATE));
   memset(&joystate, 0, sizeof(A5O_JOYSTICK_STATE));
#ifdef A5O_IPHONE
   joyaxis0 = 0.0f;
   joyaxis1 = 0.0f;
   joyaxis2 = 0.0f;
#endif
}

Input::~Input()
{
}

#ifdef A5O_IPHONE
void Input::draw(void)
{
   if (is_joypad_connected())
      return;

   int y = controls_at_top ? 0 : BB_H-size;
   int ymid = y + size/2;
   int ybot = y + size;
   int thick = 35;

   A5O_COLOR tri_color = al_map_rgba_f(0.4, 0.4, 0.4, 0.4);   
   A5O_COLOR fire_color = al_map_rgba_f(0.4, 0.1, 0.1, 0.4);
   
   al_draw_triangle(thick, ymid, size-thick, y+thick, size-thick, ybot-thick, tri_color, thick/4);
   al_draw_triangle(size*2-thick, ymid, size+thick, y+thick, size+thick, ybot-thick, tri_color, thick/4);

   al_draw_filled_circle(BB_W-size/2, ymid, (size-thick)/2, fire_color);
}

bool Input::button_pressed(int x, int y, int w, int h, bool check_if_controls_at_top)
{
   ResourceManager& rm = ResourceManager::getInstance();
   A5O_DISPLAY *display = (A5O_DISPLAY *)rm.getData(RES_DISPLAY);
   
   if (al_get_display_width(display) < 960) {
      x /= 2;
      y /= 2;
      w /= 2;
      h /= 2;
   }
   
   if (check_if_controls_at_top) {
      if (controls_at_top) {
         y -= (BB_H-size);
      }
   }
   
   #define COLL(xx, yy) (xx >= x && xx <= x+w && yy >= y && yy <= y+h)
   
   for (size_t i = 0; i < touches.size(); i++) {
      if (COLL(touches[i].x, touches[i].y)) {
         return true;
      }
   }
   
   return false;
}
#endif

void Input::poll(void)
{
   if (is_joypad_connected()) {
      get_joypad_state(&joypad_u, &joypad_d, &joypad_l, &joypad_r, &joypad_b, &joypad_esc);
      return;
   }

#ifdef A5O_IPHONE
   while (!al_event_queue_is_empty(input_queue)) {
      A5O_EVENT e;
      al_get_next_event(input_queue, &e);
      if (e.type == A5O_EVENT_TOUCH_BEGIN) {
         Touch t;
	 t.id = e.touch.id;
	 t.x = e.touch.x;
	 t.y = e.touch.y;
	 int xx = t.x;
	 int yy = t.y;
         ResourceManager& rm = ResourceManager::getInstance();
         A5O_DISPLAY *display = (A5O_DISPLAY *)rm.getData(RES_DISPLAY);
	 if (al_get_display_width(display) < 960) {
	    xx *= 2;
	    yy *= 2;
	 }
	 if (xx > (BB_W/5*2) && xx < (BB_W/5*3)) {
            if (yy < BB_H / 3) {
               controls_at_top = true;
            }
	    else if (yy > BB_H*2/3) {
	       controls_at_top = false;
	    }
	 }
	 touches.push_back(t);
      }
      else if (e.type == A5O_EVENT_TOUCH_END) {
         for (size_t i = 0; i < touches.size(); i++) {
	    if (touches[i].id == e.touch.id) {
	       touches.erase(touches.begin() + i);
	       break;
            }
	 }
      }
      else if (e.type == A5O_EVENT_TOUCH_MOVE) {
         for (size_t i = 0; i < touches.size(); i++) {
	    if (touches[i].id == e.touch.id) {
	       touches[i].x = e.touch.x;
	       touches[i].y = e.touch.y;
	       break;
            }
	 }
      }
      else if (e.type == A5O_EVENT_JOYSTICK_AXIS) {
	if (e.joystick.axis == 0) {
	   joyaxis0 = e.joystick.pos;
	}
	else if (e.joystick.axis == 1) {
	   joyaxis1 = e.joystick.pos;
	}
	else {
	   joyaxis2 = e.joystick.pos;
	}
      }
   }
#else
   while (!al_event_queue_is_empty(input_queue)) {
      A5O_EVENT e;
      al_get_next_event(input_queue, &e);
      if (e.type == A5O_EVENT_JOYSTICK_CONFIGURATION) {
         al_reconfigure_joysticks();
	 if (al_get_num_joysticks() <= 0) {
	    joystick = NULL;
	 }
	 else {
	    joystick = al_get_joystick(0);
	 }
      }
   }
   if (kb_installed)
      al_get_keyboard_state(&kbdstate);
   if (joystick)
      al_get_joystick_state(joystick, &joystate);
#endif
}

float Input::lr(void)
{
   if (is_joypad_connected()) {
      if (joypad_l) return -1;
      if (joypad_r) return 1;
      return 0;
   }

#ifdef A5O_IPHONE
   if (button_pressed(0, BB_H-size, size, size))
      return -1;
   return button_pressed(size, BB_H-size, size, size) ? 1 : 0;
#else
   if (al_key_down(&kbdstate, A5O_KEY_LEFT))
      return -1.0f;
   else if (al_key_down(&kbdstate, A5O_KEY_RIGHT))
      return 1.0f;
   else if (joystick) {
      float pos = joystate.stick[0].axis[0];
      return fabs(pos) > 0.1 ? pos : 0;
   }
   else
      return 0;
#endif
}

float Input::ud(void)
{
   if (is_joypad_connected()) {
      if (joypad_u) return -1;
      if (joypad_d) return 1;
      return 0;
   }

#ifdef A5O_IPHONE
   float magnitude = fabs(joyaxis0) + fabs(joyaxis1);
   if (magnitude < 0.3) return 0;
   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);
   float player_a = player->getAngle();
   
   A5O_DISPLAY *display = (A5O_DISPLAY *)rm.getData(RES_DISPLAY);
   if (al_get_display_orientation(display) == A5O_DISPLAY_ORIENTATION_90_DEGREES)
	player_a = player_a - A5O_PI;
   
   while (player_a < 0) player_a += A5O_PI*2;
   while (player_a > A5O_PI*2) player_a -= A5O_PI*2;
   
   float device_a = atan2(-joyaxis0, -joyaxis1);
   if (device_a < 0) device_a += A5O_PI*2;
  
   float ab = fabs(player_a - device_a); 
   if (ab < A5O_PI/4)
      return -1;
  
   // brake against velocity vector
   float vel_a, dx, dy;
   player->getSpeed(&dx, &dy);
   vel_a = atan2(dy, dx);
   vel_a -= A5O_PI;
   if (al_get_display_orientation(display) == A5O_DISPLAY_ORIENTATION_90_DEGREES)
	vel_a = vel_a - A5O_PI;
   
   while (vel_a < 0) vel_a += A5O_PI*2;
   while (vel_a > A5O_PI*2) vel_a -= A5O_PI*2;
   
   ab = fabs(vel_a - device_a);
   if (ab < A5O_PI/4)
      return 1;

   return 0;

#else
   if (al_key_down(&kbdstate, A5O_KEY_UP))
      return -1.0f;
   else if (al_key_down(&kbdstate, A5O_KEY_DOWN))
      return 1.0f;
   else if (joystick) {
      float pos = joystate.stick[0].axis[1];
      return fabs(pos) > 0.1 ? pos : 0;
   }
   else
      return 0;
#endif
}

bool Input::esc(void)
{
   if (is_joypad_connected()) {
      return joypad_esc;
   }

   if (al_key_down(&kbdstate, A5O_KEY_ESCAPE))
      return true;
   else if (joystick)
      return joystate.button[1];
   else
      return 0;
}

bool Input::b1(void)
{
   if (is_joypad_connected()) {
      return joypad_b;
   }

#ifdef A5O_IPHONE
   return button_pressed(BB_W-size, BB_H-size, size, size) ? 1 : 0;
#else
   if (al_key_down(&kbdstate, A5O_KEY_Z) || al_key_down(&kbdstate, A5O_KEY_Y))
      return true;
   else if (joystick)
      return joystate.button[0];
   else
      return 0;
#endif
}

bool Input::cheat(void)
{
   if (al_key_down(&kbdstate, A5O_KEY_LSHIFT)
         && al_key_down(&kbdstate, A5O_KEY_EQUALS))
      return true;
   else
      return false;
}

void Input::destroy(void)
{
}

bool Input::load(void)
{
   input_queue = al_create_event_queue();
#ifdef A5O_IPHONE
   al_install_touch_input();
   al_register_event_source(input_queue, al_get_touch_input_event_source());
   controls_at_top = false;
   size = BB_W / 5;
#else
   if (!kb_installed)
      kb_installed = al_install_keyboard();
#endif
   if (!joy_installed)
      joy_installed = al_install_joystick();

   if (joy_installed && !joystick && al_get_num_joysticks()) {
      joystick = al_get_joystick(0);
   }
   if (kb_installed)
      debug_message("Keyboard driver installed.\n");
   if (joystick)
      debug_message("Joystick found.\n");

   if (joystick)
      al_register_event_source(input_queue, al_get_joystick_event_source());

#ifdef A5O_IPHONE
   return true;
#else
   return kb_installed || joystick;
#endif
}

void* Input::get(void)
{
   return this;
}
