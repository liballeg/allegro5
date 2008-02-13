#include <allegro.h>
#include "../include/defines.h"
#include "../include/global.h"
#include "../include/menu.h"
#include "../include/menus.h"


static int _id = DEMO_STATE_CONTROLS;

static int id(void)
{
   return _id;
}


static char *choice_controls[] = { "keyboard", "gamepad", 0 };


static void on_controller(DEMO_MENU * item);


static DEMO_MENU menu[] = {
   {demo_text_proc, "SETUP CONTROLS", 0, 0, 0, 0},
   {demo_choice_proc, "Controller", DEMO_MENU_SELECTABLE, 0,
    (void *)choice_controls, on_controller},
   {demo_key_proc, "Left", DEMO_MENU_SELECTABLE, DEMO_BUTTON_LEFT, 0,
    0},
   {demo_key_proc, "Right", DEMO_MENU_SELECTABLE, DEMO_BUTTON_RIGHT,
    0, 0},
   {demo_key_proc, "Jump", DEMO_MENU_SELECTABLE, DEMO_BUTTON_JUMP, 0,
    0},
   {demo_button_proc, "Back", DEMO_MENU_SELECTABLE,
    DEMO_STATE_OPTIONS, 0,
    0},
   {0, 0, 0, 0, 0, 0}
};


static void init(void)
{
   init_demo_menu(menu, TRUE);

   menu[1].extra = controller_id;
}


static int update(void)
{
   int ret = update_demo_menu(menu);

   switch (ret) {
      case DEMO_MENU_CONTINUE:
         return id();

      case DEMO_MENU_BACK:
         return DEMO_STATE_OPTIONS;

      default:
         return ret;
   };
}


static void draw(BITMAP *canvas)
{
   draw_demo_menu(canvas, menu);
}


void create_controls_menu(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}


static void on_controller(DEMO_MENU * item)
{
   controller_id = item->extra;
}
