/* An example demonstrating how to use A5O_TRANSFORM to represent a 3D
 * camera.
 */
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <math.h>

#include "common.c"

#define pi A5O_PI

typedef struct {
   float x, y, z;
} Vector;

typedef struct {
   Vector position;
   Vector xaxis; /* This represent the direction looking to the right. */
   Vector yaxis; /* This is the up direction. */
   Vector zaxis; /* This is the direction towards the viewer ('backwards'). */
   double vertical_field_of_view; /* In radians. */
} Camera;

typedef struct {
   Camera camera;

   /* controls sensitivity */
   double mouse_look_speed;
   double movement_speed;

   /* keyboard and mouse state */
   int button[10];
   int key[A5O_KEY_MAX];
   int keystate[A5O_KEY_MAX];
   int mouse_dx, mouse_dy;

   /* control scheme selection */
   int controls;
   char const *controls_names[3];

   /* the vertex data */
   int n, v_size;
   A5O_VERTEX *v;

   /* used to draw some info text */
   A5O_FONT *font;

   /* if not NULL the skybox picture to use */
   A5O_BITMAP *skybox;
} Example;

Example ex;

/* Calculate the dot product between two vectors. This corresponds to the
 * angle between them times their lengths.
 */
static double vector_dot_product(Vector a, Vector b)
{
   return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* Calculate the cross product of two vectors. This produces a normal to the
 * plane containing the operands.
 */
static Vector vector_cross_product(Vector a, Vector b)
{
   Vector v = {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
   return v;
}

/* Return a vector multiplied by a scalar. */
static Vector vector_mul(Vector a, float s)
{
   Vector v = {a.x * s, a.y * s, a.z * s};
   return v;
}

/* Return the vector norm (length). */
static double vector_norm(Vector a)
{
   return sqrt(vector_dot_product(a, a));
}

/* Return a normalized version of the given vector. */
static Vector vector_normalize(Vector a)
{
   double s = vector_norm(a);
   if (s == 0)
      return a;
   return vector_mul(a, 1 / s);
}

/* In-place add another vector to a vector. */
static void vector_iadd(Vector *a, Vector b)
{
   a->x += b.x;
   a->y += b.y;
   a->z += b.z;
}

/* Rotate the camera around the given axis. */
static void camera_rotate_around_axis(Camera *c, Vector axis, double radians)
{
   A5O_TRANSFORM t;
   al_identity_transform(&t);
   al_rotate_transform_3d(&t, axis.x, axis.y, axis.z, radians);
   al_transform_coordinates_3d(&t, &c->yaxis.x, &c->yaxis.y, &c->yaxis.z);
   al_transform_coordinates_3d(&t, &c->zaxis.x, &c->zaxis.y, &c->zaxis.z);

   /* Make sure the axes remain orthogonal to each other. */
   c->zaxis = vector_normalize(c->zaxis);
   c->xaxis = vector_cross_product(c->yaxis, c->zaxis);
   c->xaxis = vector_normalize(c->xaxis);
   c->yaxis = vector_cross_product(c->zaxis, c->xaxis);
}

/* Move the camera along its x axis and z axis (which corresponds to
 * right and backwards directions).
 */
static void camera_move_along_direction(Camera *camera, double right,
   double forward)
{
   vector_iadd(&camera->position, vector_mul(camera->xaxis, right));
   vector_iadd(&camera->position, vector_mul(camera->zaxis, -forward));
}

/* Get a vector with y = 0 looking in the opposite direction as the camera z
 * axis. If looking straight up or down returns a 0 vector instead.
 */
static Vector get_ground_forward_vector(Camera *camera)
{
   Vector move = vector_mul(camera->zaxis, -1);
   move.y = 0;
   return vector_normalize(move);
}

/* Get a vector with y = 0 looking in the same direction as the camera x axis.
 * If looking straight up or down returns a 0 vector instead.
 */
static Vector get_ground_right_vector(Camera *camera)
{
   Vector move = camera->xaxis;
   move.y = 0;
   return vector_normalize(move);
}

/* Like camera_move_along_direction but moves the camera along the ground plane
 * only.
 */
static void camera_move_along_ground(Camera *camera, double right,
   double forward)
{
   Vector f = get_ground_forward_vector(camera);
   Vector r = get_ground_right_vector(camera);
   camera->position.x += f.x * forward + r.x * right;
   camera->position.z += f.z * forward + r.z * right;
}

/* Calculate the pitch of the camera. This is the angle between the z axis
 * vector and our direction vector on the y = 0 plane.
 */
static double get_pitch(Camera *c)
{
   Vector f = get_ground_forward_vector(c);
   return asin(vector_dot_product(f, c->yaxis));
}

/* Calculate the yaw of the camera. This is basically the compass direction.
 */
static double get_yaw(Camera *c)
{
   return atan2(c->zaxis.x, c->zaxis.z);
}

/* Calculate the roll of the camera. This is the angle between the x axis
 * vector and its project on the y = 0 plane.
 */
static double get_roll(Camera *c)
{
   Vector r = get_ground_right_vector(c);
   return asin(vector_dot_product(r, c->yaxis));
}

/* Set up a perspective transform. We make the screen span
 * 2 vertical units (-1 to +1) with square pixel aspect and the camera's
 * vertical field of view. Clip distance is always set to 1.
 */
static void setup_3d_projection(void)
{
   A5O_TRANSFORM projection;
   A5O_DISPLAY *display = al_get_current_display();
   double dw = al_get_display_width(display);
   double dh = al_get_display_height(display);
   double f;
   al_identity_transform(&projection);
   al_translate_transform_3d(&projection, 0, 0, -1);
   f = tan(ex.camera.vertical_field_of_view / 2);
   al_perspective_transform(&projection, -1 * dw / dh * f, f,
      1,
      f * dw / dh, -f, 1000);
   al_use_projection_transform(&projection);
}

/* Adds a new vertex to our scene. */
static void add_vertex(double x, double y, double z, double u, double v,
      A5O_COLOR color)
{
   int i = ex.n++;
   if (i >= ex.v_size) {
      ex.v_size += 1;
      ex.v_size *= 2;
      ex.v = realloc(ex.v, ex.v_size * sizeof *ex.v);
   }
   ex.v[i].x = x;
   ex.v[i].y = y;
   ex.v[i].z = z;
   ex.v[i].u = u;
   ex.v[i].v = v;
   ex.v[i].color = color;
}

/* Adds two triangles (6 vertices) to the scene. */
static void add_quad(double x, double y, double z, double u, double v,
   double ux, double uy, double uz, double uu, double uv,
   double vx, double vy, double vz, double vu, double vv,
   A5O_COLOR c1, A5O_COLOR c2)
{
   add_vertex(x, y, z, u, v, c1);
   add_vertex(x + ux, y + uy, z + uz, u + uu, v + uv, c1);
   add_vertex(x + vx, y + vy, z + vz, u + vu, v + vv, c2);
   add_vertex(x + vx, y + vy, z + vz, u + vu, v + vv, c2);
   add_vertex(x + ux, y + uy, z + uz, u + uu, v + uv, c1);
   add_vertex(x + ux + vx, y + uy + vy, z + uz + vz, u + uu + vu,
      v + uv + vv, c2);
}

/* Create a checkerboard made from colored quads. */
static void add_checkerboard(void)
{
   int x, y;
   A5O_COLOR c1 = al_color_name("yellow");
   A5O_COLOR c2 = al_color_name("green");

   for (y = 0; y < 20; y++) {
      for (x = 0; x < 20; x++) {
         double px = x - 20 * 0.5;
         double py = 0.2;
         double pz = y - 20 * 0.5;
         A5O_COLOR c = c1;
         if ((x + y) & 1) {
            c = c2;
            py -= 0.1;
         }
         add_quad(px, py, pz, 0, 0,
            1, 0, 0, 0, 0,
            0, 0, 1, 0, 0,
            c, c);
      }
   }
}

/* Create a skybox. This is simply 5 quads with a fixed distance to the
 * camera.
 */
static void add_skybox(void)
{
   Vector p = ex.camera.position;
   A5O_COLOR c1 = al_color_name("black");
   A5O_COLOR c2 = al_color_name("blue");
   A5O_COLOR c3 = al_color_name("white");

   double a = 0, b = 0;
   if (ex.skybox) {
      a = al_get_bitmap_width(ex.skybox) / 4.0;
      b = al_get_bitmap_height(ex.skybox) / 3.0;
      c1 = c2 = c3;
   }

   /* Back skybox wall. */
   add_quad(p.x - 50, p.y - 50, p.z - 50, a * 4, b * 2,
      100, 0, 0, -a, 0,
      0, 100, 0, 0, -b,
      c1, c2);
   /* Front skybox wall. */
   add_quad(p.x - 50, p.y - 50, p.z + 50, a, b * 2,
      100, 0, 0, a, 0,
      0, 100, 0, 0, -b,
      c1, c2);
   /* Left skybox wall. */
   add_quad(p.x - 50, p.y - 50, p.z - 50, 0, b * 2,
      0, 0, 100, a, 0,
      0, 100, 0, 0, -b,
      c1, c2);
   /* Right skybox wall. */
   add_quad(p.x + 50, p.y - 50, p.z - 50, a * 3, b * 2,
      0, 0, 100, -a, 0,
      0, 100, 0, 0, -b,
      c1, c2);

   /* Top of skybox. */
   add_vertex(p.x - 50, p.y + 50, p.z - 50, a, 0, c2);
   add_vertex(p.x + 50, p.y + 50, p.z - 50, a * 2, 0, c2);
   add_vertex(p.x, p.y + 50, p.z, a * 1.5, b * 0.5, c3);

   add_vertex(p.x + 50, p.y + 50, p.z - 50, a * 2, 0, c2);
   add_vertex(p.x + 50, p.y + 50, p.z + 50, a * 2, b, c2);
   add_vertex(p.x, p.y + 50, p.z, a * 1.5, b * 0.5, c3);

   add_vertex(p.x + 50, p.y + 50, p.z + 50, a * 2, b, c2);
   add_vertex(p.x - 50, p.y + 50, p.z + 50, a, b, c2);
   add_vertex(p.x, p.y + 50, p.z, a * 1.5, b * 0.5, c3);

   add_vertex(p.x - 50, p.y + 50, p.z + 50, a, b, c2);
   add_vertex(p.x - 50, p.y + 50, p.z - 50, a, 0, c2);
   add_vertex(p.x, p.y + 50, p.z, a * 1.5, b * 0.5, c3);
}

static void draw_scene(void)
{
   Camera *c = &ex.camera;
   /* We save Allegro's projection so we can restore it for drawing text. */
   A5O_TRANSFORM projection = *al_get_current_projection_transform();
   A5O_TRANSFORM t;
   A5O_COLOR back = al_color_name("black");
   A5O_COLOR front = al_color_name("white");
   int th;
   double pitch, yaw, roll;

   setup_3d_projection();
   al_clear_to_color(back);

   /* We use a depth buffer. */
   al_set_render_state(A5O_DEPTH_TEST, 1);
   al_clear_depth_buffer(1);

   /* Recreate the entire scene geometry - this is only a very small example
    * so this is fine.
    */
   ex.n = 0;
   add_checkerboard();
   add_skybox();

   /* Construct a transform corresponding to our camera. This is an inverse
    * translation by the camera position, followed by an inverse rotation
    * from the camera orientation.
    */
   al_build_camera_transform(&t, 
      ex.camera.position.x, ex.camera.position.y, ex.camera.position.z,
      ex.camera.position.x - ex.camera.zaxis.x,
      ex.camera.position.y - ex.camera.zaxis.y,
      ex.camera.position.z - ex.camera.zaxis.z,
      ex.camera.yaxis.x, ex.camera.yaxis.y, ex.camera.yaxis.z);
   al_use_transform(&t);
   al_draw_prim(ex.v, NULL, ex.skybox, 0, ex.n, A5O_PRIM_TRIANGLE_LIST);

   /* Restore projection. */
   al_identity_transform(&t);
   al_use_transform(&t);
   al_use_projection_transform(&projection);
   al_set_render_state(A5O_DEPTH_TEST, 0);

   /* Draw some text. */
   th = al_get_font_line_height(ex.font);
   al_draw_textf(ex.font, front, 0, th * 0, 0,
      "look: %+3.1f/%+3.1f/%+3.1f (change with left mouse button and drag)",
         -c->zaxis.x, -c->zaxis.y, -c->zaxis.z);
   pitch = get_pitch(c) * 180 / pi;
   yaw = get_yaw(c) * 180 / pi;
   roll = get_roll(c) * 180 / pi;
   al_draw_textf(ex.font, front, 0, th * 1, 0,
      "pitch: %+4.0f yaw: %+4.0f roll: %+4.0f", pitch, yaw, roll);
   al_draw_textf(ex.font, front, 0, th * 2, 0,
      "vertical field of view: %3.1f (change with Z/X)",
         c->vertical_field_of_view * 180 / pi);
   al_draw_textf(ex.font, front, 0, th * 3, 0, "move with WASD or cursor");
   al_draw_textf(ex.font, front, 0, th * 4, 0, "control style: %s (space to change)",
      ex.controls_names[ex.controls]);
}

static void setup_scene(void)
{
   ex.camera.xaxis.x = 1;
   ex.camera.yaxis.y = 1;
   ex.camera.zaxis.z = 1;
   ex.camera.position.y = 2;
   ex.camera.vertical_field_of_view = 60 * pi / 180;

   ex.mouse_look_speed = 0.03;
   ex.movement_speed = 0.05;

   ex.controls_names[0] = "FPS";
   ex.controls_names[1] = "airplane";
   ex.controls_names[2] = "spaceship";

   ex.font = al_create_builtin_font();
}

static void handle_input(void)
{
   double x = 0, y = 0;
   double xy;
   if (ex.key[A5O_KEY_A] || ex.key[A5O_KEY_LEFT]) x = -1;
   if (ex.key[A5O_KEY_S] || ex.key[A5O_KEY_DOWN]) y = -1;
   if (ex.key[A5O_KEY_D] || ex.key[A5O_KEY_RIGHT]) x = 1;
   if (ex.key[A5O_KEY_W] || ex.key[A5O_KEY_UP]) y = 1;

   /* Change field of view with Z/X. */
   if (ex.key[A5O_KEY_Z]) {
      double m = 20 * pi / 180;
      ex.camera.vertical_field_of_view -= 0.01;
      if (ex.camera.vertical_field_of_view < m)
         ex.camera.vertical_field_of_view = m;
   }
   if (ex.key[A5O_KEY_X]) {
      double m = 120 * pi / 180;
      ex.camera.vertical_field_of_view += 0.01;
      if (ex.camera.vertical_field_of_view > m)
         ex.camera.vertical_field_of_view = m;
   }

   /* In FPS style, always move the camera to height 2. */
   if (ex.controls == 0) {
      if (ex.camera.position.y > 2)
         ex.camera.position.y -= 0.1;
      if (ex.camera.position.y < 2)
         ex.camera.position.y = 2;
   }

   /* Set the roll (leaning) angle to 0 if not in airplane style. */
   if (ex.controls == 0 || ex.controls == 2) {
      double roll = get_roll(&ex.camera);
      camera_rotate_around_axis(&ex.camera, ex.camera.zaxis, roll / 60);
   }

   /* Move the camera, either freely or along the ground. */
   xy = sqrt(x * x + y * y);
   if (xy > 0) {
      x /= xy;
      y /= xy;
      if (ex.controls == 0) {
         camera_move_along_ground(&ex.camera, ex.movement_speed * x,
            ex.movement_speed * y);
      }
      if (ex.controls == 1 || ex.controls == 2) {
         camera_move_along_direction(&ex.camera, ex.movement_speed * x,
            ex.movement_speed * y);
      }
   }

   /* Rotate the camera, either freely or around world up only. */
   if (ex.button[1]) {
      if (ex.controls == 0 || ex.controls == 2) {
         Vector up = {0, 1, 0};
         camera_rotate_around_axis(&ex.camera, ex.camera.xaxis,
            -ex.mouse_look_speed * ex.mouse_dy);
         camera_rotate_around_axis(&ex.camera, up,
            -ex.mouse_look_speed * ex.mouse_dx);
      }
      if (ex.controls == 1) {
         camera_rotate_around_axis(&ex.camera, ex.camera.xaxis,
            -ex.mouse_look_speed * ex.mouse_dy);
         camera_rotate_around_axis(&ex.camera, ex.camera.zaxis,
            -ex.mouse_look_speed * ex.mouse_dx);
      }
   }
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   int redraw = 0;
   bool halt_drawing = false;
   char const *skybox_name = NULL;

   if (argc > 1) {
      skybox_name = argv[1];
   }

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_font_addon();
   al_init_primitives_addon();
   init_platform_specific();
   al_install_keyboard();
   al_install_mouse();

   al_set_config_value(al_get_system_config(), "osx", "allow_live_resize", "false");
   al_set_new_display_option(A5O_SAMPLE_BUFFERS, 1, A5O_SUGGEST);
   al_set_new_display_option(A5O_SAMPLES, 8, A5O_SUGGEST);
   al_set_new_display_option(A5O_DEPTH_SIZE, 16, A5O_SUGGEST);
   al_set_new_display_flags(A5O_RESIZABLE);
   display = al_create_display(640, 360);
   if (!display) {
      abort_example("Error creating display\n");
   }

   if (skybox_name) {
      al_init_image_addon();
      ex.skybox = al_load_bitmap(skybox_name);
      if (ex.skybox) {
         printf("Loaded skybox %s: %d x %d\n", skybox_name,
            al_get_bitmap_width(ex.skybox),
            al_get_bitmap_height(ex.skybox));
      }
      else {
         printf("Failed loading skybox %s\n", skybox_name);
      }
   }

   timer = al_create_timer(1.0 / 60);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));

   setup_scene();

   al_start_timer(timer);
   while (true) {
      A5O_EVENT event;

      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
         break;
      else if (event.type == A5O_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(display);
      }
      else if (event.type == A5O_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE)
            break;
         if (event.keyboard.keycode == A5O_KEY_SPACE) {
            ex.controls++;
            ex.controls %= 3;
         }
         ex.key[event.keyboard.keycode] = 1;
         ex.keystate[event.keyboard.keycode] = 1;
      }
      else if (event.type == A5O_EVENT_KEY_UP) {
         /* In case a key gets pressed and immediately released, we will still
          * have set ex.key so it is not lost.
          */
         ex.keystate[event.keyboard.keycode] = 0;
      }
      else if (event.type == A5O_EVENT_TIMER) {
         int i;
         handle_input();
         redraw = 1;

         /* Reset keyboard state for keys not held down anymore. */
         for (i = 0; i < A5O_KEY_MAX; i++) {
            if (ex.keystate[i] == 0)
               ex.key[i] = 0;
         }
         ex.mouse_dx = 0;
         ex.mouse_dy = 0;
      }
      else if (event.type == A5O_EVENT_MOUSE_BUTTON_DOWN) {
         ex.button[event.mouse.button] = 1;
      }
      else if (event.type == A5O_EVENT_MOUSE_BUTTON_UP) {
         ex.button[event.mouse.button] = 0;
      }
      else if (event.type == A5O_EVENT_DISPLAY_HALT_DRAWING) {
         halt_drawing = true;
         al_acknowledge_drawing_halt(display);
      }
      else if (event.type == A5O_EVENT_DISPLAY_RESUME_DRAWING) {
         halt_drawing = false;
         al_acknowledge_drawing_resume(display);
      }
      else if (event.type == A5O_EVENT_MOUSE_AXES) {
         ex.mouse_dx += event.mouse.dx;
         ex.mouse_dy += event.mouse.dy;
      }

      if (!halt_drawing && redraw && al_is_event_queue_empty(queue)) {
         draw_scene();

         al_flip_display();
         redraw = 0;
      }
   }

   return 0;
}
