/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    Modified by Francisco Pires to include a FPS counter and
 *    option to enable/disable waiting for vsync.
 *
 *    This program demonstrates how to use the get_camera_matrix()
 *    function to view a 3d world from any position and angle. The
 *    example draws a checkered floor through a viewport region
 *    on the screen. You can use the keyboard to move around the
 *    camera or modify the size of the viewport. The keys that can
 *    be used with this example are displayed between brackets at
 *    the top of the screen.
 */


#include <stdio.h>
#include <math.h>

#include <allegro.h>


#ifndef M_PI
   #define M_PI   3.14159
#endif


/* display a nice 8x8 chessboard grid */
#define GRID_SIZE    8


/* convert radians to degrees */
#define DEG(n)    ((n) * 180.0 / M_PI)


/* how many times per second the fps will be checked */
#define FPS_INT 2


/* uncomment to disable waiting for vsync */
/* #define DISABLE_VSYNC */


/* parameters controlling the camera and projection state */
int viewport_w = 320;
int viewport_h = 240;
int fov = 48;
float aspect = 1.33f;
float xpos = 0;
float ypos = -2;
float zpos = -4;
float heading = 0;
float pitch = 0;
float roll = 0;

int fps;
int framecount;



/* render a tile of the grid */
void draw_square(BITMAP *bmp, MATRIX_f *camera, int x, int z)
{
   V3D_f _v[4], _vout[8], _vtmp[8];
   V3D_f *v[4], *vout[8], *vtmp[8];
   int flags[4], out[8];
   int c, vc;

   for (c=0; c<4; c++)
      v[c] = &_v[c];

   for (c=0; c<8; c++) {
      vout[c] = &_vout[c];
      vtmp[c] = &_vtmp[c];
   }

   /* set up four vertices with the world-space position of the tile */
   v[0]->x = x - GRID_SIZE/2;
   v[0]->y = 0;
   v[0]->z = z - GRID_SIZE/2;

   v[1]->x = x - GRID_SIZE/2 + 1;
   v[1]->y = 0;
   v[1]->z = z - GRID_SIZE/2;

   v[2]->x = x - GRID_SIZE/2 + 1;
   v[2]->y = 0;
   v[2]->z = z - GRID_SIZE/2 + 1;

   v[3]->x = x - GRID_SIZE/2;
   v[3]->y = 0;
   v[3]->z = z - GRID_SIZE/2 + 1;

   /* apply the camera matrix, translating world space -> view space */
   for (c=0; c<4; c++) {
      apply_matrix_f(camera, v[c]->x, v[c]->y, v[c]->z,
		     &v[c]->x, &v[c]->y, &v[c]->z);

      flags[c] = 0;

      /* set flags if this vertex is off the edge of the screen */
      if (v[c]->x < -v[c]->z)
	 flags[c] |= 1;
      else if (v[c]->x > v[c]->z)
	 flags[c] |= 2;

      if (v[c]->y < -v[c]->z)
	 flags[c] |= 4;
      else if (v[c]->y > v[c]->z)
	 flags[c] |= 8;

      if (v[c]->z < 0.1)
	 flags[c] |= 16;
   }

   /* quit if all vertices are off the same edge of the screen */
   if (flags[0] & flags[1] & flags[2] & flags[3])
      return;

   if (flags[0] | flags[1] | flags[2] | flags[3]) {
      /* clip if any vertices are off the edge of the screen */
      vc = clip3d_f(POLYTYPE_FLAT, 0.1, 0.1, 4, (AL_CONST V3D_f **)v,
		    vout, vtmp, out);

      if (vc <= 0)
	 return;
   }
   else {
      /* no need to bother clipping this one */
      vout[0] = v[0];
      vout[1] = v[1];
      vout[2] = v[2];
      vout[3] = v[3];

      vc = 4;
   }

   /* project view space -> screen space */
   for (c=0; c<vc; c++)
      persp_project_f(vout[c]->x, vout[c]->y, vout[c]->z,
		      &vout[c]->x, &vout[c]->y);

   /* set the color */
   vout[0]->c = ((x + z) & 1) ? makecol(0, 255, 0) : makecol(255, 255, 0);

   /* render the polygon */
   polygon3d_f(bmp, POLYTYPE_FLAT, NULL, vc, vout);
}



