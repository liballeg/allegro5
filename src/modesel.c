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
 *      GUI routines.
 *
 *      The graphics mode selection dialog.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/aintern.h"



typedef struct GFX_MODE_DATA
{
   int w, h;
   AL_CONST char *s;
} GFX_MODE_DATA;



static GFX_MODE_DATA gfx_mode_data[] =
{
   { 320,   200,  "320x200"   },
   { 320,   240,  "320x240"   },
   { 640,   400,  "640x400"   },
   { 640,   480,  "640x480"   },
   { 800,   600,  "800x600"   },
   { 1024,  768,  "1024x768"  },
   { 1280,  1024, "1280x1024" },
   { 1600,  1200, "1600x1200" },
   { 80,    80,   "80x80"     },
   { 160,   120,  "160x120"   },
   { 256,   200,  "256x200"   },
   { 256,   224,  "256x224"   },
   { 256,   240,  "256x240"   },
   { 256,   256,  "256x256"   },
   { 320,   100,  "320x100"   },
   { 320,   350,  "320x350"   },
   { 320,   400,  "320x400"   },
   { 320,   480,  "320x480"   },
   { 320,   600,  "320x600"   },
   { 360,   200,  "360x200"   },
   { 360,   240,  "360x240"   },
   { 360,   270,  "360x270"   },
   { 360,   360,  "360x360"   },
   { 360,   400,  "360x400"   },
   { 360,   480,  "360x480"   },
   { 360,   600,  "360x600"   },
   { 376,   282,  "376x282"   },
   { 376,   308,  "376x308"   },
   { 376,   564,  "376x564"   },
   { 400,   150,  "400x150"   },
   { 400,   300,  "400x300"   },
   { 400,   600,  "400x600"   },
   { 0,     0,    NULL        }
};



/* gfx_mode_getter:
 *  Listbox data getter routine for the graphics mode list.
 */
static AL_CONST char *gfx_mode_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(gfx_mode_data) / sizeof(GFX_MODE_DATA) - 1;
      return NULL;
   }

   return get_config_text(gfx_mode_data[index].s);
}



static _DRIVER_INFO *driver_list;

static int *gfx_card_list;
static int gfx_card_count;



/* gfx_card_cmp:
 *  qsort() callback for sorting the graphics driver list.
 */
static int gfx_card_cmp(AL_CONST void *e1, AL_CONST void *e2)
{
   /* note: drivers don't have to be in this array in order to be listed
    * by the dialog. This is just to control how the list is sorted.
    */
   static int driver_order[] = 
   { 
      #ifdef GFX_AUTODETECT
	 GFX_AUTODETECT,
      #endif
      #ifdef GFX_AUTODETECT_FULLSCREEN
	 GFX_AUTODETECT_FULLSCREEN,
      #endif
      #ifdef GFX_AUTODETECT_WINDOWED
	 GFX_AUTODETECT_WINDOWED,
      #endif
      #ifdef GFX_VGA
	 GFX_VGA,
      #endif
      #ifdef GFX_MODEX
	 GFX_MODEX,
      #endif
      #ifdef GFX_VESA1
	 GFX_VESA1,
      #endif
      #ifdef GFX_VESA2B
	 GFX_VESA2B,
      #endif
      #ifdef GFX_VESA2L
	 GFX_VESA2L,
      #endif
      #ifdef GFX_VESA3
	 GFX_VESA3,
      #endif
      #ifdef GFX_VBEAF
	 GFX_VBEAF,
      #endif
      #ifdef GFX_XTENDED
	 GFX_XTENDED
      #endif
   };

   int d1 = *((int *)e1);
   int d2 = *((int *)e2);
   AL_CONST char *n1, *n2;
   int i;

   if (d1 < 0)
      return -1;
   else if (d2 < 0)
      return 1;

   n1 = ((GFX_DRIVER *)driver_list[d1].driver)->ascii_name;
   n2 = ((GFX_DRIVER *)driver_list[d2].driver)->ascii_name;

   d1 = driver_list[d1].id;
   d2 = driver_list[d2].id;

   for (i=0; i<(int)(sizeof(driver_order)/sizeof(int)); i++) {
      if (d1 == driver_order[i])
	 return -1;
      else if (d2 == driver_order[i])
	 return 1;
   }

   return stricmp(n1, n2);
}



/* setup_card_list:
 *  Fills the list of video cards with info about the available drivers.
 */
