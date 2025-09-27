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

#define ALLEGRO_UNSTABLE

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include <stdio.h>

#include "common.c"

typedef void (*Screen)(int);
int ScreenW = 800, ScreenH = 600;
#define NUM_SCREENS 14
#define ROTATE_SPEED 0.0001f
Screen Screens[NUM_SCREENS];
const char *ScreenName[NUM_SCREENS];
ALLEGRO_FONT* Font;
ALLEGRO_TRANSFORM Identity;
ALLEGRO_BITMAP* Buffer;
ALLEGRO_BITMAP* Texture;
ALLEGRO_COLOR solid_white;

bool UseShader = false;
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
   DRAW,
   DEINIT
};

typedef struct
{
   ALLEGRO_COLOR color;
   short u, v;
   short x, y;
   int junk[6];
} CUSTOM_VERTEX;

static void CustomVertexFormatPrimitives(int mode)
{
   static CUSTOM_VERTEX vtx[4];
   static ALLEGRO_VERTEX_DECL* decl;
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_VERTEX_ELEMENT elems[] = {
         {ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_SHORT_2, offsetof(CUSTOM_VERTEX, x)},
         {ALLEGRO_PRIM_TEX_COORD_PIXEL, ALLEGRO_PRIM_SHORT_2, offsetof(CUSTOM_VERTEX, u)},
         {ALLEGRO_PRIM_COLOR_ATTR, 0, offsetof(CUSTOM_VERTEX, color)},
         {0, 0, 0}
      };
      decl = al_create_vertex_decl(elems, sizeof(CUSTOM_VERTEX));

      for (ii = 0; ii < 4; ii++) {
         float x, y;
         x = 200 * cosf((float)ii / 4.0f * 2 * ALLEGRO_PI);
         y = 200 * sinf((float)ii / 4.0f * 2 * ALLEGRO_PI);

         vtx[ii].x = x; vtx[ii].y = y;
         vtx[ii].u = 64 * x / 100; vtx[ii].v = 64 * y / 100;
         vtx[ii].color = al_map_rgba_f(1, 1, 1, 1);
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_prim(vtx, decl, Texture, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);

      al_use_transform(&Identity);
   }
}

static void TexturePrimitives(int mode)
{
   static ALLEGRO_VERTEX vtx[13];
   static ALLEGRO_VERTEX vtx2[13];
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_COLOR color;
      for (ii = 0; ii < 13; ii++) {
         float x, y;
         x = 200 * cosf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         y = 200 * sinf((float)ii / 13.0f * 2 * ALLEGRO_PI);

         color = al_map_rgb((ii + 1) % 3 * 64, (ii + 2) % 3 * 64, (ii) % 3 * 64);

         vtx[ii].x = x; vtx[ii].y = y; vtx[ii].z = 0;
         vtx2[ii].x = 0.1 * x; vtx2[ii].y = 0.1 * y;
         vtx[ii].u = 64 * x / 100; vtx[ii].v = 64 * y / 100;
         vtx2[ii].u = 64 * x / 100; vtx2[ii].v = 64 * y / 100;
         if(ii < 10)
            vtx[ii].color = al_map_rgba_f(1, 1, 1, 1);
         else
            vtx[ii].color = color;
         vtx2[ii].color = vtx[ii].color;
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_prim(vtx, 0, Texture, 0, 4, ALLEGRO_PRIM_LINE_LIST);
      al_draw_prim(vtx, 0, Texture, 4, 9, ALLEGRO_PRIM_LINE_STRIP);
      al_draw_prim(vtx, 0, Texture, 9, 13, ALLEGRO_PRIM_LINE_LOOP);
      al_draw_prim(vtx2, 0, Texture, 0, 13, ALLEGRO_PRIM_POINT_LIST);

      al_use_transform(&Identity);
   }
}


