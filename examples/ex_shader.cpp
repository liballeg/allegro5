#include <cstdio>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/allegro_shader.h"

/* The ALLEGRO_CFG_* defines are actually internal to Allegro so don't use them
 * in your own programs.
 */
#ifdef ALLEGRO_CFG_DIRECT3D
   #include "allegro5/allegro_direct3d.h"
#endif
#ifdef ALLEGRO_CFG_OPENGL
   #include "allegro5/allegro_opengl.h"
#endif
#ifdef ALLEGRO_CFG_SHADER_HLSL
   #include "allegro5/allegro_shader_hlsl.h"
#endif
#ifdef ALLEGRO_CFG_SHADER_GLSL
   #include "allegro5/allegro_shader_glsl.h"
#endif
#ifdef ALLEGRO_CFG_SHADER_CG
   #include <Cg/cg.h>
#endif

#include "common.c"

static int display_flags = 0;
static ALLEGRO_SHADER_PLATFORM shader_platform = ALLEGRO_SHADER_AUTO;

static void parse_args(int argc, char **argv)
{
   int i;

   /*
    * --opengl and --d3d specify the display type.
    * --hlsl, --glsl, --cg specify the shader platform.
    * --cg can be combined with --opengl or --d3d.
    */

   for (i = 1; i < argc; i++) {
      if (0 == strcmp(argv[i], "--opengl")) {
         display_flags = ALLEGRO_OPENGL;
         continue;
      }
#ifdef ALLEGRO_DIRECT3D
      if (0 == strcmp(argv[i], "--d3d")) {
         display_flags = ALLEGRO_DIRECT3D;
         continue;
      }
#endif
      if (0 == strcmp(argv[i], "--hlsl")) {
         shader_platform = ALLEGRO_SHADER_HLSL;
         continue;
      }
      if (0 == strcmp(argv[i], "--glsl")) {
         shader_platform = ALLEGRO_SHADER_GLSL;
         continue;
      }
      if (0 == strcmp(argv[i], "--cg")) {
         shader_platform = ALLEGRO_SHADER_CG;
         continue;
      }
      abort_example("Unrecognised argument: %s\n", argv[i]);
   }
}

static void choose_vertex_source(int display_flags,
   ALLEGRO_SHADER_PLATFORM shader_platform,
   char const **vsource, char const **psource)
{
   if (shader_platform == 0) {
      /* Shader platform depends on display type. */
      if (display_flags & ALLEGRO_OPENGL)
         shader_platform = ALLEGRO_SHADER_GLSL;
      else
         shader_platform = ALLEGRO_SHADER_HLSL;
   }

   if (shader_platform & ALLEGRO_SHADER_CG) {
      *vsource = "data/ex_shader_vertex.cg";
      *psource = "data/ex_shader_pixel.cg";
   }
   else if (shader_platform & ALLEGRO_SHADER_HLSL) {
      *vsource = "data/ex_shader_vertex.hlsl";
      *psource = "data/ex_shader_pixel.hlsl";
   }
   else if (shader_platform & ALLEGRO_SHADER_GLSL) {
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
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;
   const char *vsource;
   const char *psource;

   parse_args(argc, argv);

   al_init();
   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_USE_PROGRAMMABLE_PIPELINE | display_flags);

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   display_flags = al_get_display_flags(display);

   /* Reject incompatible display and shader types. */
   if ((display_flags & ALLEGRO_OPENGL)
         && (shader_platform & ALLEGRO_SHADER_HLSL)) {
      abort_example("Shader platform incompatible with display type.\n");
   }
#ifdef ALLEGRO_DIRECT3D
   if ((display_flags & ALLEGRO_DIRECT3D)
         && (shader_platform & ALLEGRO_SHADER_GLSL)) {
      abort_example("Shader platform incompatible with display type.\n");
   }
#endif

   bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Could not load bitmap.\n");
   }

   shader = al_create_shader(shader_platform);
   if (!shader) {
      abort_example("Could not create shader.\n");
   }

   choose_vertex_source(display_flags, shader_platform, &vsource, &psource);
   if (!vsource|| !psource) {
      abort_example("Could not load source files.\n");
   }

   if (!al_attach_shader_source_file(shader, ALLEGRO_VERTEX_SHADER, vsource)) {
      abort_example("al_attach_shader_source_file failed: %s\n",
         al_get_shader_log(shader));
   }
   if (!al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, psource)) {
      abort_example("al_attach_shader_source_file failed: %s\n",
         al_get_shader_log(shader));
   }

   if (!al_link_shader(shader)) {
      abort_example("al_link_shader failed: %s\n", al_get_shader_log(shader));
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

      al_set_shader_float_vector(shader, "tint", 3, &tints[0], 1);
      al_use_shader(shader, true);
      al_draw_bitmap(bmp, 0, 0, 0);
      al_use_shader(shader, false);

      al_set_shader_float_vector(shader, "tint", 3, &tints[3], 1);
      al_use_shader(shader, true);
      al_draw_bitmap(bmp, 320, 0, 0);
      al_use_shader(shader, false);
      
      al_set_shader_float_vector(shader, "tint", 3, &tints[6], 1);
      al_use_shader(shader, true);
      al_draw_bitmap(bmp, 0, 240, 0);
      al_use_shader(shader, false);
      
      /* Draw the last one transformed */
      ALLEGRO_TRANSFORM trans, backup;
      al_copy_transform(&backup, al_get_current_transform());
      al_identity_transform(&trans);
      al_translate_transform(&trans, 320, 240);
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
