/*
 *    Example program for the Allegro library, by Dave Thomson.
 *
 *    This program draws a 3D star field (depth-cued) and a polygon
 *    starship (controllable with the keyboard cursor keys), using
 *    the Allegro math functions.
 */


#include <allegro.h>



/* star field system */
typedef struct VECTOR 
{
   fixed x, y, z;
} VECTOR;


#define NUM_STARS          512

#define Z_NEAR             24
#define Z_FAR              1024
#define XY_CUBE            2048

#define SPEED_LIMIT        20

VECTOR stars[NUM_STARS];

fixed star_x[NUM_STARS];
fixed star_y[NUM_STARS];

VECTOR delta;


/* polygonal models */
#define NUM_VERTS          4
#define NUM_FACES          4

#define ENGINE             3     /* which face is the engine */
#define ENGINE_ON          64    /* colour index */
#define ENGINE_OFF         32


typedef struct FACE              /* for triangular models */
{
   int v1, v2, v3;
   int colour, range;
   VECTOR normal, rnormal;
} FACE;


typedef struct MODEL 
{
   VECTOR points[NUM_VERTS];
   FACE faces[NUM_FACES];
   fixed x, y, z;
   fixed rx, ry, rz;
   int minx, miny, maxx, maxy;
   VECTOR aim;
   int velocity;
} MODEL;


MODEL ship;
VECTOR direction;


BITMAP *buffer;



/* initialises the star field system */
void init_stars(void)
{
   int i;

   for (i=0; i<NUM_STARS; i++) {
      stars[i].x = itofix((AL_RAND() % XY_CUBE) - (XY_CUBE >> 1));
      stars[i].y = itofix((AL_RAND() % XY_CUBE) - (XY_CUBE >> 1));
      stars[i].z = itofix((AL_RAND() % (Z_FAR - Z_NEAR)) + Z_NEAR);
   }

   delta.x = 0;
   delta.y = 0;
   delta.z = 0;
}



/* draws the star field */
void draw_stars(void)
{
   int i, c;
   MATRIX m;
   VECTOR outs[NUM_STARS];

   for (i=0; i<NUM_STARS; i++) {
      get_translation_matrix(&m, delta.x, delta.y, delta.z);
      apply_matrix(&m, stars[i].x, stars[i].y, stars[i].z,
		   &outs[i].x, &outs[i].y, &outs[i].z);
      persp_project(outs[i].x, outs[i].y, outs[i].z, &star_x[i], &star_y[i]);
      c = (fixtoi(outs[i].z) >> 8) + 16;
      putpixel(buffer, fixtoi(star_x[i]), fixtoi(star_y[i]),
	       palette_color[c]);
   }
}



/* deletes the stars from the screen */
void erase_stars(void)
{
   int i;

   for (i=0; i<NUM_STARS; i++)
      putpixel(buffer, fixtoi(star_x[i]), fixtoi(star_y[i]),
	       palette_color[0]);
}



/* moves the stars */
void move_stars(void)
{
   int i;

   for (i=0; i<NUM_STARS; i++) {
      stars[i].x += delta.x;
      stars[i].y += delta.y;
      stars[i].z += delta.z;

      if (stars[i].x > itofix(XY_CUBE >> 1))
	 stars[i].x = itofix(-(XY_CUBE >> 1));
      else if (stars[i].x < itofix(-(XY_CUBE >> 1)))
	 stars[i].x = itofix(XY_CUBE >> 1);

      if (stars[i].y > itofix(XY_CUBE >> 1))
	 stars[i].y = itofix(-(XY_CUBE >> 1));
      else if (stars[i].y < itofix(-(XY_CUBE >> 1)))
	 stars[i].y = itofix(XY_CUBE >> 1);

      if (stars[i].z > itofix(Z_FAR))
	 stars[i].z = itofix(Z_NEAR);
      else if (stars[i].z < itofix(Z_NEAR))
	 stars[i].z = itofix(Z_FAR);
   }
}



