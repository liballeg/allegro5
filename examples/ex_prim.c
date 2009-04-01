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
 *      A sampler of the primitive addon.
 *      All primitives are rendered using the additive blender, so overdraw will manifest itself as overly bright pixels.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <allegro5/a5_iio.h>
#include "allegro5/a5_primitives.h"
#include <math.h>
#include <stdio.h>

typedef void (*Screen)(int);
int ScreenW = 800, ScreenH = 600;
#define NUM_SCREENS 8
#define ROTATE_SPEED 0.0001f
Screen Screens[NUM_SCREENS];
ALLEGRO_FONT* Font;
ALLEGRO_TRANSFORM Identity;
ALLEGRO_BITMAP* Buffer;

int Soft = 0;
int Blend = 1;
float Speed = ROTATE_SPEED;
float Theta;
int Background = 1;
float Thickness = 0;
ALLEGRO_TRANSFORM MainTrans;

enum MODE {
   INIT,
   LOGIC,
   DRAW
};

void FilledPrimitives(int mode)
{
   static ALLEGRO_VBUFFER* vbuff;
   if (mode == INIT) {
      int ii = 0;
      vbuff = al_create_vbuff(21, 0);
      for (ii = 0; ii < 21; ii++) {
         float x, y;
         ALLEGRO_COLOR color;
         if (ii % 2 == 0) {
            x = 150 * cosf((float)ii / 20 * 2 * ALLEGRO_PI);
            y = 150 * sinf((float)ii / 20 * 2 * ALLEGRO_PI);
         } else {
            x = 200 * cosf((float)ii / 20 * 2 * ALLEGRO_PI);
            y = 200 * sinf((float)ii / 20 * 2 * ALLEGRO_PI);
         }
         
         if (ii == 0) {
            x = y = 0;
         }
         
         color = al_map_rgb((7 * ii + 1) % 3 * 64, (2 * ii + 2) % 3 * 64, (ii) % 3 * 64);
         
         al_set_vbuff_pos(vbuff, ii, x, y, 0);
         al_set_vbuff_color(vbuff, ii, color);
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(1, 1, 1, 1));
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "Low Level Filled Primitives");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, al_map_rgba_f(1, 1, 1, 1));
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_prim(vbuff, 0, 0, 6, ALLEGRO_PRIM_TRIANGLE_FAN);
      al_draw_prim(vbuff, 0, 7, 13, ALLEGRO_PRIM_TRIANGLE_LIST);
      al_draw_prim(vbuff, 0, 14, 20, ALLEGRO_PRIM_TRIANGLE_STRIP);
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}

void IndexedFilledPrimitives(int mode)
{
   static ALLEGRO_VBUFFER* vbuff;
   static int indices1[] = {12, 13, 14, 16, 17, 18};
   static int indices2[] = {6, 7, 8, 9, 10, 11};
   static int indices3[] = {0, 1, 2, 3, 4, 5};
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_COLOR color;
      vbuff = al_create_vbuff(21, 0);
      for (ii = 0; ii < 21; ii++) {
         float x, y;
         if (ii % 2 == 0) {
            x = 150 * cosf((float)ii / 20 * 2 * ALLEGRO_PI);
            y = 150 * sinf((float)ii / 20 * 2 * ALLEGRO_PI);
         } else {
            x = 200 * cosf((float)ii / 20 * 2 * ALLEGRO_PI);
            y = 200 * sinf((float)ii / 20 * 2 * ALLEGRO_PI);
         }
         
         if (ii == 0) {
            x = y = 0;
         }
         
         color = al_map_rgb((7 * ii + 1) % 3 * 64, (2 * ii + 2) % 3 * 64, (ii) % 3 * 64);
         
         al_set_vbuff_pos(vbuff, ii, x, y, 0);
         al_set_vbuff_color(vbuff, ii, color);
      }
   } else if (mode == LOGIC) {
      int ii;
      Theta += Speed;
      for (ii = 0; ii < 6; ii++) {
         indices1[ii] = ((int)al_current_time() + ii) % 20 + 1;
         indices2[ii] = ((int)al_current_time() + ii + 6) % 20 + 1;
         if (ii > 0)
            indices3[ii] = ((int)al_current_time() + ii + 12) % 20 + 1;
      }
      
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(1, 1, 1, 1));
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "Indexed Filled Primitives");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, al_map_rgba_f(1, 1, 1, 1));
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_indexed_prim(vbuff, 0, indices1, 6, ALLEGRO_PRIM_TRIANGLE_LIST);
      al_draw_indexed_prim(vbuff, 0, indices2, 6, ALLEGRO_PRIM_TRIANGLE_STRIP);
      al_draw_indexed_prim(vbuff, 0, indices3, 6, ALLEGRO_PRIM_TRIANGLE_FAN);
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}