static void FilledTexturePrimitives(int mode)
{
   static ALLEGRO_VERTEX vtx[21];
   if (mode == INIT) {
      int ii = 0;
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

         vtx[ii].x = x; vtx[ii].y = y; vtx[ii].z = 0;
         vtx[ii].u = 64 * x / 100; vtx[ii].v = 64 * y / 100;
         if(ii < 10)
            vtx[ii].color = al_map_rgba_f(1, 1, 1, 1);
         else
            vtx[ii].color = color;
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_prim(vtx, 0, Texture, 0, 6, ALLEGRO_PRIM_TRIANGLE_FAN);
      al_draw_prim(vtx, 0, Texture, 7, 13, ALLEGRO_PRIM_TRIANGLE_LIST);
      al_draw_prim(vtx, 0, Texture, 14, 20, ALLEGRO_PRIM_TRIANGLE_STRIP);

      al_use_transform(&Identity);
   }
}

static void FilledPrimitives(int mode)
{
   static ALLEGRO_VERTEX vtx[21];
   if (mode == INIT) {
      int ii = 0;
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

         vtx[ii].x = x; vtx[ii].y = y; vtx[ii].z = 0;
         vtx[ii].color = color;
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_prim(vtx, 0, 0, 0, 6, ALLEGRO_PRIM_TRIANGLE_FAN);
      al_draw_prim(vtx, 0, 0, 7, 13, ALLEGRO_PRIM_TRIANGLE_LIST);
      al_draw_prim(vtx, 0, 0, 14, 20, ALLEGRO_PRIM_TRIANGLE_STRIP);

      al_use_transform(&Identity);
   }
}

static void IndexedFilledPrimitives(int mode)
{
   static ALLEGRO_VERTEX vtx[21];
   static int indices1[] = {12, 13, 14, 16, 17, 18};
   static int indices2[] = {6, 7, 8, 9, 10, 11};
   static int indices3[] = {0, 1, 2, 3, 4, 5};
   if (mode == INIT) {
      int ii = 0;
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

         vtx[ii].x = x; vtx[ii].y = y; vtx[ii].z = 0;
         vtx[ii].color = color;
      }
   } else if (mode == LOGIC) {
      int ii;
      Theta += Speed;
      for (ii = 0; ii < 6; ii++) {
         indices1[ii] = ((int)al_get_time() + ii) % 20 + 1;
         indices2[ii] = ((int)al_get_time() + ii + 6) % 20 + 1;
         if (ii > 0)
            indices3[ii] = ((int)al_get_time() + ii + 12) % 20 + 1;
      }

      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_indexed_prim(vtx, 0, 0, indices1, 6, ALLEGRO_PRIM_TRIANGLE_LIST);
      al_draw_indexed_prim(vtx, 0, 0, indices2, 6, ALLEGRO_PRIM_TRIANGLE_STRIP);
      al_draw_indexed_prim(vtx, 0, 0, indices3, 6, ALLEGRO_PRIM_TRIANGLE_FAN);

      al_use_transform(&Identity);
   }
}

static void HighPrimitives(int mode)
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

      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_line(-300, -200, 300, 200, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_triangle(-150, -250, 0, 250, 150, -250, al_map_rgba_f(0.5, 0, 0.5, 1), Thickness);
      al_draw_rectangle(-300, -200, 300, 200, al_map_rgba_f(0.5, 0, 0, 1), Thickness);
      al_draw_rounded_rectangle(-200, -125, 200, 125, 50, 100, al_map_rgba_f(0.2, 0.2, 0, 1), Thickness);

      al_draw_ellipse(0, 0, 300, 150, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_elliptical_arc(-20, 0, 300, 200, -ALLEGRO_PI / 2, -ALLEGRO_PI, al_map_rgba_f(0.25, 0.25, 0.5, 1), Thickness);
      al_draw_arc(0, 0, 200, -ALLEGRO_PI / 2, ALLEGRO_PI, al_map_rgba_f(0.5, 0.25, 0, 1), Thickness);
      al_draw_spline(points, al_map_rgba_f(0.1, 0.2, 0.5, 1), Thickness);
      al_draw_pieslice(0, 25, 150, ALLEGRO_PI * 3 / 4, -ALLEGRO_PI / 2, al_map_rgba_f(0.4, 0.3, 0.1, 1), Thickness);

      al_use_transform(&Identity);
   }
}