static void setup_card_list(int *list)
{
   if (system_driver->gfx_drivers)
      driver_list = system_driver->gfx_drivers();
   else
      driver_list = _gfx_driver_list;

   gfx_card_list = list;
   gfx_card_count = 0;

   while (driver_list[gfx_card_count].driver) {
      gfx_card_list[gfx_card_count] = gfx_card_count;
      gfx_card_count++;
   }

   gfx_card_list[gfx_card_count++] = -1;
   gfx_card_list[gfx_card_count++] = -2;
   gfx_card_list[gfx_card_count++] = -3;

   qsort(gfx_card_list, gfx_card_count, sizeof(int), gfx_card_cmp);
}



/* gfx_card_getter:
 *  Listbox data getter routine for the graphics card list.
 */
static AL_CONST char *gfx_card_getter(int index, int *list_size)
{
   int i;

   if (index < 0) {
      if (list_size)
	 *list_size = gfx_card_count;
      return NULL;
   }

   i = gfx_card_list[index];

   if (i >= 0)
      return get_config_text(((GFX_DRIVER *)driver_list[i].driver)->ascii_name);
   else if (i == -1)
      return get_config_text("Autodetect");
   else if (i == -2)
      return get_config_text("Auto fullscreen");
   else
      return get_config_text("Auto windowed");
}



/* gfx_depth_getter:
 *  Listbox data getter routine for the color depth list.
 */
static AL_CONST char *gfx_depth_getter(int index, int *list_size)
{
   static AL_CONST char *depth[] = {
      " 8 bpp (256 color)",
      "15 bpp (32K color)",
      "16 bpp (64K color)",
      "24 bpp (16M color)",
      "32 bpp (16M color)"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(depth) / sizeof(char *);
      return NULL;
   }

   return get_config_text(depth[index]);
}



