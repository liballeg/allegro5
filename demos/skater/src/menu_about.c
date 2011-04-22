#include <allegro5/allegro.h>
#include <stdio.h>
#include "defines.h"
#include "global.h"
#include "menu.h"
#include "menus.h"


static int _id = DEMO_STATE_ABOUT;

static int id(void)
{
   return _id;
}


static DEMO_MENU menu[] = {
   {demo_text_proc, "Looks like rain. And soon!", 0, 0, 0, 0},
   {demo_text_proc, " ", 0, 0, 0, 0},
   {demo_text_proc, "Help coastal outdoor confectioner Ted collect the cherries,", 0, 0, 0, 0},
   {demo_text_proc, "bananas, sliced oranges, sweets and ice creams he has on display", 0, 0, 0, 0},
   {demo_text_proc, "before the rain begins!", 0, 0, 0, 0}, 
   {demo_text_proc, "Luckily he has a skateboard so it should be a breeze!", 0, 0, 0, 0},
   {demo_text_proc, " ", 0, 0, 0, 0},
   {demo_text_proc, NULL, 0, 0, 0, 0},
   {demo_text_proc, "by Shawn Hargreaves and many others", 0, 0, 0, 0},
   {demo_text_proc, " ", 0, 0, 0, 0},
   {demo_text_proc, "Allegro Demo Game", 0, 0, 0, 0},
   {demo_text_proc, "By Miran Amon, Nick Davies, Elias Pschernig, Thomas Harte & Jakub Wasilewski", 0, 0, 0, 0},
   {demo_text_proc, " ", 0, 0, 0, 0},
   {demo_button_proc, "Back", DEMO_MENU_SELECTABLE, DEMO_STATE_MAIN_MENU, 0, 0},
   {NULL, NULL, 0, 0, 0, 0}
};


static void init(void)
{
   static char version[256];
   int v = al_get_allegro_version();
   menu[7].name = version;
   sprintf(menu[7].name, "Allegro %d.%d.%d", (v >> 24), (v >> 16) & 255, (v >> 8) & 255);
   init_demo_menu(menu, true);
}


static int update(void)
{
   int ret = update_demo_menu(menu);

   switch (ret) {
      case DEMO_MENU_CONTINUE:
         return id();

      case DEMO_MENU_BACK:
         return DEMO_STATE_MAIN_MENU;

      default:
         return ret;
   };
}


static void draw(void)
{
   draw_demo_menu(menu);
}


void create_about_menu(GAMESTATE *state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}
