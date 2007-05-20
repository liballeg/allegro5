/*
 *    Example program for the Allegro library, by Bertrand Coconnier.
 *
 *    This program demonstrates how to use scanline sorting algorithm
 *    in Allegro (create_scene, clear_scene, ... functions). It
 *    also provides an example of how to use the 3D clipping
 *    function. The example consists of a flyby through a lot of
 *    rotating 3d cubes.
 *
 */


#include <allegro.h>



#define MAX_CUBES 4


typedef struct QUAD
{
   int v[4];
} QUAD;


V3D_f vertex[] =
{
   { -10., -10., -10., 0., 0., 72 },
   { -10.,  10., -10., 0., 0., 80 },
   {  10.,  10., -10., 0., 0., 95 },
   {  10., -10., -10., 0., 0., 88 },
   { -10., -10.,  10., 0., 0., 72 },
   { -10.,  10.,  10., 0., 0., 80 },
   {  10.,  10.,  10., 0., 0., 95 },
   {  10., -10.,  10., 0., 0., 88 }
};


QUAD cube[] =
{
   {{ 2, 1, 0, 3 }},
   {{ 4, 5, 6, 7 }},
   {{ 0, 1, 5, 4 }},
   {{ 2, 3, 7, 6 }},
   {{ 4, 7, 3, 0 }},
   {{ 1, 2, 6, 5 }}
};


V3D_f v[4], vout[12], vtmp[12];
V3D_f *pv[4], *pvout[12], *pvtmp[12];



volatile int t;

/* timer interrupt handler */
void tick(void)
{
   t++;
}

END_OF_FUNCTION(tick)



/* init_gfx:
 *  Set up graphics mode.
 */
int init_gfx(PALETTE *pal)
{
   int i;
   int gfx_mode = GFX_AUTODETECT;
   int w, h ,bpp;
   
   /* color 0 = black */
   (*pal)[0].r = (*pal)[0].g = (*pal)[0].b = 0;
   /* color 1 = red */
   (*pal)[1].r = 63;
   (*pal)[1].g = (*pal)[1].b = 0;

   /* copy the desktop palette */
   for (i=2; i<64; i++)
      (*pal)[i] = desktop_palette[i];

   /* make a blue gradient */
   for (i=64; i<96; i++) {
      (*pal)[i].b = (i-64) * 2;
      (*pal)[i].g = (*pal)[i].r = 0;
   }

   for (i=96; i<256; i++)
      (*pal)[i].r = (*pal)[i].g = (*pal)[i].b = 0; 

   /* set the graphics mode */
   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);

   w = SCREEN_W;
   h = SCREEN_H;
   bpp = bitmap_color_depth(screen);
   if (!gfx_mode_select_ex(&gfx_mode, &w, &h, &bpp)) {
      allegro_exit();
      return 1;
   }

   set_color_depth(bpp);

   if (set_gfx_mode(gfx_mode, w, h, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   return 0;
}



/* draw_cube:
 *  Translates, rotates, clips, projects, culls backfaces and draws a cube.
 */
void draw_cube(MATRIX_f *matrix, int num_poly)
{
   int i, j, nv;
   int out[12];
   
   for (i=0; i<num_poly; i++) {
      for (j=0; j<4; j++) {
	 v[j] = vertex[cube[i].v[j]];
	 apply_matrix_f(matrix, v[j].x, v[j].y, v[j].z,
	    &v[j].x, &v[j].y, &v[j].z);
      }
      
      /* nv: number of vertices after clipping is done */
      nv = clip3d_f(POLYTYPE_GCOL, 0.1, 1000., 4, (AL_CONST V3D_f**)pv, pvout,
		    pvtmp, out);
      if (nv) {
	 for (j=0; j<nv; j++)
	    persp_project_f(vout[j].x, vout[j].y, vout[j].z, &vout[j].x,
			    &vout[j].y);

	 if (polygon_z_normal_f(&vout[0], &vout[1], &vout[2]) > 0)
	    scene_polygon3d_f(POLYTYPE_GCOL, NULL, nv, pvout);
      }
   }
}



int main(void)
{
   BITMAP *buffer;
   PALETTE pal;
   MATRIX_f matrix, matrix1, matrix2, matrix3;
   int rx = 0, ry = 0, tz = 40;
   int rot = 0, inc = 1;
   int i, j, k;

   int frame = 0;
   float fps = 0.;
   
   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();
   
   LOCK_VARIABLE(t);
   LOCK_FUNCTION(tick);
   
   install_int(tick, 10);
   
   /* set up graphics mode */
   if (init_gfx(&pal))
      return 1;
   
   set_palette(pal);
   
   /* initialize buffers and viewport */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   create_scene(24 * MAX_CUBES * MAX_CUBES * MAX_CUBES,
		6 * MAX_CUBES * MAX_CUBES * MAX_CUBES);
   set_projection_viewport(0, 0, SCREEN_W, SCREEN_H);
   
   /* initialize pointers */
   for (j=0; j<4; j++)
      pv[j] = &v[j];
   
   for (j=0; j<12; j++) {
      pvtmp[j] = &vtmp[j];
      pvout[j] = &vout[j];
   }
   

   while(!key[KEY_ESC]) {
      clear_bitmap(buffer);
      clear_scene(buffer);

      /* matrix2: rotates cube */
      get_rotation_matrix_f(&matrix2, rx, ry, 0);
      /* matrix3: turns head right/left */
      get_rotation_matrix_f(&matrix3, 0, rot, 0);

      for (k=MAX_CUBES-1; k>=0; k--)
         for (j=0; j<MAX_CUBES; j++)
	      for (i=0; i<MAX_CUBES; i++) {
		/* matrix1: locates cubes */
		get_translation_matrix_f(&matrix1, j*40-MAX_CUBES*20+20,
					 i*40-MAX_CUBES*20+20, tz+k*40);
		
		/* matrix: rotates cube THEN locates cube THEN turns
		 * head right/left */
		matrix_mul_f(&matrix2, &matrix1, &matrix);
		matrix_mul_f(&matrix, &matrix3, &matrix);

		/* cubes are just added to the scene.
		 * No sorting is done at this stage.
		 */
		draw_cube(&matrix, 6);
	      }
      
      /* sorts and renders polys */
      render_scene();
      textprintf_ex(buffer, font, 2, 2, palette_color[1], -1,
		    "(%.1f fps)", fps);
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      frame++;
      
      /* manage cubes movement */
      tz -= 2;
      if (!tz) tz = 40;
      rx += 4;
      ry += 4;
      rot += inc;
      if ((rot >= 25) || (rot <= -25)) inc = -inc;
      
      /* computes fps */
      if (t > 100) {
	 fps = (100. * frame) / t;
	 t = 0;
	 frame = 0;
      }
   }
   
   destroy_bitmap(buffer);
   destroy_scene();
   
   return 0;
}

END_OF_MAIN()
