/*
 *    Example program for the Allegro library, by Jason Wilkins.
 *
 *    Euler angles are convenient for storing and creating 3D orientations.
 *    However, this program demonstrates that they are not good when
 *    interpolating between two different orientations. The problem is
 *    solved by using Allegro's quaternion operations.
 *
 *    In this program, two cubes are rotated between random orientations.
 *    Notice that although they have the same beginning and ending
 *    orientations, they do not follow the same path between orientations.
 *
 *    One cube is being rotated by directly incrementing or decrementing
 *    the Euler angles from the starting point to the ending point.
 *    This is an intuitive notion, but it is incorrect because it does not
 *    cause the object to turn around a single unchanging axis of rotation.
 *    The axis of rotation wobbles resulting in the object spinning in
 *    strange ways. The object will eventually end up in the orientation
 *    that the user intended, but it gets there in a way that is unattractive.
 *    Imagine if this method was used to update the position of a camera in a
 *    game! Sometimes it would swing wildly and disorient the player.
 *
 *    The other cube is animated using quaternions. This results in a much
 *    more pleasing animation because the cube turns around a single axis
 *    of rotation.
 */


#include <stdlib.h>
#include <time.h>

#include <allegro.h>



/* the number of steps to get from the starting to the ending orientation */
#define NUM_STEPS    200



/* this structure holds an orientation expressed as Euler angles. Each number
 * represents a rotation about the x, y, and z axis. In the case of Allegro
 * there are 256 degrees to a circle.  Yaw, pitch, and roll correspond to
 * x, y, and z 
 */
typedef struct EULER 
{
   float x, y, z;
} EULER;



/* matrix to transform world coordinates into normalized eye coordinates */
MATRIX_f camera;
MATRIX_f rotation;



/* the parts of the screen that display the demo boxes */
BITMAP *euler_screen;
BITMAP *quat_screen;



/* these are backbuffers, drawing is done here before updating the screen */
BITMAP *euler_buffer;
BITMAP *quat_buffer;



/* In these identifiers, 'from' refers to the starting orientation, 'to'
 * refers to the ending orientation and 'in' refers to the interpolated
 * orientation. 'q' refers to quaternion, 'e' refers to Euler
 */
MATRIX_f q_from_matrix;
MATRIX_f q_to_matrix;
MATRIX_f q_in_matrix;

MATRIX_f e_from_matrix;
MATRIX_f e_to_matrix;
MATRIX_f e_in_matrix;

QUAT q_to;
QUAT q_in;
QUAT q_from;

EULER e_from;
EULER e_to;
EULER e_in;



/* Here is defined a 2x2x2 cube centered about the origin, and
 * an arrow pointing straight up. They are wireframe objects
 * so only the points and edges are specified.
 *
 * It should be noted that the world coordinate system in this
 * program is oriented like it is in most math books. X and Y
 * are oriented like a floor and Z refers to the height above
 * that floor. (Mathematically, this is known as right-handed
 * coordinate system.)
 *
 * N - North
 * S - South
 * W - West
 * E - East
 * U - Up
 * D - Down
 */

float box_points[8][3] =
{
   /* X,    Y,    Z   */
   { -1.0, -1.0, -1.0 },   /* NWD */
   { -1.0, -1.0,  1.0 },   /* NWU */
   { -1.0,  1.0, -1.0 },   /* NED */
   { -1.0,  1.0,  1.0 },   /* NEU */
   {  1.0, -1.0, -1.0 },   /* SWD */
   {  1.0, -1.0,  1.0 },   /* SWU */
   {  1.0,  1.0, -1.0 },   /* SED */
   {  1.0,  1.0,  1.0 },   /* SEU */
};



int box_edges[12][2] =
{
   /* from, to */
   { 0, 2 },               /* bottom */
   { 2, 6 },
   { 6, 4 },
   { 4, 0 },
   { 1, 3 },               /* top */
   { 3, 7 },
   { 7, 5 },
   { 5, 1 },
   { 0, 1 },               /* sides */
   { 2, 3 },
   { 4, 5 },
   { 6, 7 }
};



float arrow_points[4][3] =
{
   /* X,    Y,    Z  */
   { 0.0,  0.0,  0.0 },    /* tail of the arrow, at the origin */
   { 0.0,  0.0,  2.0 },    /* tip of the arrow head */
   { 0.0,  0.25, 1.5 },    /* eastern part of the head */
   { 0.0, -0.25, 1.5 }     /* western part of the head */
};



int arrow_edges[3][2] =
{
   /* from, to */
   { 0, 1 },
   { 1, 2 },
   { 1, 3 }
};



/* Each demo box has associated with it two paths (stored as wireframe
 * objects). These are used to store a history of the orientation of their
 * interpolated axis. These sets of points are used to draw ribbons that
 * show how an object rotated from one orientation to another.
 */
float e_path_points_1[NUM_STEPS+1][3];
float e_path_points_2[NUM_STEPS+1][3];
float q_path_points_1[NUM_STEPS+1][3];
float q_path_points_2[NUM_STEPS+1][3];