void HighPrimitives(int mode)
{
   if (mode == INIT) {
   
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      float points[8] = {
         -300, -200,
         700, 200,
         -700, 200,
         300, -200
      };

      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(1, 1, 1, 1));
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "High Level Primitives");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, al_map_rgba_f(1, 1, 1, 1));
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_line(-300, -200, 300, 200, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_triangle(-150, -250, 0, 250, 150, -250, al_map_rgba_f(0.5, 0, 0.5, 1), Thickness);
      al_draw_rectangle(-300, -200, 300, 200, al_map_rgba_f(0.5, 0, 0, 1), Thickness);
      
      al_draw_ellipse(0, 0, 300, 150, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_arc(0, 0, 200, -ALLEGRO_PI / 2, ALLEGRO_PI, al_map_rgba_f(0.5, 0.25, 0, 1), Thickness);
      al_draw_spline(points, al_map_rgba_f(0.1, 0.2, 0.5, 1), Thickness);
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}

void HighFilledPrimitives(int mode)
{
   if (mode == INIT) {
   
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(1, 1, 1, 1));
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "High Level Filled Primitives");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, al_map_rgba_f(1, 1, 1, 1));
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_filled_triangle(-100, -100, -150, 200, 100, 200, al_map_rgb_f(0.5, 0.7, 0.3));
      al_draw_filled_rectangle(20, -50, 200, 50, al_map_rgb_f(0.3, 0.2, 0.6));
      al_draw_filled_ellipse(-250, 0, 100, 150, al_map_rgb_f(0.3, 0.3, 0.3));
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}

void ShadePrimitives(int mode)
{
   static ALLEGRO_COLOR shade_color;
   float t = al_current_time();
   if (mode == INIT) {
   
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
      shade_color = al_map_rgba_f(1 + 0.5 * sinf(t), 1 + 0.5 * sinf(t + ALLEGRO_PI / 3), 1 + 0.5 * sinf(t + 2 * ALLEGRO_PI / 3), 1);
   } else if (mode == DRAW) {
      float points[8] = {
         -300, -200,
         700, 200,
         -700, 200,
         300, -200
      };
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, shade_color);
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "Shaded Primitives");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, shade_color);
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_line(-300, -200, 300, 200, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_triangle(-150, -250, 0, 250, 150, -250, al_map_rgba_f(0.5, 0, 0.5, 1), Thickness);
      al_draw_rectangle(-300, -200, 300, 200, al_map_rgba_f(0.5, 0, 0, 1), Thickness);
      
      al_draw_ellipse(0, 0, 300, 150, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_arc(0, 0, 200, -ALLEGRO_PI / 2, ALLEGRO_PI, al_map_rgba_f(0.5, 0.25, 0, 1), Thickness);
      al_draw_spline(points, al_map_rgba_f(0.1, 0.2, 0.5, 1), Thickness);
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}

void TransformationsPrimitives(int mode)
{
   float t = al_current_time();
   if (mode == INIT) {
   
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, sinf(t / 5), cosf(t / 5), Theta);
   } else if (mode == DRAW) {
      float points[8] = {
         -300, -200,
         700, 200,
         -700, 200,
         300, -200
      };
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(1, 1, 1, 1));
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "Transformations");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, al_map_rgba_f(1, 1, 1, 1));
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_line(-300, -200, 300, 200, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_triangle(-150, -250, 0, 250, 150, -250, al_map_rgba_f(0.5, 0, 0.5, 1), Thickness);
      al_draw_rectangle(-300, -200, 300, 200, al_map_rgba_f(0.5, 0, 0, 1), Thickness);
      
      al_draw_ellipse(0, 0, 300, 150, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_arc(0, 0, 200, -ALLEGRO_PI / 2, ALLEGRO_PI, al_map_rgba_f(0.5, 0.25, 0, 1), Thickness);
      al_draw_spline(points, al_map_rgba_f(0.1, 0.2, 0.5, 1), Thickness);
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}


void LowPrimitives(int mode)
{
   static ALLEGRO_VBUFFER* vbuff;
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_COLOR color;
      vbuff = al_create_vbuff(13, 0);
      for (ii = 0; ii < 13; ii++) {
         float x, y;
         x = 200 * cosf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         y = 200 * sinf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         
         color = al_map_rgb((ii + 1) % 3 * 64, (ii + 2) % 3 * 64, (ii) % 3 * 64);
         
         al_set_vbuff_pos(vbuff, ii, x, y, 0);
         al_set_vbuff_color(vbuff, ii, color);
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(1, 1, 1, 1));
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "Low Level Primitives");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, al_map_rgba_f(1, 1, 1, 1));
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_prim(vbuff, 0, 0, 4, ALLEGRO_PRIM_LINE_LIST);
      
      al_draw_prim(vbuff, 0, 4, 9, ALLEGRO_PRIM_LINE_STRIP);
      
      al_draw_prim(vbuff, 0, 9, 13, ALLEGRO_PRIM_LINE_LOOP);
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}

