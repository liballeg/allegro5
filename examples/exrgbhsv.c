/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program shows how to convert colors between the RGB and HSV
 *    representations.
 */


#include "allegro.h"



/* slider types (R, G, B, and H, S, V) */
#define S_R    0
#define S_G    1
#define S_B    2
#define S_H    3
#define S_S    4
#define S_V    5



/* the current color values */
static int colors[6] =
{
   255,     /* red */
   255,     /* green */
   255,     /* blue */
   0,       /* hue */
   0,       /* saturation */
   255      /* value */
};



/* helper for changing one of the color values */
int update_color(void *dp3, int val)
{
   int type = ((unsigned long)dp3 - (unsigned long)colors) / sizeof(colors[0]);
   int r, g, b;
   float h, s, v;
   RGB rgb;

   if (colors[type] != val) {
      colors[type] = val;

      if ((type == S_R) || (type == S_G) || (type == S_B)) {
	 /* convert RGB color to HSV */
	 r = colors[S_R];
	 g = colors[S_G];
	 b = colors[S_B];

	 rgb_to_hsv(r, g, b, &h, &s, &v);

	 colors[S_H] = h * 255.0 / 360.0;
	 colors[S_S] = s * 255.0;
	 colors[S_V] = v * 255.0;
      }
      else {
	 /* convert HSV color to RGB */
	 h = colors[S_H] * 360.0 / 255.0;
	 s = colors[S_S] / 255.0;
	 v = colors[S_V] / 255.0;

	 hsv_to_rgb(h, s, v, &r, &g, &b);

	 colors[S_R] = r;
	 colors[S_G] = g;
	 colors[S_B] = b;
      }

      /* set the screen background to the new color */
      rgb.r = colors[S_R]/4;
      rgb.g = colors[S_G]/4;
      rgb.b = colors[S_B]/4;

      vsync();
      set_color(0, &rgb);
   } 

   return D_O_K;
}



/* gui object procedure for the color selection sliders */
int my_slider_proc(int msg, DIALOG *d, int c)
{
   int *color = (int *)d->dp3;

   switch (msg) {

      case MSG_START:
	 /* initialise the slider position */
	 d->d2 = *color;
	 break;

      case MSG_IDLE:
	 /* has the slider position changed? */
	 if (d->d2 != *color) {
	    d->d2 = *color;
	    show_mouse(NULL);
	    object_message(d, MSG_DRAW, 0);
	    show_mouse(screen);
	 }
	 break;
   }

   return d_slider_proc(msg, d, c);
}



DIALOG the_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)     (dp2)          (dp3) */
   { my_slider_proc,    32,   32,   256,  16,   1,    0,    0,    0,       255,  0,    NULL,    update_color,  &colors[S_R]  },
   { my_slider_proc,    48,   64,   256,  16,   2,    0,    0,    0,       255,  0,    NULL,    update_color,  &colors[S_G]  },
   { my_slider_proc,    64,   96,   256,  16,   4,    0,    0,    0,       255,  0,    NULL,    update_color,  &colors[S_B]  },
   { my_slider_proc,    320,  368,  256,  16,   255,  0,    0,    0,       255,  0,    NULL,    update_color,  &colors[S_H]  },
   { my_slider_proc,    336,  400,  256,  16,   255,  0,    0,    0,       255,  0,    NULL,    update_color,  &colors[S_S]  },
   { my_slider_proc,    352,  432,  256,  16,   255,  0,    0,    0,       255,  0,    NULL,    update_color,  &colors[S_V]  },
   { d_text_proc,       300,  38,   0,    0,    255,  0,    0,    0,       0,    0,    "R",     NULL,          NULL          },
   { d_text_proc,       316,  70,   0,    0,    255,  0,    0,    0,       0,    0,    "G",     NULL,          NULL          },
   { d_text_proc,       332,  102,  0,    0,    255,  0,    0,    0,       0,    0,    "B",     NULL,          NULL          },
   { d_text_proc,       302,  374,  0,    0,    255,  0,    0,    0,       0,    0,    "H",     NULL,          NULL          },
   { d_text_proc,       318,  406,  0,    0,    255,  0,    0,    0,       0,    0,    "S",     NULL,          NULL          },
   { d_text_proc,       334,  438,  0,    0,    255,  0,    0,    0,       0,    0,    "V",     NULL,          NULL          },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,    NULL,          NULL          }
};



int main()
{
   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();
   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);
   clear_bitmap(screen);

   do_dialog(the_dlg, -1);

   return 0;
}

END_OF_MAIN();
