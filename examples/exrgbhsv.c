/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    Modified by Andrei Ellman to include color-bars.
 *
 *    This program shows how to convert colors between the different
 *    color-space representations. The central area of the screen
 *    will display the current color. On the top left corner of the
 *    screen, three sliders allow you to modify the red, green and
 *    blue value of the color. On the bottom right corner of the
 *    screen, three sliders allow you to modify the hue, saturation
 *    and value of the color. The color bars beneath the sliders
 *    show what the resulting color will look like when the slider
 *    is dragged to that position.
 *
 *    Additionally this example also shows how to "inherit" the
 *    behaviour of a GUI object and extend it, here used to create
 *    the sliders.
 */


#include <allegro.h>


#define DIALOG_NUM_SLIDERS       6
#define DIALOG_FIRST_COLOR_BAR   12
#define DIALOG_COLOR_BOX         18

/* slider types (R, G, B, and H, S, V) */
#define S_R    0
#define S_G    1
#define S_B    2
#define S_H    3
#define S_S    4
#define S_V    5



/* the current color values */
int colors[DIALOG_NUM_SLIDERS] =
{
   255,     /* red */
   255,     /* green */
   255,     /* blue */
   0,       /* hue */
   0,       /* saturation */
   255,     /* value */
};


/* the bitmaps containing the color-bars */
BITMAP *color_bar_bitmap[DIALOG_NUM_SLIDERS];

#if 0
/* RGB -> color mapping table. Not needed, but speeds things up in 8-bit mode */
RGB_MAP rgb_table;
#endif

int my_slider_proc(int msg, DIALOG *d, int c);
int update_color_value(void *dp3, int val);



DIALOG the_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)     (dp2)          (dp3) */
   { my_slider_proc,    32,   16,   256,  16,   0,    255,  0,    0,       255,  0,    NULL,    (void *)update_color_value,  &colors[S_R]  },
   { my_slider_proc,    32,   64,   256,  16,   0,    255,  0,    0,       255,  0,    NULL,    (void *)update_color_value,  &colors[S_G]  },
   { my_slider_proc,    32,   112,  256,  16,   0,    255,  0,    0,       255,  0,    NULL,    (void *)update_color_value,  &colors[S_B]  },
   { my_slider_proc,    352,  336,  256,  16,   0,    255,  0,    0,       255,  0,    NULL,    (void *)update_color_value,  &colors[S_H]  },
   { my_slider_proc,    352,  384,  256,  16,   0,    255,  0,    0,       255,  0,    NULL,    (void *)update_color_value,  &colors[S_S]  },
   { my_slider_proc,    352,  432,  256,  16,   0,    255,  0,    0,       255,  0,    NULL,    (void *)update_color_value,  &colors[S_V]  },
   { d_text_proc,       308,  22,   0,    0,    0,    255,  0,    0,       0,    0,    "R",     NULL,          NULL          },
   { d_text_proc,       308,  70,   0,    0,    0,    255,  0,    0,       0,    0,    "G",     NULL,          NULL          },
   { d_text_proc,       308,  118,  0,    0,    0,    255,  0,    0,       0,    0,    "B",     NULL,          NULL          },
   { d_text_proc,       326,  342,  0,    0,    0,    255,  0,    0,       0,    0,    "H",     NULL,          NULL          },
   { d_text_proc,       326,  390,  0,    0,    0,    255,  0,    0,       0,    0,    "S",     NULL,          NULL          },
   { d_text_proc,       326,  438,  0,    0,    0,    255,  0,    0,       0,    0,    "V",     NULL,          NULL          },
   { d_bitmap_proc,     32,   32,   256,  16,   0,    255,  0,    0,       0,    0,    NULL,    NULL,          NULL          },
   { d_bitmap_proc,     32,   80,   256,  16,   0,    255,  0,    0,       0,    0,    NULL,    NULL,          NULL          },
   { d_bitmap_proc,     32,   128,  256,  16,   0,    255,  0,    0,       0,    0,    NULL,    NULL,          NULL          },
   { d_bitmap_proc,     352,  352,  256,  16,   0,    255,  0,    0,       0,    0,    NULL,    NULL,          NULL          },
   { d_bitmap_proc,     352,  400,  256,  16,   0,    255,  0,    0,       0,    0,    NULL,    NULL,          NULL          },
   { d_bitmap_proc,     352,  448,  256,  16,   0,    255,  0,    0,       0,    0,    NULL,    NULL,          NULL          },
   { d_box_proc,        170,  170,  300,  140,  0,    0,    0,    0,       0,    0,    NULL,    NULL,          NULL          },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,    NULL,          NULL          }
};





/* helper that updates the color box */
void update_color_rectangle(int r, int g, int b)
{
   the_dlg[DIALOG_COLOR_BOX].bg = makecol(r, g, b);
}



/* helper that updates all six color-bar bitmaps according to the
 * given RGB and HSV values */