static void HighFilledPrimitives(int mode)
{
   if (mode == INIT) {

   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_filled_triangle(-100, -100, -150, 200, 100, 200, al_map_rgb_f(0.5, 0.7, 0.3));
      al_draw_filled_rectangle(20, -50, 200, 50, al_map_rgb_f(0.3, 0.2, 0.6));
      al_draw_filled_ellipse(-250, 0, 100, 150, al_map_rgb_f(0.3, 0.3, 0.3));
      al_draw_filled_rounded_rectangle(50, -250, 350, -75, 50, 70, al_map_rgb_f(0.4, 0.2, 0));
      al_draw_filled_pieslice(200, 125, 50, ALLEGRO_PI / 4, 3 * ALLEGRO_PI / 2, al_map_rgb_f(0.3, 0.3, 0.1));

      al_use_transform(&Identity);
   }
}

static void HighFilledPrimitivesShader(int mode)
{
   static ALLEGRO_SHADER *shader;
   if (mode == INIT) {
      if (!UseShader)
         return;
      shader = al_create_shader(ALLEGRO_SHADER_AUTO);

      if (!shader) {
         abort_example("Failed to create shader.");
      }

      const char *vertex_shader_file;
      const char *pixel_shader_file;
      if (al_get_shader_platform(shader) == ALLEGRO_SHADER_GLSL) {
         vertex_shader_file = "data/ex_prim_high_vertex.glsl";
         pixel_shader_file = "data/ex_prim_high_pixel.glsl";
      }
      else {
         vertex_shader_file = "data/ex_prim_high_vertex.hlsl";
         pixel_shader_file = "data/ex_prim_high_pixel.hlsl";
      }

      if (!al_attach_shader_source_file(shader, ALLEGRO_VERTEX_SHADER, vertex_shader_file)) {
         abort_example("al_attach_shader_source_file for vertex shader failed: %s\n",
            al_get_shader_log(shader));
      }
      if (!al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, pixel_shader_file)) {
         abort_example("al_attach_shader_source_file for pixel shader failed: %s\n",
            al_get_shader_log(shader));
      }
      if (!al_build_shader(shader)) {
         abort_example("al_build_shader for link failed: %s\n", al_get_shader_log(shader));
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);
      if (!UseShader) {
         al_draw_text(Font, al_map_rgb_f(1, 1, 1), 0, -40, 0, "Enable shaders (by using --shader arg)");
      }
      else if (Soft) {
         al_draw_text(Font, al_map_rgb_f(1, 1, 1), 0, -40, 0, "Shaders don't work with software rendering");
      }
      else {
         ALLEGRO_SHADER* old_shader = al_get_current_shader();
         al_use_shader(shader);
         al_draw_filled_triangle(-100, -100, -150, 200, 100, 200, al_map_rgb_f(0.5, 0.7, 0.3));
         al_draw_filled_rectangle(20, -50, 200, 50, al_map_rgb_f(0.3, 0.2, 0.6));
         al_draw_filled_ellipse(-250, 0, 100, 150, al_map_rgb_f(0.3, 0.3, 0.3));
         al_draw_filled_rounded_rectangle(50, -250, 350, -75, 50, 70, al_map_rgb_f(0.4, 0.2, 0));
         al_draw_filled_pieslice(200, 125, 50, ALLEGRO_PI / 4, 3 * ALLEGRO_PI / 2, al_map_rgb_f(0.3, 0.3, 0.1));
         al_use_shader(old_shader);
      }
      al_use_transform(&Identity);
   }
}

