/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    A follow up of the exgui.c example showing how to customise the
 *    default Allegro framework. In this case a dialog procedure
 *    animates a graphical clock without disrupting other GUI
 *    dialogs.  A more simple option shows how to dynamically change
 *    the font used by all GUI elements.
 */


#include <time.h>

#include <allegro.h>
#include "example.h"



/* we need to load example.dat to access the big font */
DATAFILE *datafile;


/* for the d_edit_proc() object */
#define LEN 32
char the_string[(LEN + 1) * 6] = "Change Me!";


/* since we change the font, we need to store a copy of the original one */
FONT *original_font;


/* the current time, for the clock object */
struct tm the_time;



/* A custom dialog procedure for the 'change font' button. This uses a
 * simple form of inheritance: it calls d_button_proc() to do most of
 * the work, so it behaves exactly like any other button, but when the
 * button is clicked and d_button_proc() returns D_CLOSE, it intercepts
 * the message and changes the font instead.
 */
int change_font_proc(int msg, DIALOG *d, int c)
{
   int ret;

   /* call the parent object */
   ret = d_button_proc(msg, d, c);

   /* trap the close return value and change the font */
   if (ret == D_CLOSE) {
      if (font == original_font)
	 font = datafile[BIG_FONT].dat;
      else
	 font = original_font;

      return D_REDRAW; 
   }

   /* otherwise just return */
   return ret;
}



/* helper function to draw a hand on the clock */
void draw_hand(BITMAP *bmp, int value, int range, int v2, int range2, fixed length, int color)
{
   fixed angle;
   fixed x, y;
   int w, h;

   angle = ((itofix(value) * 256) / range) + 
	   ((itofix(v2) * 256) / (range * range2)) - itofix(64);

   x = fixmul(fixcos(angle), length);
   y = fixmul(fixsin(angle), length);
   w = bmp->w / 2;
   h = bmp->h / 2;

   line(bmp, w, h, w + fixtoi(x*w), h + fixtoi(y*h), color);
}



/* custom dialog procedure for the clock object */
int clock_proc(int msg, DIALOG *d, int c)
{
   time_t current_time;
   struct tm *t;
   BITMAP *temp;
   fixed angle, x, y;

   /* process the message */
   switch (msg) {

      /* initialise when we get a start message */
      case MSG_START:
	 /* store the current time */
	 current_time = time(NULL);
	 t = localtime(&current_time);
	 the_time = *t;

	 /* draw the clock background onto a memory bitmap */
	 temp = create_bitmap(d->w, d->h);
	 clear_to_color(temp, d->bg);

	 /* draw borders and a nobble in the middle */
	 circle(temp, temp->w/2, temp->h/2, temp->w/2-1, d->fg);
	 circlefill(temp, temp->w/2, temp->h/2, 2, d->fg);

	 /* draw ticks around the edge */
	 for (angle=0; angle<itofix(256); angle+=itofix(256)/12) {
	    x = fixcos(angle);
	    y = fixsin(angle);
	    line(temp, temp->w/2+fixtoi(x*temp->w*15/32), 
		       temp->h/2+fixtoi(y*temp->w*15/32), 
		       temp->w/2+fixtoi(x*temp->w/2), 
		       temp->h/2+fixtoi(y*temp->w/2), d->fg);
	 }

	 /* store the clock background bitmap in d->dp */
	 d->dp = temp;
	 break;

      /* shutdown when we get an end message */
      case MSG_END:
	 /* destroy the clock background bitmap */
	 destroy_bitmap(d->dp);
	 break;

      /* update the clock in response to idle messages */
      case MSG_IDLE:
	 /* read the current time */
	 current_time = time(NULL);
	 t = localtime(&current_time);

	 /* check if it has changed */
	 if ((the_time.tm_sec != t->tm_sec) ||
	     (the_time.tm_min != t->tm_min) ||
	     (the_time.tm_hour != t->tm_hour)) {
	    the_time = *t;

	    /* Redraw ourselves if the time has changed. Note that the dialog
	     * manager automatically turns off the mouse pointer whenever a
	     * MSG_DRAW message is sent to an individual object or an entire
	     * dialog, so we don't have to do it explicitly. Also note the use
	     * of the object_message function rather than a simple recursive
	     * call to clock_proc(). This vectors the call through the function
	     * pointer in the dialog object, which allows other object
	     * procedures to hook it, for example a different type of clock
	     * could process the draw messages itself but pass idle messages
	     * on to this procedure.
	     */
	    object_message(d, MSG_DRAW, 0);
	 }
	 break;

      /* draw the clock in response to draw messages */
      case MSG_DRAW:
	 /* draw onto a temporary memory bitmap to prevent flicker */
	 temp = create_bitmap(d->w, d->h);

	 /* copy the clock background onto the temporary bitmap */
	 blit(d->dp, temp, 0, 0, 0, 0, d->w, d->h);

	 /* draw the hands */
	 draw_hand(temp, the_time.tm_sec, 60, 0, 1, itofix(9)/10, d->fg);
	 draw_hand(temp, the_time.tm_min, 60, the_time.tm_sec, 60, itofix(5)/6, d->fg);
	 draw_hand(temp, the_time.tm_hour, 12, the_time.tm_min, 60, itofix(1)/2, d->fg);

	 /* copy the temporary bitmap onto the screen */
	 blit(temp, screen, 0, 0, d->x, d->y, d->w, d->h);
	 destroy_bitmap(temp);
	 break; 
   }

   /* always return OK status, since the clock doesn't ever need to close 
    * the dialog or get the input focus.
    */
   return D_O_K;
}



DIALOG the_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1) (d2)  (dp)           (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    255,  0,    0,    0,       0,   0,    NULL,          NULL, NULL  },
   { d_edit_proc,       12,   82,   256,  48,   255,  0,    0,    0,       LEN, 0,    the_string,    NULL, NULL  },
   { d_check_proc,      12,   12,   161,  49,   255,  0,    't',  0,       0,   0,    "&Toggle Me",  NULL, NULL  },
   { clock_proc,        242,  12,   64,   64,   255,  0,    0,    0,       0,   0,    NULL,          NULL, NULL  },
   { change_font_proc,  12,   142,  141,  49,   255,  0,    'f',  D_EXIT,  0,   0, "Change &Font",   NULL, NULL  },
   { d_button_proc,     162,  142,  141,  49,   255,  0,    0,    D_EXIT,  0,   0,    "Exit",        NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,       0,   0,    NULL,          NULL, NULL  }
};



int main(int argc, char *argv[])
{
   int item;
   char buf[256];

   if (allegro_init() != 0)
      return 1;
   install_keyboard(); 
   install_mouse();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);

   /* We set up colors to match screen color depth (in case it changed) */
   for (item = 0; the_dialog[item].proc; item++) {
      the_dialog[item].fg = makecol(0, 0, 0);
      the_dialog[item].bg = makecol(255, 255, 255);
   }
   
   /* load the datafile */
   replace_filename(buf, argv[0], "example.dat", sizeof(buf));
   datafile = load_datafile(buf);
   if (!datafile) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s!\n", buf);
      return 1;
   }

   /* store a copy of the default font */
   original_font = font;

   do_dialog(the_dialog, -1);
   
   unload_datafile(datafile);

   return 0;
}

END_OF_MAIN()
