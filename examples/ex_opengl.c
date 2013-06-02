#include <stdio.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>

#include "common.c"

/* Simple example showing how to use an extension. It draws a yellow triangle
 * on red background onto a texture, then draws a quad with that texture.
 */

GLuint tex, fbo;
bool no_fbo = false;

static void draw_opengl(void)
{
   double secs = al_get_time();

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   /* Let's create a texture. It will only be visible if we cannot draw over
    * it with the framebuffer extension.
    */
   if (!tex) {
      unsigned char *pixels = malloc(256 * 256 * 4);
      int x, y;

      for (y = 0; y < 256; y++) {
         for (x = 0; x < 256; x++) {
            unsigned char r = x, g = y, b = 0, a = 255;
            pixels[y * 256 * 4 + x * 4 + 0] = r;
            pixels[y * 256 * 4 + x * 4 + 1] = g;
            pixels[y * 256 * 4 + x * 4 + 2] = b;
            pixels[y * 256 * 4 + x * 4 + 3] = a;
         }
      }
      glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA,
         GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      free(pixels);
   }

   /* Let's create a framebuffer object. */
   if (!fbo && !no_fbo) {
      /* Did Allegro discover the OpenGL extension for us? */
      if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object) {
         /* If yes, then it also filled in the function pointer. How nice. */
         glGenFramebuffersEXT(1, &fbo);

         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
         /* Attach the framebuffer object to our texture. */
         glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
            GL_TEXTURE_2D, tex, 0);
         if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) !=
            GL_FRAMEBUFFER_COMPLETE_EXT) {
            no_fbo = true;
         }
         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      }
      else {
         /* We are screwed, the extension is not available. */
         no_fbo = true;
      }
   }

  /* Draw a yellow triangle on red background to the framebuffer object. */
   if (fbo && !no_fbo) {
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

      glPushAttrib(GL_VIEWPORT_BIT | GL_TRANSFORM_BIT);
      glViewport(0, 0, 256, 256);
      glClearColor(1, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      glMatrixMode(GL_PROJECTION);
      glPushMatrix();

      glLoadIdentity();
      glOrtho(0, 256, 256, 0, -1, 1);

      glDisable(GL_TEXTURE_2D);
      glColor3f(1, 1, 0);
      glTranslatef(128, 128 + sin(secs * ALLEGRO_PI * 2 * 2) * 20, 0);
      glBegin(GL_TRIANGLES);
      glVertex2f(0, -100);
      glVertex2f(100, 0);
      glVertex2f(-100, 0);
      glEnd();

      glPopMatrix();
      glPopAttrib();

      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   }

   /* Draw a quad with our texture. */
   glClearColor(0, 0, 1, 1);
   glClear(GL_COLOR_BUFFER_BIT);

   glLoadIdentity();
   glTranslatef(320, 240, 0);
   glRotatef(secs * 360 / 4, 0, 0, 1);
   glColor3f(1, 1, 1);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, tex);
   glBegin(GL_QUADS);
   glTexCoord2f(0, 0); glVertex2f(-100, -100);
   glTexCoord2f(1, 0); glVertex2f(+100, -100);
   glTexCoord2f(1, 1); glVertex2f(+100, +100);
   glTexCoord2f(0, 1); glVertex2f(-100, +100);
   glEnd();
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   int frames = 0;
   double start;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_install_keyboard();
   al_set_new_display_flags(ALLEGRO_OPENGL);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   start = al_get_time();
   while (true) {
      /* Check for ESC key or close button event and quit in either case. */
      if (!al_is_event_queue_empty(queue)) {
         while (al_get_next_event(queue, &event)) {
            switch (event.type) {
               case ALLEGRO_EVENT_DISPLAY_CLOSE:
                  goto done;

               case ALLEGRO_EVENT_KEY_DOWN:
                  if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                     goto done;
                  break;
            }
         }
      }
      draw_opengl();
      al_flip_display();
      frames++;
   }

done:

   log_printf("%.1f FPS\n", frames / (al_get_time() - start));
   al_destroy_event_queue(queue);
   al_destroy_display(display);

   close_log(true);

   return 0;
}

