/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Text message displaying functions.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

#include "speed.h"



typedef struct MESSAGE
{
   char text[80];
   int time;
   float x;
   float y;
   struct MESSAGE *next;
} MESSAGE;


static MESSAGE *msg;



/* initialises the message functions */
void init_message()
{
   msg = NULL;
}



/* closes down the message module */
void shutdown_message()
{
   MESSAGE *m;

   while (msg) {
      m = msg;
      msg = msg->next;
      free(m);
   }
}



/* adds a new message to the display */
void message(char *text)
{
   MESSAGE *m = malloc(sizeof(MESSAGE));

   strcpy(m->text, text);

   m->time = 0;

   m->x = 0;
   m->y = 0;

   m->next = msg;
   msg = m;
}



/* updates the message position */
void update_message()
{
   int SCREEN_W = al_get_display_width(screen);
   int SCREEN_H = al_get_display_height(screen);
   MESSAGE **p = &msg;
   MESSAGE *m = msg;
   MESSAGE *tmp;
   int y = SCREEN_H/2;

   while (m) {
      if (m->time < 100) {
         m->x *= 0.9;
         m->x += (float)SCREEN_W * 0.05;
      }
      else {
         m->x += (m->time - 100);
      }

      m->y *= 0.9;
      m->y += y * 0.1;

      m->time++;

      if (m->x > SCREEN_W + strlen(m->text)/4) {
         *p = m->next;
         tmp = m;
         m = m->next;
         free(tmp);
      }
      else {
         p = &m->next;
         m = m->next;
      }

      y += 16;
   }
}



/* draws messages */
void draw_message()
{
   MESSAGE *m = msg;

   while (m) {
      textout_centre(font_video, m->text, m->x, m->y, makecol(255, 255, 255));
      m = m->next;
   }
}