static DIALOG gfx_mode_dialog[] =
{
   /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)                     (dp2) (dp3) */
   { _gui_shadow_box_proc, 0,    0,    313,  159,  0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { _gui_ctext_proc,      156,  8,    1,    1,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  105,  101,  17,   0,    0,    0,    D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  127,  101,  17,   0,    0,    27,   D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_list_proc,       16,   28,   165,  116,  0,    0,    0,    D_EXIT,  0,    0,    (void*)gfx_card_getter,  NULL, NULL  },
   { _gui_list_proc,       196,  28,   101,  68,   0,    0,    0,    D_EXIT,  3,    0,    (void*)gfx_mode_getter,  NULL, NULL  },
   { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { NULL,                 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  }
};



static DIALOG gfx_mode_ex_dialog[] =
{
   /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)                     (dp2) (dp3) */
   { _gui_shadow_box_proc, 0,    0,    313,  159,  0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { _gui_ctext_proc,      156,  8,    1,    1,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  105,  101,  17,   0,    0,    0,    D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  127,  101,  17,   0,    0,    27,   D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_list_proc,       16,   28,   165,  68,   0,    0,    0,    D_EXIT,  0,    0,    (void*)gfx_card_getter,  NULL, NULL  },
   { _gui_list_proc,       196,  28,   101,  68,   0,    0,    0,    D_EXIT,  3,    0,    (void*)gfx_mode_getter,  NULL, NULL  },
   { _gui_list_proc,       16,   105,  165,  44,   0,    0,    0,    D_EXIT,  0,    0,    (void*)gfx_depth_getter, NULL, NULL  },
   { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { NULL,                 0,    0,    0,    0,    0,    0,    0,    0,          0, 0,    NULL,                    NULL, NULL  }
};



#define GFX_TITLE          1
#define GFX_OK             2
#define GFX_CANCEL         3
#define GFX_DRIVER_LIST    4
#define GFX_MODE_LIST      5
#define GFX_DEPTH_LIST     6



/* gfx_mode_select:
 *  Displays the Allegro graphics mode selection dialog, which allows the
 *  user to select a screen mode and graphics card. Stores the selection
 *  in the three variables, and returns zero if it was closed with the 
 *  Cancel button, or non-zero if it was OK'd.
 */
int gfx_mode_select(int *card, int *w, int *h)
{
   int card_list[256];
   int i, ret;

   clear_keybuf();

   do {
   } while (gui_mouse_b());

   gfx_mode_dialog[GFX_TITLE].dp = (void*)get_config_text("Graphics Mode");
   gfx_mode_dialog[GFX_OK].dp = (void*)get_config_text("OK");
   gfx_mode_dialog[GFX_CANCEL].dp = (void*)get_config_text("Cancel");

   setup_card_list(card_list);

   centre_dialog(gfx_mode_dialog);
   set_dialog_color(gfx_mode_dialog, gui_fg_color, gui_bg_color);
   ret = popup_dialog(gfx_mode_dialog, GFX_DRIVER_LIST);

   i = card_list[gfx_mode_dialog[GFX_DRIVER_LIST].d1];

   if (i >= 0)
      *card = driver_list[i].id;
   else if (i == -1)
      *card = GFX_AUTODETECT;
   else if (i == -2)
      *card = GFX_AUTODETECT_FULLSCREEN;
   else
      *card = GFX_AUTODETECT_WINDOWED;

   *w = gfx_mode_data[gfx_mode_dialog[GFX_MODE_LIST].d1].w;
   *h = gfx_mode_data[gfx_mode_dialog[GFX_MODE_LIST].d1].h;

   if (ret == GFX_CANCEL)
      return FALSE;
   else 
      return TRUE;
}



/* gfx_mode_select_ex:
 *  Extended version of the graphics mode selection dialog, which allows the 
 *  user to select the color depth as well as the resolution and hardware 
 *  driver. This version of the function reads the initial values from the 
 *  parameters when it activates, so you can specify the default values.
 */
int gfx_mode_select_ex(int *card, int *w, int *h, int *color_depth)
{
   int card_list[256];
   static int depth_list[] = { 8, 15, 16, 24, 32, 0 };
   int i, ret;

   clear_keybuf();

   do {
   } while (gui_mouse_b());

   setup_card_list(card_list);

   gfx_mode_ex_dialog[GFX_DRIVER_LIST].d1 = 0;

   for (i=0; i<gfx_card_count; i++) {
      if ((card_list[i] >= 0) && (driver_list[card_list[i]].id == *card)) {
	 gfx_mode_ex_dialog[GFX_DRIVER_LIST].d1 = i;
	 break;
      }
   }
   if (*card == GFX_AUTODETECT_FULLSCREEN)
      gfx_mode_ex_dialog[GFX_DRIVER_LIST].d1 = 1;
   else if (*card == GFX_AUTODETECT_WINDOWED)
      gfx_mode_ex_dialog[GFX_DRIVER_LIST].d1 = 2;

   for (i=0; gfx_mode_data[i].s; i++) {
      if ((gfx_mode_data[i].w == *w) && (gfx_mode_data[i].h == *h)) {
	 gfx_mode_ex_dialog[GFX_MODE_LIST].d1 = i;
	 break; 
      }
   }

   for (i=0; depth_list[i]; i++) {
      if (depth_list[i] == *color_depth) {
	 gfx_mode_ex_dialog[GFX_DEPTH_LIST].d1 = i;
	 break;
      }
   }

   gfx_mode_ex_dialog[GFX_TITLE].dp = (void*)get_config_text("Graphics Mode");
   gfx_mode_ex_dialog[GFX_OK].dp = (void*)get_config_text("OK");
   gfx_mode_ex_dialog[GFX_CANCEL].dp = (void*)get_config_text("Cancel");

   centre_dialog(gfx_mode_ex_dialog);
   set_dialog_color(gfx_mode_ex_dialog, gui_fg_color, gui_bg_color);

   ret = popup_dialog(gfx_mode_ex_dialog, GFX_DRIVER_LIST);

   i = card_list[gfx_mode_ex_dialog[GFX_DRIVER_LIST].d1];

   if (i >= 0)
      *card = driver_list[i].id;
   else if (i == -1)
      *card = GFX_AUTODETECT;
   else if (i == -2)
      *card = GFX_AUTODETECT_FULLSCREEN;
   else
      *card = GFX_AUTODETECT_WINDOWED;

   *w = gfx_mode_data[gfx_mode_ex_dialog[GFX_MODE_LIST].d1].w;
   *h = gfx_mode_data[gfx_mode_ex_dialog[GFX_MODE_LIST].d1].h;

   *color_depth = depth_list[gfx_mode_ex_dialog[GFX_DEPTH_LIST].d1];

   if (ret == GFX_CANCEL)
      return FALSE;
   else 
      return TRUE;
}