void update_color_bars(int r, int g, int b, float h, float s, float v)
{
   int i;
   int hr,hg,hb;   /* Temp RGB values for drawing the HSV sliders */


   for (i=0; i<256; i++) {
      /* Red color-bar */
      vline(color_bar_bitmap[S_R], i, 0, 15, makecol32(i, g, b));

      /* Green color-bar */
      vline(color_bar_bitmap[S_G], i, 0, 15, makecol32(r, i, b));

      /* Blue color-bar */
      vline(color_bar_bitmap[S_B], i, 0, 15, makecol32(r, g, i));

      /* Hue color-bar */
      hsv_to_rgb(i*360.0f/255.0f, s, v, &hr, &hg, &hb);
      vline(color_bar_bitmap[S_H], i, 0, 15, makecol32(hr, hg, hb));

      /* Saturation color-bar */
      hsv_to_rgb(h, i/255.0f, v, &hr, &hg, &hb);
      vline(color_bar_bitmap[S_S], i, 0, 15, makecol32(hr, hg, hb));

      /* Value color-bar */
      hsv_to_rgb(h, s, i/255.0f, &hr, &hg, &hb);
      vline(color_bar_bitmap[S_V], i, 0, 15, makecol32(hr, hg, hb));
   }
}



/* helper for reacting to the changing one of the sliders */
int update_color_value(void *dp3, int val)
{
   /* 'val' is the value of the slider's position (0-255),
      'type' is which slider was changed */
   int type = ((uintptr_t)dp3 - (uintptr_t)colors) / sizeof(colors[0]);
   int r, g, b;
   float h, s, v;

   if (colors[type] != val) {
      colors[type] = val;

      if ((type == S_R) || (type == S_G) || (type == S_B)) {
	 /* The slider that's changed is either R, G, or B.
	    Convert RGB color to HSV. */
	 r = colors[S_R];
	 g = colors[S_G];
	 b = colors[S_B];

	 rgb_to_hsv(r, g, b, &h, &s, &v);

	 colors[S_H] = h * 255.0f / 360.0f + 0.5f;
	 colors[S_S] = s * 255.0f + 0.5f;
	 colors[S_V] = v * 255.0f + 0.5f;
      }
      else {
	 /* The slider that's changed is either H, S, or V.
	    Convert HSV color to RGB. */
	 h = colors[S_H] * 360.0f / 255.0f;
	 s = colors[S_S] / 255.0f;
	 v = colors[S_V] / 255.0f;

	 hsv_to_rgb(h, s, v, &r, &g, &b);

	 colors[S_R] = r;
	 colors[S_G] = g;
	 colors[S_B] = b;
      }

      update_color_bars(r,g,b,h,s,v);

      vsync();

      if (get_color_depth() == 8) {
	 /* set the screen background to the new color if in paletted mode */
	 RGB rgb;
	 rgb.r = colors[S_R]>>2;
	 rgb.g = colors[S_G]>>2;
	 rgb.b = colors[S_B]>>2;

	 set_color(255, &rgb);   /* in 8-bit color modes, we're changing the
				    'white' background-color */
      }

      /* Update the rectangle in the middle to the new color */
      update_color_rectangle(r,g,b);
      object_message(&the_dlg[DIALOG_COLOR_BOX], MSG_DRAW, 0);

      /* Display the updated color-bar bitmaps */
      object_message(&the_dlg[DIALOG_FIRST_COLOR_BAR], MSG_DRAW, 0);
      object_message(&the_dlg[DIALOG_FIRST_COLOR_BAR+1], MSG_DRAW, 0);
      object_message(&the_dlg[DIALOG_FIRST_COLOR_BAR+2], MSG_DRAW, 0);
      object_message(&the_dlg[DIALOG_FIRST_COLOR_BAR+3], MSG_DRAW, 0);
      object_message(&the_dlg[DIALOG_FIRST_COLOR_BAR+4], MSG_DRAW, 0);
      object_message(&the_dlg[DIALOG_FIRST_COLOR_BAR+5], MSG_DRAW, 0);
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
	    object_message(d, MSG_DRAW, 0);
	 }
	 break;
   }

   return d_slider_proc(msg, d, c);
}



