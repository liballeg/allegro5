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
 *      A really silly little Worms grabber plugin.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>

#include "allegro.h"
#include "../datedit.h"



/* what a pointless way to spend my Sunday afternoon :-) */
static int worms(void)
{
   #define MAX_SIZE     65536

   int *xpos = malloc(sizeof(int)*MAX_SIZE);
   int *ypos = malloc(sizeof(int)*MAX_SIZE);
   int head, tail, dir, dead, quit, score, counter;
   int tx, ty, x, y, i, c1, c2, c3;

   again:

   head = 0;
   tail = 0;
   dir = 0;
   dead = FALSE;
   quit = FALSE;
   score = 0;
   counter = 0;
   tx = -100;
   ty = -100;

   show_mouse(NULL);

   if (bitmap_color_depth(screen) <= 8) {
      PALETTE pal;

      get_palette(pal);

      pal[0].r = 0;
      pal[0].g = 0;
      pal[0].b = 0;

      pal[1].r = 31;
      pal[1].g = 31;
      pal[1].b = 31;

      pal[254].r = 31;
      pal[254].g = 31;
      pal[254].b = 31;

      pal[255].r = 63;
      pal[255].g = 63;
      pal[255].b = 63;

      set_palette(pal);

      c1 = 0;
      c2 = 1;
      c3 = 255;
   }
   else {
      c1 = gui_fg_color;
      c2 = gui_mg_color;
      c3 = gui_bg_color;
   }

   clear_to_color(screen, c1);

   xor_mode(TRUE);

   xpos[0] = SCREEN_W/2;
   ypos[0] = SCREEN_H/2;

   putpixel(screen, xpos[0], ypos[0], c3);

   while ((!dead) && (!quit)) {
      x = xpos[head];
      y = ypos[head];

      switch (dir) {
	 case 0: x++; break;
	 case 1: y++; break;
	 case 2: x--; break;
	 case 3: y--; break;
      }

      if (x >= SCREEN_W)
	 x -= SCREEN_W;
      else if (x < 0)
	 x += SCREEN_W;

      if (y >= SCREEN_H)
	 y -= SCREEN_H;
      else if (y < 0)
	 y += SCREEN_H;

      head++;
      if (head >= MAX_SIZE)
	 head = 0;

      xpos[head] = x;
      ypos[head] = y;

      counter++;

      if (counter & 15) {
	 putpixel(screen, xpos[tail], ypos[tail], c3);

	 tail++;
	 if (tail >= MAX_SIZE)
	    tail = 0;
      }

      i = tail;

      while (i != head) {
	 if ((x == xpos[i]) && (y == ypos[i])) {
	    dead = TRUE;
	    break;
	 }

	i++;
	 if (i >= MAX_SIZE)
	    i = 0;
      }

      if (!(counter % (1+(score+2)/3)))
	 vsync();

      putpixel(screen, x, y, c3);

      if ((tx < 0) || (ty < 0)) {
	 do {
	    tx = 16+AL_RAND()%(SCREEN_W-32);
	    ty = 16+AL_RAND()%(SCREEN_H-32);
	 } while ((ABS(x-tx)+ABS(y-ty)) < 64);

	 circle(screen, tx, ty, 8, c2);
	 circle(screen, tx, ty, 5, c2);
	 circle(screen, tx, ty, 2, c2);
      }

      if ((ABS(x-tx)+ABS(y-ty)) < 9) {
	 circle(screen, tx, ty, 8, c2);
	 circle(screen, tx, ty, 5, c2);
	 circle(screen, tx, ty, 2, c2);

	 tx = -100;
	 ty = -100;

	 score++;
      }

      textprintf_ex(screen, font, 0, 0, c2, c1, "Score: %d", score);

      if (keypressed()) {
	 switch (readkey()>>8) {

	    case KEY_RIGHT:
	       dir = 0;
	       break;

	    case KEY_DOWN:
	       dir = 1;
	       break;

	    case KEY_LEFT:
	       dir = 2;
	       break;

	    case KEY_UP:
	       dir = 3;
	       break;

	    case KEY_ESC:
	       quit = TRUE;
	       break;
	 }
      }
   }

   xor_mode(FALSE);
   clear_to_color(screen, gui_fg_color);
   set_palette(datedit_current_palette);

   do {
      poll_keyboard();
   } while ((key[KEY_ESC]) || (key[KEY_RIGHT]) || (key[KEY_LEFT]) || (key[KEY_UP]) || (key[KEY_DOWN]));

   clear_keybuf();
   show_mouse(screen);

   if (dead) {
      char buf[64];
      sprintf(buf, "Score: %d", score);
      if (alert(buf, NULL, NULL, "Play", "Quit", 'p', 'q') == 1)
	 goto again;
   }

   free(xpos);
   free(ypos);

   return D_REDRAW;
}



/* hook ourselves into the grabber menu system */
static MENU worms_menu =
{
   "Worms",
   worms,
   NULL,
   0,
   NULL
};



DATEDIT_MENU_INFO datworms_menu =
{
   &worms_menu,
   NULL,
   DATEDIT_MENU_HELP,
   0,
   NULL
};

