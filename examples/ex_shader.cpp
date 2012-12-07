#include "allegro5/allegro5.h"
#include "allegro5/allegro_shader.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"
#include <cstdio>

// FIXME: supported drivers should go in alplatf.h
// Uncomment one of these three blocks depending what driver you want

//#define HLSL
//#include "allegro5/allegro_direct3d.h"
//#include "allegro5/allegro_shader_hlsl.h"

#define GLSL
#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_shader_glsl.h"

/*
#define CG
#include <Cg/cg.h>
*/

#ifdef HLSL
   #define PLATFORM     ALLEGRO_SHADER_HLSL
   #define VSOURCE_NAME "data/ex_shader_vertex.hlsl"
   #define PSOURCE_NAME "data/ex_shader_pixel.hlsl"
#elif defined GLSL
   #define PLATFORM     ALLEGRO_SHADER_GLSL
   #define VSOURCE_NAME "data/ex_shader_vertex.glsl"
   #define PSOURCE_NAME "data/ex_shader_pixel.glsl"
#else
   #define PLATFORM     ALLEGRO_SHADER_CG
   #define VSOURCE_NAME "data/ex_shader_vertex.cg"
   #define PSOURCE_NAME "data/ex_shader_pixel.cg"
#endif

#ifdef GLSL
#define EX_SHADER_FLAGS ALLEGRO_OPENGL
#else
#define EX_SHADER_FLAGS 0
#endif

#if defined HLSL && !defined CG
#include <allegro5/allegro_direct3d.h>
#include <d3d9.h>
#include <d3dx9.h>

#define A5V_FVF (D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE4(1))

void drawD3D(ALLEGRO_VERTEX *v, int start, int count)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   LPDIRECT3DDEVICE9 device = al_get_d3d_device(display);

   device->SetFVF(A5V_FVF);

   device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, count / 3,
      &v[start].x, sizeof(ALLEGRO_VERTEX));
}
#endif

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;

   (void)argc;
   (void)argv;

   al_init();
   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_USE_PROGRAMMABLE_PIPELINE | EX_SHADER_FLAGS);

   display = al_create_display(640, 480);

   bmp = al_load_bitmap("data/mysha.pcx");

   shader = al_create_shader(PLATFORM);
   if (!shader) {
      fprintf(stderr, "Could not create shader\n");
      return 1;
   }

   if (!al_attach_shader_source_file(shader, ALLEGRO_VERTEX_SHADER,
         VSOURCE_NAME)) {
      fprintf(stderr, "%s\n", al_get_shader_log(shader));
      return 1;
   }
   if (!al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER,
         PSOURCE_NAME)) {
      fprintf(stderr, "%s\n", al_get_shader_log(shader));
      return 1;
   }

   if (!al_link_shader(shader)) {
      fprintf(stderr, "%s\n", al_get_shader_log(shader));
      return 1;
   }

   al_set_shader(display, shader);
      
   float tints[12] = {
      4.0, 0.0, 1.0,
      0.0, 4.0, 1.0,
      1.0, 0.0, 4.0,
      4.0, 4.0, 1.0
   };

   while (1) {
      ALLEGRO_KEYBOARD_STATE s;
      al_get_keyboard_state(&s);
      if (al_key_down(&s, ALLEGRO_KEY_ESCAPE))
         break;

      al_clear_to_color(al_map_rgb(140, 40, 40));

      al_set_shader_sampler(shader, "tex", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[0], 1);
      al_use_shader(shader, true);
      al_draw_bitmap(bmp, 0, 0, 0);
      al_use_shader(shader, false);

      al_set_shader_sampler(shader, "tex", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[3], 1);
      al_use_shader(shader, true);
      al_draw_bitmap(bmp, 320, 0, 0);
      al_use_shader(shader, false);
      
      al_set_shader_sampler(shader, "tex", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[6], 1);
      al_use_shader(shader, true);
      al_draw_bitmap(bmp, 0, 240, 0);
      al_use_shader(shader, false);
      
      /* Draw the last one transformed */
      ALLEGRO_TRANSFORM trans, backup;
      al_copy_transform(&backup, al_get_current_transform());
      al_identity_transform(&trans);
      al_translate_transform(&trans, 320, 240);
      al_set_shader_sampler(shader, "tex", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[9], 1);
      al_use_shader(shader, true);
      al_use_transform(&trans);
      al_draw_bitmap(bmp, 0, 0, 0);
      al_use_transform(&backup);
      al_use_shader(shader, false);

      al_flip_display();

      al_rest(0.01);
   }

   al_destroy_shader(shader);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
