#include <allegro.h>
#include "../include/defines.h"
#include "../include/global.h"
#include "../include/menu.h"
#include "../include/menus.h"


static int _id = DEMO_STATE_GFX;

static int id(void)
{
   return _id;
}


static char *choice_yes_no[] = { "no", "yes", 0 };
static char *choice_on_off[] = { "off", "on", 0 };
static char *choice_bpp[] = { "15 bpp", "16 bpp", "24 bpp", "32 bpp", 0 };
static char *choice_res[] =
   { "640x480", "720x480", "800x600", "896x600", "1024x768",
   "1152x768",
   "1280x960", "1440x960", "1600x1200", "1800x1200", 0
};

static char *choice_update[] =
   { "double buffer", "page flipping", "triple buffer", 0 };


static void apply(DEMO_MENU * item);

static void on_vsync(DEMO_MENU * item)
{
   use_vsync = item->extra;
}


static DEMO_MENU menu[] = {
   {demo_text_proc, "GFX SETTINGS", 0, 0, 0, 0},
   {demo_choice_proc, "Fullscreen", DEMO_MENU_SELECTABLE, 0,
    (void *)choice_yes_no, 0},
   {demo_choice_proc, "Bit Depth", DEMO_MENU_SELECTABLE, 0,
    (void *)choice_bpp, 0},
   {demo_choice_proc, "Screen Size", DEMO_MENU_SELECTABLE, 0,
    (void *)choice_res, 0},
#ifdef DEMO_USE_ALLEGRO_GL
   {demo_choice_proc, "Update Method", 0, 0,
    (void *)choice_update, 0},
#else
   {demo_choice_proc, "Update Method", DEMO_MENU_SELECTABLE, 0,
    (void *)choice_update, 0},
#endif
   {demo_choice_proc, "Vsync", DEMO_MENU_SELECTABLE, 0,
    (void *)choice_on_off, on_vsync},
   {demo_button_proc, "Apply", DEMO_MENU_SELECTABLE, DEMO_STATE_GFX,
    0,
    apply},
   {demo_button_proc, "Back", DEMO_MENU_SELECTABLE,
    DEMO_STATE_OPTIONS, 0,
    0},
   {0, 0, 0, 0, 0, 0}
};


static void init(void)
{
   init_demo_menu(menu, TRUE);

   menu[1].extra = fullscreen;

   switch (bit_depth) {
      case 15:
         menu[2].extra = 0;
         break;
      case 16:
         menu[2].extra = 1;
         break;
      case 24:
         menu[2].extra = 2;
         break;
      case 32:
         menu[2].extra = 3;
         break;
   };

   switch (screen_height) {
      case 480:
         menu[3].extra = 0;
         break;
      case 600:
         menu[3].extra = 2;
         break;
      case 768:
         menu[3].extra = 4;
         break;
      case 960:
         menu[3].extra = 6;
         break;
      case 1200:
         menu[3].extra = 8;
         break;
   };
   if (((screen_width * 3) / 4) != screen_height)
      menu[3].extra |= 1;

   switch (update_driver_id) {
      case DEMO_DOUBLE_BUFFER:
         menu[4].extra = 0;
         break;
      case DEMO_PAGE_FLIPPING:
         menu[4].extra = 1;
         break;
      case DEMO_TRIPLE_BUFFER:
         menu[4].extra = 2;
         break;
   };

   menu[5].extra = use_vsync;
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


void create_gfx_menu(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}


static void apply(DEMO_MENU * item)
{
   int old_fullscreen = fullscreen;
   int old_bit_depth = bit_depth;
   int old_screen_width = screen_width;
   int old_screen_height = screen_height;
   int old_update_driver_id = update_driver_id;
   int old_use_vsync = use_vsync;

   ++item->extra;
   --item->extra;

   fullscreen = menu[1].extra;

   switch (menu[2].extra) {
      case 0:
         bit_depth = 15;
         break;
      case 1:
         bit_depth = 16;
         break;
      case 2:
         bit_depth = 24;
         break;
      case 3:
         bit_depth = 32;
         break;
   };

   switch (menu[3].extra >> 1) {
      case 0:
         screen_height = 480;
         break;
      case 1:
         screen_height = 600;
         break;
      case 2:
         screen_height = 768;
         break;
      case 3:
         screen_height = 960;
         break;
      case 4:
         screen_height = 1200;
         break;
   };
   if (menu[3].extra & 1)
      screen_width = (screen_height == 600) ? 896 : (screen_height * 3) / 2;
   else
      screen_width = (screen_height * 4) / 3;

   switch (menu[4].extra) {
      case 0:
         update_driver_id = DEMO_DOUBLE_BUFFER;
         break;
      case 1:
         update_driver_id = DEMO_PAGE_FLIPPING;
         break;
      case 2:
         update_driver_id = DEMO_TRIPLE_BUFFER;
         break;
   };

   use_vsync = menu[5].extra;

   if (fullscreen == old_fullscreen && bit_depth == old_bit_depth &&
       screen_width == old_screen_width &&
       screen_height == old_screen_height &&
       update_driver_id == old_update_driver_id) {
      return;
   }

   if (change_gfx_mode() != DEMO_OK) {
      fullscreen = old_fullscreen;
      bit_depth = old_bit_depth;
      screen_width = old_screen_width;
      screen_height = old_screen_height;
      update_driver_id = old_update_driver_id;
      use_vsync = old_use_vsync;
      change_gfx_mode();
   }

   init();
}