int main(void)
{
   int i;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();

   /* Set the deepest color depth we can set */
   set_color_depth(32);
   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      set_color_depth(24);
      if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
	 set_color_depth(16);
	 if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
	    set_color_depth(15);
	    if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
	       set_color_depth(8);
	       if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
		 allegro_message("Error setting a graphics mode\n%s\n",
				 allegro_error);
		 return 1;
	       }
	    }
	 }
      }
   }

   /* In the case we're using an 8-bit color screen, we must set up the palette */
   if (get_color_depth() == 8) {
      PALETTE pal332;
      generate_332_palette(pal332);

      /* Set the palette to the best approximation of a truecolor palette
         we can get with 8-bit color */
      set_palette(pal332);

      /* In 8-bit color mode, if there's an RGB table, the sliders
	 will move a lot more smoothly and the updating will be
	 a lot quicker. But if there's no RGB table, this has the
	 advantage that the color conversion routines will take into
	 account any changes in the background color. Instead of
	 changing background color, we could also rely on the colored
	 rectangle like in the other color modes, but using the 3-3-2
	 palette, this doesn't display the color as accurately as
	 changing the background color. */
#if 0
      /* Create an RGB table to speedup makecol8() */
      create_rgb_table(&rgb_table, pal332, NULL);
      rgb_map = &rgb_table;
#endif
   }

   clear_to_color(screen,makecol(255,255,255));

   /* color the dialog controls appropriately */
   /* R -> Red */
   the_dlg[S_R].fg = the_dlg[DIALOG_NUM_SLIDERS+S_R].fg = makecol(255,0,0);
   /* G -> Green */
   the_dlg[S_G].fg = the_dlg[DIALOG_NUM_SLIDERS+S_G].fg = makecol(0,255,0);
   /* B -> Blue */
   the_dlg[S_B].fg = the_dlg[DIALOG_NUM_SLIDERS+S_B].fg = makecol(0,0,255);

   /* H -> Grey */
   the_dlg[S_H].fg = the_dlg[DIALOG_NUM_SLIDERS+S_H].fg = makecol(192,192,192);
   /* S -> Dark Grey */
   the_dlg[S_S].fg = the_dlg[DIALOG_NUM_SLIDERS+S_S].fg = makecol(128,128,128);
   /* V -> Black */
   the_dlg[S_V].fg = the_dlg[DIALOG_NUM_SLIDERS+S_V].fg = makecol(0,0,0);

   /* Create the bitmaps for the color-bars */
   for (i=0; i<DIALOG_NUM_SLIDERS; i++) {
      if (!(color_bar_bitmap[i] = create_bitmap_ex(32,256,16))) {
         allegro_message("Error creating a color-bar bitmap\n");
         return 1;
      }

      the_dlg[DIALOG_FIRST_COLOR_BAR+i].dp = color_bar_bitmap[i];
   }

   for (i=0; i<DIALOG_NUM_SLIDERS*3; i++)
      the_dlg[i].bg = makecol(255,255,255);

   the_dlg[DIALOG_COLOR_BOX].fg = makecol(0,0,0);

   textout_ex(screen,font,"RGB<->HSV color spaces example.",344,4,
	      makecol(0,0,0),-1);
   textout_ex(screen,font,"Drag sliders to change color values.",344,12,
	      makecol(0,0,0),-1);

   textout_ex(screen,font,"The color-bars beneath the sliders",24,384,
	      makecol(128,128,128),-1);
   textout_ex(screen,font,"show what the resulting color will",24,392,
	      makecol(128,128,128),-1);
   textout_ex(screen,font,"look like when the slider is",24,400,
	      makecol(128,128,128),-1);
   textout_ex(screen,font,"dragged to that position.",24,408,
	      makecol(128,128,128),-1);

   switch (get_color_depth()) {

      case 32:
	 textout_ex(screen,font,"Running in truecolor (32-bit 888)",352,24,
		    makecol(128,128,128),-1);
	 textout_ex(screen,font,"16777216 colors",352,32,
		    makecol(128,128,128),-1);
	 break;

      case 24:
	 textout_ex(screen,font,"Running in truecolor (24-bit 888)",352,24,
		    makecol(128,128,128),-1);
	 textout_ex(screen,font,"16777216 colors",352,32,
		    makecol(128,128,128),-1);
	 break;

      case 16:
	 textout_ex(screen,font,"Running in hicolor (16-bit 565)",352,24,
		    makecol(128,128,128),-1);
	 textout_ex(screen,font,"65536 colors",352,32,
		    makecol(128,128,128),-1);
	 break;

      case 15:
	 textout_ex(screen,font,"Running in hicolor (15-bit 555)",352,24,
		    makecol(128,128,128),-1);
	 textout_ex(screen,font,"32768 colors",352,32,
		    makecol(128,128,128),-1);
	 break;

      case 8:
	 textout_ex(screen,font,"Running in paletted mode (8-bit 332)",352,24,
		    makecol(128,128,128),-1);
	 textout_ex(screen,font,"256 colors",352,32,
		    makecol(128,128,128),-1);
	 break;

      default:
	 textout_ex(screen,font,"Unknown color depth",400,16,0,-1);
   }

   update_color_rectangle(colors[S_R], colors[S_G], colors[S_B]);
   update_color_bars(colors[S_R], colors[S_G], colors[S_B],
		     colors[S_H]*360.0f/255.0f,
		     colors[S_S]/255.0f, colors[S_V]/255.0f);

   do_dialog(the_dlg, -1);

   for (i=0; i<DIALOG_NUM_SLIDERS; i++)
      destroy_bitmap(color_bar_bitmap[i]);

   return 0;
}
END_OF_MAIN()
