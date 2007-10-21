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
 *      Main test program for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "allegro.h"



int gfx_card = GFX_AUTODETECT;
int gfx_w = 640;
int gfx_h = 480;
int gfx_bpp = 8;
int gfx_rate = 0;

int mode = DRAW_MODE_SOLID;
char mode_string[80];

#define CHECK_TRANS_BLENDER()                \
   if (bitmap_color_depth(screen) > 8)       \
      set_trans_blender(0, 0, 0, 0x40);

#define TIME_SPEED   2

BITMAP *global_sprite = NULL;
RLE_SPRITE *global_rle_sprite = NULL;
COMPILED_SPRITE *global_compiled_sprite = NULL;

BITMAP *realscreen = NULL;

PALETTE mypal;

#define NUM_PATTERNS    8
BITMAP *pattern[NUM_PATTERNS];

int cpu_has_capabilities = 0;
int type3d = POLYTYPE_FLAT;

char allegro_version_str[120];
char internal_version_str[80];

char gfx_specs[80];
char gfx_specs2[80];
char gfx_specs3[80];
char mouse_specs[80];
char cpu_specs[120];

char buf[160];

int xoff, yoff;

long tm = 0;          /* counter, incremented once a second */
volatile int _tm = 0; /* volatile so we can try to sync to the timer */

long ct;

#define SHOW_TIME_MACRO()               \
      if (ct >= 0) {                    \
	 if (tm >= TIME_SPEED) {        \
	    if (profile)                \
	       return;                  \
	    show_time(ct, screen, 16);  \
	    /* sync to timer */         \
	    _tm = 0;                    \
	    while (_tm < 2) ;           \
	    ct = 0;                     \
	    tm = 0;                     \
	    _tm = 0;                    \
	 }                              \
	 else                           \
	    ct++;                       \
      }


int profile = FALSE;

COLOR_MAP *trans_map = NULL;
COLOR_MAP *light_map = NULL;



