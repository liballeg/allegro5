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
   DEMO_MENU_ITEM2(demo_text_proc, "Looks like rain. And soon!"),
   DEMO_MENU_ITEM2(demo_text_proc, " "),
   DEMO_MENU_ITEM2(demo_text_proc, "Help coastal outdoor confectioner Ted collect the cherries,"),
   DEMO_MENU_ITEM2(demo_text_proc, "bananas, sliced oranges, sweets and ice creams he has on display"),
   DEMO_MENU_ITEM2(demo_text_proc, "before the rain begins!"),
   DEMO_MENU_ITEM2(demo_text_proc, "Luckily he has a skateboard so it should be a breeze!"),
   DEMO_MENU_ITEM2(demo_text_proc, " "),
   DEMO_MENU_ITEM2(demo_text_proc, NULL),
   DEMO_MENU_ITEM2(demo_text_proc, "by Shawn Hargreaves and many others"),
   DEMO_MENU_ITEM2(demo_text_proc, " "),
   DEMO_MENU_ITEM2(demo_text_proc, "Allegro Demo Game"),
   DEMO_MENU_ITEM2(demo_text_proc, "By Miran Amon, Nick Davies, Elias Pschernig, Thomas Harte & Jakub Wasilewski"),
   DEMO_MENU_ITEM2(demo_text_proc, " "),
   DEMO_MENU_ITEM4(demo_button_proc, "Back", DEMO_MENU_SELECTABLE, DEMO_STATE_MAIN_MENU),
   DEMO_MENU_END
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
