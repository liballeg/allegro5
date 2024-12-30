#include "mouse.h"


/*
 * bit 0: key is down
 * bit 1: key was pressed
 * bit 2: key was released
 */
#define MOUSE_BUTTONS 10
static int mouse_array[MOUSE_BUTTONS];
static int mx, my;

bool mouse_button_down(int b)
{
   return mouse_array[b] & 1;
}

bool mouse_button_pressed(int b)
{
   return mouse_array[b] & 2;
}

int mouse_x(void)
{
   return mx;
}

int mouse_y(void)
{
   return my;
}

void mouse_handle_event(ALLEGRO_EVENT *event)
{
   switch (event->type) {
      case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
         mouse_array[event->mouse.button] |= (1 << 0);
         mouse_array[event->mouse.button] |= (1 << 1);
         break;

      case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
         mouse_array[event->mouse.button] &= ~(1 << 0);
         mouse_array[event->mouse.button] |= (1 << 2);
         break;

      case ALLEGRO_EVENT_MOUSE_AXES:
         mx = event->mouse.x;
         my = event->mouse.y;
         break;
   }
}

void mouse_tick(void)
{
   /* clear pressed/released bits */
   int i;
   for (i = 0; i < MOUSE_BUTTONS; i++) {
      mouse_array[i] &= ~(1 << 1);
      mouse_array[i] &= ~(1 << 2);
   }
}
