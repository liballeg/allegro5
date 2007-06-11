/*
 *    Example program for the Allegro library, by Bertrand Coconnier.
 *
 *    This program demonstrates how to use Z-buffered polygons and
 *    floating point 3D math routines. It also provides a simple
 *    way to compute fps (frames per second) using a timer. After
 *    selecting a screen resolution through the standard GUI dialog,
 *    the example shows two 3D cubes rotating and intersecting each
 *    other. Rather than having full polygons incorrectly overlap
 *    other polygons due to per-polygon sorting, each pixel is drawn
 *    at the correct depth.
 */


#include <allegro.h>



typedef struct FACE
{
  int v1, v2, v3, v4;
} FACE;


V3D_f cube1[] =
{
   { -32., -32., -32., 0., 0., 72},
   { -32.,  32., -32., 0., 0., 80},
   {  32.,  32., -32., 0., 0., 95},
   {  32., -32., -32., 0., 0., 88},
   { -32., -32.,  32., 0., 0., 72},
   { -32.,  32.,  32., 0., 0., 80},
   {  32.,  32.,  32., 0., 0., 95},
   {  32., -32.,  32., 0., 0., 88}
};


V3D_f cube2[] =
{
   { -32., -32., -32., 0., 0., 104},
   { -32.,  32., -32., 0., 0., 112},
   {  32.,  32., -32., 0., 0., 127},
   {  32., -32., -32., 0., 0., 120},
   { -32., -32.,  32., 0., 0., 104},
   { -32.,  32.,  32., 0., 0., 112},
   {  32.,  32.,  32., 0., 0., 127},
   {  32., -32.,  32., 0., 0., 120}
};


FACE faces[] =
{
   { 2, 1, 0, 3 },
   { 4, 5, 6, 7 },
   { 0, 1, 5, 4 },
   { 2, 3, 7, 6 },
   { 4, 7, 3, 0 },
   { 1, 2, 6, 5 }
};



volatile int t;


/* timer interrupt handler */
void tick(void)
{
   t++;
}

END_OF_FUNCTION(tick)



/* update cube positions */
void anim_cube(MATRIX_f* matrix1, MATRIX_f* matrix2, V3D_f x1[], V3D_f x2[])
{
   int i;

   for (i=0; i<8; i++) {
      apply_matrix_f(matrix1, cube1[i].x, cube1[i].y, cube1[i].z,
		     &(x1[i].x), &(x1[i].y), &(x1[i].z));
      apply_matrix_f(matrix2, cube2[i].x, cube2[i].y, cube2[i].z,
		     &(x2[i].x), &(x2[i].y), &(x2[i].z));
      persp_project_f(x1[i].x, x1[i].y, x1[i].z, &(x1[i].x), &(x1[i].y));
      persp_project_f(x2[i].x, x2[i].y, x2[i].z, &(x2[i].x), &(x2[i].y));
   }
}



/* cull backfaces and draw cubes */
void draw_cube(BITMAP* buffer, V3D_f x1[], V3D_f x2[])
{
   int i;
   
   for (i=0; i<6; i++) {
      V3D_f vtx1, vtx2, vtx3, vtx4;
      
      vtx1 = x1[faces[i].v1];
      vtx2 = x1[faces[i].v2];
      vtx3 = x1[faces[i].v3];
      vtx4 = x1[faces[i].v4];
      if (polygon_z_normal_f(&vtx1, &vtx2, &vtx3) > 0)
         quad3d_f(buffer, POLYTYPE_GCOL | POLYTYPE_ZBUF, NULL,
		  &vtx1, &vtx2, &vtx3, &vtx4);

      vtx1 = x2[faces[i].v1];
      vtx2 = x2[faces[i].v2];
      vtx3 = x2[faces[i].v3];
      vtx4 = x2[faces[i].v4];
      if (polygon_z_normal_f(&vtx1, &vtx2, &vtx3) > 0)
         quad3d_f(buffer, POLYTYPE_GCOL | POLYTYPE_ZBUF, NULL,
		  &vtx1, &vtx2, &vtx3, &vtx4);
   }
}



