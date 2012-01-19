#include <allegro5/allegro.h>
#include "defines.h"
#include "menu.h"
#include "menus.h"


static int _id = DEMO_STATE_OPTIONS;

static int id(void)
{
   return _id;
}


static DEMO_MENU menu[] = {
   DEMO_MENU_ITEM2(demo_text_proc, "OPTIONS"),
   DEMO_MENU_ITEM4(demo_button_proc, "Graphics", DEMO_MENU_SELECTABLE, DEMO_STATE_GFX),
   DEMO_MENU_ITEM4(demo_button_proc, "Sound", DEMO_MENU_SELECTABLE, DEMO_STATE_SOUND),
   DEMO_MENU_ITEM4(demo_button_proc, "Controls", DEMO_MENU_SELECTABLE, DEMO_STATE_CONTROLS),
   DEMO_MENU_ITEM4(demo_button_proc, "System", DEMO_MENU_SELECTABLE, DEMO_STATE_MISC),
   DEMO_MENU_ITEM4(demo_button_proc, "Back", DEMO_MENU_SELECTABLE, DEMO_STATE_MAIN_MENU),
   DEMO_MENU_END
};


static void init(void)
{
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


void create_options_menu(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}
