#include <allegro5/allegro.h>
#include "defines.h"
#include "global.h"
#include "menu.h"
#include "menus.h"


static int _id = DEMO_STATE_MISC;

static int id(void)
{
   return _id;
}


static char *choice_yes_no[] = { "no", "yes", 0 };


static void on_fps(DEMO_MENU * item)
{
   display_framerate = item->extra;
}


static void on_limit(DEMO_MENU * item)
{
   limit_framerate = item->extra;
}


static void on_yield(DEMO_MENU * item)
{
   reduce_cpu_usage = item->extra;
}


static DEMO_MENU menu[] = {
   DEMO_MENU_ITEM2(demo_text_proc, "SYSTEM SETTINGS"),
   DEMO_MENU_ITEM6(demo_choice_proc, "Show Framerate", DEMO_MENU_SELECTABLE, 0, (void *)choice_yes_no, on_fps),
   DEMO_MENU_ITEM6(demo_choice_proc, "Cap Framerate", DEMO_MENU_SELECTABLE, 0, (void *)choice_yes_no, on_limit),
   DEMO_MENU_ITEM6(demo_choice_proc, "Conserve Power", DEMO_MENU_SELECTABLE, 0, (void *)choice_yes_no, on_yield),
   DEMO_MENU_ITEM4(demo_button_proc, "Back", DEMO_MENU_SELECTABLE, DEMO_STATE_OPTIONS),
   DEMO_MENU_END
};


static void init(void)
{
   init_demo_menu(menu, true);

   menu[1].extra = display_framerate;
   menu[2].extra = limit_framerate;
   menu[3].extra = reduce_cpu_usage;
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


static void draw(void)
{
   draw_demo_menu(menu);
}


void create_misc_menu(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}
