#include <allegro5/allegro.h>
#include <stdio.h>
#include "global.h"
#include "keyboard.h"

#define KEYBUF_SIZE 16

/* 
 * bit 0: key is down
 * bit 1: key was pressed
 * bit 2: key was released
 */
static int key_array[A5O_KEY_MAX];
static int unicode_array[KEYBUF_SIZE];
static int unicode_count;

bool key_down(int k)
{
   return key_array[k] & 1;
}

bool key_pressed(int k)
{
   return key_array[k] & 2;
}

int unicode_char(bool remove)
{
   int u;
   if (unicode_count == 0) return 0;
   u = unicode_array[0];
   if (remove) {
      memmove(unicode_array, unicode_array + 1,
         (KEYBUF_SIZE - 1) * sizeof(int));
   }
   return u;
}

void keyboard_event(A5O_EVENT *event)
{
   switch (event->type) {
      case A5O_EVENT_KEY_DOWN:
         key_array[event->keyboard.keycode] |= (1 << 0);
         key_array[event->keyboard.keycode] |= (1 << 1);
         break;

      case A5O_EVENT_KEY_CHAR:
         if (event->keyboard.unichar && unicode_count < KEYBUF_SIZE) {
            unicode_array[unicode_count++] = event->keyboard.unichar;
         }
         break;

      case A5O_EVENT_KEY_UP:
         key_array[event->keyboard.keycode] &= ~(1 << 0);
         key_array[event->keyboard.keycode] |= (1 << 2);
         break;
   }
}

void keyboard_tick(void)
{
   /* clear pressed/released bits */
   int i;
   for (i = 0; i < A5O_KEY_MAX; i++) {
      key_array[i] &= ~(1 << 1);
      key_array[i] &= ~(1 << 2);
   }
   
   unicode_count = 0;
}

static void read_config(VCONTROLLER * this, const char *config_path)
{
   int i;
   char tmp[64];
   int def[] = {
      A5O_KEY_LEFT,
      A5O_KEY_RIGHT,
      A5O_KEY_SPACE
   };

   A5O_CONFIG *c = al_load_config_file(config_path);
   if (!c) c = al_create_config();

   for (i = 0; i < 3; i++) {
      snprintf(tmp, sizeof(tmp), "button%d", i);
      ((int *)(this->private_data))[i] =
         get_config_int(c, "KEYBOARD", tmp, def[i]);
   }

   al_destroy_config(c);
}



static void write_config(VCONTROLLER * this, const char *config_path)
{
   int i;
   char tmp[64];

   A5O_CONFIG *c = al_load_config_file(config_path);
   if (!c) c = al_create_config();

   for (i = 0; i < 3; i++) {
      snprintf(tmp, sizeof(tmp), "button%d", i);
      set_config_int(c, "KEYBOARD", tmp, ((int *)(this->private_data))[i]);
   }

   al_save_config_file(config_path, c);
   al_destroy_config(c);
}



static void poll(VCONTROLLER * this)
{
   int i;

   int *private_data = (int *)(this->private_data);

   for (i = 0; i < 3; i++) {
      if (key_down(private_data[i])) {
         this->button[i] = 1;
      } else {
         this->button[i] = 0;
      }
   }
}



static int calibrate_button(VCONTROLLER * this, int i)
{
   int c;

   if (key_down(A5O_KEY_ESCAPE)) {
      return 0;
   }

   for (c = 1; c < A5O_KEY_MAX; c++) {
      if (key_pressed(c)) {
         ((int *)(this->private_data))[i] = c;
         return 1;
      }
   }

   return 0;
}



static const char *get_button_description(VCONTROLLER * this, int i)
{
   int *private_data = (int *)(this->private_data);

   return al_keycode_to_name(private_data[i]);
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