static void HighPrimitivesShader(int mode)
{
   static ALLEGRO_SHADER *shader;
   if (mode == INIT) {
      if (!UseShader)
         return;
      shader = al_create_shader(ALLEGRO_SHADER_AUTO);

      if (!shader) {
         abort_example("Failed to create shader.");
      }

      const char *vertex_shader_file;
      const char *pixel_shader_file;
      if (al_get_shader_platform(shader) == ALLEGRO_SHADER_GLSL) {
         vertex_shader_file = "data/ex_prim_high_vertex.glsl";
         pixel_shader_file = "data/ex_prim_high_pixel.glsl";
      }
      else {
         vertex_shader_file = "data/ex_prim_high_vertex.hlsl";
         pixel_shader_file = "data/ex_prim_high_pixel.hlsl";
      }

      if (!al_attach_shader_source_file(shader, ALLEGRO_VERTEX_SHADER, vertex_shader_file)) {
         abort_example("al_attach_shader_source_file for vertex shader failed: %s\n",
            al_get_shader_log(shader));
      }
      if (!al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, pixel_shader_file)) {
         abort_example("al_attach_shader_source_file for pixel shader failed: %s\n",
            al_get_shader_log(shader));
      }
      if (!al_build_shader(shader)) {
         abort_example("al_build_shader for link failed: %s\n", al_get_shader_log(shader));
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);
      if (!UseShader) {
         al_draw_text(Font, al_map_rgb_f(1, 1, 1), 0, -40, 0, "Enable shaders (by using --shader arg)");
      }
      else if (Soft) {
         al_draw_text(Font, al_map_rgb_f(1, 1, 1), 0, -40, 0, "Shaders don't work with software rendering");
      }
      else {
         ALLEGRO_SHADER* old_shader = al_get_current_shader();
         al_use_shader(shader);
         // float points[8] = {
         //    -300, -200,
         //    700, 200,
         //    -700, 200,
         //    300, -200
         // };

         al_draw_line(-300, -200, 300, 200, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
         al_draw_triangle(-150, -250, 0, 250, 150, -250, al_map_rgba_f(0.5, 0, 0.5, 1), Thickness);
         al_draw_rectangle(-300, -200, 300, 200, al_map_rgba_f(0.5, 0, 0, 1), Thickness);
         al_draw_rounded_rectangle(-200, -125, 200, 125, 50, 100, al_map_rgba_f(0.2, 0.2, 0, 1), Thickness);

         al_draw_ellipse(0, 0, 300, 150, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
         al_draw_elliptical_arc(-20, 0, 300, 200, -ALLEGRO_PI / 2, -ALLEGRO_PI, al_map_rgba_f(0.25, 0.25, 0.5, 1), Thickness);
         al_draw_arc(0, 0, 200, -ALLEGRO_PI / 2, ALLEGRO_PI, al_map_rgba_f(0.5, 0.25, 0, 1), Thickness);
         // al_draw_spline(points, al_map_rgba_f(0.1, 0.2, 0.5, 1), Thickness);
         al_draw_pieslice(0, 25, 150, ALLEGRO_PI * 3 / 4, -ALLEGRO_PI / 2, al_map_rgba_f(0.4, 0.3, 0.1, 1), Thickness);
         al_use_shader(old_shader);
      }
      al_use_transform(&Identity);
   }
}

static void TransformationsPrimitives(int mode)
{
   float t = al_get_time();
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

      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_line(-300, -200, 300, 200, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_triangle(-150, -250, 0, 250, 150, -250, al_map_rgba_f(0.5, 0, 0.5, 1), Thickness);
      al_draw_rectangle(-300, -200, 300, 200, al_map_rgba_f(0.5, 0, 0, 1), Thickness);
      al_draw_rounded_rectangle(-200, -125, 200, 125, 50, 100, al_map_rgba_f(0.2, 0.2, 0, 1), Thickness);

      al_draw_ellipse(0, 0, 300, 150, al_map_rgba_f(0, 0.5, 0.5, 1), Thickness);
      al_draw_elliptical_arc(-20, 0, 300, 200, -ALLEGRO_PI / 2, -ALLEGRO_PI, al_map_rgba_f(0.25, 0.25, 0.5, 1), Thickness);
      al_draw_arc(0, 0, 200, -ALLEGRO_PI / 2, ALLEGRO_PI, al_map_rgba_f(0.5, 0.25, 0, 1), Thickness);
      al_draw_spline(points, al_map_rgba_f(0.1, 0.2, 0.5, 1), Thickness);
      al_draw_pieslice(0, 25, 150, ALLEGRO_PI * 3 / 4, -ALLEGRO_PI / 2, al_map_rgba_f(0.4, 0.3, 0.1, 1), Thickness);

      al_use_transform(&Identity);
   }
}

static void LowPrimitives(int mode)
{
   static ALLEGRO_VERTEX vtx[13];
   static ALLEGRO_VERTEX vtx2[13];
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_COLOR color;
      for (ii = 0; ii < 13; ii++) {
         float x, y;
         x = 200 * cosf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         y = 200 * sinf((float)ii / 13.0f * 2 * ALLEGRO_PI);

         color = al_map_rgb((ii + 1) % 3 * 64, (ii + 2) % 3 * 64, (ii) % 3 * 64);

         vtx[ii].x = x; vtx[ii].y = y; vtx[ii].z = 0;
         vtx2[ii].x = 0.1 * x; vtx2[ii].y = 0.1 * y;
         vtx[ii].color = color;
         vtx2[ii].color = color;
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_prim(vtx, 0, 0, 0, 4, ALLEGRO_PRIM_LINE_LIST);
      al_draw_prim(vtx, 0, 0, 4, 9, ALLEGRO_PRIM_LINE_STRIP);
      al_draw_prim(vtx, 0, 0, 9, 13, ALLEGRO_PRIM_LINE_LOOP);
      al_draw_prim(vtx2, 0, 0, 0, 13, ALLEGRO_PRIM_POINT_LIST);

      al_use_transform(&Identity);
   }
}

static void IndexedPrimitives(int mode)
{
   static int indices1[] = {0, 1, 3, 4};
   static int indices2[] = {5, 6, 7, 8};
   static int indices3[] = {9, 10, 11, 12};
   static ALLEGRO_VERTEX vtx[13];
   static ALLEGRO_VERTEX vtx2[13];
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_COLOR color;
      for (ii = 0; ii < 13; ii++) {
         float x, y;
         x = 200 * cosf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         y = 200 * sinf((float)ii / 13.0f * 2 * ALLEGRO_PI);

         color = al_map_rgb((ii + 1) % 3 * 64, (ii + 2) % 3 * 64, (ii) % 3 * 64);

         vtx[ii].x = x; vtx[ii].y = y; vtx[ii].z = 0;
         vtx2[ii].x = 0.1 * x; vtx2[ii].y = 0.1 * y;
         vtx[ii].color = color;
         vtx2[ii].color = color;
      }
   } else if (mode == LOGIC) {
      int ii;
      Theta += Speed;
      for (ii = 0; ii < 4; ii++) {
         indices1[ii] = ((int)al_get_time() + ii) % 13;
         indices2[ii] = ((int)al_get_time() + ii + 4) % 13;
         indices3[ii] = ((int)al_get_time() + ii + 8) % 13;
      }

      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      al_draw_indexed_prim(vtx, 0, 0, indices1, 4, ALLEGRO_PRIM_LINE_LIST);
      al_draw_indexed_prim(vtx, 0, 0, indices2, 4, ALLEGRO_PRIM_LINE_STRIP);
      al_draw_indexed_prim(vtx, 0, 0, indices3, 4, ALLEGRO_PRIM_LINE_LOOP);
      al_draw_indexed_prim(vtx2, 0, 0, indices3, 4, ALLEGRO_PRIM_POINT_LIST);

      al_use_transform(&Identity);
   }
}

