#define ALLEGRO_UNSTABLE

#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_image.h>
#include <stdio.h>

#include "common.c"

GLhandleARB tinter;
GLhandleARB tinter_shader;

int main(int argc, char **argv)
{
   float r = 0.5, g = 0.5, b = 1, ratio = 0;
   int dir = 1;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *mysha;
   ALLEGRO_BITMAP *buffer;

   const char *tinter_shader_src[] = {
      "uniform sampler2D backBuffer;",
      "uniform float r;",
      "uniform float g;",
      "uniform float b;",
      "uniform float ratio;",
      "void main() {",
      "        vec4 color;",
      "        float avg, dr, dg, db;",
      "        color = texture2D(backBuffer, gl_TexCoord[0].st);",
      "        avg = (color.r + color.g + color.b) / 3.0;",
      "        dr = avg * r;",
      "        dg = avg * g;",
      "        db = avg * b;",
      "        color.r = color.r - (ratio * (color.r - dr));",
      "        color.g = color.g - (ratio * (color.g - dg));",
      "        color.b = color.b - (ratio * (color.b - db));",
      "        gl_FragColor = color;",
      "}"
   };
   const int TINTER_LEN = 18;
   double start;
   GLint loc;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro\n");
   }

   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_OPENGL);
   display = al_create_display(320, 200);
   if (!display) {
      abort_example("Error creating display\n");
   }

   mysha = al_load_bitmap("data/mysha.pcx");
   if (!mysha) {
      abort_example("Could not load image.\n");
   }

   buffer = al_create_bitmap(320, 200);

   if (!al_have_opengl_extension("GL_EXT_framebuffer_object")
      && !al_have_opengl_extension("GL_ARB_fragment_shader")) {
      abort_example("Fragment shaders not supported.\n");
   }

   tinter_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

   glShaderSourceARB(tinter_shader, TINTER_LEN, tinter_shader_src, NULL);
   glCompileShaderARB(tinter_shader);
   tinter = glCreateProgramObjectARB();
   glAttachObjectARB(tinter, tinter_shader);
   glLinkProgramARB(tinter);
   loc = glGetUniformLocationARB(tinter, "backBuffer");
   glUniform1iARB(loc, al_get_opengl_texture(buffer));

   start = al_get_time();

   while (1) {
      double now, diff;
      ALLEGRO_KEYBOARD_STATE state;
      al_get_keyboard_state(&state);
      if (al_key_down(&state, ALLEGRO_KEY_ESCAPE)) {
         break;
      }
      now = al_get_time();
      diff = now - start;
      start = now;
      ratio += diff * 0.5 * dir;
      if (dir < 0 && ratio < 0) {
         ratio = 0;
         dir = -dir;
      }
      else if (dir > 0 && ratio > 1) {
         ratio = 1;
         dir = -dir;
      }

      al_set_target_bitmap(buffer);

      glUseProgramObjectARB(tinter);
      loc = glGetUniformLocationARB(tinter, "ratio");
      glUniform1fARB(loc, ratio);
      loc = glGetUniformLocationARB(tinter, "r");
      glUniform1fARB(loc, r);
      loc = glGetUniformLocationARB(tinter, "g");
      glUniform1fARB(loc, g);
      loc = glGetUniformLocationARB(tinter, "b");
      glUniform1fARB(loc, b);
      al_draw_bitmap(mysha, 0, 0, 0);
      glUseProgramObjectARB(0);

      al_set_target_backbuffer(display);
      al_draw_bitmap(buffer, 0, 0, 0);
      al_flip_display();
      al_rest(0.001);
   }

   glDetachObjectARB(tinter, tinter_shader);
   glDeleteObjectARB(tinter_shader);

   al_uninstall_system();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