void IndexedPrimitives(int mode)
{
   static ALLEGRO_VBUFFER* vbuff;
   static int indices1[] = {0, 1, 3, 4};
   static int indices2[] = {5, 6, 7, 8};
   static int indices3[] = {9, 10, 11, 12};
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_COLOR color;
      vbuff = al_create_vbuff(13, 0);
      for (ii = 0; ii < 13; ii++) {
         float x, y;
         x = 200 * cosf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         y = 200 * sinf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         
         color = al_map_rgb((ii + 1) % 3 * 64, (ii + 2) % 3 * 64, (ii) % 3 * 64);
         
         al_set_vbuff_pos(vbuff, ii, x, y, 0);
         al_set_vbuff_color(vbuff, ii, color);
      }
   } else if (mode == LOGIC) {
      int ii;
      Theta += Speed;
      for (ii = 0; ii < 4; ii++) {
         indices1[ii] = ((int)al_current_time() + ii) % 13;
         indices2[ii] = ((int)al_current_time() + ii + 4) % 13;
         indices3[ii] = ((int)al_current_time() + ii + 8) % 13;
      }
      
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(1, 1, 1, 1));
      
      al_font_textprintf_centre(Font, ScreenW / 2, ScreenH - 20, "Indexed Primitives");
      
      if (Blend)
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, al_map_rgba_f(1, 1, 1, 1));
      else
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         
      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear(al_map_rgba_f(0, 0, 0, 0));
      }
      
      al_use_transform(&MainTrans);
      
      al_draw_indexed_prim(vbuff, 0, indices1, 4, ALLEGRO_PRIM_LINE_LIST);
      al_draw_indexed_prim(vbuff, 0, indices2, 4, ALLEGRO_PRIM_LINE_STRIP);
      al_draw_indexed_prim(vbuff, 0, indices3, 4, ALLEGRO_PRIM_LINE_LOOP);
      
      al_use_transform(&Identity);
      
      if (Soft == 1) {
         al_set_target_bitmap(al_get_backbuffer());
         al_draw_bitmap(Buffer, 0, 0, 0);
      }
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP* bkg;
   ALLEGRO_COLOR white;
   ALLEGRO_COLOR black;
   ALLEGRO_EVENT_QUEUE *queue;

   // Initialize Allegro 5 and the font routines
   al_init();
   al_init_font_addon();
   
   // Create a window to display things on: 640x480 pixels
   display = al_create_display(ScreenW, ScreenH);
   if (!display) {
      printf("Error creating display.\n");
      return 1;
   }
   
   // Install the keyboard handler
   if (!al_install_keyboard()) {
      printf("Error installing keyboard.\n");
      return 1;
   }
   
   // Load a font
   Font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!Font) {
      printf("Error loading \"data/fixed_font.tga\".\n");
      return 1;
   }
   
   bkg = al_iio_load("data/bkg.png");
   
   // Make and set some color to draw with
   white = al_map_rgba_f(1.0, 1.0, 1.0, 1.0);
   black = al_map_rgba_f(0.0, 0.0, 0.0, 1.0);
   
   // Start the event queue to handle keyboard input and our timer
   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE*)al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE*)al_get_current_display());
   
   al_set_window_title("Primitives Example");
   
   {
   int refresh_rate = 60;
   int frames_done = 0;
   double time_diff = al_current_time();
   double fixed_timestep = 1.0f / refresh_rate;
   double real_time = al_current_time();
   double game_time = al_current_time();
   int ii;
   int cur_screen = 0;
   bool done = false;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *timer_queue;
   int old;

   timer = al_install_timer(ALLEGRO_BPS_TO_SECS(refresh_rate));
   al_start_timer(timer);
   timer_queue = al_create_event_queue();
   al_register_event_source(timer_queue, (ALLEGRO_EVENT_SOURCE*)timer);
   
   old = al_get_new_bitmap_flags();
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   Buffer = al_create_bitmap(ScreenW, ScreenH);
   al_set_new_bitmap_flags(old);
   
   al_identity_transform(&Identity);
   
   Screens[0] = LowPrimitives;
   Screens[1] = IndexedPrimitives;
   Screens[2] = HighPrimitives;
   Screens[3] = ShadePrimitives;
   Screens[4] = TransformationsPrimitives;
   Screens[5] = FilledPrimitives;
   Screens[6] = IndexedFilledPrimitives;
   Screens[7] = HighFilledPrimitives;
   
   for (ii = 0; ii < NUM_SCREENS; ii++)
      Screens[ii](INIT);
      
   while (!done) {
      double frame_duration = al_current_time() - real_time;
      al_rest(fixed_timestep - frame_duration); //rest at least fixed_dt
      frame_duration = al_current_time() - real_time;
      real_time = al_current_time();
      
      if (real_time - game_time > frame_duration) { //eliminate excess overflow
         game_time += fixed_timestep * floor((real_time - game_time) / fixed_timestep);
      }
      
      while (real_time - game_time >= 0) {
         ALLEGRO_EVENT key_event;
         double start_time = al_current_time();
         game_time += fixed_timestep;
         
         Screens[cur_screen](LOGIC);
         
         while (al_get_next_event(queue, &key_event)) {
            switch (key_event.type) {
               case ALLEGRO_EVENT_DISPLAY_CLOSE: {
                  done = true;
                  break;
               }
               case ALLEGRO_EVENT_KEY_DOWN:
               case ALLEGRO_EVENT_KEY_REPEAT: {
                  switch (key_event.keyboard.keycode) {
                     case ALLEGRO_KEY_ESCAPE: {
                        done = true;
                        break;
                     }
                     case ALLEGRO_KEY_S: {
                        Soft = !Soft;
                        time_diff = al_current_time();
                        frames_done = 0;
                        break;
                     }
                     case ALLEGRO_KEY_L: {
                        Blend = !Blend;
                        time_diff = al_current_time();
                        frames_done = 0;
                        break;
                     }
                     case ALLEGRO_KEY_B: {
                        Background = !Background;
                        break;
                     }
                     case ALLEGRO_KEY_LEFT: {
                        Speed -= ROTATE_SPEED;
                        break;
                     }
                     case ALLEGRO_KEY_RIGHT: {
                        Speed += ROTATE_SPEED;
                        break;
                     }
                     case ALLEGRO_KEY_PGUP: {
                        Thickness += 0.5f;
                        if (Thickness < 1.0f)
                           Thickness = 1.0f;
                        break;
                     }
                     case ALLEGRO_KEY_PGDN: {
                        Thickness -= 0.5f;
                        if (Thickness < 1.0f)
                           Thickness = 0.0f;
                        break;
                     }
                     case ALLEGRO_KEY_UP: {
                        cur_screen++;
                        if (cur_screen >= NUM_SCREENS) {
                           cur_screen = 0;
                        }
                        break;
                     }
                     case ALLEGRO_KEY_SPACE: {
                        Speed = 0;
                        break;
                     }
                     case ALLEGRO_KEY_DOWN: {
                        cur_screen--;
                        if (cur_screen < 0) {
                           cur_screen = NUM_SCREENS - 1;
                        }
                        break;
                     }
                  }
               }
            }
         }
         
         if (al_current_time() - start_time >= fixed_timestep) { //break if we start taking too long
            break;
         }
      }
      al_clear(black);
      
      if (Background && bkg) {
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, white);
         al_draw_scaled_bitmap(bkg, 0, 0, al_get_bitmap_width(bkg), al_get_bitmap_height(bkg), 0, 0, ScreenW, ScreenH, 0);
      }
      
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, white);
      al_font_textprintf(Font, 0, 0, "FPS: %f", (float)frames_done / (al_current_time() - time_diff));
      al_font_textprintf(Font, 0, 20, "Change Screen (Up/Down). Esc to Quit.");
      al_font_textprintf(Font, 0, 40, "Rotation (Left/Right/Space): %f", Speed);
      al_font_textprintf(Font, 0, 60, "Thickness (PgUp/PgDown): %f", Thickness);
      al_font_textprintf(Font, 0, 80, "Software (S): %d", Soft);
      al_font_textprintf(Font, 0, 100, "Blending (L): %d", Blend);
      al_font_textprintf(Font, 0, 120, "Background (B): %d", Background);

      
      Screens[cur_screen](DRAW);
      
      al_wait_for_vsync();
      al_flip_display();
      frames_done++;
   }
   }
   
   return 0;
}
END_OF_MAIN()
