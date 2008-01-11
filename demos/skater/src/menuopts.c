#include <allegro.h>
#include "../include/defines.h"
#include "../include/menu.h"
#include "../include/menus.h"


static int _id = DEMO_STATE_OPTIONS;

static int id(void)
{
   return _id;
}


static DEMO_MENU menu[] = {
   {demo_text_proc, "OPTIONS", 0, 0, 0, 0},
   {demo_button_proc, "Graphics", DEMO_MENU_SELECTABLE,
    DEMO_STATE_GFX, 0,
    0},
   {demo_button_proc, "Sound", DEMO_MENU_SELECTABLE, DEMO_STATE_SOUND,
    0,
    0},
   {demo_button_proc, "Controls", DEMO_MENU_SELECTABLE,
    DEMO_STATE_CONTROLS, 0, 0},
   {demo_button_proc, "System", DEMO_MENU_SELECTABLE, DEMO_STATE_MISC,
    0,
    0},
   {demo_button_proc, "Back", DEMO_MENU_SELECTABLE,
    DEMO_STATE_MAIN_MENU,
    0, 0},
   {0, 0, 0, 0, 0, 0}
};


static void init(void)
{
   init_demo_menu(menu, TRUE);
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


static void draw(BITMAP *canvas)
{
   draw_demo_menu(canvas, menu);
}


void create_options_menu(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}
