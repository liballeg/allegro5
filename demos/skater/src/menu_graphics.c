#include <allegro5/allegro.h>
#include "defines.h"
#include "global.h"
#include "menu.h"
#include "menus.h"


static int _id = DEMO_STATE_GFX;

static int id(void)
{
   return _id;
}


static char *choice_on_off[] = { "off", "on", 0 };
static char *choice_bpp[] = { "15 bpp", "16 bpp", "24 bpp", "32 bpp", 0 };
static char **choice_res;

static char *choice_samples[] =
   { "1x", "2x", "4x", "8x", 0 };

static char *choice_fullscreen[] =
   { "window", "fullscreen window", "fullscreen", 0 };

static void apply(DEMO_MENU * item);

static void on_fullscreen(DEMO_MENU *item);

static void on_vsync(DEMO_MENU * item)
{
   use_vsync = item->extra;
}

static DEMO_MENU menu[] = {
   DEMO_MENU_ITEM2(demo_text_proc, "GFX SETTINGS"),
   DEMO_MENU_ITEM6(demo_choice_proc, "Mode", DEMO_MENU_SELECTABLE, 0, (void *)choice_fullscreen, on_fullscreen),
   DEMO_MENU_ITEM6(demo_choice_proc, "Bit Depth", DEMO_MENU_SELECTABLE, 0, (void *)choice_bpp, 0),
   DEMO_MENU_ITEM6(demo_choice_proc, "Screen Size", DEMO_MENU_SELECTABLE, 0, NULL, 0),
   DEMO_MENU_ITEM6(demo_choice_proc, "Supersampling", DEMO_MENU_SELECTABLE, 0, (void *)choice_samples, 0),
   DEMO_MENU_ITEM6(demo_choice_proc, "Vsync", DEMO_MENU_SELECTABLE, 0, (void *)choice_on_off, on_vsync),
   DEMO_MENU_ITEM6(demo_button_proc, "Apply", DEMO_MENU_SELECTABLE, DEMO_STATE_GFX, 0, apply),
   DEMO_MENU_ITEM6(demo_button_proc, "Back", DEMO_MENU_SELECTABLE, DEMO_STATE_OPTIONS, 0, 0),
   DEMO_MENU_END
};

static void on_fullscreen(DEMO_MENU *item)
{
   menu[3].flags = item->extra == 2 ? DEMO_MENU_SELECTABLE : 0;
}

static bool already(char *mode)
{
   int i;
   for (i = 0; choice_res[i]; i++) {
      if (!strcmp(choice_res[i], mode)) return true;
   }
   return false;
}

static void init(void)
{
   if (!choice_res) {
      int i, n = al_get_num_display_modes(), j = 0;
      choice_res = calloc(n + 1, sizeof *choice_res);
      menu[3].data = (void *)choice_res;
      for (i = 0; i < n; i++) {
         ALLEGRO_DISPLAY_MODE m;
         char str[100];
         al_get_display_mode(i, &m);
         sprintf(str, "%dx%d", m.width, m.height);
         if (!already(str)) {
            if (m.width == screen_width && m.height == screen_height) {
               menu[3].extra = j;
            }
            choice_res[j++] = strdup(str);
         }
      }
   }

   init_demo_menu(menu, true);

   menu[1].extra = fullscreen;

   menu[5].extra = use_vsync;

   on_fullscreen(menu + 1);
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
   int old_screen_samples = screen_samples;
   int old_use_vsync = use_vsync;
   char *p;

   (void)item;

   fullscreen = menu[1].extra;

   bit_depth = 0;

   p = choice_res[menu[3].extra];
   screen_width = strtol(p, &p, 10);
   screen_height = strtol(p + 1, &p, 10);

   screen_samples = 1 << menu[4].extra;

   use_vsync = menu[5].extra;

   if (fullscreen == old_fullscreen &&
       bit_depth == old_bit_depth &&
       use_vsync == old_use_vsync &&
       screen_width == old_screen_width &&
       screen_height == old_screen_height &&
       screen_samples == old_screen_samples) {
      return;
   }

   if (change_gfx_mode() != DEMO_OK) {
      fullscreen = old_fullscreen;
      bit_depth = old_bit_depth;
      screen_width = old_screen_width;
      screen_height = old_screen_height;
      screen_samples = old_screen_samples;
      use_vsync = old_use_vsync;
      change_gfx_mode();
   }

   init();
}