void tm_tick(void)
{
   if (++_tm >= 100) {
      _tm = 0;
      tm++;

      if (realscreen)
	 blit(screen, realscreen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }
}

END_OF_FUNCTION(tm_tick)



void show_time(long t, BITMAP *bmp, int y)
{
   int s, x1, y1, x2, y2;

   get_clip_rect(bmp, &x1, &y1, &x2, &y2);
   s = get_clip_state(bmp);

   sprintf(buf, " %ld per second ", t / TIME_SPEED);
   set_clip_rect(bmp, 0, 0, SCREEN_W-1, SCREEN_H-1);
   set_clip_state(bmp, TRUE);
   textout_centre_ex(bmp, font, buf, SCREEN_W/2, y, palette_color[15], palette_color[0]);

   set_clip_rect(bmp, x1, y1, x2, y2);
   set_clip_state(bmp, s);
}



void message(char *s)
{
   textout_centre_ex(screen, font, s, SCREEN_W/2, 6, palette_color[15], palette_color[0]);

   if (!profile)
      textout_centre_ex(screen, font, "Press a key or mouse button", SCREEN_W/2, SCREEN_H-10, palette_color[15], palette_color[0]);
}



int next(void)
{
   if (keypressed()) {
      clear_keybuf();
      return TRUE;
   }

   poll_mouse();

   if (mouse_b) {
      do {
	 poll_mouse();
      } while (mouse_b);
      return TRUE;
   }

   return FALSE;
}



BITMAP *make_sprite(void)
{
   BITMAP *b;

   solid_mode();
   b = create_bitmap(32, 32);
   clear_to_color(b, bitmap_mask_color(b));
   circlefill(b, 16, 16, 8, palette_color[2]);
   circle(b, 16, 16, 8, palette_color[1]);
   line(b, 0, 0, 31, 31, palette_color[3]);
   line(b, 31, 0, 0, 31, palette_color[3]);
   textout_ex(b, font, "Test", 1, 12, palette_color[15], -1);
   return b;
}



int check_tables(void)
{
   if (bitmap_color_depth(screen) > 8) {
      set_trans_blender(0, 0, 0, 128);
   }
   else if ((!rgb_map) || (!trans_map) || (!light_map)) {
      if (!rgb_map) {
	 rgb_map = malloc(sizeof(RGB_MAP));
	 create_rgb_table(rgb_map, mypal, NULL);
      }

      if (!trans_map) {
	 trans_map = malloc(sizeof(COLOR_MAP));
	 create_trans_table(trans_map, mypal, 96, 96, 96, NULL);
      }

      if (!light_map) {
	 light_map = malloc(sizeof(COLOR_MAP));
	 create_light_table(light_map, mypal, 0, 0, 0, NULL);
      }

      color_map = trans_map;

      return D_REDRAW;
   }

   return D_O_K;
}



void make_patterns(void)
{
   int c;

   pattern[0] = create_bitmap(2, 2);
   clear_to_color(pattern[0], bitmap_mask_color(pattern[0]));
   putpixel(pattern[0], 0, 0, palette_color[255]);

   pattern[1] = create_bitmap(2, 2);
   clear_to_color(pattern[1], bitmap_mask_color(pattern[1]));
   putpixel(pattern[1], 0, 0, palette_color[255]);
   putpixel(pattern[1], 1, 1, palette_color[255]);

   pattern[2] = create_bitmap(4, 4);
   clear_to_color(pattern[2], bitmap_mask_color(pattern[2]));
   vline(pattern[2], 0, 0, 4, palette_color[255]);
   hline(pattern[2], 0, 0, 4, palette_color[255]);

   pattern[3] = create_bitmap(4, 4);
   clear_to_color(pattern[3], bitmap_mask_color(pattern[3]));
   line(pattern[3], 0, 3, 3, 0, palette_color[255]);

   pattern[4] = create_bitmap(8, 8);
   clear_to_color(pattern[4], bitmap_mask_color(pattern[4]));
   for (c=0; c<16; c+=2)
      circle(pattern[4], 4, 4, c, palette_color[c]);

   pattern[5] = create_bitmap(8, 8);
   clear_to_color(pattern[5], bitmap_mask_color(pattern[5]));
   for (c=0; c<8; c++)
      hline(pattern[5], 0, c, 8, palette_color[c]);

   pattern[6] = create_bitmap(8, 8);
   clear_to_color(pattern[6], bitmap_mask_color(pattern[6]));
   circle(pattern[6], 0, 4, 3, palette_color[2]);
   circle(pattern[6], 8, 4, 3, palette_color[2]);
   circle(pattern[6], 4, 0, 3, palette_color[1]);
   circle(pattern[6], 4, 8, 3, palette_color[1]);

   pattern[7] = create_bitmap(64, 8);
   clear_to_color(pattern[7], bitmap_mask_color(pattern[7]));
   textout_ex(pattern[7], font, "PATTERN!", 0, 0, palette_color[255], bitmap_mask_color(pattern[7]));
}



void getpix_demo(void)
{
   int c, ox, oy;

   scare_mouse();
   acquire_screen();

   clear_to_color(screen, palette_color[0]); 
   message("getpixel test");

   for (c=0; c<16; c++)
      rectfill(screen, xoff+100+((c&3)<<5), yoff+50+((c>>2)<<5),
		       xoff+120+((c&3)<<5), yoff+70+((c>>2)<<5), palette_color[c]);

   release_screen();
   unscare_mouse();

   ox = -1;
   oy = -1;

   while (!next()) {
      rest(1);
      poll_mouse();

      if ((mouse_x != ox) || (mouse_y != oy)) {
	 ox = mouse_x;
	 oy = mouse_y;

	 scare_mouse();
	 acquire_screen();

	 c = getpixel(screen, ox-2, oy-2);
	 sprintf(buf, "     %X     ", c);
	 textout_centre_ex(screen, font, buf, SCREEN_W/2, yoff+24, palette_color[15], palette_color[0]);

	 release_screen();
	 unscare_mouse();
      }
   }
}



void putpix_test(int xpos, int ypos)
{
   int c, x, y;

   for (c=0; c<16; c++)
      for (x=0; x<16; x+=2)
	 for (y=0; y<16; y+=2)
	    putpixel(screen, xpos+((c&3)<<4)+x, ypos+((c>>2)<<4)+y, palette_color[c]);
}



void hline_test(int xpos, int ypos)
{
   int c;

   for (c=0; c<16; c++)
      hline(screen, xpos+48-c*3, ypos+c*3, xpos+48+c*3, palette_color[c]);
}



void vline_test(int xpos, int ypos)
{
   int c;

   for (c=0; c<16; c++)
      vline(screen, xpos+c*4, ypos+36-c*3, ypos+36+c*3, palette_color[c]); 
}



void line_test(int xpos, int ypos)
{
   int c;

   for (c=0; c<16; c++) {
      line(screen, xpos+32, ypos+32, xpos+32+((c-8)<<2), ypos, palette_color[c%15+1]);
      line(screen, xpos+32, ypos+32, xpos+32-((c-8)<<2), ypos+64, palette_color[c%15+1]);
      line(screen, xpos+32, ypos+32, xpos, ypos+32-((c-8)<<2), palette_color[c%15+1]);
      line(screen, xpos+32, ypos+32, xpos+64, ypos+32+((c-8)<<2), palette_color[c%15+1]);
   }
}



void rectfill_test(int xpos, int ypos)
{
   int c;

   for (c=0; c<16; c++)
      rectfill(screen, xpos+((c&3)*17), ypos+((c>>2)*17),
		       xpos+15+((c&3)*17), ypos+15+((c>>2)*17), palette_color[c]);
}



void triangle_test(int xpos, int ypos)
{
   int c;

   for (c=0; c<16; c++)
      triangle(screen, xpos+22+((c&3)<<4), ypos+15+((c>>2)<<4),
		       xpos+13+((c&3)<<3), ypos+7+((c>>2)<<4),
		       xpos+7+((c&3)<<4), ypos+27+((c>>2)<<3), palette_color[c]);
}



void circle_test(int xpos, int ypos)
{
   int c;

   for (c=0; c<16; c++)
      circle(screen, xpos+32, ypos+32, c*2, palette_color[c%15+1]);
}



void circlefill_test(int xpos, int ypos)
{
   int c;

   for (c=15; c>=0; c--)
      circlefill(screen, xpos+8+((c&3)<<4), ypos+8+((c>>2)<<4), c, palette_color[c%15+1]);
}



void ellipse_test(int xpos, int ypos)
{
   int c;

   for (c=15; c>=0; c--)
      ellipse(screen, xpos+8+((c&3)<<4), ypos+8+((c>>2)<<4), (16-c)*2, (c+1)*2, palette_color[c%15+1]);
}



void ellipsefill_test(int xpos, int ypos)
{
   int c;

   for (c=15; c>=0; c--)
      ellipsefill(screen, xpos+8+((c&3)<<4), ypos+8+((c>>2)<<4), (16-c)*2, (c+1)*2, palette_color[c%15+1]);
}



void arc_test(int xpos, int ypos)
{
   int c;

   for (c=0; c<16; c++)
      arc(screen, xpos+32, ypos+32, itofix(c*12), itofix(64+c*16), c*2, palette_color[c%15+1]);
}



void textout_test(int xpos, int ypos)
{
   textout_ex(screen, font,"This is a", xpos-8, ypos, palette_color[1], palette_color[0]);
   textout_ex(screen, font,"test of the", xpos+3, ypos+10, palette_color[1], palette_color[0]);
   textout_ex(screen, font,"textout", xpos+14, ypos+20, palette_color[1], palette_color[0]);
   textout_ex(screen, font,"function.", xpos+25, ypos+30, palette_color[1], palette_color[0]);

   textout_ex(screen, font,"solid background", xpos, ypos+48, palette_color[2], palette_color[0]);
   textout_ex(screen, font,"solid background", xpos+4, ypos+52, palette_color[4], palette_color[0]);

   textout_ex(screen, font,"transparent background", xpos, ypos+68, palette_color[2], -1);
   textout_ex(screen, font,"transparent background", xpos+4, ypos+72, palette_color[4], -1);
}



void sprite_test(int xpos, int ypos)
{
   int x,y;

   for (y=0;y<82;y++)
      for (x=0;x<82;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   for (x=6; x<64; x+=global_sprite->w+6)
      for (y=6; y<64; y+=global_sprite->w+6)
	 draw_sprite(screen, global_sprite, xpos+x, ypos+y);
}



void xlu_sprite_test(int xpos, int ypos)
{
   int x,y;

   solid_mode();

   for (y=0;y<82;y++)
      for (x=0;x<82;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   for (x=6; x<64; x+=global_sprite->w+6) {
      for (y=6; y<64; y+=global_sprite->w+6) {
	 set_trans_blender(0, 0, 0, x+y*3);
	 draw_trans_sprite(screen, global_sprite, xpos+x, ypos+y);
      }
   }
}



void lit_sprite_test(int xpos, int ypos)
{
   int x,y;

   solid_mode();

   for (y=0;y<82;y++)
      for (x=0;x<82;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   for (x=6; x<64; x+=global_sprite->w+6) {
      for (y=6; y<64; y+=global_sprite->w+6) {
	 set_trans_blender(x*4, (x+y)*2, y*4, 0);
	 draw_lit_sprite(screen, global_sprite, xpos+x, ypos+y, ((x*2+y)*5)&0xFF);
      }
   }
}



void rle_xlu_sprite_test(int xpos, int ypos)
{
   int x,y;

   solid_mode();

   for (y=0;y<82;y++)
      for (x=0;x<82;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   for (x=6; x<64; x+=global_sprite->w+6) {
      for (y=6; y<64; y+=global_sprite->w+6) {
	 set_trans_blender(0, 0, 0, x+y*3);
	 draw_trans_rle_sprite(screen, global_rle_sprite, xpos+x, ypos+y);
      }
   }
}



void rle_lit_sprite_test(int xpos, int ypos)
{
   int x,y;

   solid_mode();

   for (y=0;y<82;y++)
      for (x=0;x<82;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   for (x=6; x<64; x+=global_sprite->w+6) {
      for (y=6; y<64; y+=global_sprite->w+6) {
	 set_trans_blender(x*4, (x+y)*2, y*4, 0);
	 draw_lit_rle_sprite(screen, global_rle_sprite, xpos+x, ypos+y, ((x*2+y)*5)&0xFF);
      }
   }
}



void rle_sprite_test(int xpos, int ypos)
{
   int x,y;

   for (y=0;y<82;y++)
      for (x=0;x<82;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   for (x=6; x<64; x+=global_sprite->w+6)
      for (y=6; y<64; y+=global_sprite->w+6)
	 draw_rle_sprite(screen, global_rle_sprite, xpos+x, ypos+y);
}



void compiled_sprite_test(int xpos, int ypos)
{
   int x,y;

   for (y=0;y<82;y++)
      for (x=0;x<82;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   for (x=6; x<64; x+=global_sprite->w+6)
      for (y=6; y<64; y+=global_sprite->w+6)
	 draw_compiled_sprite(screen, global_compiled_sprite, xpos+x, ypos+y);
}



void flipped_sprite_test(int xpos, int ypos)
{
   int x, y;

   for (y=0;y<88;y++)
      for (x=0;x<88;x+=2)
	 putpixel(screen, xpos+x+(y&1), ypos+y, palette_color[8]);

   draw_sprite(screen, global_sprite, xpos+8, ypos+8);
   draw_sprite_h_flip(screen, global_sprite, xpos+48, ypos+8);
   draw_sprite_v_flip(screen, global_sprite, xpos+8, ypos+48);
   draw_sprite_vh_flip(screen, global_sprite, xpos+48, ypos+48);
}



void putpix_demo(void)
{
   int c = 0;
   int x, y;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 255) + 32;
      y = (AL_RAND() & 127) + 40;
      putpixel(screen, xoff+x, yoff+y, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void hline_demo(void)
{
   int c = 0;
   int x1, x2, y;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x1 = (AL_RAND() & 255) + 32;
      x2 = (AL_RAND() & 255) + 32;
      y = (AL_RAND() & 127) + 40;
      hline(screen, xoff+x1, yoff+y, xoff+x2, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void vline_demo(void)
{
   int c = 0;
   int x, y1, y2;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 255) + 32;
      y1 = (AL_RAND() & 127) + 40;
      y2 = (AL_RAND() & 127) + 40;
      vline(screen, xoff+x, yoff+y1, yoff+y2, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void line_demo(void)
{
   int c = 0;
   int x1, y1, x2, y2;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x1 = (AL_RAND() & 255) + 32;
      x2 = (AL_RAND() & 255) + 32;
      y1 = (AL_RAND() & 127) + 40;
      y2 = (AL_RAND() & 127) + 40;
      line(screen, xoff+x1, yoff+y1, xoff+x2, yoff+y2, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void rectfill_demo(void)
{
   int c = 0;
   int x1, y1, x2, y2;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x1 = (AL_RAND() & 255) + 32;
      y1 = (AL_RAND() & 127) + 40;
      x2 = (AL_RAND() & 255) + 32;
      y2 = (AL_RAND() & 127) + 40;
      rectfill(screen, xoff+x1, yoff+y1, xoff+x2, yoff+y2, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void triangle_demo(void)
{
   int c = 0;
   int x1, y1, x2, y2, x3, y3;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x1 = (AL_RAND() & 255) + 32;
      x2 = (AL_RAND() & 255) + 32;
      x3 = (AL_RAND() & 255) + 32;
      y1 = (AL_RAND() & 127) + 40;
      y2 = (AL_RAND() & 127) + 40;
      y3 = (AL_RAND() & 127) + 40;
      triangle(screen, xoff+x1, yoff+y1, xoff+x2, yoff+y2, xoff+x3, yoff+y3, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void triangle3d_demo(void)
{
   V3D v1, v2, v3;
   int x0 = xoff+32;
   int y0 = yoff+40;

   v1.u = 0; 
   v1.v = 0; 

   v2.u = itofix(32);
   v2.v = 0;

   v3.u = 0;
   v3.v = itofix(32);

   tm = _tm = 0;
   ct = 0;

   while (!next()) {
      v1.x = itofix((AL_RAND() & 255) + x0);
      v2.x = itofix((AL_RAND() & 255) + x0);
      v3.x = itofix((AL_RAND() & 255) + x0);
      v1.y = itofix((AL_RAND() & 127) + y0);
      v2.y = itofix((AL_RAND() & 127) + y0);
      v3.y = itofix((AL_RAND() & 127) + y0);
      v1.z = itofix((AL_RAND() & 127) + 400);
      v2.z = itofix((AL_RAND() & 127) + 400);
      v3.z = itofix((AL_RAND() & 127) + 400);

      if ((type3d == POLYTYPE_ATEX_LIT) || (type3d == POLYTYPE_PTEX_LIT) ||
	  (type3d == POLYTYPE_ATEX_MASK_LIT) || (type3d == POLYTYPE_PTEX_MASK_LIT)) {
	 v1.c = AL_RAND() & 255;
	 v2.c = AL_RAND() & 255;
	 v3.c = AL_RAND() & 255;
      } 
      else {
	 v1.c = palette_color[AL_RAND() & 255];
	 v2.c = palette_color[AL_RAND() & 255];
	 v3.c = palette_color[AL_RAND() & 255];
      }

      triangle3d(screen, type3d, pattern[AL_RAND()%NUM_PATTERNS], &v1, &v2, &v3);

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void circle_demo(void)
{
   int c = 0;
   int x, y, r;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 127) + 92;
      y = (AL_RAND() & 63) + 76;
      r = (AL_RAND() & 31) + 16;
      circle(screen, xoff+x, yoff+y, r, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void circlefill_demo(void)
{
   int c = 0;
   int x, y, r;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 127) + 92;
      y = (AL_RAND() & 63) + 76;
      r = (AL_RAND() & 31) + 16;
      circlefill(screen, xoff+x, yoff+y, r, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void ellipse_demo(void)
{
   int c = 0;
   int x, y, rx, ry;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 127) + 92;
      y = (AL_RAND() & 63) + 76;
      rx = (AL_RAND() & 31) + 16;
      ry = (AL_RAND() & 31) + 16;
      ellipse(screen, xoff+x, yoff+y, rx, ry, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void ellipsefill_demo(void)
{
   int c = 0;
   int x, y, rx, ry;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 127) + 92;
      y = (AL_RAND() & 63) + 76;
      rx = (AL_RAND() & 31) + 16;
      ry = (AL_RAND() & 31) + 16;
      ellipsefill(screen, xoff+x, yoff+y, rx, ry, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void arc_demo(void)
{
   int c = 0;
   int x, y, r;
   fixed a1, a2;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 127) + 92;
      y = (AL_RAND() & 63) + 76;
      r = (AL_RAND() & 31) + 16;
      a1 = (AL_RAND() & 0xFF) << 16;
      a2 = (AL_RAND() & 0xFF) << 16;
      arc(screen, xoff+x, yoff+y, a1, a2, r, palette_color[c]);

      if (mode >= DRAW_MODE_COPY_PATTERN)
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void textout_demo(void)
{
   int c = 0;
   int x, y;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & 127) + 40;
      y = (AL_RAND() & 127) + 40;
      textout_ex(screen, font, "textout test", xoff+x, yoff+y, palette_color[c], palette_color[0]);

      if (++c >= 16)
	 c = 0;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void sprite_demo(void)
{
   int x, y;
   int xand = (SCREEN_W >= 320) ? 255 : 127;
   int xadd = (SCREEN_W >= 320) ? 16 : 80;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & xand) + xadd;
      y = (AL_RAND() & 127) + 30;
      draw_sprite(screen, global_sprite, xoff+x, yoff+y);

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void xlu_sprite_demo(void)
{
   int x, y;
   int xand = (SCREEN_W >= 320) ? 255 : 127;
   int xadd = (SCREEN_W >= 320) ? 16 : 80;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & xand) + xadd;
      y = (AL_RAND() & 127) + 30;

      if (bitmap_color_depth(screen) > 8)
	 set_trans_blender(0, 0, 0, AL_RAND()&0x7F);

      draw_trans_sprite(screen, global_sprite, xoff+x, yoff+y);

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void lit_sprite_demo(void)
{
   int c = 1;
   int x, y;
   int xand = (SCREEN_W >= 320) ? 255 : 127;
   int xadd = (SCREEN_W >= 320) ? 16 : 80;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & xand) + xadd;
      y = (AL_RAND() & 127) + 30;

      if (bitmap_color_depth(screen) > 8)
	 set_trans_blender(AL_RAND()&0xFF, AL_RAND()&0xFF, AL_RAND()&0xFF, 0);

      draw_lit_sprite(screen, global_sprite, xoff+x, yoff+y, c);

      c = (c+13) & 0xFF;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void rle_xlu_sprite_demo(void)
{
   int x, y;
   int xand = (SCREEN_W >= 320) ? 255 : 127;
   int xadd = (SCREEN_W >= 320) ? 16 : 80;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & xand) + xadd;
      y = (AL_RAND() & 127) + 30;

      if (bitmap_color_depth(screen) > 8)
	 set_trans_blender(0, 0, 0, AL_RAND()&0x7F);

      draw_trans_rle_sprite(screen, global_rle_sprite, xoff+x, yoff+y);

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void rle_lit_sprite_demo(void)
{
   int c = 1;
   int x, y;
   int xand = (SCREEN_W >= 320) ? 255 : 127;
   int xadd = (SCREEN_W >= 320) ? 16 : 80;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & xand) + xadd;
      y = (AL_RAND() & 127) + 30;

      if (bitmap_color_depth(screen) > 8)
	 set_trans_blender(AL_RAND()&0xFF, AL_RAND()&0xFF, AL_RAND()&0xFF, 0);

      draw_lit_rle_sprite(screen, global_rle_sprite, xoff+x, yoff+y, c);

      c = (c+13) & 0xFF;

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void rle_sprite_demo(void)
{
   int x, y;
   int xand = (SCREEN_W >= 320) ? 255 : 127;
   int xadd = (SCREEN_W >= 320) ? 16 : 80;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & xand) + xadd;
      y = (AL_RAND() & 127) + 30;
      draw_rle_sprite(screen, global_rle_sprite, xoff+x, yoff+y);

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



void compiled_sprite_demo(void)
{
   int x, y;
   int xand = (SCREEN_W >= 320) ? 255 : 127;
   int xadd = (SCREEN_W >= 320) ? 16 : 80;

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {

      x = (AL_RAND() & xand) + xadd;
      y = (AL_RAND() & 127) + 30;
      draw_compiled_sprite(screen, global_compiled_sprite, xoff+x, yoff+y);

      SHOW_TIME_MACRO();
   }

   ct = -1;
}



int blit_from_screen = FALSE;
int blit_align = FALSE;
int blit_mask = FALSE;


void blit_demo(void)
{
   int x, y;
   int sx, sy;
   BITMAP *b;

   solid_mode();

   b = create_bitmap(64, 32);
   if (!b) {
      clear_to_color(screen, palette_color[0]);
      textout_ex(screen, font, "Out of memory!", 50, 50, palette_color[15], palette_color[0]);
      destroy_bitmap(b);
      while (!next())
	 ;
      return;
   }

   clear_to_color(b, (blit_mask ? bitmap_mask_color(b) : palette_color[0]));
   circlefill(b, 32, 16, 16, palette_color[4]);
   circlefill(b, 32, 16, 10, palette_color[2]);
   circlefill(b, 32, 16, 6, palette_color[1]);
   line(b, 0, 0, 63, 31, palette_color[3]);
   line(b, 0, 31, 63, 0, palette_color[3]);
   rect(b, 8, 4, 56, 28, palette_color[3]);
   rect(b, 0, 0, 63, 31, palette_color[15]);

   tm = 0; _tm = 0;
   ct = 0;

   sx = ((SCREEN_W-64) / 2) & 0xFFFC;
   sy = yoff + 32;

   if (blit_from_screen) 
      blit(b, screen, 0, 0, sx, sy, 64, 32);

   while (!next()) {

      x = (AL_RAND() & 127) + 60;
      y = (AL_RAND() & 63) + 50;

      if (blit_align)
	 x &= 0xFFFC;

      if (blit_from_screen) {
	 if (blit_mask)
	    masked_blit(screen, screen, sx, sy, xoff+x, yoff+y+24, 64, 32);
	 else
	    blit(screen, screen, sx, sy, xoff+x, yoff+y+24, 64, 32);
      }
      else {
	 if (blit_mask)
	    masked_blit(b, screen, 0, 0, xoff+x, yoff+y, 64, 32);
	 else
	    blit(b, screen, 0, 0, xoff+x, yoff+y, 64, 32);
      }

      SHOW_TIME_MACRO();
   }

   destroy_bitmap(b);

   ct = -1;
}



void misc(void)
{
   BITMAP *p;
   volatile fixed x, y, z;
   volatile float fx, fy, fz;

   clear_to_color(screen, palette_color[0]);
   textout_ex(screen,font,"Timing some other routines...", xoff+44, 6, palette_color[15], palette_color[0]);

   p = create_bitmap(320, 200);
   if (!p)
      textout_ex(screen,font,"Out of memory!", 16, 50, palette_color[15], palette_color[0]);
   else {
      tm = 0; _tm = 0;
      ct = 0;
      do {
	 clear_bitmap(p);
	 ct++;
	 if (next())
	    return;
      } while (tm < TIME_SPEED);
      destroy_bitmap(p);
      sprintf(buf,"clear_bitmap(320x200): %ld per second", ct/TIME_SPEED);
      textout_ex(screen, font, buf, xoff+16, yoff+15, palette_color[15], palette_color[0]);
   }

   x = y = z = 0;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      z += fixmul(x,y);
      x += 1317;
      y += 7143;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixmul(): %ld per second", ct/TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+25, palette_color[15], palette_color[0]);

   fx = fy = fz = 0;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      fz += fx * fy;
      fx += 1317;
      fy += 7143;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "float *:  %ld per second", ct/TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+35, palette_color[15], palette_color[0]);

   x = y = 0;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      z += fixdiv(x,y);
      x += 1317;
      y += 7143;
      if (y==0)
	 y++;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixdiv(): %ld per second", ct/TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+45, palette_color[15], palette_color[0]);

   fx = fy = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      fz += fx / fy;
      fx += 1317;
      fy += 7143;
      if (fy==0)
	 fy++;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "float /:  %ld per second", ct/TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+55, palette_color[15], palette_color[0]);

   x = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      y += fixsqrt(x);
      x += 7361;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixsqrt():    %ld per second", ct/TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+65, palette_color[15], palette_color[0]);

   fx = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      fy += sqrt(fx);
      fx += 7361;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "libc sqrtf(): %ld per second", ct/TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+75, palette_color[15], palette_color[0]);

   x = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      y += fixsin(x);
      x += 4283;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixsin():    %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+85, palette_color[15], palette_color[0]);

   fx = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      fy += sin(fx);
      fx += 4283;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "libc sin(): %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+95, palette_color[15], palette_color[0]);

   x = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      y += fixcos(x);
      x += 4283;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixcos(): %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+105, palette_color[15], palette_color[0]);

   x = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      y += fixtan(x);
      x += 8372;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixtan():    %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+115, palette_color[15], palette_color[0]);

   fx = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      fy += tan(fx);
      fx += 8372;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "libc tan(): %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+125, palette_color[15], palette_color[0]);

   x = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      y += fixasin(x);
      x += 5621;
      x &= 0xffff;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixasin(): %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+135, palette_color[15], palette_color[0]);

   x = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      y += fixacos(x);
      x += 5621;
      x &= 0xffff;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf,"fixacos(): %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+145, palette_color[15], palette_color[0]);

   x = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      y += fixatan(x);
      x += 7358;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixatan():    %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+155, palette_color[15], palette_color[0]);

   fx = 1;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      fy += atan(fx);
      fx += 7358;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "libc atan(): %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+165, palette_color[15], palette_color[0]);

   x = 1, y = 2;
   tm = 0; _tm = 0;
   ct = 0;

   do {
      z += fixatan2(x, y);
      x += 5621;
      y += 7335;
      ct++;
      if (next())
	 return;
   } while (tm < TIME_SPEED);

   sprintf(buf, "fixatan2(): %ld per second", ct / TIME_SPEED);
   textout_ex(screen, font, buf, xoff+16, yoff+175, palette_color[15], palette_color[0]);

   textout_ex(screen, font, "Press a key or mouse button", xoff+52, SCREEN_H-10, palette_color[15], palette_color[0]);

   while (!next())
      ;
}



void rainbow(void)
{
   char buf[80];
   int x, y;
   int r, g, b;
   float h, s, v, c;

   acquire_screen();

   clear_to_color(screen, palette_color[0]);
   sprintf(buf, "%d bit color...", bitmap_color_depth(screen));
   textout_centre_ex(screen, font, buf, SCREEN_W/2, 6, palette_color[15], palette_color[0]);

   for (h=0; h<360; h+=0.25) {
      for (c=0; c<1; c+=0.005) {
	 s = 1.0-ABS(1.0-c*2);
	 v = MIN(c*2, 1.0);

	 x = cos(h*3.14159/180.0)*c*128.0;
	 y = sin(h*3.14159/180.0)*c*128.0;

	 hsv_to_rgb(h, s, v, &r, &g, &b);
	 putpixel(screen, SCREEN_W/2+x, SCREEN_H/2+y, makecol(r, g, b));
      }
   }

   textout_ex(screen, font, "Press a key or mouse button", xoff+52, SCREEN_H-10, palette_color[15], palette_color[0]);

   release_screen();

   while (!next())
      ;
}



static void caps(void)
{
   static char *s[] =
   {
      "(scroll)",
      "(triple buffer)",
      "(hardware cursor)",
      "solid hline:",
      "xor hline:",
      "solid/masked pattern hline:",
      "copy pattern hline:",
      "solid fill:",
      "xor fill:",
      "solid/masked pattern fill:",
      "copy pattern fill:",
      "solid line:",
      "xor line:",
      "solid triangle:",
      "xor triangle:",
      "mono text:",
      "vram->vram blit:",
      "masked vram->vram blit:",
      "mem->screen blit:",
      "masked mem->screen blit:",
      "system->screen blit:",
      "masked system->screen blit:",
      "Mouse pointer:",
      "stretch vram->vram blit:",
      "masked stretch vram->vram blit:",
      "stretch system->vram blit:",
      "masked stretch system->vram blit:",
      NULL
   };

   int c;

   acquire_screen();

   clear_to_color(screen, palette_color[0]);
   textout_ex(screen,font,"Hardware accelerated features", xoff+44, 6, palette_color[15], palette_color[0]);

   for (c=3; s[c]; c++) {
      textout_ex(screen, font, s[c], SCREEN_W/2+64-text_length(font, s[c]), SCREEN_H/2-184+c*16, palette_color[15], palette_color[0]);
      textout_ex(screen, font, (gfx_capabilities & (1<<c)) ? "yes" : "no", SCREEN_W/2+80, SCREEN_H/2-184+c*16, palette_color[15], palette_color[0]);
   }

   textout_ex(screen, font, "Press a key or mouse button", xoff+52, SCREEN_H-10, palette_color[15], palette_color[0]);

   release_screen();

   while (!next()) {
      rest(1);
   }
}



int mouse_proc(void)
{
   int mickey_x, mickey_y;

   show_mouse(NULL);
   clear_to_color(screen, palette_color[0]);
   textout_centre_ex(screen, font, "Mouse test", SCREEN_W/2, 6, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Press a key", SCREEN_W/2, SCREEN_H-10, palette_color[15], palette_color[0]);

   do {
      rest(1);
      poll_mouse();
      get_mouse_mickeys(&mickey_x, &mickey_y);
      sprintf(buf, "X axis:   pos=%-4d   ", mouse_x);
      if (mickey_x)
         strcat(buf,"moving");
      else
         strcat(buf,"      ");
      textout_centre_ex(screen, font, buf, SCREEN_W/2, SCREEN_H/2 - 30, palette_color[15], palette_color[0]);
      sprintf(buf, "Y axis:   pos=%-4d   ", mouse_y);
      if (mickey_y)
         strcat(buf,"moving");
      else
         strcat(buf,"      ");
      textout_centre_ex(screen, font, buf, SCREEN_W/2, SCREEN_H/2 - 10, palette_color[15], palette_color[0]);
      sprintf(buf, "Z axis:   pos=%-4d         ", mouse_z);
      textout_centre_ex(screen, font, buf, SCREEN_W/2, SCREEN_H/2 + 10, palette_color[15], palette_color[0]);
      sprintf(buf, "Buttons:    ");
      if (mouse_b & 1)
         strcat(buf,"left");
      else
         strcat(buf,"    ");
      if (mouse_b & 4)
         strcat(buf,"middle");
      else
         strcat(buf,"      ");
      if (mouse_b & 2)
         strcat(buf,"right"); 
      else
         strcat(buf,"     ");
      textout_centre_ex(screen, font, buf, SCREEN_W/2, SCREEN_H/2 + 30, palette_color[15], palette_color[0]);
   } while (!keypressed());

   clear_keybuf();
   show_mouse(screen);
   return D_REDRAW;
}



int keyboard_proc(void)
{
   int k, c;

   show_mouse(NULL);
   clear_to_color(screen, palette_color[0]);
   textout_centre_ex(screen, font, "Keyboard test", SCREEN_W/2, 6, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Press a mouse button", SCREEN_W/2, SCREEN_H-10, palette_color[15], palette_color[0]);

   do {
      if (keypressed()) {
	 blit(screen, screen, xoff+0, yoff+48, xoff+0, yoff+40, 320, 112);
	 k = readkey();
	 c = k & 0xFF;
	 usprintf(buf,"0x%04X - '%c'", k, c);
	 textout_centre_ex(screen, font, buf, SCREEN_W/2, yoff+152, palette_color[15], palette_color[0]);
      }
      poll_mouse();
      rest(1);
   } while (!mouse_b);

   do {
      poll_mouse();
   } while (mouse_b);

   show_mouse(screen);
   return D_REDRAW;
}



int int_c1 = 0;
int int_c2 = 0;
int int_c3 = 0;



void int1(void)
{
   if (++int_c1 >= 16)
      int_c1 = 0;
}

END_OF_FUNCTION(int1)



void int2(void)
{
   if (++int_c2 >= 16)
      int_c2 = 0;
}

END_OF_FUNCTION(int2)



void int3(void)
{
   if (++int_c3 >= 16)
      int_c3 = 0;
}

END_OF_FUNCTION(int3)



void interrupt_test(void)
{
   clear_to_color(screen, palette_color[0]);
   message("Timer interrupt test");

   textout_ex(screen,font,"1/4", xoff+108, yoff+78, palette_color[15], palette_color[0]);
   textout_ex(screen,font,"1", xoff+156, yoff+78, palette_color[15], palette_color[0]);
   textout_ex(screen,font,"5", xoff+196, yoff+78, palette_color[15], palette_color[0]);

   LOCK_VARIABLE(int_c1);
   LOCK_VARIABLE(int_c2);
   LOCK_VARIABLE(int_c3);
   LOCK_FUNCTION(int1);
   LOCK_FUNCTION(int2);
   LOCK_FUNCTION(int3);

   install_int(int1, 250);
   install_int(int2, 1000);
   install_int(int3, 5000);

   while (!next()) {
      rectfill(screen, xoff+110, yoff+90, xoff+130, yoff+110, palette_color[int_c1]);
      rectfill(screen, xoff+150, yoff+90, xoff+170, yoff+110, palette_color[int_c2]);
      rectfill(screen, xoff+190, yoff+90, xoff+210, yoff+110, palette_color[int_c3]);
      rest(1);
   }

   remove_int(int1);
   remove_int(int2);
   remove_int(int3);
}



int fade_color = 63;

void fade(void)
{
   RGB rgb;

   rgb.r = rgb.g = rgb.b = (fade_color < 64) ? fade_color : 127 - fade_color;
   rgb.filler = 0;

   _set_color(0, &rgb);

   fade_color++;
   if (fade_color >= 128)
      fade_color = 0;
}

END_OF_FUNCTION(fade)



void rotate_test(void)
{
   fixed c = 0;
   BITMAP *b;
   BITMAP *new_sprite;

   solid_mode();
   new_sprite = create_bitmap(33, 33);
   clear_to_color(new_sprite, bitmap_mask_color(new_sprite));
   circlefill(new_sprite, 16, 16, 9, palette_color[2]);
   circle(new_sprite, 16, 16, 9, palette_color[1]);
   line(new_sprite, 0, 0, 32, 32, palette_color[3]);
   line(new_sprite, 32, 0, 0, 32, palette_color[3]);
   textout_ex(new_sprite, font, "Test", 1, 12, palette_color[15], -1);

   set_clip_rect(screen, 0, 0, VIRTUAL_W-1, VIRTUAL_H-1);
   clear_to_color(screen, palette_color[0]);
   message("Bitmap rotation test");

   b = create_bitmap(33, 33);

   draw_sprite(screen, new_sprite, SCREEN_W/2-16-33, SCREEN_H/2-16-33);
   draw_sprite(screen, new_sprite, SCREEN_W/2-16-66, SCREEN_H/2-16-66);

   draw_sprite_v_flip(screen, new_sprite, SCREEN_W/2-16-33, SCREEN_H/2-16+33);
   draw_sprite_v_flip(screen, new_sprite, SCREEN_W/2-16-66, SCREEN_H/2-16+66);

   draw_sprite_h_flip(screen, new_sprite, SCREEN_W/2-16+33, SCREEN_H/2-16-33);
   draw_sprite_h_flip(screen, new_sprite, SCREEN_W/2-16+66, SCREEN_H/2-16-66);

   draw_sprite_vh_flip(screen, new_sprite, SCREEN_W/2-16+33, SCREEN_H/2-16+33);
   draw_sprite_vh_flip(screen, new_sprite, SCREEN_W/2-16+66, SCREEN_H/2-16+66);

   tm = 0; _tm = 0;
   ct = 0;

   while (!next()) {
      clear_to_color(b, palette_color[0]);
      rotate_sprite(b, new_sprite, 0, 0, c);
      blit(b, screen, 0, 0, SCREEN_W/2-16, SCREEN_H/2-16, b->w, b->h);

      c += itofix(1) / 16;

      SHOW_TIME_MACRO();
   }

   destroy_bitmap(new_sprite);
   destroy_bitmap(b);
}



void stretch_test(void)
{
   BITMAP *b;
   int c;

   set_clip_rect(screen, 0, 0, VIRTUAL_W-1, VIRTUAL_H-1);

   clear_to_color(screen, palette_color[0]);
   message("Bitmap scaling test");

   tm = 0; _tm = 0;
   ct = 0;
   c = 1;

   rect(screen, SCREEN_W/2-128, SCREEN_H/2-64, SCREEN_W/2+128, SCREEN_H/2+64, palette_color[15]);
   set_clip_rect(screen, SCREEN_W/2-127, SCREEN_H/2-63, SCREEN_W/2+127, SCREEN_H/2+63);

   solid_mode();
   b = create_bitmap(32, 32);
   clear_to_color(b, palette_color[0]);
   circlefill(b, 16, 16, 8, palette_color[2]);
   circle(b, 16, 16, 8, palette_color[1]);
   line(b, 0, 0, 31, 31, palette_color[3]);
   line(b, 31, 0, 0, 31, palette_color[3]);
   textout_ex(b, font, "Test", 1, 12, palette_color[15], -1);

   while (!next()) {
      stretch_blit(b, screen, 0, 0, 32, 32, SCREEN_W/2-c, SCREEN_H/2-(256-c), c*2, (256-c)*2);

      if (c >= 255) {
	 c = 1;
	 rectfill(screen, SCREEN_W/2-127, SCREEN_H/2-63, SCREEN_W/2+127, SCREEN_H/2+63, palette_color[0]);
      }
      else
	 c++;

      SHOW_TIME_MACRO();
   }

   destroy_bitmap(b);
}



void hscroll_test(void)
{
   int x, y;
   int done = FALSE;
   int ox = mouse_x;
   int oy = mouse_y;

   set_clip_rect(screen, 0, 0, VIRTUAL_W-1, VIRTUAL_H-1);
   clear_to_color(screen, palette_color[0]);
   rect(screen, 0, 0, VIRTUAL_W-1, VIRTUAL_H-1, palette_color[15]);

   for (x=1; x<16; x++) {
      vline(screen, VIRTUAL_W*x/16, 1, VIRTUAL_H-2, palette_color[x]);
      hline(screen, 1, VIRTUAL_H*x/16, VIRTUAL_W-2, palette_color[x]);
      sprintf(buf, "%x", x);
      textout_ex(screen, font, buf, 2, VIRTUAL_H*x/16-4, palette_color[15], -1);
      textout_ex(screen, font, buf, VIRTUAL_W-9, VIRTUAL_H*x/16-4, palette_color[15], -1);
      textout_ex(screen, font, buf, VIRTUAL_W*x/16-4, 2, palette_color[15], -1);
      textout_ex(screen, font, buf, VIRTUAL_W*x/16-4, VIRTUAL_H-9, palette_color[15], -1);
   }

   sprintf(buf, "Graphics driver: %s", gfx_driver->name);
   textout_ex(screen, font, buf, 32, 32, palette_color[15], -1);

   sprintf(buf, "Description: %s", gfx_driver->desc);
   textout_ex(screen, font, buf, 32, 48, palette_color[15], -1);

   sprintf(buf, "Specs: %s", gfx_specs);
   textout_ex(screen, font, buf, 32, 64, palette_color[15], -1);

   sprintf(buf, "Color depth: %s", gfx_specs2);
   textout_ex(screen, font, buf, 32, 80, palette_color[15], -1);

   textout_ex(screen, font, gfx_specs3, 32, 96, palette_color[15], -1);

   if (gfx_driver->scroll == NULL)
      textout_ex(screen, font, "Hardware scrolling not supported", 32, 112, palette_color[15], -1);

   x = y = 0;
   position_mouse(32, 32);

   while ((!done) && (!mouse_b)) {
      poll_mouse();

      if ((mouse_x != 32) || (mouse_y != 32)) {
	 x += mouse_x - 32;
	 y += mouse_y - 32;
	 position_mouse(32, 32);
      }

      if (keypressed()) {
	 switch (readkey() >> 8) {
	    case KEY_LEFT:  x--;          break;
	    case KEY_RIGHT: x++;          break;
	    case KEY_UP:    y--;          break;
	    case KEY_DOWN:  y++;          break;
	    default:       done = TRUE;   break;
	 }
      }

      if (x < 0)
	 x = 0;
      else if (x > (VIRTUAL_W - SCREEN_W))
	 x = VIRTUAL_W - SCREEN_W;

      if (y < 0)
	 y = 0;
      else if (y > VIRTUAL_H)
	 y = VIRTUAL_H;

      scroll_screen(x, y);
      rest(1);
   }

   do {
      poll_mouse();
   } while (mouse_b);

   position_mouse(ox, oy);
   clear_keybuf();

   scroll_screen(0, 0);
}



void test_it(char *msg, void (*func)(int, int))
{ 
   int x = 0;
   int y = 0;
   int c = 0;
   int pat = AL_RAND()%NUM_PATTERNS;

   do { 
      acquire_screen();

      set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
      clear_to_color(screen, palette_color[0]); 
      message(msg);

      textout_centre_ex(screen, font, "(arrow keys to slide)", SCREEN_W/2, 28, palette_color[15], palette_color[0]);
      textout_ex(screen, font, "unclipped:", xoff+48, yoff+50, palette_color[15], palette_color[0]);
      textout_ex(screen, font, "clipped:", xoff+180, yoff+62, palette_color[15], palette_color[0]);
      rect(screen, xoff+191, yoff+83, xoff+240, yoff+114, palette_color[15]);
      set_clip_rect(screen, xoff+192, yoff+84, xoff+239, yoff+113);

      drawing_mode(mode, pattern[pat], 0, 0);
      set_clip_state(screen, FALSE);
      (*func)(xoff+x+60, yoff+y+70);
      set_clip_state(screen, TRUE);
      (*func)(xoff+x+180, yoff+y+70);
      solid_mode();

      release_screen();

      do {
	 poll_mouse();

	 if (mouse_b) {
	    do {
	       poll_mouse();
	    } while (mouse_b);
	    c = KEY_ESC<<8;
	    break;
	 }

	 if (keypressed())
	    c = readkey();

      } while (!c);

      if ((c>>8) == KEY_LEFT) {
	 if (x > -32)
	    x--;
	 c = 0;
      }
      else if ((c>>8) == KEY_RIGHT) {
	 if (x < 32)
	    x++;
	 c = 0;
      }
      else if ((c>>8) == KEY_UP) {
	 if (y > -32)
	    y--;
	 c = 0;
      }
      else if ((c>>8) == KEY_DOWN) {
	 if (y < 32)
	    y++;
	 c = 0;
      }

   } while (!c);
}



void do_it(char *msg, int clip_flag, void (*func)(void))
{ 
   int x1, y1, x2, y2;

   set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
   clear_to_color(screen, palette_color[0]);
   message(msg);

   if (clip_flag) {
      do {
	 x1 = (AL_RAND() & 255) + 32;
	 x2 = (AL_RAND() & 255) + 32;
      } while (x2-x1 < 30);
      do {
	 y1 = (AL_RAND() & 127) + 40;
	 y2 = (AL_RAND() & 127) + 40;
      } while (y2-y1 < 20);
      set_clip_rect(screen, xoff+x1, yoff+y1, xoff+x2, yoff+y2);
   }
   else
      set_clip_state(screen, FALSE);

   drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);

   (*func)();

   solid_mode();

   if (!clip_flag)
      set_clip_state(screen, TRUE);
}



void circler(BITMAP *b, int x, int y, int c)
{
   circlefill(b, x, y, 4, palette_color[c]);
}



int floodfill_proc(void)
{
   int ox, oy, nx, ny;
   int c;

   scare_mouse();

   clear_to_color(screen, palette_color[0]);

   textout_centre_ex(screen, font, "floodfill test", SCREEN_W/2, 6, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Press a mouse button to draw,", SCREEN_W/2, 64, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "a key 0-9 to floodfill,", SCREEN_W/2, 80, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "and ESC to finish", SCREEN_W/2, 96, palette_color[15], palette_color[0]);

   unscare_mouse();

   ox = -1;
   oy = -1;

   do {
      poll_mouse();

      c = mouse_b;

      if (c) {
	 nx = mouse_x;
	 ny = mouse_y;
	 if ((ox >= 0) && (oy >= 0)) {
	    scare_mouse();
	    if (c&1) {
	       line(screen, ox, oy, nx, ny, palette_color[255]);
	    }
	    else {
	       acquire_screen();
	       do_line(screen, ox, oy, nx, ny, 255, circler);
	       release_screen();
	    }
	    unscare_mouse();
	 }
	 ox = nx;
	 oy = ny;
      } 
      else 
	 ox = oy = -1;

      if (keypressed()) {
	 c = readkey() & 0xff;
	 if ((c >= '0') && (c <= '9')) {
	    scare_mouse();
	    drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);
	    floodfill(screen, mouse_x, mouse_y, palette_color[c-'0']);
	    solid_mode();
	    unscare_mouse();
	 }
      } 

      rest(1);
   } while (c != 27);

   return D_REDRAW;
}



void draw_spline(int points[8])
{
   int i, c1, c2;

   if (bitmap_color_depth(screen) == 8) {
      c1 = 255;
      c2 = 1;
   }
   else {
      c1 = makecol(255, 255, 255);
      c2 = makecol(0, 255, 255);
   }

   spline(screen, points, c1);

   for (i=0; i<4; i++)
      rect(screen, points[i*2]-6, points[i*2+1]-6, 
		   points[i*2]+5, points[i*2+1]+5, c2);
}



int spline_proc(void)
{
   int points[8];
   int nx, ny, ox, oy;
   int sel, os;
   int c;

   scare_mouse();
   acquire_screen();

   clear_to_color(screen, palette_color[0]);

   textout_centre_ex(screen, font, "spline test", SCREEN_W/2, 6, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Drag boxes to change guide points,", SCREEN_W/2, 64, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "and press ESC to finish", SCREEN_W/2, 80, palette_color[15], palette_color[0]);

   for (c=0; c<4; c++) {
      points[c*2] = SCREEN_W/2 + c*64 - 96;
      points[c*2+1] = SCREEN_H/2 + ((c&1) ? 32 : -32);
   }

   xor_mode(TRUE);

   ox = mouse_x;
   oy = mouse_x;
   sel = -1;

   draw_spline(points); 

   release_screen();
   unscare_mouse();

   for (;;) {
      poll_mouse();

      nx = mouse_x;
      ny = mouse_y;
      os = sel;

      if (mouse_b) {
	 if (sel < 0) {
	    for (sel=3; sel>=0; sel--) {
	       if ((nx >= points[sel*2]-6) &&
		   (nx < points[sel*2]+6) &&
		   (ny >= points[sel*2+1]-6) &&
		   (ny < points[sel*2+1]+6))
		  break;
	    }
	 }
	 if ((sel >= 0) && ((ox != nx) || (oy != ny) || (os != sel))) {
	    scare_mouse();
	    acquire_screen();
	    draw_spline(points); 

	    points[sel*2] = nx;
	    points[sel*2+1] = ny;

	    draw_spline(points); 
	    release_screen();
	    unscare_mouse();
	 }
      }
      else
	 sel = -1;

      if (keypressed()) {
	 c = readkey();
	 if ((c&0xFF) == 27)
	    break;
      }

      ox = nx;
      oy = ny;
      rest(1);
   }

   xor_mode(FALSE);

   return D_REDRAW;
}



int polygon_proc(void)
{
   #define MAX_POINTS      256

   int k = 0;
   int num_points = 0;
   int points[MAX_POINTS*2];

   CHECK_TRANS_BLENDER();

   scare_mouse();

   clear_to_color(screen, palette_color[0]);

   textout_centre_ex(screen, font, "polygon test", SCREEN_W/2, 6, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Press left mouse button to add a", SCREEN_W/2, 64, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "point, right mouse button to draw,", SCREEN_W/2, 80, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "and ESC to finish", SCREEN_W/2, 96, palette_color[15], palette_color[0]);

   unscare_mouse();

   do {
      poll_mouse();
      rest(1);
   } while (mouse_b);

   do {
      poll_mouse();

      if ((mouse_b & 1) && (num_points < MAX_POINTS)) {
	 points[num_points*2] = mouse_x;
	 points[num_points*2+1] = mouse_y;

	 scare_mouse();

	 if (num_points > 0)
	    line(screen, points[(num_points-1)*2], points[(num_points-1)*2+1], 
			 points[num_points*2], points[num_points*2+1], palette_color[255]);

	 circlefill(screen, points[num_points*2], points[num_points*2+1], 2, palette_color[255]);

	 num_points++;

	 unscare_mouse();
	 do {
	    poll_mouse();
	 } while (mouse_b);
      }

      if ((mouse_b & 2) && (num_points > 2)) {
	 scare_mouse();

	 line(screen, points[(num_points-1)*2], points[(num_points-1)*2+1], 
						   points[0], points[1], palette_color[255]);
	 drawing_mode(mode, pattern[AL_RAND()%NUM_PATTERNS], 0, 0);
	 polygon(screen, num_points, points, palette_color[1]);
	 solid_mode();

	 num_points = 0;

	 unscare_mouse();
	 do {
	    poll_mouse();
	 } while (mouse_b);
      }

      if (keypressed())
	 k = readkey() & 0xff;

      rest(1);
   } while (k != 27);

   return D_REDRAW;
}



int putpixel_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("putpixel test", putpix_test);
   do_it("timing putpixel", FALSE, putpix_demo);
   do_it("timing putpixel [clipped]", TRUE, putpix_demo);
   unscare_mouse();
   return D_REDRAW;
}



int getpixel_proc(void)
{
   getpix_demo();
   return D_REDRAW;
}



int hline_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("hline test", hline_test);
   do_it("timing hline", FALSE, hline_demo);
   do_it("timing hline [clipped]", TRUE, hline_demo);
   unscare_mouse();
   return D_REDRAW;
}



int vline_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("vline test", vline_test);
   do_it("timing vline", FALSE, vline_demo);
   do_it("timing vline [clipped]", TRUE, vline_demo);
   unscare_mouse();
   return D_REDRAW;
}



int line_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("line test", line_test);
   do_it("timing line", FALSE, line_demo);
   do_it("timing line [clipped]", TRUE, line_demo);
   unscare_mouse();
   return D_REDRAW;
}



int rectfill_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("rectfill test", rectfill_test);
   do_it("timing rectfill", FALSE, rectfill_demo);
   do_it("timing rectfill [clipped]", TRUE, rectfill_demo);
   unscare_mouse();
   return D_REDRAW;
}



int triangle_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("triangle test", triangle_test);
   do_it("timing triangle", FALSE, triangle_demo);
   do_it("timing triangle [clipped]", TRUE, triangle_demo);
   unscare_mouse();
   return D_REDRAW;
}



int triangle3d_proc(void)
{
   check_tables();

   scare_mouse();
   type3d = POLYTYPE_FLAT;
   do_it("timing triangle 3D [flat]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_GCOL;
   do_it("timing triangle 3D [gcol]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_GRGB;
   do_it("timing triangle 3D [grgb]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_ATEX;
   do_it("timing triangle 3D [atex]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_PTEX;
   do_it("timing triangle 3D [ptex]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_ATEX_MASK;
   do_it("timing triangle 3D [atex mask]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_PTEX_MASK;
   do_it("timing triangle 3D [ptex mask]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_ATEX_LIT;
   do_it("timing triangle 3D [atex lit]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_PTEX_LIT;
   do_it("timing triangle 3D [ptex lit]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_ATEX_MASK_LIT;
   do_it("timing triangle 3D [atex mask lit]", FALSE, triangle3d_demo);
   type3d = POLYTYPE_PTEX_MASK_LIT;
   do_it("timing triangle 3D [ptex mask lit]", FALSE, triangle3d_demo);
   unscare_mouse();
   return D_REDRAW;
}



int circle_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("circle test", circle_test);
   do_it("timing circle", FALSE, circle_demo);
   do_it("timing circle [clipped]", TRUE, circle_demo);
   unscare_mouse();
   return D_REDRAW;
}



int circlefill_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("circlefill test", circlefill_test);
   do_it("timing circlefill", FALSE, circlefill_demo);
   do_it("timing circlefill [clipped]", TRUE, circlefill_demo);
   unscare_mouse();
   return D_REDRAW;
}



int ellipse_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("ellipse test", ellipse_test);
   do_it("timing ellipse", FALSE, ellipse_demo);
   do_it("timing ellipse [clipped]", TRUE, ellipse_demo);
   unscare_mouse();
   return D_REDRAW;
}



int ellipsefill_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("ellipsefill test", ellipsefill_test);
   do_it("timing ellipsefill", FALSE, ellipsefill_demo);
   do_it("timing ellipsefill [clipped]", TRUE, ellipsefill_demo);
   unscare_mouse();
   return D_REDRAW;
}



int arc_proc(void)
{
   CHECK_TRANS_BLENDER();

   scare_mouse();
   test_it("arc test", arc_test);
   do_it("timing arc", FALSE, arc_demo);
   do_it("timing arc [clipped]", TRUE, arc_demo);
   unscare_mouse();
   return D_REDRAW;
}



int textout_proc(void)
{
   scare_mouse();
   test_it("textout test", textout_test);
   do_it("timing textout", FALSE, textout_demo);
   do_it("timing textout [clipped]", TRUE, textout_demo);
   unscare_mouse();
   return D_REDRAW;
}



/* Some helpers for blit_proc */
int pause_mode = FALSE;


int blit_pause_proc(void)
{
   static int pause_count = 0;
   int ch;

   /* Don't go too fast. We want to be able to see if it's working the way it
    * was intended to and without this all you see are colors zipping arround.
    */
   pause_count++;
   if (pause_count >= 2) {
      pause_count = 0;
      rest(5);
   }

   if (!pause_mode) {
      if (mouse_needs_poll())
         poll_mouse();

      if (mouse_b)
         return TRUE;

      if (keypressed()) {
         if (((ch = readkey()) & 0xff) == 'f') {
            pause_mode = TRUE;
            return FALSE;
         }
         return TRUE;
      }
   }
   else {
      while (!keypressed()) {
         if (mouse_needs_poll())
            poll_mouse();

         if (mouse_b)
            return FALSE;

	 rest(2);
      }
      if (((ch = readkey()) & 0xff) == 'f') {
         pause_mode = FALSE;
         return FALSE;
      }
      if (((ch & 0xff) == 27) || ((ch & 0xff) == 'q'))
         return TRUE;
   }
   return FALSE;
}



int blit_proc(void)
{
   int i, c, y, tpat_size=64;
   int done = FALSE;

   scare_mouse();
   acquire_screen();
   set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
   clear_to_color(screen, palette_color[0]);

   textout_centre_ex(screen, font, "Blit to self test.  Options:           ", SCREEN_W/2,  4, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "  Press 'f' to toggle frame mode.      ", SCREEN_W/2, 12, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "  Press any other key to do next test. ", SCREEN_W/2, 20, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Frame mode options:                    ", SCREEN_W/2, 40, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "  Press any key to draw next frame.    ", SCREEN_W/2, 48, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "  Press 'f' to toggle frame mode.      ", SCREEN_W/2, 56, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "  Press 'q' to continue with next test.", SCREEN_W/2, 64, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Press a key or mouse button to start.",   SCREEN_W/2, 80, palette_color[15], palette_color[0]);

   release_screen();
   unscare_mouse();

   while ((!keypressed()) && (!mouse_b)) {
      if (mouse_needs_poll())
         poll_mouse();

      rest(2);
   }

   pause_mode = FALSE;
   if (keypressed()) {
      if ((readkey() & 0xff) == 'f')
         pause_mode = TRUE;
   }
   else {
      while (mouse_b) {
         if (mouse_needs_poll())
            poll_mouse();
      }
   }

   scare_mouse();
   acquire_screen();


   /* Find a good size for the test pattern. If its too small its difficult
    * to see and if its too big then the test can run too slow.
    */
   if ((SCREEN_W < 256) || (SCREEN_H < 256))
      tpat_size = 32;
   else if ((SCREEN_W < 640) || (SCREEN_H < 480))
      tpat_size = 64;
   else
      tpat_size = 96;

   /* Draw test pattern */
   for (y=0; y < tpat_size; y+=(tpat_size/8)) {
      for (c=0; c < 8; c++)
         rectfill(screen, c*(tpat_size/8), y,
                         (c*(tpat_size/8))+(tpat_size/8)-1, y+(tpat_size/8)-1,
                         palette_color[(c+(y/8))&31]);
   }
   vline(screen, 5,           0, tpat_size-1, palette_color[15]);
   vline(screen, tpat_size-6, 0, tpat_size-1, palette_color[15]);
   vline(screen, 4,           0, tpat_size-1, palette_color[0]);
   vline(screen, tpat_size-5, 0, tpat_size-1, palette_color[0]);
   vline(screen, 3,           0, tpat_size-1, palette_color[15]);
   vline(screen, tpat_size-4, 0, tpat_size-1, palette_color[15]);
   hline(screen, 0,           5, tpat_size-1, palette_color[15]);
   hline(screen, 0, tpat_size-6, tpat_size-1, palette_color[15]);
   hline(screen, 0,           4, tpat_size-1, palette_color[0]);
   hline(screen, 0, tpat_size-5, tpat_size-1, palette_color[0]);
   hline(screen, 0,           3, tpat_size-1, palette_color[15]);
   hline(screen, 0, tpat_size-4, tpat_size-1, palette_color[15]);
   release_screen();

   #define B2S_CHECK_X  \
      rest(20);         \
      if (done)         \
         break;


   /* Blit to self test */
   for (i = 0; i < 2; i++) {
      /* down+right */
      for (c = y = 0; (c < SCREEN_W-tpat_size) && (y < SCREEN_H-tpat_size); c++, y++) {
         blit(screen, screen, c, y, c+1, y+1, tpat_size, tpat_size);
         vline(screen, c, y, y+tpat_size-1, palette_color[0]);
         hline(screen, c, y, c+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X

      /* up+left */
      for ( ; (c > 0) && (y > 0); c--, y--) {
         blit(screen, screen, c, y, c-1, y-1, tpat_size, tpat_size);
         vline(screen, c+tpat_size-1, y, y+tpat_size-1, palette_color[0]);
         hline(screen, c, y+tpat_size-1, c+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X

      /* right */
      for ( ; c < SCREEN_W-tpat_size; c++) {
         blit(screen, screen, c, y, c+1, y, tpat_size, tpat_size);
         vline(screen, c, y, y+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X

      /* down+left */
      for ( ; (c > 0) && (y < SCREEN_H-tpat_size); c--, y++) {
         blit(screen, screen, c, y, c-1, y+1, tpat_size, tpat_size);
         vline(screen, c+tpat_size-1, y, y+tpat_size-1, palette_color[0]);
         hline(screen, c, y, c+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X

      /* up+right */
      for ( ; (c < SCREEN_W-tpat_size) && (y > 0); c++, y--) {
         blit(screen, screen, c, y, c+1, y-1, tpat_size, tpat_size);
         vline(screen, c, y, y+tpat_size-1, palette_color[0]);
         hline(screen, c, y+tpat_size-1, c+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X

      /* down */
      for ( ; y < SCREEN_H-tpat_size; y++) {
         blit(screen, screen, c, y, c, y+1, tpat_size, tpat_size);
         hline(screen, c, y, c+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X

      /* left */
      for ( ; c > 0; c--) {
         blit(screen, screen, c, y, c-1, y, tpat_size, tpat_size);
         vline(screen, c+tpat_size-1, y, y+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X

      /* up */
      for ( ; y > 0; y--) {
         blit(screen, screen, c, y, c, y-1, tpat_size, tpat_size);
         hline(screen, c, y+tpat_size-1, c+tpat_size-1, palette_color[0]);
         if ((done = blit_pause_proc()))
            break;
      }
      B2S_CHECK_X
   }

   if (keypressed())
      readkey();

   blit_from_screen = TRUE;
   do_it("timing blit screen->screen", FALSE, blit_demo);
   blit_align = TRUE;
   do_it("timing blit screen->screen (aligned)", FALSE, blit_demo);
   blit_align = FALSE;
   blit_from_screen = FALSE;
   do_it("timing blit memory->screen", FALSE, blit_demo);
   blit_align = TRUE;
   do_it("timing blit memory->screen (aligned)", FALSE, blit_demo);
   blit_align = FALSE;
   blit_mask = TRUE;
   if (gfx_capabilities & GFX_HW_VRAM_BLIT_MASKED) {
      blit_from_screen = TRUE;
      do_it("timing masked blit screen->screen", FALSE, blit_demo);
      blit_from_screen = FALSE;
   }
   do_it("timing masked blit memory->screen", FALSE, blit_demo);
   blit_mask = FALSE;
   do_it("timing blit [clipped]", TRUE, blit_demo);

   unscare_mouse();
   return D_REDRAW;
}



int sprite_proc(void)
{
   scare_mouse();

   test_it("sprite test", sprite_test);
   do_it("timing draw_sprite", FALSE, sprite_demo);
   do_it("timing draw_sprite [clipped]", TRUE, sprite_demo);

   test_it("RLE sprite test", rle_sprite_test);
   do_it("timing draw_rle_sprite", FALSE, rle_sprite_demo);
   do_it("timing draw_rle_sprite [clipped]", TRUE, rle_sprite_demo);

   global_compiled_sprite = get_compiled_sprite(global_sprite, is_planar_bitmap(screen));

   test_it("compiled sprite test", compiled_sprite_test);
   do_it("timing draw_compiled_sprite", FALSE, compiled_sprite_demo);

   destroy_compiled_sprite(global_compiled_sprite);

   unscare_mouse();
   return D_REDRAW;
}



int xlu_sprite_proc(void)
{
   check_tables();

   scare_mouse();

   test_it("translucent sprite test", xlu_sprite_test);
   do_it("timing draw_trans_sprite", FALSE, xlu_sprite_demo);
   do_it("timing draw_trans_sprite [clipped]", TRUE, xlu_sprite_demo);

   test_it("translucent RLE sprite test", rle_xlu_sprite_test);
   do_it("timing draw_trans_rle_sprite", FALSE, rle_xlu_sprite_demo);
   do_it("timing draw_trans_rle_sprite [clipped]", TRUE, rle_xlu_sprite_demo);

   unscare_mouse();
   return D_REDRAW;
}



int lit_sprite_proc(void)
{
   check_tables();
   color_map = light_map;

   scare_mouse();

   test_it("tinted sprite test", lit_sprite_test);
   do_it("timing draw_lit_sprite", FALSE, lit_sprite_demo);
   do_it("timing draw_lit_sprite [clipped]", TRUE, lit_sprite_demo);

   test_it("tinted RLE sprite test", rle_lit_sprite_test);
   do_it("timing draw_lit_rle_sprite", FALSE, rle_lit_sprite_demo);
   do_it("timing draw_lit_rle_sprite [clipped]", TRUE, rle_lit_sprite_demo);

   color_map = trans_map;
   unscare_mouse();
   return D_REDRAW;
}



int rotate_proc(void)
{
   scare_mouse();
   test_it("Flipped sprite test", flipped_sprite_test);
   rotate_test();
   unscare_mouse();
   return D_REDRAW;
}



int polygon3d_proc(void)
{
   #define NUM_POINTS   8+6
   #define NUM_FACES    6
   #define BUFFER_SIZE  128
   #define NUM_MODES    15

   /* a 3d x,y,z position */
   typedef struct POINT
   {
      float x, y, z;
   } POINT;

   /* four vertices plus a normal make up a quad */
   typedef struct QUAD 
   {
      int v1, v2, v3, v4;
      int normal;
      int visible;
   } QUAD;

   /* vertices of the cube */
   static POINT point[NUM_POINTS] =
   {
      /* regular vertices */
      { -32, -32, -32 },
      { -32,  32, -32 },
      {  32,  32, -32 },
      {  32, -32, -32 },
      { -32, -32,  32 },
      { -32,  32,  32 },
      {  32,  32,  32 },
      {  32, -32,  32 },

      /* normals */
      { -32, -32, -33 },
      { -32, -32,  33 },
      { -33,  32, -32 },
      {  33,  32, -32 },
      {  32, -33,  32 },
      {  32,  33,  32 }
   };

   /* output vertex list */
   static V3D_f vtx[NUM_POINTS] =
   {
     /* x  y  z  u   v   c    */
      { 0, 0, 0, 0,  0,  0x30 },
      { 0, 0, 0, 0,  32, 0x99 },
      { 0, 0, 0, 32, 32, 0x55 },
      { 0, 0, 0, 32, 0,  0xDD },
      { 0, 0, 0, 32, 0,  0x40 },
      { 0, 0, 0, 32, 32, 0xBB },
      { 0, 0, 0, 0,  32, 0x77 },
      { 0, 0, 0, 0,  0,  0xF0 }
   };

   /* six faces makes up a cube */
   QUAD face[NUM_FACES] = 
   {
     /* v1 v2 v3 v4 nrm v */
      { 0, 3, 2, 1, 8,  0 },
      { 4, 5, 6, 7, 9,  0 },
      { 1, 5, 4, 0, 10, 0 },
      { 2, 3, 7, 6, 11, 0 },
      { 7, 3, 0, 4, 12, 0 },
      { 6, 5, 1, 2, 13, 0 }
   };

   /* descriptions of the render modes */
   static char *mode_desc[NUM_MODES] = 
   {
      "POLYTYPE_FLAT",
      "POLYTYPE_GCOL",
      "POLYTYPE_GRGB",
      "POLYTYPE_ATEX",
      "POLYTYPE_PTEX",
      "POLYTYPE_ATEX_MASK",
      "POLYTYPE_PTEX_MASK",
      "POLYTYPE_ATEX_LIT",
      "POLYTYPE_PTEX_LIT",
      "POLYTYPE_ATEX_MASK_LIT",
      "POLYTYPE_PTEX_MASK_LIT",
      "POLYTYPE_ATEX_TRANS",
      "POLYTYPE_PTEX_TRANS",
      "POLYTYPE_ATEX_MASK_TRANS",
      "POLYTYPE_PTEX_MASK_TRANS"
   };

   int c;
   int key;
   int mode = POLYTYPE_FLAT;
   int tile = 1;

   float xr = -16;
   float yr = 24;
   float zr = 0;
   float dist = 128;
   float vx, vy, vz;
   float nx, ny, nz;
   int redraw_mode = TRUE;
   MATRIX_f transform, camera;
   V3D_f *vertex, *normal;
   BITMAP *buffer, *texture;

   buffer = create_bitmap(BUFFER_SIZE, BUFFER_SIZE);
   texture = create_bitmap(32, 32);

   check_tables();

   blit(global_sprite, texture, 0, 0, 0, 0, 32, 32);
   rect(texture, 0, 0, 31, 31, palette_color[1]);

   scare_mouse();
   acquire_screen();
   clear_to_color(screen, palette_color[0]);

   textout_centre_ex(screen, font, "3d polygon test", SCREEN_W/2, 6, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "Use the arrow keys to rotate the", SCREEN_W/2, 64, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "cube, + and - to zoom, space to", SCREEN_W/2, 80, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "change drawing mode, enter to tile", SCREEN_W/2, 96, palette_color[15], palette_color[0]);
   textout_centre_ex(screen, font, "the texture, and ESC to finish", SCREEN_W/2, 112, palette_color[15], palette_color[0]);

   release_screen();

   /* set projection parameters */
   set_projection_viewport(0, 0, BUFFER_SIZE, BUFFER_SIZE);

   get_camera_matrix_f(&camera, 
		     0, 0, 0,             /* eye position */
		     0, 0, 1,             /* front vector */
		     0, -1, 0,            /* up vector */
		     32,                  /* field of view */
		     1);                  /* aspect ratio */

   for (;;) {
      if (redraw_mode) {
	 rectfill(screen, 0, 24, SCREEN_W, 32, palette_color[0]);
	 textout_centre_ex(screen, font, mode_desc[mode], SCREEN_W/2, 24, palette_color[255], palette_color[0]);
	 redraw_mode = FALSE;
      }

      clear_to_color(buffer, 8);

      /* build a transformation matrix */
      get_transformation_matrix_f(&transform, 1, xr, yr, zr, 0, 0, dist);

      /* transform vertices into view space */
      for (c=0; c<NUM_POINTS; c++)
	 apply_matrix_f(&transform, point[c].x, point[c].y, point[c].z, &vtx[c].x, &vtx[c].y, &vtx[c].z);

      /* check for hidden faces */
      for (c=0; c<NUM_FACES; c++) {
	 vertex = &vtx[face[c].v1];
	 normal = &vtx[face[c].normal];
	 vx = vertex->x;
	 vy = vertex->y;
	 vz = vertex->z;
	 nx = normal->x - vx;
	 ny = normal->y - vy;
	 nz = normal->z - vz;
	 if (dot_product_f(vx, vy, vz, nx, ny, nz) < 0)
	    face[c].visible = TRUE;
	 else
	    face[c].visible = FALSE;
      }

      /* project vertices into screen space */
      for (c=0; c<NUM_POINTS; c++) {
	 apply_matrix_f(&camera, vtx[c].x, vtx[c].y, vtx[c].z, &vtx[c].x, &vtx[c].y, &vtx[c].z);
	 persp_project_f(vtx[c].x, vtx[c].y, vtx[c].z, &vtx[c].x, &vtx[c].y);
      }

      /* color_map to use */
      if ((mode == POLYTYPE_ATEX_MASK_TRANS) || (mode == POLYTYPE_PTEX_MASK_TRANS) ||
	  (mode == POLYTYPE_ATEX_TRANS) || (mode == POLYTYPE_PTEX_TRANS)) {
	 color_map = trans_map;
      }
      else {
	 color_map = light_map;
      }

      /* if mask mode, draw backfaces that may be seen through holes */
      if ((mode == POLYTYPE_ATEX_MASK) || (mode == POLYTYPE_PTEX_MASK) || 
	  (mode == POLYTYPE_ATEX_MASK_LIT) || (mode == POLYTYPE_PTEX_MASK_LIT) ||
	  (mode == POLYTYPE_ATEX_MASK_TRANS) || (mode == POLYTYPE_PTEX_MASK_TRANS) ||
	  (mode == POLYTYPE_ATEX_TRANS) || (mode == POLYTYPE_PTEX_TRANS)) {
	 for (c=0; c<NUM_FACES; c++) {
	    if (!face[c].visible) {
	       quad3d_f(buffer, mode, texture, 
			&vtx[face[c].v1], &vtx[face[c].v2], 
			&vtx[face[c].v3], &vtx[face[c].v4]);
	    }
	 }
      }

      /* draw the cube */
      for (c=0; c<NUM_FACES; c++) {
	 if (face[c].visible) {
	    quad3d_f(buffer, mode, texture, 
		     &vtx[face[c].v1], &vtx[face[c].v2], 
		     &vtx[face[c].v3], &vtx[face[c].v4]);
	 }
      }

      /* blit to the screen */
      vsync();
      blit(buffer, screen, 0, 0, 
	   (SCREEN_W-BUFFER_SIZE)/2, 
	   (SCREEN_H-BUFFER_SIZE)/2, BUFFER_SIZE, BUFFER_SIZE);

      /* handle user input */
      if (keypressed()) {
	 key = readkey();

	 switch (key >> 8) {

	    case KEY_DOWN:
	       xr -= 4;
	       break;

	    case KEY_UP:
	       xr += 4;
	       break;

	    case KEY_LEFT:
	       yr -= 4;
	       break;

	    case KEY_RIGHT:
	       yr += 4;
	       break;

	    case KEY_ESC:
	       goto getout;

	    case KEY_SPACE:
	       mode++;
	       if (mode >= NUM_MODES)
		  mode = 0;
	       redraw_mode = TRUE;
	       break;

	    case KEY_ENTER:
	       tile = (tile == 1) ? 2 : 1;
	       for (c=0; c<8; c++) {
		  if (vtx[c].u)
		     vtx[c].u = 32 * tile;
		  if (vtx[c].v)
		     vtx[c].v = 32 * tile;
	       }
	       break;

	    default:
	       if ((key & 0xFF) == '+') {
		  if (dist > 64)
		     dist -= 16;
	       }
	       else if ((key & 0xFF) == '-') {
		  if (dist < 1024)
		     dist += 16;
	       }
	       break;
	 }
      }
      rest(1);
   }

   getout:
   color_map = trans_map;
   destroy_bitmap(buffer);
   destroy_bitmap(texture);
   unscare_mouse();
   return D_REDRAW;
}



int do_profile(BITMAP *old_screen)
{
   int putpixel_time[6];
   int hline_time[6];
   int vline_time[6];
   int line_time[6];
   int rectfill_time[6];
   int circle_time[6];
   int circlefill_time[6];
   int ellipse_time[6];
   int ellipsefill_time[6];
   int arc_time[6];
   int triangle_time[6];
   int textout_time;
   int vram_blit_time;
   int aligned_vram_blit_time;
   int mem_blit_time;
   int aligned_mem_blit_time;
   int masked_vram_blit_time;
   int masked_blit_time;
   int draw_sprite_time;
   int draw_rle_sprite_time;
   int draw_compiled_sprite_time;
   int draw_trans_sprite_time;
   int draw_trans_rle_sprite_time;
   int draw_lit_sprite_time;
   int draw_lit_rle_sprite_time;

   int old_mode = mode;

   static char fname[80*6] = EMPTY_STRING; /* 80 chars * max UTF8 char width */
   FILE *f;

   int i;

   global_compiled_sprite = get_compiled_sprite(global_sprite, is_planar_bitmap(screen));
   check_tables();
   profile = TRUE;

   for (mode=0; mode<6; mode++) {

      do_it("profiling putpixel", FALSE, putpix_demo);
      if (ct < 0)
	 goto abort;
      else
	 putpixel_time[mode] = ct;

      do_it("profiling hline", FALSE, hline_demo);
      if (ct < 0)
	 goto abort;
      else
	 hline_time[mode] = ct;

      do_it("profiling vline", FALSE, vline_demo);
      if (ct < 0)
	 goto abort;
      else
	 vline_time[mode] = ct;

      do_it("profiling line", FALSE, line_demo);
      if (ct < 0)
	 goto abort;
      else
	 line_time[mode] = ct;

      do_it("profiling rectfill", FALSE, rectfill_demo);
      if (ct < 0)
	 goto abort;
      else
	 rectfill_time[mode] = ct;

      do_it("profiling circle", FALSE, circle_demo);
      if (ct < 0)
	 goto abort;
      else
	 circle_time[mode] = ct;

      do_it("profiling circlefill", FALSE, circlefill_demo);
      if (ct < 0)
	 goto abort;
      else
	 circlefill_time[mode] = ct;

      do_it("profiling ellipse", FALSE, ellipse_demo);
      if (ct < 0)
	 goto abort;
      else
	 ellipse_time[mode] = ct;

      do_it("profiling ellipsefill", FALSE, ellipsefill_demo);
      if (ct < 0)
	 goto abort;
      else
	 ellipsefill_time[mode] = ct;

      do_it("profiling arc", FALSE, arc_demo);
      if (ct < 0)
	 goto abort;
      else
	 arc_time[mode] = ct;

      do_it("profiling triangle", FALSE, triangle_demo);
      if (ct < 0)
	 goto abort;
      else
	 triangle_time[mode] = ct;
   }

   mode = DRAW_MODE_SOLID;

   do_it("profiling textout", FALSE, textout_demo);
   if (ct < 0)
      goto abort;
   else
      textout_time = ct;

   if (!old_screen) {
      blit_from_screen = TRUE;
      do_it("profiling blit screen->screen", FALSE, blit_demo);
      blit_from_screen = FALSE;
      if (ct < 0)
	 goto abort;
      else
	 vram_blit_time = ct;

      blit_from_screen = TRUE;
      blit_align = TRUE;
      do_it("profiling blit screen->screen (aligned)", FALSE, blit_demo);
      blit_from_screen = FALSE;
      blit_align = FALSE;
      if (ct < 0)
	 goto abort;
      else
	 aligned_vram_blit_time = ct;
   }
   else {
      vram_blit_time = -1;
      aligned_vram_blit_time = -1;
   }

   do_it("profiling blit memory->screen", FALSE, blit_demo);
   if (ct < 0)
      goto abort;
   else
      mem_blit_time = ct;

   blit_align = TRUE;
   do_it("profiling blit memory->screen (aligned)", FALSE, blit_demo);
   blit_align = FALSE;
   if (ct < 0)
      goto abort;
   else
      aligned_mem_blit_time = ct;

   if ((!old_screen) && (gfx_capabilities & GFX_HW_VRAM_BLIT_MASKED)) {
      blit_from_screen = TRUE;
      blit_mask = TRUE;
      do_it("profiling masked blit screen->screen", FALSE, blit_demo);
      blit_from_screen = FALSE;
      blit_mask = FALSE;
      if (ct < 0)
	 goto abort;
      else
	 masked_vram_blit_time = ct;
   }
   else
      masked_vram_blit_time = -1;

   blit_mask = TRUE;
   do_it("profiling masked blit memory->screen", FALSE, blit_demo);
   blit_mask = FALSE;
   if (ct < 0)
      goto abort;
   else
      masked_blit_time = ct;

   do_it("profiling draw_sprite", FALSE, sprite_demo);
   if (ct < 0)
      goto abort;
   else
      draw_sprite_time = ct;

   do_it("profiling draw_rle_sprite", FALSE, rle_sprite_demo);
   if (ct < 0)
      goto abort;
   else
      draw_rle_sprite_time = ct;

   do_it("profiling draw_compiled_sprite", FALSE, compiled_sprite_demo);
   if (ct < 0)
      goto abort;
   else
      draw_compiled_sprite_time = ct;

   do_it("profiling draw_trans_sprite", FALSE, xlu_sprite_demo);
   if (ct < 0)
      goto abort;
   else
      draw_trans_sprite_time = ct;

   do_it("profiling draw_trans_rle_sprite", FALSE, rle_xlu_sprite_demo);
   if (ct < 0)
      goto abort;
   else
      draw_trans_rle_sprite_time = ct;

   color_map = light_map;
   do_it("profiling draw_lit_sprite", FALSE, lit_sprite_demo);
   color_map = trans_map;
   if (ct < 0)
      goto abort;
   else
      draw_lit_sprite_time = ct;

   color_map = light_map;
   do_it("profiling draw_lit_rle_sprite", FALSE, rle_lit_sprite_demo);
   color_map = trans_map;
   if (ct < 0)
      goto abort;
   else
      draw_lit_rle_sprite_time = ct;

   if (old_screen)
      screen = old_screen;

   clear_to_color(screen, palette_color[0]);
   show_mouse(screen);

   if (file_select_ex("Save profile log", fname, NULL, sizeof(fname), 0, 0)) {
      if (exists(fname))
	 if (alert(fname, "already exists: overwrite?", NULL, "OK", "Cancel", 13, 27) == 2)
	    goto abort;

      f = fopen(fname, "wt");
      if (!f) {
	 alert("Error writing", fname, NULL, "OK", NULL, 13, 0);
      }
      else {
	 fprintf(f, "%s %sprofile results\n\n", allegro_version_str, internal_version_str);

	 if (old_screen) {
	    fprintf(f, "Memory bitmap size: %dx%d\n", SCREEN_W, SCREEN_H);
	    fprintf(f, "Color depth: %d bpp\n\n\n", bitmap_color_depth(screen));
	 }
	 else {
	    fprintf(f, "Graphics driver: %s\n", gfx_driver->name);
	    fprintf(f, "Description: %s\n\n", gfx_driver->desc);
	    fprintf(f, "Screen size: %dx%d\n", SCREEN_W, SCREEN_H);
	    fprintf(f, "Virtual screen size: %dx%d\n", VIRTUAL_W, VIRTUAL_H);
	    fprintf(f, "Color depth: %d bpp\n\n\n", bitmap_color_depth(screen));

	    fprintf(f, "Hardware acceleration:\n\n");

	    if (gfx_capabilities & GFX_HW_HLINE)                     fprintf(f, "    solid scanline fills\n");
	    if (gfx_capabilities & GFX_HW_HLINE_XOR)                 fprintf(f, "    xor scanline fills\n");
	    if (gfx_capabilities & GFX_HW_HLINE_SOLID_PATTERN)       fprintf(f, "    solid pattern scanline fills\n");
	    if (gfx_capabilities & GFX_HW_HLINE_COPY_PATTERN)        fprintf(f, "    copy pattern scanline fills\n");
	    if (gfx_capabilities & GFX_HW_FILL)                      fprintf(f, "    solid area fills\n");
	    if (gfx_capabilities & GFX_HW_FILL_XOR)                  fprintf(f, "    xor area fills\n");
	    if (gfx_capabilities & GFX_HW_FILL_SOLID_PATTERN)        fprintf(f, "    solid pattern area fills\n");
	    if (gfx_capabilities & GFX_HW_FILL_COPY_PATTERN)         fprintf(f, "    copy pattern area fills\n");
	    if (gfx_capabilities & GFX_HW_LINE)                      fprintf(f, "    solid lines\n");
	    if (gfx_capabilities & GFX_HW_LINE_XOR)                  fprintf(f, "    xor lines\n");
	    if (gfx_capabilities & GFX_HW_TRIANGLE)                  fprintf(f, "    solid triangles\n");
	    if (gfx_capabilities & GFX_HW_TRIANGLE_XOR)              fprintf(f, "    xor triangles\n");
	    if (gfx_capabilities & GFX_HW_GLYPH)                     fprintf(f, "    mono text output\n");
	    if (gfx_capabilities & GFX_HW_VRAM_BLIT)                 fprintf(f, "    vram->vram blits\n");
	    if (gfx_capabilities & GFX_HW_VRAM_BLIT_MASKED)          fprintf(f, "    masked vram->vram blits\n");
	    if (gfx_capabilities & GFX_HW_MEM_BLIT)                  fprintf(f, "    mem->vram blits\n");
	    if (gfx_capabilities & GFX_HW_MEM_BLIT_MASKED)           fprintf(f, "    masked mem->vram blits\n");
	    if (gfx_capabilities & GFX_HW_SYS_TO_VRAM_BLIT)          fprintf(f, "    system->vram blits\n");
	    if (gfx_capabilities & GFX_HW_SYS_TO_VRAM_BLIT_MASKED)   fprintf(f, "    masked system->vram blits\n");
	    if (gfx_capabilities & GFX_HW_VRAM_STRETCH_BLIT)         fprintf(f, "    vram->vram stretch blits\n");
	    if (gfx_capabilities & GFX_HW_VRAM_STRETCH_BLIT_MASKED)  fprintf(f, "    vram->vram masked stretch blits\n");
	    if (gfx_capabilities & GFX_HW_SYS_STRETCH_BLIT)          fprintf(f, "    system->vram stretch blits\n");
	    if (gfx_capabilities & GFX_HW_SYS_STRETCH_BLIT_MASKED)   fprintf(f, "    system->vram masked stretch blits\n");

	    if (!(gfx_capabilities & ~(GFX_CAN_SCROLL | GFX_CAN_TRIPLE_BUFFER | GFX_HW_CURSOR)))
	       fprintf(f, "    <none>\n");

	    fprintf(f, "\n\n");
	 }

	 for (i=0; i<6; i++) {
	    switch (i) {

	       case DRAW_MODE_SOLID:
		  fprintf(f, "DRAW_MODE_SOLID results:\n\n");
		  break;

	       case DRAW_MODE_XOR:
		  fprintf(f, "DRAW_MODE_XOR results:\n\n");
		  break;

	       case DRAW_MODE_COPY_PATTERN:
		  fprintf(f, "DRAW_MODE_COPY_PATTERN results:\n\n");
		  break;

	       case DRAW_MODE_SOLID_PATTERN :
		  fprintf(f, "DRAW_MODE_SOLID_PATTERN results:\n\n");
		  break;

	       case DRAW_MODE_MASKED_PATTERN:
		  fprintf(f, "DRAW_MODE_MASKED_PATTERN results:\n\n");
		  break;

	       case DRAW_MODE_TRANS:
		  fprintf(f, "DRAW_MODE_TRANS results:\n\n");
		  break;
	    }

	    fprintf(f, "    putpixel()      - %d\n", putpixel_time[i]/TIME_SPEED);
	    fprintf(f, "    hline()         - %d\n", hline_time[i]/TIME_SPEED);
	    fprintf(f, "    vline()         - %d\n", vline_time[i]/TIME_SPEED);
	    fprintf(f, "    line()          - %d\n", line_time[i]/TIME_SPEED);
	    fprintf(f, "    rectfill()      - %d\n", rectfill_time[i]/TIME_SPEED);
	    fprintf(f, "    circle()        - %d\n", circle_time[i]/TIME_SPEED);
	    fprintf(f, "    circlefill()    - %d\n", circlefill_time[i]/TIME_SPEED);
	    fprintf(f, "    ellipse()       - %d\n", ellipse_time[i]/TIME_SPEED);
	    fprintf(f, "    ellipsefill()   - %d\n", ellipsefill_time[i]/TIME_SPEED);
	    fprintf(f, "    arc()           - %d\n", arc_time[i]/TIME_SPEED);
	    fprintf(f, "    triangle()      - %d\n", triangle_time[i]/TIME_SPEED);

	    fprintf(f, "\n\n");
	 }

	 fprintf(f, "Other functions:\n\n");

	 fprintf(f, "    textout()                    - %d\n", textout_time/TIME_SPEED);

	 if (vram_blit_time > 0)
	    fprintf(f, "    vram->vram blit()            - %d\n", vram_blit_time/TIME_SPEED);
	 else
	    fprintf(f, "    vram->vram blit()            - N/A\n");

	 if (aligned_vram_blit_time > 0)
	    fprintf(f, "    aligned vram->vram blit()    - %d\n", aligned_vram_blit_time/TIME_SPEED);
	 else
	    fprintf(f, "    aligned vram->vram blit()    - N/A\n");

	 fprintf(f, "    blit() from memory           - %d\n", mem_blit_time/TIME_SPEED);
	 fprintf(f, "    aligned blit() from memory   - %d\n", aligned_mem_blit_time/TIME_SPEED);

	 if (masked_vram_blit_time > 0)
	    fprintf(f, "    vram->vram masked_blit()     - %d\n", masked_vram_blit_time/TIME_SPEED);
	 else
	    fprintf(f, "    vram->vram masked_blit()     - N/A\n");

	 fprintf(f, "    masked_blit() from memory    - %d\n", masked_blit_time/TIME_SPEED);
	 fprintf(f, "    draw_sprite()                - %d\n", draw_sprite_time/TIME_SPEED);
	 fprintf(f, "    draw_rle_sprite()            - %d\n", draw_rle_sprite_time/TIME_SPEED);
	 fprintf(f, "    draw_compiled_sprite()       - %d\n", draw_compiled_sprite_time/TIME_SPEED);
	 fprintf(f, "    draw_trans_sprite()          - %d\n", draw_trans_sprite_time/TIME_SPEED);
	 fprintf(f, "    draw_trans_rle_sprite()      - %d\n", draw_trans_rle_sprite_time/TIME_SPEED);
	 fprintf(f, "    draw_lit_sprite()            - %d\n", draw_lit_sprite_time/TIME_SPEED);
	 fprintf(f, "    draw_lit_rle_sprite()        - %d\n", draw_lit_rle_sprite_time/TIME_SPEED);

	 fprintf(f, "\n");
	 fclose(f);
      }
   }

   abort:
   destroy_compiled_sprite(global_compiled_sprite);
   profile = FALSE;
   mode = old_mode;
   return D_REDRAW;
}



int do_p3d_profile(int tims[])
{
   profile = TRUE;
   check_tables();

   type3d = POLYTYPE_FLAT;
   do_it("profiling triangle 3D [flat]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else
      tims[0] = ct;

   type3d = POLYTYPE_GCOL;
   do_it("profiling triangle 3D [gcol]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[1] = ct;

   type3d = POLYTYPE_GRGB;
   do_it("profiling triangle 3D [grgb]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[2] = ct;

   type3d = POLYTYPE_ATEX;
   do_it("profiling triangle 3D [atex]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[3] = ct;

   type3d = POLYTYPE_PTEX;
   do_it("profiling triangle 3D [ptex]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[4] = ct;

   type3d = POLYTYPE_ATEX_MASK;
   do_it("profiling triangle 3D [atex mask]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[5] = ct;

   type3d = POLYTYPE_PTEX_MASK;
   do_it("profiling triangle 3D [ptex mask]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[6] = ct;

   type3d = POLYTYPE_ATEX_LIT;
   do_it("profiling triangle 3D [atex lit]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[7] = ct;

   type3d = POLYTYPE_PTEX_LIT;
   do_it("profiling triangle 3D [ptex lit]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[8] = ct;

   type3d = POLYTYPE_ATEX_MASK_LIT;
   do_it("profiling triangle 3D [atex mask lit]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[9] = ct;

   type3d = POLYTYPE_PTEX_MASK_LIT;
   do_it("profiling triangle 3D [ptex mask lit]", FALSE, triangle3d_demo);
   if (ct < 0) 
      goto abort;
   else 
      tims[10] = ct;

   profile = FALSE;
   return 0;

   abort:
   profile = FALSE;
   return -1;
}



int p3d_profile_proc(void)
{
   int scr_0_tims[11], scr_1_tims[11], mem_0_tims[11], mem_1_tims[11];
   int *tims = scr_0_tims;
   int old_cpu_capabilities = cpu_capabilities;
   BITMAP *old_screen = screen;
   BITMAP *buffer;
   static char fname[80*6] = EMPTY_STRING; /* 80 chars * max UTF8 char width */
   FILE *f;
   int i;

   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   show_mouse(NULL);

   cpu_capabilities &= ~CPU_MMX;

   if (do_p3d_profile(scr_0_tims)) 
      goto abort;

   if (old_cpu_capabilities & CPU_MMX) {
      cpu_capabilities |= CPU_MMX;
      if (do_p3d_profile(scr_1_tims)) 
	 goto abort;
   }

   clear_to_color(screen, palette_color[0]);
   textout_centre_ex(screen, font, "Profiling 3D memory rendering", SCREEN_W/2, SCREEN_H/2-16, palette_color[255], palette_color[0]);
   textout_centre_ex(screen, font, "This will take a few minutes...", SCREEN_W/2, SCREEN_H/2+8, palette_color[255], palette_color[0]);

   screen = buffer;
   cpu_capabilities &= ~CPU_MMX;
   if (do_p3d_profile(mem_0_tims)) 
      goto abort;

   if (old_cpu_capabilities & CPU_MMX) {
      cpu_capabilities |= CPU_MMX;
      if (do_p3d_profile(mem_1_tims)) 
	 goto abort;
   }

   screen = old_screen;
   clear_to_color(screen, palette_color[0]);
   show_mouse(screen);

   if (file_select_ex("Save profile log", fname, NULL, sizeof(fname), 0, 0)) {
      if (exists(fname))
	 if (alert(fname, "already exists: overwrite?", NULL, "OK", "Cancel", 13, 27) == 2)
	    goto abort;

      f = fopen(fname, "wt");
      if (!f) {
	 alert("Error writing", fname, NULL, "OK", NULL, 13, 0);
      }
      else {
	 fprintf(f, "%s %sprofile results (3D rendering)\n\n", allegro_version_str, internal_version_str);

	 fprintf(f, "Graphics driver: %s\n", gfx_driver->name);
	 fprintf(f, "Description: %s\n\n", gfx_driver->desc);
	 fprintf(f, "Screen size: %dx%d\n", SCREEN_W, SCREEN_H);
	 fprintf(f, "Virtual screen size: %dx%d\n", VIRTUAL_W, VIRTUAL_H);
	 fprintf(f, "Color depth: %d bpp\n\n\n", bitmap_color_depth(screen));

	 for (i=0; i<4; i++) {
	    switch (i) {

	       case 0:
		  tims = scr_0_tims;
		  fprintf(f, "Screen profile results, no MMX:\n\n");
		  break;

	       case 1:
		  if (!(old_cpu_capabilities & CPU_MMX)) 
		     continue;
		  tims = scr_1_tims;
		  fprintf(f, "Screen profile results, using MMX:\n\n");
		  break;

	       case 2:
		  tims = mem_0_tims;
		  fprintf(f, "Memory profile results, no MMX:\n\n");
		  break;

	       case 3:
		  if (!(old_cpu_capabilities & CPU_MMX)) 
		     continue;
		  tims = mem_1_tims;
		  fprintf(f, "Memory profile results, using MMX:\n\n");
		  break;
	    }

	    fprintf(f, "    flat           - %d\n", tims[0]/TIME_SPEED);
	    fprintf(f, "    gcol           - %d\n", tims[1]/TIME_SPEED);
	    fprintf(f, "    grgb           - %d\n", tims[2]/TIME_SPEED);
	    fprintf(f, "    atex           - %d\n", tims[3]/TIME_SPEED);
	    fprintf(f, "    ptex           - %d\n", tims[4]/TIME_SPEED);
	    fprintf(f, "    atex mask      - %d\n", tims[5]/TIME_SPEED);
	    fprintf(f, "    ptex mask      - %d\n", tims[6]/TIME_SPEED);
	    fprintf(f, "    atex lit       - %d\n", tims[7]/TIME_SPEED);
	    fprintf(f, "    ptex lit       - %d\n", tims[8]/TIME_SPEED);
	    fprintf(f, "    atex mask lit  - %d\n", tims[9]/TIME_SPEED);
	    fprintf(f, "    ptex mask lit  - %d\n", tims[10]/TIME_SPEED);
	    fprintf(f, "\n\n");
	 }

	 fclose(f);
      }
   }

   abort:
   screen = old_screen;
   destroy_bitmap(buffer);
   
   cpu_capabilities = old_cpu_capabilities;
   show_mouse(screen);
   return D_REDRAW;
}



int profile_proc(void)
{
   show_mouse(NULL);
   do_profile(NULL);
   show_mouse(screen);
   return D_REDRAW;
}



int mem_profile_proc(void)
{
   BITMAP *old_screen = screen;
   BITMAP *buffer = create_bitmap(SCREEN_W, SCREEN_H);

   show_mouse(NULL);
   clear_to_color(screen, palette_color[0]);

   textout_centre_ex(screen, font, "Profiling memory bitmap routines", SCREEN_W/2, SCREEN_H/2-32, palette_color[255], palette_color[0]);
   textout_centre_ex(screen, font, "This will take a few minutes, so you", SCREEN_W/2, SCREEN_H/2-8, palette_color[255], palette_color[0]);
   textout_centre_ex(screen, font, "may wish to go make a cup of coffee,", SCREEN_W/2, SCREEN_H/2+4, palette_color[255], palette_color[0]);
   textout_centre_ex(screen, font, "watch some TV, read a book, or think", SCREEN_W/2, SCREEN_H/2+16, palette_color[255], palette_color[0]);
   textout_centre_ex(screen, font, "of something interesting to do.", SCREEN_W/2, SCREEN_H/2+28, palette_color[255], palette_color[0]);

   screen = buffer;

   do_profile(old_screen);

   screen = old_screen;
   destroy_bitmap(buffer);

   show_mouse(screen);

   return D_REDRAW;
}



int stretch_proc(void)
{
   scare_mouse();
   stretch_test();
   unscare_mouse();
   return D_REDRAW;
}



int hscroll_proc(void)
{
   show_mouse(NULL);
   hscroll_test();
   show_mouse(screen);
   return D_REDRAW;
}



int misc_proc(void)
{
   scare_mouse();
   misc();
   unscare_mouse();
   return D_REDRAW;
}



int rainbow_proc(void)
{
   scare_mouse();
   rainbow();
   unscare_mouse();
   return D_REDRAW;
}



int caps_proc(void)
{
   scare_mouse();
   caps();
   unscare_mouse();
   return D_REDRAW;
}



int interrupts_proc(void)
{
   scare_mouse();
   interrupt_test();
   unscare_mouse();
   return D_REDRAW;
}



int quit_proc(void)
{
   return D_CLOSE;
}



void set_mode_str(void)
{
   extern MENU mode_menu[];

   static char *mode_name[] =
   {
      "solid",
      "xor",
      "copy pattern",
      "solid pattern",
      "masked pattern",
      "translucent"
   };

   int i;

   sprintf(mode_string, "&Drawing mode (%s)", mode_name[mode]);

   for (i=0; mode_menu[i].proc; i++)
      mode_menu[i].flags = 0;

   mode_menu[mode].flags = D_SELECTED;
}



int solid_proc(void)
{
   mode = DRAW_MODE_SOLID;
   set_mode_str();
   return D_O_K;
}



int xor_proc(void)
{
   mode = DRAW_MODE_XOR;
   set_mode_str();
   return D_O_K;
}



int copy_pat_proc(void)
{
   mode = DRAW_MODE_COPY_PATTERN;
   set_mode_str();
   return D_O_K;
}



int solid_pat_proc(void)
{
   mode = DRAW_MODE_SOLID_PATTERN;
   set_mode_str();
   return D_O_K;
}



int masked_pat_proc(void)
{
   mode = DRAW_MODE_MASKED_PATTERN;
   set_mode_str();
   return D_O_K;
}



int trans_proc(void)
{
   mode = DRAW_MODE_TRANS;
   set_mode_str();

   return check_tables();
}



int mmx_auto_proc(void)
{
   extern MENU mmx_menu[];

   cpu_capabilities = cpu_has_capabilities;

   mmx_menu[0].flags = D_SELECTED;
   mmx_menu[1].flags = 0;
   mmx_menu[2].flags = 0;
   mmx_menu[3].flags = 0;

   return D_O_K;
}



int toggle_3dnow_proc(void)
{
   extern MENU mmx_menu[];

   cpu_capabilities ^= CPU_3DNOW | CPU_ENH3DNOW;
   cpu_capabilities &= cpu_has_capabilities;

   mmx_menu[0].flags = 0;
   mmx_menu[1].flags ^= D_SELECTED;

   if (((mmx_menu[1].flags | mmx_menu[2].flags | mmx_menu[3].flags) & D_SELECTED) == 0)
      mmx_menu[0].flags = D_SELECTED;

   return D_O_K;
}



int toggle_mmx_proc(void)
{
   extern MENU mmx_menu[];

   cpu_capabilities ^= CPU_MMX | CPU_MMXPLUS;
   cpu_capabilities &= cpu_has_capabilities;

   mmx_menu[0].flags = 0;
   mmx_menu[2].flags ^= D_SELECTED;

   if (((mmx_menu[1].flags | mmx_menu[2].flags | mmx_menu[3].flags) & D_SELECTED) == 0)
      mmx_menu[0].flags = D_SELECTED;

   return D_O_K;
}


int toggle_sse_proc(void)
{
   extern MENU mmx_menu[];

   cpu_capabilities ^= CPU_SSE | CPU_SSE2;
   cpu_capabilities &= cpu_has_capabilities;

   mmx_menu[0].flags = 0;
   mmx_menu[3].flags ^= D_SELECTED;

   if (((mmx_menu[1].flags | mmx_menu[2].flags | mmx_menu[3].flags) & D_SELECTED) == 0)
      mmx_menu[0].flags = D_SELECTED;

   return D_O_K;
}


void set_switch_mode_menu(int mode)
{
   extern MENU switch_menu[];
   int i;

   if (set_display_switch_mode(mode) == 0) {
      for (i=0; i<5; i++)
         switch_menu[i].flags = 0;

      switch_menu[mode-SWITCH_NONE].flags = D_SELECTED;
   }
   else
      alert("Error setting switch mode:", "not supported by this graphics driver", NULL, "Sorry", NULL, 13, 0);
}



int sw_none_proc(void)
{
   set_switch_mode_menu(SWITCH_NONE);
   return D_O_K;
}



int sw_pause_proc(void)
{
   set_switch_mode_menu(SWITCH_PAUSE);
   return D_O_K;
}



int sw_amnesia_proc(void)
{
   set_switch_mode_menu(SWITCH_AMNESIA);
   return D_O_K;
}



int sw_backgr_proc(void)
{
   set_switch_mode_menu(SWITCH_BACKGROUND);
   return D_O_K;
}



int sw_backamn_proc(void)
{
   set_switch_mode_menu(SWITCH_BACKAMNESIA);
   return D_O_K;
}



int gfx_mode_proc(void)
{
   int gfx_mode(void);

   show_mouse(NULL);
   clear_to_color(screen, palette_color[0]);
   gfx_mode();
   show_mouse(screen);

   return D_REDRAW;
}



int refresh_proc(void)
{
   void refresh_rate(void);

   show_mouse(NULL);
   clear_to_color(screen, palette_color[0]);
   refresh_rate();
   show_mouse(screen);

   return D_REDRAW;
}



MENU mode_menu[] =
{
   { "&Solid",                   solid_proc,       NULL,    D_SELECTED,    NULL  },
   { "&XOR",                     xor_proc,         NULL,    0,             NULL  },
   { "&Copy pattern",            copy_pat_proc,    NULL,    0,             NULL  },
   { "Solid &pattern",           solid_pat_proc,   NULL,    0,             NULL  },
   { "&Masked pattern",          masked_pat_proc,  NULL,    0,             NULL  },
   { "&Translucent",             trans_proc,       NULL,    0,             NULL  },
   { NULL,                       NULL,             NULL,    0,             NULL  }
};



MENU mmx_menu[] =
{
   { "&Autodetect",              mmx_auto_proc,    NULL,    D_SELECTED,    NULL  },
   { "&Disable 3DNow!/Enh3DNow!", toggle_3dnow_proc,NULL,    0,             NULL  },
   { "&Disable MMX/MMX+",         toggle_mmx_proc,  NULL,    0,             NULL  },
   { "&Disable SSE/SSE2",         toggle_sse_proc,  NULL,    0,             NULL  },
   { NULL,                       NULL,             NULL,    0,             NULL  }
};



MENU switch_menu[] =
{
   { "&None",                    sw_none_proc,     NULL,    D_SELECTED,    NULL  },
   { "&Pause",                   sw_pause_proc,    NULL,    0,             NULL  },
   { "&Amnesia",                 sw_amnesia_proc,  NULL,    0,             NULL  },
   { "&Background",              sw_backgr_proc,   NULL,    0,             NULL  },
   { "Backa&mnesia",             sw_backamn_proc,  NULL,    0,             NULL  },
   { NULL,                       NULL,             NULL,    0,             NULL  }
};



MENU test_menu[] =
{
   { "&Graphics mode",           gfx_mode_proc,    NULL,       0,          NULL  },
   { "&Refresh rate",            refresh_proc,     NULL,       0,          NULL  },
   { mode_string,                NULL,             mode_menu,  0,          NULL  },
 #ifdef ALLEGRO_I386
   { "MM&X mode",                NULL,             mmx_menu,   0,          NULL  },
 #endif
   { "&Switch mode",             NULL,             switch_menu,0,          NULL  },
   { "",                         NULL,             NULL,       0,          NULL  },
   { "&Profile Screen",          profile_proc,     NULL,       0,          NULL  },
   { "Profile &Memory",          mem_profile_proc, NULL,       0,          NULL  },
   { "Profile &3D",              p3d_profile_proc, NULL,       0,          NULL  },
   { "",                         NULL,             NULL,       0,          NULL  },
   { "&Quit",                    quit_proc,        NULL,       0,          NULL  },
   { NULL,                       NULL,             NULL,       0,          NULL  }
};



MENU primitives_menu[] =
{
   { "&putpixel()",              putpixel_proc,    NULL, 0, NULL  },
   { "&hline()",                 hline_proc,       NULL, 0, NULL  },
   { "&vline()",                 vline_proc,       NULL, 0, NULL  },
   { "&line()",                  line_proc,        NULL, 0, NULL  },
   { "&rectfill()",              rectfill_proc,    NULL, 0, NULL  },
   { "&circle()",                circle_proc,      NULL, 0, NULL  },
   { "c&irclefill()",            circlefill_proc,  NULL, 0, NULL  },
   { "&ellipse()",               ellipse_proc,     NULL, 0, NULL  },
   { "ellip&sefill()",           ellipsefill_proc, NULL, 0, NULL  },
   { "&arc()",                   arc_proc,         NULL, 0, NULL  },
   { "&triangle()",              triangle_proc,    NULL, 0, NULL  },
   { "triangle&3d()",            triangle3d_proc,  NULL, 0, NULL  },
   { NULL,                       NULL,             NULL, 0, NULL  }
};



MENU blitter_menu[] =
{
   { "&textout()",               textout_proc,     NULL, 0, NULL  },
   { "&blit()",                  blit_proc,        NULL, 0, NULL  },
   { "&stretch_blit()",          stretch_proc,     NULL, 0, NULL  },
   { "&draw_sprite()",           sprite_proc,      NULL, 0, NULL  },
   { "draw_tr&ans_sprite()",     xlu_sprite_proc,  NULL, 0, NULL  },
   { "draw_&lit_sprite()",       lit_sprite_proc,  NULL, 0, NULL  },
   { "&rotate_sprite()",         rotate_proc,      NULL, 0, NULL  },
   { NULL,                       NULL,             NULL, 0, NULL  }
};



MENU interactive_menu[] =
{
   { "&getpixel()",              getpixel_proc,    NULL, 0, NULL  },
   { "&polygon()",               polygon_proc,     NULL, 0, NULL  },
   { "polygon&3d()",             polygon3d_proc,   NULL, 0, NULL  },
   { "&floodfill()",             floodfill_proc,   NULL, 0, NULL  },
   { "&spline()",                spline_proc,      NULL, 0, NULL  },
   { NULL,                       NULL,             NULL, 0, NULL  }
};



MENU gfx_menu[] =
{
   { "&Primitives",              NULL,             primitives_menu,  0, NULL  },
   { "&Blitting functions",      NULL,             blitter_menu,     0, NULL  },
   { "&Interactive tests",       NULL,             interactive_menu, 0, NULL  },
   { NULL,                       NULL,             NULL,             0, NULL  }
};



MENU io_menu[] =
{
   { "&Mouse",                   mouse_proc,       NULL, 0, NULL  },
   { "&Keyboard",                keyboard_proc,    NULL, 0, NULL  },
   { "&Timers",                  interrupts_proc,  NULL, 0, NULL  },
   { NULL,                       NULL,             NULL, 0, NULL  }
};



MENU misc_menu[] =
{
   { "&Scrolling",               hscroll_proc,     NULL, 0, NULL  },
   { "&Time some stuff",         misc_proc,        NULL, 0, NULL  },
   { "&Color rainbows",          rainbow_proc,     NULL, 0, NULL  },
   { "&Accelerated features",    caps_proc,        NULL, 0, NULL  },
   { NULL,                       NULL,             NULL, 0, NULL  }
};



MENU menu[] =
{
   { "&Test",                    NULL,             test_menu,  0, NULL  },
   { "&Graphics",                NULL,             gfx_menu,   0, NULL  },
   { "&I/O",                     NULL,             io_menu,    0, NULL  },
   { "&Misc",                    NULL,             misc_menu,  0, NULL  },
   { NULL,                       NULL,             NULL,       0, NULL  }
};



DIALOG title_screen[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,          NULL, NULL  },
   { d_menu_proc,       0,    0,    0,    0,    255,  0,    0,    0,       0,    0,    menu,          NULL, NULL  },
   { d_ctext_proc,      0,    0,    0,    0,    255,  0,    0,    0,       0,    0,    allegro_version_str, NULL, NULL },
   { d_ctext_proc,      0,    8,    0,    0,    255,  0,    0,    0,       0,    0,    internal_version_str, NULL, NULL  },
   { d_ctext_proc,      0,    24,   0,    0,    255,  0,    0,    0,       0,    0,    "By Shawn Hargreaves, " ALLEGRO_DATE_STR, NULL, NULL },
   { d_ctext_proc,      0,    64,   0,    0,    255,  0,    0,    0,       0,    0,    "",            NULL, NULL  },
   { d_ctext_proc,      0,    80,   0,    0,    255,  0,    0,    0,       0,    0,    "",            NULL, NULL  },
   { d_ctext_proc,      0,    96,   0,    0,    255,  0,    0,    0,       0,    0,    gfx_specs,     NULL, NULL  },
   { d_ctext_proc,      0,    112,  0,    0,    255,  0,    0,    0,       0,    0,    gfx_specs2,    NULL, NULL  },
   { d_ctext_proc,      0,    128,  0,    0,    255,  0,    0,    0,       0,    0,    gfx_specs3,    NULL, NULL  },
   { d_ctext_proc,      0,    160,  0,    0,    255,  0,    0,    0,       0,    0,    mouse_specs,   NULL, NULL  },
   { d_ctext_proc,      0,    192,  0,    0,    255,  0,    0,    0,       0,    0,    cpu_specs,     NULL, NULL  },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,          NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,          NULL, NULL  }
};

#define DIALOG_NAME     5
#define DIALOG_DESC     6



void change_mode(void)
{
   int c;

   show_mouse(NULL);

   if (realscreen) {
      BITMAP *b = realscreen;
      realscreen = NULL;
      destroy_bitmap(screen);
      screen = b;
   }

   /* try to set a wide virtual screen... */
   set_color_depth(gfx_bpp);
   request_refresh_rate(gfx_rate);

   if (set_gfx_mode(gfx_card, gfx_w, gfx_h, (gfx_w >= 512) ? 1024 : 512, (gfx_w >= 512) ? 1024 : 512) != 0) {
      if (set_gfx_mode(gfx_card, gfx_w, gfx_h, (gfx_w >= 512) ? 1024 : 512, 0) != 0) {
	 if (set_gfx_mode(gfx_card, gfx_w, gfx_h, 0, 0) != 0) {

	    if ((gfx_card == GFX_AUTODETECT) && (gfx_bpp > 8) && (gfx_w == 640) && (gfx_h == 480)) {
	       set_color_depth(8);
	       request_refresh_rate(0);
	       if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0)!=0) {
		  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		  allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
		  abort();
	       }
	       set_palette(mypal);

	       gui_fg_color = palette_color[255];
	       gui_mg_color = palette_color[8];
	       gui_bg_color = palette_color[0];

	       if (alert("Error setting mode. Do you want to", "emulate it? (very unstable and slow,", "intended only for Shawn's debugging)", "Go for it...", "Cancel", 13, 27) == 1) {
		  BITMAP *b;
		  set_color_depth(32);
		  set_gfx_mode(GFX_AUTODETECT, gfx_w, gfx_h, 0, 0);
		  set_palette(mypal);
		  set_color_depth(gfx_bpp);
		  b = screen;
		  screen = create_bitmap(gfx_w, gfx_h);
		  realscreen = b;
	       }
	    }
	    else {
	       char buf[ALLEGRO_ERROR_SIZE];
	       ustrzcpy(buf, sizeof(buf), allegro_error);
	       set_color_depth(8);
	       request_refresh_rate(0);
	       if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0)!=0) {
		  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		  allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
		  abort();
	       }
	       set_palette(mypal);

	       gui_fg_color = palette_color[255];
	       gui_mg_color = palette_color[8];
	       gui_bg_color = palette_color[0];

	       alert("Error setting mode:", buf, NULL, "Sorry", NULL, 13, 0);
	    }
	 }
      }
   }

   set_palette(mypal);

   if (mode == DRAW_MODE_TRANS)
      check_tables();

   xoff = (SCREEN_W - 320) / 2;
   yoff = (SCREEN_H - 200) / 2;

   sprintf(gfx_specs, "%dx%d (%dx%d), %ldk vram", 
	   SCREEN_W, SCREEN_H, VIRTUAL_W, VIRTUAL_H, gfx_driver->vid_mem/1024);

   if (!gfx_driver->linear)
      sprintf(gfx_specs+strlen(gfx_specs), ", %ldk banks, %ldk granularity",
	      gfx_driver->bank_size/1024, gfx_driver->bank_gran/1024);

   c = get_refresh_rate();
   if (c > 0)
      sprintf(gfx_specs+strlen(gfx_specs), ", %d hz", c);

   if (gfx_rate > 0)
      sprintf(gfx_specs+strlen(gfx_specs), " (req. %d hz)", gfx_rate);

   switch (bitmap_color_depth(screen)) {

      case 8:
	 strcpy(gfx_specs2, "8 bit (256 color)");
	 break;

      case 15:
	 strcpy(gfx_specs2, "15 bit (32K HiColor)");
	 break;

      case 16:
	 strcpy(gfx_specs2, "16 bit (64K HiColor)");
	 break;

      case 24:
	 strcpy(gfx_specs2, "24 bit (16M TrueColor)");
	 break;

      case 32:
	 strcpy(gfx_specs2, "32 bit (16M TrueColor)");
	 break;

      default:
	 strcpy(gfx_specs2, "Unknown color depth!");
	 break;
   }

   strcpy(gfx_specs3, "Capabilities: ");
   c = 0;

   if (gfx_capabilities & GFX_CAN_SCROLL) {
      strcat(gfx_specs3, "scroll");
      c++;
   }

   if (gfx_capabilities & GFX_CAN_TRIPLE_BUFFER) {
      if (c)
	 strcat(gfx_specs3, ", ");
      strcat(gfx_specs3, "triple buffer");
      c++;
   }

   show_mouse(screen);

   if (gfx_capabilities & GFX_HW_CURSOR) {
      if (c)
	 strcat(gfx_specs3, ", ");
      strcat(gfx_specs3, "hardware cursor");
      c++;
   }

   show_mouse(NULL);

   if (gfx_capabilities & ~(GFX_CAN_SCROLL | GFX_CAN_TRIPLE_BUFFER | GFX_HW_CURSOR)) {
      if (c)
	 strcat(gfx_specs3, ", ");
      strcat(gfx_specs3, "hardware acceleration");
      c++;
   }

   if (!c)
      strcat(gfx_specs3, "0");

   set_switch_mode_menu(get_display_switch_mode());

   if (global_sprite) {
      destroy_bitmap(global_sprite);
      destroy_rle_sprite(global_rle_sprite);

      for (c=0; c<NUM_PATTERNS; c++)
	 destroy_bitmap(pattern[c]);
   }

   make_patterns();

   global_sprite = make_sprite();
   global_rle_sprite = get_rle_sprite(global_sprite);

   gui_fg_color = palette_color[255];
   gui_mg_color = palette_color[8];
   gui_bg_color = palette_color[0];

   title_screen[DIALOG_NAME].dp = (void*)gfx_driver->name;
   title_screen[DIALOG_DESC].dp = (void*)gfx_driver->desc;
   centre_dialog(title_screen+2);
   set_dialog_color(title_screen, gui_fg_color, gui_bg_color);

   show_mouse(screen);
}



int gfx_mode(void)
{
   gfx_w = SCREEN_W;
   gfx_h = SCREEN_H;
   gfx_bpp = bitmap_color_depth(screen);
   if (!gfx_mode_select_ex(&gfx_card, &gfx_w, &gfx_h, &gfx_bpp))
      return -1;

   change_mode();

   return 0;
}



char *refresh_getter(int index, int *list_size)
{
   static char *list[] =
   {
      "Default",
      "50 Hz",
      "60 Hz",
      "70 Hz",
      "75 Hz",
      "80 Hz",
      "85 Hz",
      "90 Hz",
      "100 Hz",
      "120 Hz"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = 10;
      return NULL;
   }

   return list[index];
}



DIALOG refresh_dlg[] =
{
   /* (dialog proc)     (x)   (y)  (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)            (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,   229,  111,  0,    0,    0,    0,       0,    0,    NULL,           NULL, NULL  },
   { d_ctext_proc,      114,  8,   1,    1,    0,    0,    0,    0,       0,    0,    "Refresh Rate", NULL, NULL  },
   { d_button_proc,     132,  40,  81,   17,   0,    0,    0,    D_EXIT,  0,    0,    "OK",           NULL, NULL  },
   { d_button_proc,     132,  64,  81,   17,   0,    0,    27,   D_EXIT,  0,    0,    "Cancel",       NULL, NULL  },
   { d_list_proc,       16,   28,  101,  68,   0,    0,    0,    D_EXIT,  0,    0,    (void *)refresh_getter, NULL, NULL  },
   { d_yield_proc,      0,    0,   0,    0,    0,    0,    0,    0,       0,    0,    NULL,           NULL, NULL  },
   { NULL,              0,    0,   0,    0,    0,    0,    0,    0,       0,    0,    NULL,           NULL, NULL  }
};

#define REFRESH_OK      2
#define REFRESH_CANCEL  3
#define REFRESH_LIST    4



void refresh_rate(void)
{
   int old_sel = refresh_dlg[REFRESH_LIST].d1;

   centre_dialog(refresh_dlg);
   set_dialog_color(refresh_dlg, gui_fg_color, gui_bg_color);

   if (do_dialog(refresh_dlg, REFRESH_LIST) != REFRESH_CANCEL) {
      static int table[] =
      {
	 0, 50, 60, 70, 75, 80, 85, 90, 100, 120
      };

      gfx_rate = table[refresh_dlg[REFRESH_LIST].d1];

      change_mode();
   }
   else {
      refresh_dlg[REFRESH_LIST].d1 = old_sel;
   }
}



char *get_allegro_build_string(void)
{
   /* No need to print whether or not ALLEGRO_STATICLINK is defined.
    * This is implied by the presence of ".s" in ALLEGRO_PLATFORM_STR.
    */

   #ifdef DEBUGMODE
      return "DEBUG build";
   #else
   #ifdef PROFILEMODE
      return "PROFILE build";
   #else
      return "RELEASE build";
   #endif
   #endif
}



int main(void)
{
   int buttons;
   int c;

   LOCK_FUNCTION(tm_tick);
   LOCK_VARIABLE(tm);
   LOCK_VARIABLE(_tm);
   LOCK_VARIABLE(realscreen);
   
   if (allegro_init() != 0)
      return 1;

   for (c=0; c<32; c++)
      mypal[c] = desktop_palette[c];

   for (c=0; c<32; c++) {
      mypal[c+32].r = c*2;
      mypal[c+32].g = mypal[c+32].b = 0;
   }

   for (c=0; c<32; c++) {
      mypal[c+64].g = c*2;
      mypal[c+64].r = mypal[c+64].b = 0;
   }

   for (c=0; c<32; c++) {
      mypal[c+96].b = c*2;
      mypal[c+96].r = mypal[c+96].g = 0;
   }

   for (c=0; c<32; c++) {
      mypal[c+128].r = mypal[c+128].g = c*2;
      mypal[c+128].b = 0;
   }

   for (c=0; c<32; c++) {
      mypal[c+160].r = mypal[c+160].b = c*2;
      mypal[c+160].g = 0;
   }

   for (c=0; c<32; c++) {
      mypal[c+192].g = mypal[c+192].b = c*2;
      mypal[c+192].r = 0;
   }

   for (c=0; c<31; c++)
      mypal[c+224].r = mypal[c+224].g = mypal[c+224].b = c*2;

   mypal[255].r = mypal[255].g = mypal[255].b = 0;

   buttons = install_mouse();
   enable_hardware_cursor();
   sprintf(mouse_specs, "Mouse has %d buttons", buttons);

   #if defined ALLEGRO_I386 || defined ALLEGRO_AMD64

      #ifdef ALLEGRO_I386
         sprintf(cpu_specs, "CPU family: %d86 (%s)",
                 cpu_family,
                 (ustrlen(cpu_vendor) > 0) ? cpu_vendor : "unknown");
      #else
         sprintf(cpu_specs, "CPU family: AMD64 (%s)", cpu_vendor);
      #endif

   #else
      #if defined ALLEGRO_MACOSX
         sprintf(cpu_specs, "CPU family: %s (%s)", 
                 cpu_family==CPU_FAMILY_POWERPC ? "PowerPC" : "Other", 
                 cpu_vendor);
      #else
         strcpy(cpu_specs, "Non-x86 CPU (very cool)");
      #endif
   #endif

   if (cpu_capabilities & CPU_ID)
      strcat(cpu_specs, " / cpuid");

   if (cpu_capabilities & CPU_FPU)
      strcat(cpu_specs, " / FPU");

   if (cpu_capabilities & CPU_CMOV)
      strcat(cpu_specs, " / cmov");

   if (cpu_capabilities & CPU_SSE)
      strcat(cpu_specs, " / SSE");

   if (cpu_capabilities & CPU_SSE2)
      strcat(cpu_specs, " / SSE2");

   if (cpu_capabilities & CPU_SSE3)
      strcat(cpu_specs, " / SSE3");

   if (cpu_capabilities & CPU_MMX)
      strcat(cpu_specs, " / MMX");

   if (cpu_capabilities & CPU_MMXPLUS)
      strcat(cpu_specs, " / MMX+");

   if (cpu_capabilities & CPU_3DNOW)
      strcat(cpu_specs, " / 3DNow!");

   if (cpu_capabilities & CPU_ENH3DNOW)
      strcat(cpu_specs, " / Enh 3DNow!");

   cpu_has_capabilities = cpu_capabilities;

   install_keyboard();
   install_timer();
   install_int(tm_tick, 10);

   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0)!=0) {
     set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
     allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
     return 1;
   }
   set_palette(mypal);

   if (gfx_mode() != 0) {
      allegro_exit();
      return 0;
   }

   set_mode_str();


   sprintf(allegro_version_str, "Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR " %s", get_allegro_build_string());

   /* If using a dynamically linked version of Allegro, the version may be different. */
   #ifdef ALLEGRO_STATICLINK
      sprintf(internal_version_str, "");
   #else
      sprintf(internal_version_str, "(linked: %s) ", allegro_id);
   #endif

   do_dialog(title_screen, -1);

   destroy_bitmap(global_sprite);
   destroy_rle_sprite(global_rle_sprite);

   for (c=0; c<NUM_PATTERNS; c++)
      destroy_bitmap(pattern[c]);

   if (rgb_map)
      free(rgb_map);

   if (trans_map)
      free(trans_map);

   if (light_map)
      free(light_map);

   if (realscreen) {
      BITMAP *b = realscreen;
      realscreen = NULL;
      destroy_bitmap(screen);
      screen = b;
   }

   return 0;
}

END_OF_MAIN()
