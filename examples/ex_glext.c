/* This examples demonstrates how to use the extension mechanism.
 * Taken from AllegroGL.
 */

#define ALLEGRO_UNSTABLE

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>

#include "common.c"

#ifdef __APPLE_CC__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#define WINDOW_W 640
#define WINDOW_H 480

#define MESH_SIZE 64

GLfloat mesh[MESH_SIZE][MESH_SIZE][3];
GLfloat wave_movement = 0.0f;


/* Define our vertex program.
 * It basically does:
 *  pos   = vertex.position;
 *  pos.y = (sin(wave.x + pos.x / 5) + sin(wave.x + pos.z / 4)) * 2.5;
 *  result.position = modelview * projection * pos;
 */

/* Plain ARBvp doesn't have a SIN opcode, so we provide one, built on a taylor
 * expansion, with some fugding.
 *
 * First, we convert the operand to the [-pi..+pi] period by:
 *  - Dividing by 2pi, adding 1/2
 *  - Taking the fraction of the result
 *  - Multiplying by 2pi, then subtracting pi.
 *     x' = frac((x / 2pi) + 0.5) * 2pi - pi
 *
 * Then, we compute the sine using a 7th order Taylor series centered at 0:
 *     x' = x - x^3/3! + x^5/5! - x^7/7!
 *
 * Note that we begin by multiplying x by 0.98 as a fugding factor to
 * compensate for the fact that our Taylor series is just an approximation.
 * The error is then reduced to < 0.5% from the ideal sine function.
 */
#define SIN(d, s, t)                                                    \
   /* Convert to [-pi..+pi] period */                                   \
   "MAD "d", "s", one_over_pi, 0.5;\n"                                  \
   "FRC "d","d";\n"                                                     \
   "MAD "d","d", two_pi, -pi;\n"                                        \
   "MUL "d","d", 0.98;\n" /* Scale input to compensate for prec error */\
   /* Compute SIN(d), using a Taylor series */                          \
   "MUL "t".x, "d",   "d";\n"           /* x^2 */                       \
   "MUL "t".y, "t".x, "d";\n"           /* x^3 */                       \
   "MUL "t".z, "t".y, "t".x;\n"         /* x^5 */                       \
   "MUL "t".w, "t".z, "t".x;\n"         /* x^7 */                       \
   "MAD "d", "t".y,-inv_3_fact, "d";\n" /* x - x^3/3! */                \
   "MAD "d", "t".z, inv_5_fact, "d";\n" /* x - x^3/3! + x^5/5! */       \
   "MAD "d", "t".w,-inv_7_fact, "d";\n" /* x - x^3/3! + x^5/5! - x^7/7!*/


/* This is the actual vertex program. 
 * It computes sin(wave.x + pos.x / 5) and sin(wave.x + pos.z), adds them up,
 * scales the result by 2.5 and stores that as the vertex's y component.
 *
 * Then, it does the modelview-projection transform on the vertex.
 *
 * XXX<rohannessian> Broken ATI drivers need a \n after each "line"
 */
const char *program =
   "!!ARBvp1.0\n"
   "ATTRIB pos  = vertex.position;\n"
   "ATTRIB wave = vertex.attrib[1];\n"
   "PARAM modelview[4] = { state.matrix.mvp };\n"
   "PARAM one_over_pi = 0.1591549;\n"
   "PARAM pi          = 3.1415926;\n"
   "PARAM two_pi      = 6.2831853;\n"
   "PARAM inv_3_fact  = 0.1666666;\n"
   "PARAM inv_5_fact  = 0.00833333333;\n"
   "PARAM inv_7_fact  = 0.00019841269841269;\n"
   "TEMP temp, temp2;\n"

   /* temp.y = sin(wave.x + pos.x / 5) */
   "MAD temp.y, pos.x, 0.2, wave.x;\n"
   SIN("temp.y", "temp.y", "temp2")

   /* temp.y += sin(wave.x + pos.z / 4) */
   "MAD temp.x, pos.z, 0.25, wave.x;\n"
   SIN("temp.x", "temp.x", "temp2")
   "ADD temp.y, temp.x, temp.y;\n"

   /* pos.y = temp.y * 2.5 */
   "MOV temp2, pos;\n"
   "MUL temp2.y, temp.y, 2.5;\n"

   /* Transform the position by the modelview matrix */
   "DP4 result.position.w, temp2, modelview[3];\n"
   "DP4 result.position.x, temp2, modelview[0];\n"
   "DP4 result.position.y, temp2, modelview[1];\n"
   "DP4 result.position.z, temp2, modelview[2];\n"

   "MOV result.color, vertex.color;\n"
   "END";


/* NVIDIA drivers do a better job; let's use a simpler program if we can.
 */