/* initialises the ship model */
void init_ship(void)
{
   VECTOR v1, v2, *pts;
   FACE *face;
   int i;

   ship.points[0].x = itofix(0);
   ship.points[0].y = itofix(0);
   ship.points[0].z = itofix(32);

   ship.points[1].x = itofix(16);
   ship.points[1].y = itofix(-16);
   ship.points[1].z = itofix(-32);

   ship.points[2].x = itofix(-16);
   ship.points[2].y = itofix(-16);
   ship.points[2].z = itofix(-32);

   ship.points[3].x = itofix(0);
   ship.points[3].y = itofix(16);
   ship.points[3].z = itofix(-32);

   ship.faces[0].v1 = 3;
   ship.faces[0].v2 = 0;
   ship.faces[0].v3 = 1;
   pts = &ship.points[0];
   face = &ship.faces[0];
   v1.x = (pts[face->v2].x - pts[face->v1].x);
   v1.y = (pts[face->v2].y - pts[face->v1].y);
   v1.z = (pts[face->v2].z - pts[face->v1].z);
   v2.x = (pts[face->v3].x - pts[face->v1].x);
   v2.y = (pts[face->v3].y - pts[face->v1].y);
   v2.z = (pts[face->v3].z - pts[face->v1].z);
   cross_product(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z,
		 &(face->normal.x), &(face->normal.y), &(face->normal.z));

   ship.faces[1].v1 = 2;
   ship.faces[1].v2 = 0;
   ship.faces[1].v3 = 3;
   face = &ship.faces[1];
   v1.x = (pts[face->v2].x - pts[face->v1].x);
   v1.y = (pts[face->v2].y - pts[face->v1].y);
   v1.z = (pts[face->v2].z - pts[face->v1].z);
   v2.x = (pts[face->v3].x - pts[face->v1].x);
   v2.y = (pts[face->v3].y - pts[face->v1].y);
   v2.z = (pts[face->v3].z - pts[face->v1].z);
   cross_product(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, &(face->normal.x),
		 &(face->normal.y), &(face->normal.z));

   ship.faces[2].v1 = 1;
   ship.faces[2].v2 = 0;
   ship.faces[2].v3 = 2;
   face = &ship.faces[2];
   v1.x = (pts[face->v2].x - pts[face->v1].x);
   v1.y = (pts[face->v2].y - pts[face->v1].y);
   v1.z = (pts[face->v2].z - pts[face->v1].z);
   v2.x = (pts[face->v3].x - pts[face->v1].x);
   v2.y = (pts[face->v3].y - pts[face->v1].y);
   v2.z = (pts[face->v3].z - pts[face->v1].z);
   cross_product(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z,
		 &(face->normal.x), &(face->normal.y), &(face->normal.z));

   ship.faces[3].v1 = 2;
   ship.faces[3].v2 = 3;
   ship.faces[3].v3 = 1;
   face = &ship.faces[3];
   v1.x = (pts[face->v2].x - pts[face->v1].x);
   v1.y = (pts[face->v2].y - pts[face->v1].y);
   v1.z = (pts[face->v2].z - pts[face->v1].z);
   v2.x = (pts[face->v3].x - pts[face->v1].x);
   v2.y = (pts[face->v3].y - pts[face->v1].y);
   v2.z = (pts[face->v3].z - pts[face->v1].z);
   cross_product(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z,
		 &(face->normal.x), &(face->normal.y), &(face->normal.z));

   for (i=0; i<NUM_FACES; i++) {
      ship.faces[i].colour = 32;
      ship.faces[i].range = 15;
      normalize_vector(&ship.faces[i].normal.x, &ship.faces[i].normal.y,
		       &ship.faces[i].normal.z);
      ship.faces[i].rnormal.x = ship.faces[i].normal.x;
      ship.faces[i].rnormal.y = ship.faces[i].normal.y;
      ship.faces[i].rnormal.z = ship.faces[i].normal.z;
   }

   ship.x = ship.y = 0;
   ship.z = itofix(192);
   ship.rx = ship.ry = ship.rz = 0;

   ship.aim.x = direction.x = 0;
   ship.aim.y = direction.y = 0;
   ship.aim.z = direction.z = itofix(-1);
   ship.velocity = 0;
}



/* draws the ship model */
void draw_ship(void)
{
   VECTOR outs[NUM_VERTS];
   MATRIX m;
   int i, col;

   ship.minx = SCREEN_W;
   ship.miny = SCREEN_H;
   ship.maxx = ship.maxy = 0;

   get_rotation_matrix(&m, ship.rx, ship.ry, ship.rz);
   apply_matrix(&m, ship.aim.x, ship.aim.y, ship.aim.z,
		&outs[0].x, &outs[0].y, &outs[0].z);
   direction.x = outs[0].x;
   direction.y = outs[0].y;
   direction.z = outs[0].z;

   for (i=0; i<NUM_FACES; i++)
      apply_matrix(&m, ship.faces[i].normal.x, ship.faces[i].normal.y,
		   ship.faces[i].normal.z, &ship.faces[i].rnormal.x,
		   &ship.faces[i].rnormal.y, &ship.faces[i].rnormal.z);

   get_transformation_matrix(&m, itofix(1), ship.rx, ship.ry, ship.rz,
			     ship.x, ship.y, ship.z);

   for (i=0; i<NUM_VERTS; i++) {
      apply_matrix(&m, ship.points[i].x, ship.points[i].y, ship.points[i].z,
		   &outs[i].x, &outs[i].y, &outs[i].z);
      persp_project(outs[i].x, outs[i].y, outs[i].z, &outs[i].x, &outs[i].y);
      if (fixtoi(outs[i].x) < ship.minx)
	 ship.minx = fixtoi(outs[i].x);
      if (fixtoi(outs[i].x) > ship.maxx)
	 ship.maxx = fixtoi(outs[i].x);
      if (fixtoi(outs[i].y) < ship.miny)
	 ship.miny = fixtoi(outs[i].y);
      if (fixtoi(outs[i].y) > ship.maxy)
	 ship.maxy = fixtoi(outs[i].y);
   }

   for (i=0; i<NUM_FACES; i++) {
      if (fixtof(ship.faces[i].rnormal.z) < 0.0) {
	 col = fixtoi(fixmul(dot_product(ship.faces[i].rnormal.x,
					 ship.faces[i].rnormal.y,
					 ship.faces[i].rnormal.z, 0, 0,
					 itofix(1)),
			     itofix(ship.faces[i].range)));
	 if (col < 0)
	    col = -col + ship.faces[i].colour;
	 else
	    col = col + ship.faces[i].colour;

	 triangle(buffer, fixtoi(outs[ship.faces[i].v1].x),
		  fixtoi(outs[ship.faces[i].v1].y),
		  fixtoi(outs[ship.faces[i].v2].x),
		  fixtoi(outs[ship.faces[i].v2].y),
		  fixtoi(outs[ship.faces[i].v3].x),
		  fixtoi(outs[ship.faces[i].v3].y), palette_color[col]);
      }
   }
}