static void VertexBuffers(int mode)
{
   static ALLEGRO_VERTEX vtx[13];
   static ALLEGRO_VERTEX vtx2[13];
   static ALLEGRO_VERTEX_BUFFER* vbuff;
   static ALLEGRO_VERTEX_BUFFER* vbuff2;
   static bool no_soft;
   static bool no_soft2;
   if (mode == INIT) {
      int ii = 0;
      ALLEGRO_COLOR color;
      for (ii = 0; ii < 13; ii++) {
         float x, y;
         x = 200 * cosf((float)ii / 13.0f * 2 * ALLEGRO_PI);
         y = 200 * sinf((float)ii / 13.0f * 2 * ALLEGRO_PI);

         color = al_map_rgb((ii + 1) % 3 * 64, (ii + 2) % 3 * 64, (ii) % 3 * 64);

         vtx[ii].x = x; vtx[ii].y = y; vtx[ii].z = 0;
         vtx2[ii].x = 0.1 * x; vtx2[ii].y = 0.1 * y;
         vtx[ii].color = color;
         vtx2[ii].color = color;
      }
      vbuff = al_create_vertex_buffer(0, vtx, 13, ALLEGRO_PRIM_BUFFER_READWRITE);
      if (!vbuff) {
         vbuff = al_create_vertex_buffer(0, vtx, 13, 0);
         no_soft = true;
      }
      else {
         no_soft = false;
      }

      vbuff2 = al_create_vertex_buffer(0, vtx2, 13, ALLEGRO_PRIM_BUFFER_READWRITE);
      if (!vbuff2) {
         vbuff2 = al_create_vertex_buffer(0, vtx2, 13, 0);
         no_soft2 = true;
      }
      else {
         no_soft2 = false;
      }
   } else if (mode == LOGIC) {
      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      if (vbuff && !(Soft && no_soft)) {
         al_draw_vertex_buffer(vbuff, 0, 0, 4, ALLEGRO_PRIM_LINE_LIST);
         al_draw_vertex_buffer(vbuff, 0, 4, 9, ALLEGRO_PRIM_LINE_STRIP);
         al_draw_vertex_buffer(vbuff, 0, 9, 13, ALLEGRO_PRIM_LINE_LOOP);
      }
      else {
         al_draw_text(Font, al_map_rgb_f(1, 1, 1), 0, -40, 0, "Vertex buffers not supported");
      }

      if (vbuff2 && !(Soft && no_soft2)) {
         al_draw_vertex_buffer(vbuff2, 0, 0, 13, ALLEGRO_PRIM_POINT_LIST);
      }
      else {
         al_draw_text(Font, al_map_rgb_f(1, 1, 1), 0, 40, 0, "Vertex buffers not supported");
      }

      al_use_transform(&Identity);
   } else if (mode == DEINIT) {
      al_destroy_vertex_buffer(vbuff);
      al_destroy_vertex_buffer(vbuff2);
   }
}