const char *program_nv = 
   "!!ARBvp1.0"
   "OPTION NV_vertex_program2;"
   "ATTRIB wave = vertex.attrib[1];"
   "PARAM modelview[4] = { state.matrix.mvp };"
   "TEMP temp;"
   "TEMP pos;"

   "MOV pos, vertex.position;"

   /* temp.x = sin(wave.x + pos.x / 5) */
   /* temp.z = sin(wave.x + pos.z / 4) */
   "MAD temp.xz, pos, {0.2, 1.0, 0.25, 1.0}, wave.x;"
   "SIN temp.x, temp.x;"
   "SIN temp.z, temp.z;"
   
   /* temp.y = temp.x + temp.z */
   "ADD temp.y, temp.x, temp.z;"

   /* pos.y = temp.y * 2.5 */
   "MUL pos.y, temp.y, 2.5;"

   /* Transform the position by the modelview matrix */
   "DP4 result.position.w, pos, modelview[3];"
   "DP4 result.position.x, pos, modelview[0];"
   "DP4 result.position.y, pos, modelview[1];"
   "DP4 result.position.z, pos, modelview[2];"

   "MOV result.color, vertex.color;"
   "END";


static void create_mesh(void)
{
	int x, z;

	/* Create our mesh */
	for (x = 0; x < MESH_SIZE; x++) {
		for (z = 0; z < MESH_SIZE; z++) {
			mesh[x][z][0] = (float) (MESH_SIZE / 2) - x;
			mesh[x][z][1] = 0.0f;
			mesh[x][z][2] = (float) (MESH_SIZE / 2) - z;
		}
	}
}



static void draw_mesh(void)
{
   int x, z;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glColor4f(0.5f, 1.0f, 0.5f, 1.0f);

   for (x = 0; x < MESH_SIZE - 1; x++) {

      glBegin(GL_TRIANGLE_STRIP);

      for (z = 0; z < MESH_SIZE - 1; z++) {
         glVertexAttrib1fARB(1, wave_movement);
         glVertex3fv(&mesh[x][z][0]);
         glVertex3fv(&mesh[x+1][z][0]);

         wave_movement += 0.00001f;

         if (wave_movement > 2 * ALLEGRO_PI) {
               wave_movement = 0.0f;
         }
      }
      glEnd();
   }

   glFlush();
}



int main(int argc, const char *argv[])
{
   GLuint pid;
   ALLEGRO_DISPLAY *d;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   ALLEGRO_TIMER *timer;
   int frames = 0;
   double start;
   bool limited = true;

   if (argc > 1 && 0 == strcmp(argv[1], "-nolimit")) {
      limited = false;
   }

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_set_new_display_flags(ALLEGRO_OPENGL);
   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
   al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);
   d = al_create_display(WINDOW_W, WINDOW_H);
   if (!d) {
      abort_example("Unable to open a OpenGL display.\n");
   }

   if (al_get_display_option(d, ALLEGRO_SAMPLE_BUFFERS)) {
      log_printf("With multisampling, level %i\n", al_get_display_option(d, ALLEGRO_SAMPLES));
   }
   else {
      log_printf("Without multisampling.\n");
   }

   al_install_keyboard();

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(d));

   if (limited) {
      timer = al_create_timer(1/60.0);
      al_register_event_source(queue, al_get_timer_event_source(timer));
      al_start_timer(timer);
   }
   else {
      timer = NULL;
   }

   if (al_get_opengl_extension_list()->ALLEGRO_GL_ARB_multisample) {
      glEnable(GL_MULTISAMPLE_ARB);
   }

   if (!al_get_opengl_extension_list()->ALLEGRO_GL_ARB_vertex_program) {
      abort_example("This example requires a video card that supports "
                     " the ARB_vertex_program extension.\n");
   }

   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_SMOOTH);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   glDisable(GL_CULL_FACE);

   /* Setup projection and modelview matrices */
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(45.0, WINDOW_W/(float)WINDOW_H, 0.1, 100.0);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   /* Position the camera to look at our mesh from a distance */
   gluLookAt(0.0f, 20.0f, -45.0f, 0.0f, 0.0f, 0.0f, 0, 1, 0);

   create_mesh();

   /* Define the vertex program */
   glEnable(GL_VERTEX_PROGRAM_ARB);
   glGenProgramsARB(1, &pid);
   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, pid);
   glGetError();

   if (al_get_opengl_extension_list()->ALLEGRO_GL_NV_vertex_program2_option) {
      glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(program_nv), program_nv);
   }
   else {
      glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(program), program);
   }

   /* Check for errors */
   if (glGetError()) {
      const char *pgm = al_get_opengl_extension_list()->ALLEGRO_GL_NV_vertex_program2_option
                        ? program_nv : program;
      GLint error_pos;
      const GLubyte *error_str = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &error_pos);

      abort_example("Error compiling the vertex program:\n%s\n\nat "
            "character: %i\n%s\n", error_str, (int)error_pos,
            pgm + error_pos);
   }


   start = al_get_time();
   while (1) {
      if (limited) {
         al_wait_for_event(queue, NULL);
      }

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

      draw_mesh();
      al_flip_display();
      frames++;
   }

done:
   log_printf("%.1f FPS\n", frames / (al_get_time() - start));
   glDeleteProgramsARB(1, &pid);
   al_destroy_event_queue(queue);
   al_destroy_display(d);
   close_log(true);

   return 0;
}