/* draw everything */
void render(BITMAP *bmp)
{
   MATRIX_f roller, camera;
   int x, y, w, h;
   float xfront, yfront, zfront;
   float xup, yup, zup;

   /* clear the background */
   clear_to_color(bmp, makecol(255, 255, 255));

   /* set up the viewport region */
   x = (SCREEN_W - viewport_w) / 2;
   y = (SCREEN_H - viewport_h) / 2;
   w = viewport_w;
   h = viewport_h;

   set_projection_viewport(x, y, w, h);
   rect(bmp, x-1, y-1, x+w, y+h, makecol(255, 0, 0));
   set_clip_rect(bmp, x, y, x+w-1, y+h-1);

   /* calculate the in-front vector */
   xfront = sin(heading) * cos(pitch);
   yfront = sin(pitch);
   zfront = cos(heading) * cos(pitch);

   /* rotate the up vector around the in-front vector by the roll angle */
   get_vector_rotation_matrix_f(&roller, xfront, yfront, zfront,
				roll*128.0/M_PI);
   apply_matrix_f(&roller, 0, -1, 0, &xup, &yup, &zup);

   /* build the camera matrix */
   get_camera_matrix_f(&camera,
		       xpos, ypos, zpos,        /* camera position */
		       xfront, yfront, zfront,  /* in-front vector */
		       xup, yup, zup,           /* up vector */
		       fov,                     /* field of view */
		       aspect);                 /* aspect ratio */

   /* draw the grid of squares */
   for (x=0; x<GRID_SIZE; x++)
      for (y=0; y<GRID_SIZE; y++)
	 draw_square(bmp, &camera, x, y);

   /* overlay some text */
   set_clip_rect(bmp, 0, 0, bmp->w, bmp->h);
   textprintf_ex(bmp, font, 0,  0, makecol(0, 0, 0), -1,
		 "Viewport width: %d (w/W changes)", viewport_w);
   textprintf_ex(bmp, font, 0,  8, makecol(0, 0, 0), -1,
		 "Viewport height: %d (h/H changes)", viewport_h);
   textprintf_ex(bmp, font, 0, 16, makecol(0, 0, 0), -1,
		 "Field of view: %d (f/F changes)", fov);
   textprintf_ex(bmp, font, 0, 24, makecol(0, 0, 0), -1,
		 "Aspect ratio: %.2f (a/A changes)", aspect);
   textprintf_ex(bmp, font, 0, 32, makecol(0, 0, 0), -1,
		 "X position: %.2f (x/X changes)", xpos);
   textprintf_ex(bmp, font, 0, 40, makecol(0, 0, 0), -1,
		 "Y position: %.2f (y/Y changes)", ypos);
   textprintf_ex(bmp, font, 0, 48, makecol(0, 0, 0), -1,
		 "Z position: %.2f (z/Z changes)", zpos);
   textprintf_ex(bmp, font, 0, 56, makecol(0, 0, 0), -1,
		 "Heading: %.2f deg (left/right changes)", DEG(heading));
   textprintf_ex(bmp, font, 0, 64, makecol(0, 0, 0), -1,
		 "Pitch: %.2f deg (pgup/pgdn changes)", DEG(pitch));
   textprintf_ex(bmp, font, 0, 72, makecol(0, 0, 0), -1,
		 "Roll: %.2f deg (r/R changes)", DEG(roll));
   textprintf_ex(bmp, font, 0, 80, makecol(0, 0, 0), -1,
		 "Front vector: %.2f, %.2f, %.2f", xfront, yfront, zfront);
   textprintf_ex(bmp, font, 0, 88, makecol(0, 0, 0), -1,
		 "Up vector: %.2f, %.2f, %.2f", xup, yup, zup);
   textprintf_ex(bmp, font, 0, 96, makecol(0, 0, 0), -1,
		 "Frames per second: %d", fps);
}



/* deal with user input */
void process_input(void)
{
   double frac, iptr;

   poll_keyboard();

   if (key[KEY_W]) {
      if (key_shifts & KB_SHIFT_FLAG) {
	 if (viewport_w < SCREEN_W)
	    viewport_w += 8;
      }
      else { 
	 if (viewport_w > 16)
	    viewport_w -= 8;
      }
   }

   if (key[KEY_H]) {
      if (key_shifts & KB_SHIFT_FLAG) {
	 if (viewport_h < SCREEN_H)
	    viewport_h += 8;
      }
      else {
	 if (viewport_h > 16)
	    viewport_h -= 8;
      }
   }

   if (key[KEY_F]) {
      if (key_shifts & KB_SHIFT_FLAG) {
	 if (fov < 96)
	    fov++;
      }
      else {
	 if (fov > 16)
	    fov--;
      }
   }

   if (key[KEY_A]) {
      frac = modf(aspect*10.0, &iptr);

      if (key_shifts & KB_SHIFT_FLAG) {
	 if ((frac>0.59) && (frac<0.61))
	    aspect += 0.04f;
	 else
	    aspect += 0.03f;
	 if (aspect > 2)
	    aspect = 2;
      }
      else {
	 if ((frac>0.99) || (frac<0.01))
	    aspect -= 0.04f;
	 else
	    aspect -= 0.03f;
	 if (aspect < .1)
	    aspect = .1;
      }
   }

   if (key[KEY_X]) {
      if (key_shifts & KB_SHIFT_FLAG)
	 xpos += 0.05;
      else
	 xpos -= 0.05;
   }

   if (key[KEY_Y]) {
      if (key_shifts & KB_SHIFT_FLAG)
	 ypos += 0.05;
      else
	 ypos -= 0.05;
   }

   if (key[KEY_Z]) {
      if (key_shifts & KB_SHIFT_FLAG)
	 zpos += 0.05;
      else
	 zpos -= 0.05;
   }

   if (key[KEY_LEFT])
      heading -= 0.05;

   if (key[KEY_RIGHT])
      heading += 0.05;

   if (key[KEY_PGUP])
      if (pitch > -M_PI/4)
	 pitch -= 0.05;

   if (key[KEY_PGDN])
      if (pitch < M_PI/4)
	 pitch += 0.05;

   if (key[KEY_R]) {
      if (key_shifts & KB_SHIFT_FLAG) {
	 if (roll < M_PI/4)
	    roll += 0.05;
      }
      else {
	 if (roll > -M_PI/4)
	    roll -= 0.05;
      }
   }

   if (key[KEY_UP]) {
      xpos += sin(heading) / 4;
      zpos += cos(heading) / 4;
   }

   if (key[KEY_DOWN]) {
      xpos -= sin(heading) / 4;
      zpos -= cos(heading) / 4;
   }
}



void fps_check(void)
{
   fps = framecount*FPS_INT;
   framecount = 0;
}



int main(void)
{
   BITMAP *buffer;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n",
			 allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);
   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   install_int_ex(fps_check, BPS_TO_TIMER(FPS_INT));

   while (!key[KEY_ESC]) {
      render(buffer);

      #ifndef DISABLE_VSYNC
         vsync();
      #endif
      
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);       
      framecount++;

      process_input();
   }

   destroy_bitmap(buffer);
   return 0;
}

END_OF_MAIN()