int main(void)
{
   ZBUFFER *zbuf;
   BITMAP *buffer;
   PALETTE pal;
   MATRIX_f matrix1, matrix2;
   V3D_f x1[8], x2[8];

   int i;
   int c = GFX_AUTODETECT;
   int w, h, bpp;
   
   int frame = 0;
   float fps = 0.;
   
   float rx1, ry1, rz1;		/* cube #1 rotations */
   float drx1, dry1, drz1;	/* cube #1 rotation speed */
   float rx2, ry2, rz2;		/* cube #2 rotations */
   float drx2, dry2, drz2;	/* cube #1 rotation speed */
   float tx = 16.;		/* x shift between cubes */
   float tz1 = 100.;		/* cube #1 z coordinate */
   float tz2 = 105.;		/* cube #2 z coordinate */
   
   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();
   
   LOCK_VARIABLE(t);
   LOCK_FUNCTION(tick);
   
   install_int(tick, 10);
   
   /* color 0 = black */
   pal[0].r = pal[0].g = pal[0].b = 0;
   /* color 1 = red */
   pal[1].r = 255;
   pal[1].g = pal[1].b = 0;

   /* copy the desktop palette */
   for (i=1; i<64; i++)
      pal[i] = desktop_palette[i];

   /* make a blue gradient */
   for (i=64; i<96; i++) {
      pal[i].b = (i-64)*2;
      pal[i].g = pal[i].r = 0;
   }

   /* make a green gradient */
   for (i=96; i<128; i++) {
      pal[i].g = (i-96)*2;
      pal[i].r = pal[i].b = 0;
   }

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
   if (!gfx_mode_select_ex(&c, &w, &h, &bpp)) {
      allegro_exit();
      return 1;
   }

   set_color_depth(bpp);

   if (set_gfx_mode(c, w, h, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   set_palette_range(pal, 0, 127, FALSE);

   /* double buffer the animation and create the Z-buffer */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);
   zbuf = create_zbuffer(buffer);
   set_zbuffer(zbuf);

   /* set up the viewport for the perspective projection */
   set_projection_viewport(0, 0, SCREEN_W, SCREEN_H);
   
   /* compute rotations and speed rotation */
   rx1 = ry1 = rz1 = 0.;
   rx2 = ry2 = rz2 = 0.;
   
   drx1 = ((AL_RAND() & 31) - 16) / 4.;
   dry1 = ((AL_RAND() & 31) - 16) / 4.;
   drz1 = ((AL_RAND() & 31) - 16) / 4.;

   drx2 = ((AL_RAND() & 31) - 16) / 4.;
   dry2 = ((AL_RAND() & 31) - 16) / 4.;
   drz2 = ((AL_RAND() & 31) - 16) / 4.;

   /* set the transformation matrices */
   get_transformation_matrix_f(&matrix1, 1., rx1, ry1, rz1, tx, 0., tz1);
   get_transformation_matrix_f(&matrix2, 1., rx2, ry2, rz2, -tx, 0., tz2);

   /* set colors */
   for (i=0; i<8; i++) {
      x1[i].c = palette_color[cube1[i].c];
      x2[i].c = palette_color[cube2[i].c];
   }

   /* main loop */
   while(1) {
      clear_bitmap(buffer);
      clear_zbuffer(zbuf, 0.);
      
      anim_cube(&matrix1, &matrix2, x1, x2);
      draw_cube(buffer, x1, x2);
      
      /* update transformation matrices */
      rx1 += drx1;
      ry1 += dry1;
      rz1 += drz1;
      rx2 += drx2;
      ry2 += dry2;
      rz2 += drz2;
      get_transformation_matrix_f(&matrix1, 1., rx1, ry1, rz1, tx, 0., tz1);
      get_transformation_matrix_f(&matrix2, 1., rx2, ry2, rz2, -tx, 0., tz2);

      textprintf_ex(buffer, font, 10, 1, palette_color[1], 0,
		    "Z-buffered polygons (%.1f fps)", fps);
   
      vsync();
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      frame++;
      
      if (t > 100) {
	 fps = (100. * frame) / t;
	 t = 0;
	 frame = 0;
      }

      if (keypressed()){
	 if ((readkey() & 0xFF) == 27)
	    break;
      }
   }

   destroy_bitmap(buffer);
   destroy_zbuffer(zbuf);
   
   return 0;
}

END_OF_MAIN()
