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
 *      Fixed point maths test program for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"



#define SIN    1
#define COS    2
#define TAN    3
#define ASIN   4
#define ACOS   5
#define ATAN   6
#define SQRT   7

#define ADD    1
#define SUB    2
#define MUL    3
#define DIV    4

char calc_str[80];

fixed calc_value;
int calc_operation;
int calc_clear_flag;

extern DIALOG calculator[];

#define CALC_STR  3



void redraw(DIALOG *d)
{
   object_message(calculator+CALC_STR, MSG_DRAW, 0);

   if (d)
      object_message(d, MSG_DRAW, 0);
}



void reset_calc(void)
{
   calc_value = 0;
   calc_operation = 0;
   calc_clear_flag = FALSE;
}



fixed get_calc_value(void)
{
   double d = atof(calc_str);
   return ftofix(d);
}



void set_calc_value(fixed v)
{
   int c, l;
   int has_dot;
   char b[80];
   double d = fixtof(v);

   sprintf(b, "%.8f", d);

   l = 0;
   has_dot = FALSE;

   for (c=0; b[c]; c++) {
      if ((b[c] >= '0') && (b[c] <= '9')) {
	 l++;
	 if (l > 8) {
	    b[c] = 0;
	    break;
	 }
      }
      else
	 if (b[c] == '.')
	    has_dot = TRUE;
   }

   if (has_dot) {
      for (c=c-1; c>=0; c--) {
	 if (b[c] == '0')
	    b[c] = 0;
	 else {
	    if (b[c] == '.') 
	       b[c] = 0;
	    break;
	 }
      }
   }

   if (errno)
      sprintf(calc_str, "-E- (%s)", b);
   else
      strcpy(calc_str, b);

   errno = 0;
}



int right_text_proc(int msg, DIALOG *d, int c)
{
   int len;

   if (msg == MSG_DRAW) {
      len = strlen(d->dp);
      textout_ex(screen, font, d->dp, d->x+d->w-len*8, d->y, d->fg, d->bg);
      rectfill(screen, d->x, d->y, d->x+d->w-len*8-1, d->y+d->h, d->bg); 
   }

   return D_O_K;
}



int input_proc(int msg, DIALOG *d, int c)
{
   int c1, c2;
   char *s = d->dp;
   int ret = d_button_proc(msg, d, c);

   if (ret == D_CLOSE) {
      if ((calc_clear_flag) && (s[0] != '+'))
	 strcpy(calc_str, "0");
      else if (strncmp(calc_str, "-E-", 3) == 0) {
	 redraw(d);
	 return D_O_K;
      }

      calc_clear_flag = FALSE;

      if (s[0] == '.') {
	 for (c1=0; calc_str[c1]; c1++) 
	    if (calc_str[c1] == '.')
	       break;

	 if (!calc_str[c1])
	    strcat(calc_str, ".");
      }
      else if (s[0] == '+') {
	 if (calc_str[0] == '-')
	    memmove(calc_str, calc_str+1, strlen(calc_str+1)+1);
	 else {
	    memmove(calc_str+1, calc_str, strlen(calc_str)+1);
	    calc_str[0] = '-';
	 }
      }
      else {
	 if (strcmp(calc_str, "0") == 0)
	    calc_str[0] = 0;
	 else if (strcmp(calc_str, "-0") == 0) 
	    strcpy(calc_str, "-");

	 c2 = 0;
	 for (c1=0; calc_str[c1]; c1++)
	    if ((calc_str[c1] >= '0') && (calc_str[c1] <= '9'))
	       c2++;

	 if (c2 < 8)
	    strcat(calc_str, s);
      }

      redraw(d);
      return D_O_K;
   }

   return ret;
}



int unary_operator(int msg, DIALOG *d, int c)
{
   fixed x;
   int ret = d_button_proc(msg, d, c);

   if (ret == D_CLOSE) {
      x = get_calc_value();

      switch (d->d1) {

	 case SIN:
	    x = fixsin(x);
	    break;

	 case COS:
	    x = fixcos(x);
	    break;

	 case TAN:
	    x = fixtan(x);
	    break;

	 case ASIN:
	    x = fixasin(x);
	    break;

	 case ACOS:
	    x = fixacos(x);
	    break;

	 case ATAN:
	    x = fixatan(x);
	    break;

	 case SQRT:
	    x = fixsqrt(x);
	    break;
      }

      set_calc_value(x);
      calc_clear_flag = TRUE;

      redraw(d);
      return D_O_K;
   }

   return ret;
}



int work_out(void)
{
   fixed x;

   x = get_calc_value();

   switch (calc_operation) {

      case ADD:
	 x = fixadd(calc_value, x);
	 break;

      case SUB:
	 x = fixsub(calc_value, x);
	 break;

      case MUL:
	 x = fixmul(calc_value, x);
	 break;

      case DIV:
	 x = fixdiv(calc_value, x);
	 break;
   }

   set_calc_value(x);
   reset_calc();
   calc_clear_flag = TRUE;

   redraw(NULL);
   return D_O_K;
}



int equals_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);

   if (ret == D_CLOSE) { 
      ret = work_out();
      redraw(d);
   }

   return ret;
}



int binary_operator(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);

   if (ret == D_CLOSE) {
      work_out();

      calc_value = get_calc_value();
      calc_operation = d->d1;
      calc_clear_flag = TRUE;

      redraw(d);
      return D_O_K;
   }

   return ret;
}



int clearer(void)
{
   reset_calc();
   strcpy(calc_str, "0");
   redraw(NULL);
   return D_O_K;
}



