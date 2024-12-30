#include <allegro5/allegro.h>
#include <string.h>
#include <stdio.h>
#include "global.h"
#include "gamepad.h"

static int button_down = 0;
static float axis[3];

static void read_config(VCONTROLLER * this, const char *config_path)
{
   int i;
   char tmp[64];

   ALLEGRO_CONFIG *c = al_load_config_file(config_path);
   if (!c) c = al_create_config();

   for (i = 0; i < 3; i++) {
      snprintf(tmp, sizeof(tmp), "button%d", i);
      ((int *)(this->private_data))[i] = get_config_int(c, "GAMEPAD", tmp, 0);

      if (((((int *)(this->private_data))[i] >> 8) & 255) >= al_get_num_joysticks())
         ((int *)(this->private_data))[i] = 0;
   }

   al_destroy_config(c);
}


static void write_config(VCONTROLLER * this, const char *config_path)
{
   int i;
   char tmp[64];

   ALLEGRO_CONFIG *c = al_load_config_file(config_path);
   if (!c) c = al_create_config();

   for (i = 0; i < 3; i++) {
      snprintf(tmp, sizeof(tmp), "button%d", i);
      set_config_int(c, "GAMEPAD", tmp, ((int *)(this->private_data))[i]);
   }

   al_save_config_file(config_path, c);
   al_destroy_config(c);
}

static float threshold = 0.1;
static void poll(VCONTROLLER * this)
{
   //printf("%f %f %f\n", axis[0], axis[1], axis[2]);
   float a = axis[0];
   if (screen_orientation == ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES)
      a = axis[1];
   if (screen_orientation == ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES)
      a = -axis[0];
   if (screen_orientation == ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES)
      a = -axis[1];
   this->button[0] = a < -threshold;
   this->button[1] = a > threshold;
   this->button[2] = button_down;
}


static int calibrate_button(VCONTROLLER * this, int i)
{
   (void)this;
   (void)i;
// FIXME
#if 0
   int stickc, axisc, joyc, buttonc;

   poll_joystick();
   joyc = al_get_num_joysticks();
   while (joyc--) {
      /* check axes */
      stickc = joy[joyc].num_sticks;
      while (stickc--) {
         axisc = joy[joyc].stick[stickc].num_axis;
         while (axisc--) {
            if (joy[joyc].stick[stickc].axis[axisc].d1) {
               ((int *)(this->private_data))[i] =
                  4 | 2 | (joyc << 8) | (stickc << 16) | (axisc << 24);
               return 1;
            }

            if (joy[joyc].stick[stickc].axis[axisc].d2) {
               ((int *)(this->private_data))[i] =
                  4 | (joyc << 8) | (stickc << 16) | (axisc << 24);
               return 1;
            }
         }
      }

      /* check buttons */
      buttonc = joy[joyc].num_buttons;
      while (buttonc--) {
         if (joy[joyc].button[buttonc].b) {
            ((int *)(this->private_data))[i] =
               4 | 1 | (joyc << 8) | (buttonc << 16);
            return 1;
         }
      }
   }
#endif
   return 0;
}

static const char *get_button_description(VCONTROLLER * this, int i)
{
   int *private_data = (int *)(this->private_data);
   static char RetMessage[1024];

   if (!private_data[i])
      return "Unassigned";

   if (private_data[i] & 1) {
      snprintf(RetMessage, sizeof(RetMessage), "Pad %d Button %d",
                (private_data[i] >> 8) & 255, private_data[i] >> 16);
      return RetMessage;
   } else {
      snprintf(RetMessage, sizeof(RetMessage),
                "Pad %d Stick %d Axis %d (%s)",
                (private_data[i] >> 8) & 255,
                (private_data[i] >> 16) & 255, private_data[i] >> 24,
                (private_data[i] & 2) ? "-" : "+");
      return RetMessage;
   }
}


VCONTROLLER *create_gamepad_controller(const char *config_path)
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

void gamepad_event(ALLEGRO_EVENT *event)
{
    switch (event->type) {
        case ALLEGRO_EVENT_TOUCH_BEGIN:
            button_down = true;
            break;

        case ALLEGRO_EVENT_TOUCH_END:
            button_down = false;
            break;

        case ALLEGRO_EVENT_JOYSTICK_AXIS:
            if (event->joystick.axis < 3)
                axis[event->joystick.axis] = event->joystick.pos;
            break;
    }
}


bool gamepad_button(void)
{
    return button_down != 0;
}
