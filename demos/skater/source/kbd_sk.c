#include <allegro.h>
#include "../include/keyboard.h"


static void read_config(VCONTROLLER * this, const char *config_path)
{
   int i;
   char tmp[64];
   int def[] = {
      KEY_LEFT << 8,
      KEY_RIGHT << 8,
      KEY_SPACE << 8
   };

   push_config_state();
   set_config_file(config_path);

   for (i = 0; i < 3; i++) {
      uszprintf(tmp, sizeof(tmp), "button%d", i);
      ((int *)(this->private_data))[i] =
         get_config_int("KEYBOARD", tmp, def[i]);
   }

   pop_config_state();
}



static void write_config(VCONTROLLER * this, const char *config_path)
{
   int i;
   char tmp[64];

   push_config_state();
   set_config_file(config_path);

   for (i = 0; i < 3; i++) {
      uszprintf(tmp, sizeof(tmp), "button%d", i);
      set_config_int("KEYBOARD", tmp, ((int *)(this->private_data))[i]);
   }

   pop_config_state();
}



static void poll(VCONTROLLER * this)
{
   int i;

   int *private_data = (int *)(this->private_data);

   for (i = 0; i < 3; i++) {
      if (key[private_data[i] >> 8]) {
         this->button[i] = 1;
      } else {
         this->button[i] = 0;
      }
   }
}



static int calibrate_button(VCONTROLLER * this, int i)
{
   int c = 0;
   static int special[] = {
      KEY_TILDE,
      KEY_LSHIFT,
      KEY_RSHIFT,
      KEY_LCONTROL,
      KEY_RCONTROL,
      KEY_ALT,
      KEY_ALTGR,
      KEY_LWIN,
      KEY_RWIN,
      KEY_MENU,
      KEY_SCRLOCK,
      KEY_NUMLOCK,
      KEY_CAPSLOCK,
      -1
   };

   if (keypressed()) {
      c = readkey();
      if (c >> 8 != KEY_ESC) {
         ((int *)(this->private_data))[i] = c;
         return 1;
      } else {
         return 0;
      }
   }

   c = 0;
   while (special[c] != -1) {
      if (key[special[c]]) {
         ((int *)(this->private_data))[i] = special[c] << 8;
         return 1;
      }
      c++;
   }

   return 0;
}



static const char *get_button_description(VCONTROLLER * this, int i)
{
   int *private_data = (int *)(this->private_data);

   return scancode_to_name(private_data[i] >> 8);
}



VCONTROLLER *create_keyboard_controller(const char *config_path)
{
   int i;
   VCONTROLLER *ret = malloc(sizeof(VCONTROLLER));

   ret->private_data = malloc(3 * sizeof(int));
   for (i = 0; i < 3; i++) {
      ret->button[i] = 0;
      ((int *)(ret->private_data))[i] = 0;
   }
   ret->poll = poll;
   ret->calibrate_button = calibrate_button;
   ret->get_button_description = get_button_description;
   ret->read_config = read_config;
   ret->write_config = write_config;

   read_config(ret, config_path);

   return ret;
}
