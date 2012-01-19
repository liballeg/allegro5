#include <allegro5/allegro.h>
#include "defines.h"
#include "global.h"
#include "menu.h"
#include "menus.h"
#include "music.h"


static int _id = DEMO_STATE_SOUND;


static int id(void)
{
   return _id;
}


static char *choice_volume[] =
   { "0%%", "10%%", "20%%", "30%%", "40%%", "50%%", "60%%", "70%%",
   "80%%", "90%%", "100%%", 0
};


static void on_sound(DEMO_MENU * item)
{
   sound_volume = item->extra;
   set_sound_volume(sound_volume / 10.0);
}


static void on_music(DEMO_MENU * item)
{
   music_volume = item->extra;
   set_music_volume(music_volume / 10.0);
}


static DEMO_MENU menu[] = {
   DEMO_MENU_ITEM2(demo_text_proc, "SOUND LEVELS"),
   DEMO_MENU_ITEM6(demo_choice_proc, "Sound", DEMO_MENU_SELECTABLE, 0, (void *)choice_volume, on_sound),
   DEMO_MENU_ITEM6(demo_choice_proc, "Music", DEMO_MENU_SELECTABLE, 0, (void *)choice_volume, on_music),
   DEMO_MENU_ITEM4(demo_button_proc, "Back", DEMO_MENU_SELECTABLE, DEMO_STATE_OPTIONS),
   DEMO_MENU_END
};


static void init(void)
{
   init_demo_menu(menu, true);
   menu[1].extra = sound_volume;
   menu[2].extra = music_volume;
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
   }
}


static void draw(void)
{
   draw_demo_menu(menu);
}


void create_sound_menu(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}