/* these arrays are shared by both ribbons */
float tmp_points[NUM_STEPS+1][3];
int path_edges[NUM_STEPS][2];



/* draw an object defined as a set of points and edges */
void render_wireframe_object(MATRIX_f *m, BITMAP *b, float p[][3], float t[][3], int e[][2], int np, int ne, int c)
{
   int index, from, to;

   /* transform the points and store them in a buffer */
   for (index=0; index<np; index++) {
      apply_matrix_f(m, p[index][0], p[index][1], p[index][2],
		     &(t[index][0]), &(t[index][1]), &(t[index][2]));

      persp_project_f(t[index][0], t[index][1], t[index][2],
		      &(t[index][0]), &(t[index][1]));
   }

   /* draw the edges */
   for (index=0; index<ne; index++) {
      from = e[index][0];
      to = e[index][1];

      line(b, (int)(t[from][0]), (int)(t[from][1]), (int)(t[to][0]), (int)(t[to][1]), c);
   }
}



/* draws a set of objects that demonstrate whats going on. It consists
 * of a cube, an arrow showing the 'to' orientation, an another arrow 
 * showing the 'from' orientation, and another arrow showing the
 * interpolated orientation.
 */
void render_demo_box(BITMAP *b, MATRIX_f *from, MATRIX_f *in, MATRIX_f *to, int col1, int col2, int col3)
{
   float tmp_points[8][3];

   render_wireframe_object(in, b, box_points, tmp_points, box_edges, 8, 12, col1);
   render_wireframe_object(from, b, arrow_points, tmp_points, arrow_edges, 4, 3, col3);
   render_wireframe_object(to, b, arrow_points, tmp_points, arrow_edges, 4, 3, col3);
   render_wireframe_object(in, b, arrow_points, tmp_points, arrow_edges, 4, 3, col2);
}



/* Just interpolate linearly yaw, pitch, and roll. Doing this _correctly_
 * (I.E get the same results as quat_interpolate) would require one to use
 * linear integration, a subject that is in the last 100 pages of my 1500
 * page Calculus book. This function is an example of what you should NOT
 * do, as in some cases it will cause the orientation to swing wildly about.
 * The path could be anything from nearly correct, a spiral, or a curly Q.
 * The simple solution is to use quaternion interpolation, which always
 * results in a simple circular path.
 */
void euler_interpolate(EULER * from, EULER * to, float t, EULER * out)
{
   float delta;

   delta = (to->x-from->x) * t;
   out->x = from->x+delta;

   delta = (to->y-from->y) * t;
   out->y = from->y+delta;

   delta = (to->z-from->z) * t;
   out->z = from->z+delta;
}



