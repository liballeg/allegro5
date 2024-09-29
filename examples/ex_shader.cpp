#include <cstdio>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"

/* The A5O_CFG_* defines are actually internal to Allegro so don't use them
 * in your own programs.
 */
#ifdef A5O_CFG_D3D
   #include "allegro5/allegro_direct3d.h"
#endif
#ifdef A5O_CFG_OPENGL
   #include "allegro5/allegro_opengl.h"
#endif

#include "common.c"

static int display_flags = 0;

static void parse_args(int argc, char **argv)
{
   int i;

   for (i = 1; i < argc; i++) {
      if (0 == strcmp(argv[i], "--opengl")) {
         display_flags = A5O_OPENGL;
         continue;
      }
#ifdef A5O_CFG_D3D
      if (0 == strcmp(argv[i], "--d3d")) {
         display_flags = A5O_DIRECT3D;
         continue;
      }
#endif
      abort_example("Unrecognised argument: %s\n", argv[i]);
   }
}

static void choose_shader_source(A5O_SHADER *shader,
   char const **vsource, char const **psource)
{
   A5O_SHADER_PLATFORM platform = al_get_shader_platform(shader);
   if (platform == A5O_SHADER_HLSL) {
      *vsource = "data/ex_shader_vertex.hlsl";
      *psource = "data/ex_shader_pixel.hlsl";
   }
   else if (platform == A5O_SHADER_GLSL) {
      *vsource = "data/ex_shader_vertex.glsl";
      *psource = "data/ex_shader_pixel.glsl";
   }
   else {
      /* Shouldn't happen. */
      *vsource = NULL;
      *psource = NULL;
   }
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bmp;
   A5O_SHADER *shader;
   const char *vsource;
   const char *psource;

   parse_args(argc, argv);

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_install_keyboard();
   al_init_image_addon();
   init_platform_specific();

   al_set_new_display_flags(A5O_PROGRAMMABLE_PIPELINE | display_flags);

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Could not load bitmap.\n");
   }

   shader = al_create_shader(A5O_SHADER_AUTO);
   if (!shader) {
      abort_example("Could not create shader.\n");
   }

   choose_shader_source(shader, &vsource, &psource);
   if (!vsource|| !psource) {
      abort_example("Could not load source files.\n");
   }

   if (!al_attach_shader_source_file(shader, A5O_VERTEX_SHADER, vsource)) {
      abort_example("al_attach_shader_source_file failed: %s\n",
         al_get_shader_log(shader));
   }
   if (!al_attach_shader_source_file(shader, A5O_PIXEL_SHADER, psource)) {
      abort_example("al_attach_shader_source_file failed: %s\n",
         al_get_shader_log(shader));
   }

   if (!al_build_shader(shader)) {
      abort_example("al_build_shader failed: %s\n", al_get_shader_log(shader));
   }

   al_use_shader(shader);

   float tints[12] = {
      4.0, 0.0, 1.0,
      0.0, 4.0, 1.0,
      1.0, 0.0, 4.0,
      4.0, 4.0, 1.0
   };

   while (1) {
      A5O_KEYBOARD_STATE s;
      al_get_keyboard_state(&s);
      if (al_key_down(&s, A5O_KEY_ESCAPE))
         break;

      al_clear_to_color(al_map_rgb(140, 40, 40));

      al_set_shader_float_vector("tint", 3, &tints[0], 1);
      al_draw_bitmap(bmp, 0, 0, 0);

      al_set_shader_float_vector("tint", 3, &tints[3], 1);
      al_draw_bitmap(bmp, 320, 0, 0);

      al_set_shader_float_vector("tint", 3, &tints[6], 1);
      al_draw_bitmap(bmp, 0, 240, 0);

      /* Draw the last one transformed */
      A5O_TRANSFORM trans, backup;
      al_copy_transform(&backup, al_get_current_transform());
      al_identity_transform(&trans);
      al_translate_transform(&trans, 320, 240);
      al_set_shader_float_vector("tint", 3, &tints[9], 1);
      al_use_transform(&trans);
      al_draw_bitmap(bmp, 0, 0, 0);
      al_use_transform(&backup);

      al_flip_display();

      al_rest(0.01);
   }

   al_use_shader(NULL);
   al_destroy_shader(shader);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