static void IndexedBuffers(int mode)
{
   static ALLEGRO_VERTEX_BUFFER* vbuff;
   static ALLEGRO_INDEX_BUFFER* ibuff;
   static bool soft = true;
   if (mode == INIT) {
      int ii;
      ALLEGRO_COLOR color;
      ALLEGRO_VERTEX* vtx;
      int flags = ALLEGRO_PRIM_BUFFER_READWRITE;

      vbuff = al_create_vertex_buffer(NULL, NULL, 13, ALLEGRO_PRIM_BUFFER_READWRITE);
      if (vbuff == NULL) {
         vbuff = al_create_vertex_buffer(NULL, NULL, 13, 0);
         soft = false;
         flags = 0;
      }

      ibuff = al_create_index_buffer(sizeof(short), NULL, 8, flags);

      if (vbuff) {
         vtx = al_lock_vertex_buffer(vbuff, 0, 13, ALLEGRO_LOCK_WRITEONLY);

         for (ii = 0; ii < 13; ii++) {
            float x, y;
            x = 200 * cosf((float)ii / 13.0f * 2 * ALLEGRO_PI);
            y = 200 * sinf((float)ii / 13.0f * 2 * ALLEGRO_PI);

            color = al_map_rgb((ii + 1) % 3 * 64, (ii + 2) % 3 * 64, (ii) % 3 * 64);

            vtx[ii].x = x;
            vtx[ii].y = y;
            vtx[ii].z = 0;
            vtx[ii].color = color;
         }

         al_unlock_vertex_buffer(vbuff);
      }
   } else if (mode == LOGIC) {
      if (ibuff) {
         int ii;
         int t = (int)al_get_time();
         short* indices = al_lock_index_buffer(ibuff, 0, 8, ALLEGRO_LOCK_WRITEONLY);

         for (ii = 0; ii < 8; ii++) {
            indices[ii] = (t + ii) % 13;
         }
         al_unlock_index_buffer(ibuff);
      }

      Theta += Speed;
      al_build_transform(&MainTrans, ScreenW / 2, ScreenH / 2, 1, 1, Theta);
   } else if (mode == DRAW) {
      if (Blend)
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      else
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

      al_use_transform(&MainTrans);

      if(!(Soft && !soft) && vbuff && ibuff) {
         al_draw_indexed_buffer(vbuff, NULL, ibuff, 0, 4, ALLEGRO_PRIM_LINE_LIST);
         al_draw_indexed_buffer(vbuff, NULL, ibuff, 4, 8, ALLEGRO_PRIM_LINE_STRIP);
      }
      else {
         al_draw_text(Font, al_map_rgb_f(1, 1, 1), 0, 0, 0, "Indexed buffers not supported");
      }

      al_use_transform(&Identity);
   } else if (mode == DEINIT) {
      al_destroy_vertex_buffer(vbuff);
      al_destroy_index_buffer(ibuff);
   }
}

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP* bkg;
   ALLEGRO_COLOR black;
   ALLEGRO_EVENT_QUEUE *queue;

   if (argc > 1) {
      if (strcmp(argv[1], "--shader") == 0) {
         UseShader = true;
      }
      else {
         abort_example("Invalid command line option: %s", argv[1]);
      }
   }

   // Initialize Allegro 5 and addons
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_image_addon();
   al_init_font_addon();
   al_init_primitives_addon();
   init_platform_specific();

   if (UseShader) {
      al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE);
   }

   // Create a window to display things on: 640x480 pixels
   display = al_create_display(ScreenW, ScreenH);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   // Install the keyboard handler
   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   if (!al_install_mouse()) {
      abort_example("Error installing mouse.\n");
   }

   // Load a font
   Font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!Font) {
      abort_example("Error loading \"data/fixed_font.tga\".\n");
   }

   solid_white = al_map_rgba_f(1, 1, 1, 1);

   bkg = al_load_bitmap("data/bkg.png");

   al_set_new_bitmap_wrap(ALLEGRO_BITMAP_WRAP_CLAMP, ALLEGRO_BITMAP_WRAP_MIRROR);
   Texture = al_load_bitmap("data/texture.tga");

   // Make and set some color to draw with
   black = al_map_rgba_f(0.0, 0.0, 0.0, 1.0);

   // Start the event queue to handle keyboard input and our timer
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_mouse_event_source());

   al_set_window_title(display, "Primitives Example");

   {
   int refresh_rate = 60;
   int frames_done = 0;
   double time_diff = al_get_time();
   double fixed_timestep = 1.0f / refresh_rate;
   double real_time = al_get_time();
   double game_time = al_get_time();
   int ii;
   int cur_screen = 0;
   bool done = false;
   int clip = 0;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *timer_queue;
   int old;

   timer = al_create_timer(ALLEGRO_BPS_TO_SECS(refresh_rate));
   al_start_timer(timer);
   timer_queue = al_create_event_queue();
   al_register_event_source(timer_queue, al_get_timer_event_source(timer));

   old = al_get_new_bitmap_flags();
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   Buffer = al_create_bitmap(ScreenW, ScreenH);
   al_set_new_bitmap_flags(old);

   al_identity_transform(&Identity);

   Screens[0] = LowPrimitives;
   Screens[1] = IndexedPrimitives;
   Screens[2] = HighPrimitives;
   Screens[3] = TransformationsPrimitives;
   Screens[4] = FilledPrimitives;
   Screens[5] = IndexedFilledPrimitives;
   Screens[6] = HighFilledPrimitives;
   Screens[7] = TexturePrimitives;
   Screens[8] = FilledTexturePrimitives;
   Screens[9] = CustomVertexFormatPrimitives;
   Screens[10] = VertexBuffers;
   Screens[11] = IndexedBuffers;
   Screens[12] = HighPrimitivesShader;
   Screens[13] = HighFilledPrimitivesShader;

   ScreenName[0] = "Low Level Primitives";
   ScreenName[1] = "Indexed Primitives";
   ScreenName[2] = "High Level Primitives";
   ScreenName[3] = "Transformations";
   ScreenName[4] = "Low Level Filled Primitives";
   ScreenName[5] = "Indexed Filled Primitives";
   ScreenName[6] = "High Level Filled Primitives";
   ScreenName[7] = "Textured Primitives";
   ScreenName[8] = "Filled Textured Primitives";
   ScreenName[9] = "Custom Vertex Format";
   ScreenName[10] = "Vertex Buffers";
   ScreenName[11] = "Indexed Buffers";
   ScreenName[12] = "High Level Primitives + Shaders";
   ScreenName[13] = "High Level Filled Primitives + Shaders";

   for (ii = 0; ii < NUM_SCREENS; ii++) {
      Screens[ii](INIT);
      Screens[ii](LOGIC);
   }

   while (!done) {
      double frame_duration = al_get_time() - real_time;
      al_rest(fixed_timestep - frame_duration); //rest at least fixed_dt
      frame_duration = al_get_time() - real_time;
      real_time = al_get_time();

      if (real_time - game_time > frame_duration) { //eliminate excess overflow
         game_time += fixed_timestep * floor((real_time - game_time) / fixed_timestep);
      }

      while (real_time - game_time >= 0) {
         ALLEGRO_EVENT key_event;
         double start_time = al_get_time();
         game_time += fixed_timestep;

         Screens[cur_screen](LOGIC);

         while (al_get_next_event(queue, &key_event)) {
            switch (key_event.type) {
               case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN: {
                  cur_screen++;
                  if (cur_screen >= NUM_SCREENS) {
                     cur_screen = 0;
                  }
                  break;
               }

               case ALLEGRO_EVENT_DISPLAY_CLOSE: {
                  done = true;
                  break;
               }
               case ALLEGRO_EVENT_KEY_CHAR: {
                  switch (key_event.keyboard.keycode) {
                     case ALLEGRO_KEY_ESCAPE: {
                        done = true;
                        break;
                     }
                     case ALLEGRO_KEY_S: {
                        Soft = !Soft;
                        time_diff = al_get_time();
                        frames_done = 0;
                        break;
                     }
                     case ALLEGRO_KEY_C: {
                        clip = !clip;
                        time_diff = al_get_time();
                        frames_done = 0;
                        break;
                     }
                     case ALLEGRO_KEY_L: {
                        Blend = !Blend;
                        time_diff = al_get_time();
                        frames_done = 0;
                        break;
                     }
                     case ALLEGRO_KEY_B: {
                        Background = !Background;
                        time_diff = al_get_time();
                        frames_done = 0;
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

         if (al_get_time() - start_time >= fixed_timestep) { //break if we start taking too long
            break;
         }
      }

      al_clear_to_color(black);

      if (Soft == 1) {
         al_set_target_bitmap(Buffer);
         al_clear_to_color(black);
      }

      if (Background && bkg) {
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
         al_draw_scaled_bitmap(bkg, 0, 0, al_get_bitmap_width(bkg), al_get_bitmap_height(bkg), 0, 0, ScreenW, ScreenH, 0);
      }

      if (clip == 1) {
         al_set_clipping_rectangle(ScreenW / 2, ScreenH / 2, ScreenW / 2, ScreenH / 2);
      }

      Screens[cur_screen](DRAW);

      al_set_clipping_rectangle(0, 0, ScreenW, ScreenH);

      if (Soft == 1) {
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
         al_set_target_backbuffer(display);
         al_draw_bitmap(Buffer, 0, 0, 0);
      }

      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
      al_draw_textf(Font, solid_white, ScreenW / 2, ScreenH - 20, ALLEGRO_ALIGN_CENTRE, "%s", ScreenName[cur_screen]);
      al_draw_textf(Font, solid_white, 0, 0, 0, "FPS: %f", (float)frames_done / (al_get_time() - time_diff));
      al_draw_textf(Font, solid_white, 0, 20, 0, "Change Screen (Up/Down). Esc to Quit.");
      al_draw_textf(Font, solid_white, 0, 40, 0, "Rotation (Left/Right/Space): %f", Speed);
      al_draw_textf(Font, solid_white, 0, 60, 0, "Thickness (PgUp/PgDown): %f", Thickness);
      al_draw_textf(Font, solid_white, 0, 80, 0, "Software (S): %d", Soft);
      al_draw_textf(Font, solid_white, 0, 100, 0, "Blending (L): %d", Blend);
      al_draw_textf(Font, solid_white, 0, 120, 0, "Background (B): %d", Background);
      al_draw_textf(Font, solid_white, 0, 140, 0, "Clip (C): %d", clip);

      al_flip_display();
      frames_done++;
   }

   for (ii = 0; ii < NUM_SCREENS; ii++)
      Screens[ii](DEINIT);

   }

   return 0;
}