/* removes the ship model from the screen */
void erase_ship(void)
{
   rectfill(buffer, ship.minx, ship.miny, ship.maxx, ship.maxy,
	    palette_color[0]);
}



int main(int argc, char *argv[])
{
   PALETTE pal;
   int i;

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

   for (i=0; i<16; i++)
      pal[i].r = pal[i].g = pal[i].b = 0;

   /* greyscale */
   pal[16].r = pal[16].g = pal[16].b = 63;
   pal[17].r = pal[17].g = pal[17].b = 48;
   pal[18].r = pal[18].g = pal[18].b = 32;
   pal[19].r = pal[19].g = pal[19].b = 16;
   pal[20].r = pal[20].g = pal[20].b = 8;

   /* red range */
   for (i=0; i<16; i++) { 
      pal[i+32].r = 31 + i*2;
      pal[i+32].g = 15;
      pal[i+32].b = 7;
   }

   /* a nice fire orange */
   for (i=64; i<68; i++) {
      pal[i].r = 63; 
      pal[i].g = 17 + (i-64)*3;
      pal[i].b = 0;
   }

   set_palette(pal);

   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   clear_bitmap(buffer);

   set_projection_viewport(0, 0, SCREEN_W, SCREEN_H);

   init_stars();
   draw_stars();
   init_ship();
   draw_ship();

   for (;;) {
      erase_stars();
      erase_ship();

      move_stars();
      draw_stars();

      textprintf_centre_ex(buffer, font, SCREEN_W / 2, SCREEN_H-10,
			   palette_color[17], 0,
			   "     direction: [%f] [%f] [%f]     ",
			   fixtof(direction.x), fixtof(direction.y),
			   fixtof(direction.z));
      textprintf_centre_ex(buffer, font, SCREEN_W / 2, SCREEN_H-20,
			   palette_color[17], 0,
			   "   delta: [%f] [%f] [%f]   ", fixtof(delta.x),
			   fixtof(delta.y), fixtof(delta.z));
      textprintf_centre_ex(buffer, font, SCREEN_W / 2, SCREEN_H-30,
			   palette_color[17], 0, "   velocity: %d   ",
			   ship.velocity);

      textout_centre_ex(buffer, font, "Press ESC to exit", SCREEN_W/2, 16,
			palette_color[18], 0);
      textout_centre_ex(buffer, font, "Press CTRL to fire engine",
			SCREEN_W/2, 32, palette_color[18], 0);

      draw_ship();

      vsync();
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      poll_keyboard();

      /* exit program */
      if (key[KEY_ESC]) 
	 break;

      /* rotates */
      if (key[KEY_UP]) 
	 ship.rx -= itofix(5);
      else if (key[KEY_DOWN])
	 ship.rx += itofix(5);

      if (key[KEY_LEFT])
	 ship.ry -= itofix(5);
      else if (key[KEY_RIGHT])
	 ship.ry += itofix(5);

      if (key[KEY_PGUP])
	 ship.rz -= itofix(5);
      else if (key[KEY_PGDN])
	 ship.rz += itofix(5);

      /* thrust */
      if ((key[KEY_LCONTROL]) || (key[KEY_RCONTROL])) { 
	 ship.faces[ENGINE].colour = ENGINE_ON;
	 ship.faces[ENGINE].range = 3;
	 if (ship.velocity < SPEED_LIMIT)
	    ship.velocity += 2;
      }
      else {
	 ship.faces[ENGINE].colour = ENGINE_OFF;
	 ship.faces[ENGINE].range = 15;
	 if (ship.velocity > 0)
	    ship.velocity -= 2;
      }

      ship.rx &= itofix(255);
      ship.ry &= itofix(255);
      ship.rz &= itofix(255);

      delta.x = fixmul(direction.x, itofix(ship.velocity));
      delta.y = fixmul(direction.y, itofix(ship.velocity));
      delta.z = fixmul(direction.z, itofix(ship.velocity));
   }

   destroy_bitmap(buffer);
   return 0;
}

END_OF_MAIN()