int clear_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);

   if (ret == D_CLOSE) {
      ret = clearer();
      redraw(d);
   }

   return ret;
}



DIALOG calculator[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)     (d2)           (dp)        (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,    193,  173,  255,  8,    0,    0,       0,       0,             NULL,       NULL, NULL  },
   { d_box_proc,        8,    10,   177,  21,   16,   255,  0,    0,       0,       0,             NULL,       NULL, NULL  },
   { d_box_proc,        10,   12,   173,  17,   255,  16,   0,    0,       0,       0,             NULL,       NULL, NULL  },
   { right_text_proc,   24,   16,   144,  8,    255,  16,   0,    0,       0,       0,             calc_str,   NULL, NULL  },

   { unary_operator,    16,   48,   49,   13,   16,   255,  0,    D_EXIT,  SIN,     0,             "sin",      NULL, NULL  },
   { unary_operator,    72,   48,   49,   13,   16,   255,  0,    D_EXIT,  COS,     0,             "cos",      NULL, NULL  },
   { unary_operator,    128,  48,   49,   13,   16,   255,  0,    D_EXIT,  TAN,     0,             "tan",      NULL, NULL  },

   { unary_operator,    16,   64,   49,   13,   16,   255,  0,    D_EXIT,  ASIN,    0,             "asin",     NULL, NULL  },
   { unary_operator,    72,   64,   49,   13,   16,   255,  0,    D_EXIT,  ACOS,    0,             "acos",     NULL, NULL  },
   { unary_operator,    128,  64,   49,   13,   16,   255,  0,    D_EXIT,  ATAN,    0,             "atan",     NULL, NULL  },

   { input_proc,        8,    88,   33,   13,   16,   255,  '7',  D_EXIT,  0,       0,             "7",        NULL, NULL  },
   { input_proc,        44,   88,   33,   13,   16,   255,  '8',  D_EXIT,  0,       0,             "8",        NULL, NULL  },
   { input_proc,        80,   88,   33,   13,   16,   255,  '9',  D_EXIT,  0,       0,             "9",        NULL, NULL  },
   { binary_operator,   116,  88,   33,   13,   16,   255,  '/',  D_EXIT,  DIV,     0,             "/",        NULL, NULL  },
   { clear_proc,        152,  88,   33,   13,   16,   255,  'c',  D_EXIT,  0,       0,             "C",        NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,    0,       KEY_DEL, KEY_BACKSPACE, clearer,    NULL, NULL  },

   { input_proc,        8,    108,  33,   13,   16,   255,  '4',  D_EXIT,  0,       0,             "4",        NULL, NULL  },
   { input_proc,        44,   108,  33,   13,   16,   255,  '5',  D_EXIT,  0,       0,             "5",        NULL, NULL  },
   { input_proc,        80,   108,  33,   13,   16,   255,  '6',  D_EXIT,  0,       0,             "6",        NULL, NULL  },
   { binary_operator,   116,  108,  33,   13,   16,   255,  '*',  D_EXIT,  MUL,     0,             "*",        NULL, NULL  },
   { d_button_proc,     152,  108,  33,   13,   16,   255,  27,   D_EXIT,  0,       0,             "off",      NULL, NULL  },

   { input_proc,        8,    128,  33,   13,   16,   255,  '1',  D_EXIT,  0,       0,             "1",        NULL, NULL  },
   { input_proc,        44,   128,  33,   13,   16,   255,  '2',  D_EXIT,  0,       0,             "2",        NULL, NULL  },
   { input_proc,        80,   128,  33,   13,   16,   255,  '3',  D_EXIT,  0,       0,             "3",        NULL, NULL  },
   { binary_operator,   116,  128,  33,   13,   16,   255,  '-',  D_EXIT,  SUB,     0,             "-",        NULL, NULL  },
   { unary_operator,    152,  128,  33,   13,   16,   255,  0,    D_EXIT,  SQRT,    0,             "sqr",      NULL, NULL  },

   { input_proc,        8,    148,  33,   13,   16,   255,  '0',  D_EXIT,  0,       0,             "0",        NULL, NULL  },
   { input_proc,        44,   148,  33,   13,   16,   255,  '.',  D_EXIT,  0,       0,             ".",        NULL, NULL  },
   { input_proc,        80,   148,  33,   13,   16,   255,  0,    D_EXIT,  0,       0,             "+/-",      NULL, NULL  },
   { binary_operator,   116,  148,  33,   13,   16,   255,  '+',  D_EXIT,  ADD,     0,             "+",        NULL, NULL  },
   { equals_proc,       152,  148,  33,   13,   16,   255,  '=',  D_EXIT,  0,       0,             "=",        NULL, NULL  },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    13,   0,       0,       0,             work_out,   NULL, NULL  },

   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,       0,             NULL,       NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,       0,       0,             NULL,       NULL, NULL  }
};



int main(void)
{
   int i;
   
   if (allegro_init() != 0)
      return 1;
   install_mouse();
   install_keyboard();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Error setting graphics mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   textout_ex(screen, font, "Angles are binary, 0-255", 0, 0, palette_color[8], 0);
   reset_calc();
   strcpy(calc_str, "0");
   errno = 0;

   /* we set up colors to match screen color depth (in case it changed) */
   for (i = 0; calculator[i].proc; i++) {
      calculator[i].fg = palette_color[calculator[i].fg];
      calculator[i].bg = palette_color[calculator[i].bg];
   }

   centre_dialog(calculator);
   do_dialog(calculator, -1);

   return 0;
}

END_OF_MAIN()