int main(void)
{
   int index;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);
   clear_to_color(screen, palette_color[0]);

   /* Each back-buffer is one quarter the size of the screen
    */
   euler_buffer = create_bitmap(320, 240);
   quat_buffer = create_bitmap(320, 240);

   if ((!euler_buffer) || (!quat_buffer)) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error creating bitmaps\n");
      return 1;
   }

   set_palette(desktop_palette);

   /* setup the viewport for rendering into the back-buffers */
   set_projection_viewport(0, 0, 320, 240);

   /* print out something helpful for the user */
   textout_ex(screen, font, "SPACE - next interpolation", 184, 24, palette_color[15], -1);
   textout_ex(screen, font, "    R - repeat last interpolation", 184, 40, palette_color[15], -1);
   textout_ex(screen, font, "  ESC - quit", 184, 56, palette_color[15], -1);

   textout_ex(screen, font, "Interpolating Euler Angles", 56, 110, palette_color[15], -1);
   textout_ex(screen, font, "Interpolating Quaternions", 380, 110, palette_color[15], -1);

   textout_ex(screen, font, "Incorrect!", 120, 360, palette_color[15], -1);
   textout_ex(screen, font, "Correct!", 448, 360, palette_color[15], -1);

   /* initialize the path edges. This structure is used by both the Euler
    * path and the quaternion path. It connects all the points end to end
    */
   for (index=0; index<(NUM_STEPS-1); index++) {
      path_edges[index][0] = index;
      path_edges[index][1] = index + 1;
   }

   /* initialize the first destination orientation */
   srand(time(NULL));

   e_to.x = (float)(AL_RAND() % 256);
   e_to.y = (float)(AL_RAND() % 256);
   e_to.z = (float)(AL_RAND() % 256);

   /* the camera is backed away from the origin and turned to face it */
   get_camera_matrix_f(&camera, 5, 0, 0, -1, 0, 0, 0, 0, 1, 46, 1.33f);

   /* this is a for-ever loop */
   for (;;) {
      float t;

      clear_keybuf();

      for (index=0; index<(NUM_STEPS+1); index++) {
         if (keypressed())
	    break;

	 t = index * (1.0 / NUM_STEPS);

	 /* the first part shows how to animate the cube incorrectly
	  * using Euler angles
	  */

	 /* create the matrix for the starting orientation */
	 get_rotation_matrix_f(&rotation, e_from.x, e_from.y, e_from.z);
	 matrix_mul_f(&rotation, &camera, &e_from_matrix);

	 /* create the matrix for the ending orientation */
	 get_rotation_matrix_f(&rotation, e_to.x, e_to.y, e_to.z);
	 matrix_mul_f(&rotation, &camera, &e_to_matrix);

	 /* use the incorrect method to interpolate between them */
	 euler_interpolate(&e_from, &e_to, t, &e_in);
	 get_rotation_matrix_f(&rotation, e_in.x, e_in.y, e_in.z);
	 matrix_mul_f(&rotation, &camera, &e_in_matrix);

	 /* update the lines that make up the Euler orientation path */
	 apply_matrix_f(&rotation, 0, 0, 1.5,
			&(e_path_points_1[index][0]),
			&(e_path_points_1[index][1]),
			&(e_path_points_1[index][2]));

	 apply_matrix_f(&rotation, 0, 0, 2.0,
			&(e_path_points_2[index][0]),
			&(e_path_points_2[index][1]),
			&(e_path_points_2[index][2]));

	 /* render the results to the Euler sub-bitmap */
	 clear_to_color(euler_buffer, palette_color[0]);
	 render_demo_box(euler_buffer, &e_from_matrix, &e_in_matrix, &e_to_matrix,
			 palette_color[15], palette_color[1], palette_color[4]);

	 render_wireframe_object(&camera, euler_buffer, e_path_points_1,
				 tmp_points, path_edges, index+1, index,
				 palette_color[5]);

	 render_wireframe_object(&camera, euler_buffer, e_path_points_2,
				 tmp_points, path_edges, index+1, index,
				 palette_color[5]);

	 /* here is how to animate the cube correctly using quaternions */

	 /* create a matrix for the starting orientation. This time
	  * we create it using quaternions.  This is to demonstrate
	  * that the quaternion gotten with get_rotation_quat will
	  * generate the same matrix as that gotten by get_rotation_matrix
	  */
	 get_rotation_quat(&q_from, e_from.x, e_from.y, e_from.z);
	 quat_to_matrix(&q_from, &rotation);
	 matrix_mul_f(&rotation, &camera, &q_from_matrix);

	 /* this is the same as above, but for the ending orientation */
	 get_rotation_quat(&q_to, e_to.x, e_to.y, e_to.z);
	 quat_to_matrix(&q_to, &rotation);
	 matrix_mul_f(&rotation, &camera, &q_to_matrix);

	 /* quat_interpolate is the proper way to interpolate between two
	  * orientations. 
	  */
	 quat_interpolate(&q_from, &q_to, t, &q_in);
	 quat_to_matrix(&q_in, &rotation);
	 matrix_mul_f(&rotation, &camera, &q_in_matrix);

	 /* update the lines that make up the quaternion orientation path */
	 apply_matrix_f(&rotation, 0, 0, 1.5,
			&(q_path_points_1[index][0]),
			&(q_path_points_1[index][1]),
			&(q_path_points_1[index][2]));

	 apply_matrix_f(&rotation, 0, 0, 2.0,
			&(q_path_points_2[index][0]),
			&(q_path_points_2[index][1]),
			&(q_path_points_2[index][2]));

	 /* render the results to the quaternion sub-bitmap */
	 clear_to_color(quat_buffer, palette_color[0]);

	 render_demo_box(quat_buffer, &q_from_matrix, &q_in_matrix, &q_to_matrix,
	 		 palette_color[15], palette_color[1], palette_color[4]);

	 render_wireframe_object(&camera, quat_buffer, q_path_points_1,
				 tmp_points, path_edges, index+1, index,
				 palette_color[5]);

	 render_wireframe_object(&camera, quat_buffer, q_path_points_2,
				 tmp_points, path_edges, index+1, index,
				 palette_color[5]);

         acquire_bitmap(screen);
	 blit(euler_buffer, screen, 0, 0, 0,   120, 320, 240);
	 blit(quat_buffer,  screen, 0, 0, 320, 120, 320, 240);
         release_bitmap(screen);

	 rest(1);
      }

      /* handle user input */
      for (;;) {
	 int input = readkey() >> 8;

	 if (input == KEY_R) {
	    /* skip updating the EULER angles so that the last interpolation
	     * will repeat
	     */
	    break;
	 }
	 else if (input == KEY_SPACE) {
	    /* make the last ending orientation the starting orientation and
	     * generate a random new ending orientation
	     */
	    e_from = e_to;

	    e_to.x = (float)(AL_RAND() % 256);
	    e_to.y = (float)(AL_RAND() % 256);
	    e_to.z = (float)(AL_RAND() % 256);

	    break;
	 }
	 else if (input == KEY_ESC) {
	    /* quit the program */
	    destroy_bitmap(euler_buffer);
	    destroy_bitmap(quat_buffer);
	    return 0;
	 }
      }
   }
}
END_OF_MAIN()
