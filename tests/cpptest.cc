/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      A simple C++ test.
 *
 *      By Henrik Stokseth.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <time.h>
#include "allegro.h"

#define RODENTS  4


/* We define dumb memory management operators in order
 * not to have to link against libstdc++ with GCC 3.x.
 * Don't do that in real-life C++ code unless you know
 * what you're doing.
 */
void *operator new(size_t size)
{
   return malloc(size);
}

void operator delete(void *ptr)
{
  if (ptr)
    free (ptr);
}


/* Our favorite pet. */
class rodent {
   public:
      rodent(BITMAP *bmp);
      void move();
      void draw(BITMAP *bmp);
   private:
      int x, y;
      int delta_x, delta_y;
      BITMAP *sprite;
};


rodent::rodent(BITMAP *bmp)
{
   x = rand() % (SCREEN_W-bmp->w);
   y = rand() % (SCREEN_H-bmp->h);

   do {
     delta_x = (rand() % 11) - 5;
   } while(!delta_x);

   do {
      delta_y = (rand() % 11) - 5;
   } while(!delta_y);

   sprite = bmp;
}


void rodent::move()
{
   if((x+sprite->w+delta_x >= SCREEN_W) || (x+delta_x < 0)) delta_x = -delta_x;
   if((y+sprite->h+delta_y >= SCREEN_H) || (y+delta_y < 0)) delta_y = -delta_y;

   x += delta_x;
   y += delta_y;
}


void rodent::draw(BITMAP *bmp)
{
   draw_sprite(bmp, sprite, x, y);
}


/* A little counter to waste your time. */
volatile int counter = 0;

void my_timer_handler()
{
   counter++;
}
END_OF_FUNCTION(my_timer_handler)


/* Yup, you read correctly, we're creating the World here. */
class world {
   public:
      world();   /* Genesis */
      ~world();  /* Apocalypse */
      void draw();
      void logic();
      void loop();
   private:
      BITMAP *dbuffer;
      BITMAP *mouse_sprite;
      int active;
      rodent *mouse[RODENTS];
};


world::world()
{
   PALETTE pal;
   active = TRUE;

   dbuffer = create_bitmap(SCREEN_W, SCREEN_H);
   mouse_sprite = load_bitmap("../examples/mysha.pcx", pal);

   if (!mouse_sprite) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading bitmap\n%s\n", allegro_error);
      exit(1);
   }

   set_palette(pal);

   for(int what_mouse=0; what_mouse < RODENTS; what_mouse++)
      mouse[what_mouse] = new rodent(mouse_sprite);
}


world::~world()
{
   destroy_bitmap(dbuffer);
   destroy_bitmap(mouse_sprite);

   for(int what_mouse=0; what_mouse < RODENTS; what_mouse++)
      delete mouse[what_mouse];
}


void world::draw()
{
   clear_bitmap(dbuffer);

   for(int what_mouse=0; what_mouse < RODENTS; what_mouse++)
      mouse[what_mouse]->draw(dbuffer);

   blit(dbuffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
}


void world::logic()
{
   if(key[KEY_ESC]) active = FALSE;

   for(int what_mouse=0; what_mouse < RODENTS; what_mouse++)
      mouse[what_mouse]->move();
}


void world::loop()
{
   install_int_ex(my_timer_handler, BPS_TO_TIMER(10));

   while(active) {
      while(counter > 0) {
         counter--;
         logic();
      }
      draw();
   }

   remove_int(my_timer_handler);
}


int main()
{
   allegro_init();

   install_keyboard();
   install_timer();

   srand(time(NULL));
   LOCK_VARIABLE(counter);
   LOCK_FUNCTION(my_timer_handler);

   if (set_gfx_mode(GFX_AUTODETECT, 1024, 768, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   world *game = new world();  /* America! America! */
   game->loop();
   delete game;

   return 0;
}
END_OF_MAIN()
