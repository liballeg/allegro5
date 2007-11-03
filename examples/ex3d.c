/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use the 3d matrix functions.
 *    It isn't a very elegant or efficient piece of code, but it
 *    does show the stuff in action. It is left to the reader as
 *    an exercise to design a proper model structure and rendering
 *    pipeline: after all, the best way to do that sort of stuff
 *    varies hugely from one game to another.
 *
 *    The example first shows a screen resolution selection dialog.
 *    Then, a number of bouncing 3d cubes are animated. Pressing
 *    a key modifies the rendering of the cubes, which can be
 *    wireframe, the more complex transparent perspective correct
 *    texture mapped version, and many other.
 */


#include <allegro.h>



#define NUM_SHAPES         8     /* number of bouncing cubes */

#define NUM_VERTICES       8     /* a cube has eight corners */
#define NUM_FACES          6     /* a cube has six faces */


typedef struct VTX
{
   fixed x, y, z;
} VTX;


typedef struct QUAD              /* four vertices makes a quad */
{
   VTX *vtxlist;
   int v1, v2, v3, v4;
} QUAD;


typedef struct SHAPE             /* store position of a shape */
{
   fixed x, y, z;                /* x, y, z position */
   fixed rx, ry, rz;             /* rotations */
   fixed dz;                     /* speed of movement */
   fixed drx, dry, drz;          /* speed of rotation */
} SHAPE;


VTX points[] =                   /* a cube, centered on the origin */
{
   /* vertices of the cube */
   { -32 << 16, -32 << 16, -32 << 16 },
   { -32 << 16,  32 << 16, -32 << 16 },
   {  32 << 16,  32 << 16, -32 << 16 },
   {  32 << 16, -32 << 16, -32 << 16 },
   { -32 << 16, -32 << 16,  32 << 16 },
   { -32 << 16,  32 << 16,  32 << 16 },
   {  32 << 16,  32 << 16,  32 << 16 },
   {  32 << 16, -32 << 16,  32 << 16 },
};


QUAD faces[] =                   /* group the vertices into polygons */
{
   { points, 0, 3, 2, 1 },
   { points, 4, 5, 6, 7 },
   { points, 0, 1, 5, 4 },
   { points, 2, 3, 7, 6 },
   { points, 0, 4, 7, 3 },
   { points, 1, 2, 6, 5 }
};


SHAPE shapes[NUM_SHAPES];        /* a list of shapes */


/* somewhere to put translated vertices */
VTX output_points[NUM_VERTICES * NUM_SHAPES];
QUAD output_faces[NUM_FACES * NUM_SHAPES];


enum { 
   wireframe,
   flat,
   gcol,
   grgb,
   atex,
   ptex,
   atex_mask,
   ptex_mask,
   atex_lit,
   ptex_lit,
   atex_mask_lit,
   ptex_mask_lit,
   atex_trans,
   ptex_trans,
   atex_mask_trans,
   ptex_mask_trans,
   last_mode
} render_mode = wireframe;


int render_type[] =
{
   0,
   POLYTYPE_FLAT,
   POLYTYPE_GCOL,
   POLYTYPE_GRGB,
   POLYTYPE_ATEX,
   POLYTYPE_PTEX,
   POLYTYPE_ATEX_MASK,
   POLYTYPE_PTEX_MASK,
   POLYTYPE_ATEX_LIT,
   POLYTYPE_PTEX_LIT,
   POLYTYPE_ATEX_MASK_LIT,
   POLYTYPE_PTEX_MASK_LIT,
   POLYTYPE_ATEX_TRANS,
   POLYTYPE_PTEX_TRANS,
   POLYTYPE_ATEX_MASK_TRANS,
   POLYTYPE_PTEX_MASK_TRANS
};


char *mode_desc[] =
{
   "Wireframe",
   "Flat shaded",
   "Single color Gouraud shaded",
   "Gouraud shaded",
   "Texture mapped",
   "Perspective correct texture mapped",
   "Masked texture mapped",
   "Masked persp. correct texture mapped",
   "Lit texture map",
   "Lit persp. correct texture map",
   "Masked lit texture map",
   "Masked lit persp. correct texture map",
   "Transparent texture mapped",
   "Transparent perspective correct texture mapped",
   "Transparent masked texture mapped",
   "Transparent masked persp. correct texture mapped",
};


BITMAP *texture;



/* initialise shape positions */
void init_shapes(void)
{
   int c;

   for (c=0; c<NUM_SHAPES; c++) {
      shapes[c].x = ((AL_RAND() & 255) - 128) << 16;
      shapes[c].y = ((AL_RAND() & 255) - 128) << 16;
      shapes[c].z = 768 << 16;
      shapes[c].rx = 0;
      shapes[c].ry = 0;
      shapes[c].rz = 0;
      shapes[c].dz =  ((AL_RAND() & 255) - 8) << 12;
      shapes[c].drx = ((AL_RAND() & 31) - 16) << 12;
      shapes[c].dry = ((AL_RAND() & 31) - 16) << 12;
      shapes[c].drz = ((AL_RAND() & 31) - 16) << 12;
   }
}



/* update shape positions */
void animate_shapes(void)
{
   int c;

   for (c=0; c<NUM_SHAPES; c++) {
      shapes[c].z += shapes[c].dz;

      if ((shapes[c].z > itofix(1024)) ||
	  (shapes[c].z < itofix(192)))
	 shapes[c].dz = -shapes[c].dz;

      shapes[c].rx += shapes[c].drx;
      shapes[c].ry += shapes[c].dry;
      shapes[c].rz += shapes[c].drz;
   }
}



/* translate shapes from 3d world space to 2d screen space */
void translate_shapes(void)
{
   int c, d;
   MATRIX matrix;
   VTX *outpoint = output_points;
   QUAD *outface = output_faces;

   for (c=0; c<NUM_SHAPES; c++) {
      /* build a transformation matrix */
      get_transformation_matrix(&matrix, itofix(1),
				shapes[c].rx, shapes[c].ry, shapes[c].rz,
				shapes[c].x, shapes[c].y, shapes[c].z);

      /* output the vertices */
      for (d=0; d<NUM_VERTICES; d++) {
	 apply_matrix(&matrix, points[d].x, points[d].y, points[d].z,
		      &outpoint[d].x, &outpoint[d].y, &outpoint[d].z);
	 persp_project(outpoint[d].x, outpoint[d].y, outpoint[d].z,
		       &outpoint[d].x, &outpoint[d].y);
      }

      /* output the faces */
      for (d=0; d<NUM_FACES; d++) {
	 outface[d] = faces[d];
	 outface[d].vtxlist = outpoint;
      }

      outpoint += NUM_VERTICES;
      outface += NUM_FACES;
   }
}



/* draw a line (for wireframe display) */
void wire(BITMAP *b, VTX *v1, VTX *v2)
{
   int col = CLAMP(128, 255 - fixtoi(v1->z+v2->z) / 16, 255);
   line(b, fixtoi(v1->x), fixtoi(v1->y), fixtoi(v2->x), fixtoi(v2->y),
	palette_color[col]);
}



/* draw a quad */
void draw_quad(BITMAP *b, VTX *v1, VTX *v2, VTX *v3, VTX *v4, int mode)
{
   int col;

   /* four vertices */
   V3D vtx1 = { 0, 0, 0, 0,      0,      0 };
   V3D vtx2 = { 0, 0, 0, 32<<16, 0,      0 };
   V3D vtx3 = { 0, 0, 0, 32<<16, 32<<16, 0 };
   V3D vtx4 = { 0, 0, 0, 0,      32<<16, 0 };

   vtx1.x = v1->x;   vtx1.y = v1->y;   vtx1.z = v1->z;
   vtx2.x = v2->x;   vtx2.y = v2->y;   vtx2.z = v2->z;
   vtx3.x = v3->x;   vtx3.y = v3->y;   vtx3.z = v3->z;
   vtx4.x = v4->x;   vtx4.y = v4->y;   vtx4.z = v4->z;

   /* cull backfaces */
   if ((mode != POLYTYPE_ATEX_MASK) && (mode != POLYTYPE_PTEX_MASK) &&
       (mode != POLYTYPE_ATEX_MASK_LIT) && (mode != POLYTYPE_PTEX_MASK_LIT) &&
       (polygon_z_normal(&vtx1, &vtx2, &vtx3) < 0))
      return;

   /* set up the vertex color, differently for each rendering mode */
   switch (mode) {

      case POLYTYPE_FLAT:
	 col = CLAMP(128, 255 - fixtoi(v1->z+v2->z) / 16, 255);
	 vtx1.c = vtx2.c = vtx3.c = vtx4.c = palette_color[col];
	 break;

      case POLYTYPE_GCOL:
	 vtx1.c = palette_color[0xD0];
	 vtx2.c = palette_color[0x80];
	 vtx3.c = palette_color[0xB0];
	 vtx4.c = palette_color[0xFF];
	 break;

      case POLYTYPE_GRGB:
	 vtx1.c = 0x000000;
	 vtx2.c = 0x7F0000;
	 vtx3.c = 0xFF0000;
	 vtx4.c = 0x7F0000;
	 break;

      case POLYTYPE_ATEX_LIT:
      case POLYTYPE_PTEX_LIT:
      case POLYTYPE_ATEX_MASK_LIT:
      case POLYTYPE_PTEX_MASK_LIT:
	 vtx1.c = CLAMP(0, 255 - fixtoi(v1->z) / 4, 255);
	 vtx2.c = CLAMP(0, 255 - fixtoi(v2->z) / 4, 255);
	 vtx3.c = CLAMP(0, 255 - fixtoi(v3->z) / 4, 255);
	 vtx4.c = CLAMP(0, 255 - fixtoi(v4->z) / 4, 255);
	 break; 
   }

   /* draw the quad */
   quad3d(b, mode, texture, &vtx1, &vtx2, &vtx3, &vtx4);
}



/* callback for qsort() */
int quad_cmp(const void *e1, const void *e2)
{
   QUAD *q1 = (QUAD *)e1;
   QUAD *q2 = (QUAD *)e2;

   fixed d1 = q1->vtxlist[q1->v1].z + q1->vtxlist[q1->v2].z +
	      q1->vtxlist[q1->v3].z + q1->vtxlist[q1->v4].z;

   fixed d2 = q2->vtxlist[q2->v1].z + q2->vtxlist[q2->v2].z +
	      q2->vtxlist[q2->v3].z + q2->vtxlist[q2->v4].z;

   return d2 - d1;
}



/* draw the shapes calculated by translate_shapes() */
void draw_shapes(BITMAP *b)
{
   int c;
   QUAD *face = output_faces;
   VTX *v1, *v2, *v3, *v4;

   /* depth sort */
   qsort(output_faces, NUM_FACES * NUM_SHAPES, sizeof(QUAD), quad_cmp);

   for (c=0; c < NUM_FACES * NUM_SHAPES; c++) {
      /* find the vertices used by the face */
      v1 = face->vtxlist + face->v1;
      v2 = face->vtxlist + face->v2;
      v3 = face->vtxlist + face->v3;
      v4 = face->vtxlist + face->v4;

      /* draw the face */
      if (render_mode == wireframe) {
	 wire(b, v1, v2);
	 wire(b, v2, v3);
	 wire(b, v3, v4);
	 wire(b, v4, v1);
      }
      else {
	 draw_quad(b, v1, v2, v3, v4, render_type[render_mode]);
      }

      face++;
   }
}



/* RGB -> color mapping table. Not needed, but speeds things up */
RGB_MAP rgb_table;


/* lighting color mapping table */
COLOR_MAP light_table;

/* transparency color mapping table */
COLOR_MAP trans_table;



int main(void)
{
   BITMAP *buffer;
   PALETTE pal;
   int c, w, h, bpp;
   int last_retrace_count;

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();

   /* color 0 = black */
   pal[0].r = pal[0].g = pal[0].b = 0;

   /* copy the desktop palette */
   for (c=1; c<64; c++)
      pal[c] = desktop_palette[c];

   /* make a red gradient */
   for (c=64; c<96; c++) {
      pal[c].r = (c-64)*2;
      pal[c].g = pal[c].b = 0;
   }

   /* make a green gradient */
   for (c=96; c<128; c++) {
      pal[c].g = (c-96)*2;
      pal[c].r = pal[c].b = 0;
   }

   /* set up a greyscale in the top half of the palette */
   for (c=128; c<256; c++)
      pal[c].r = pal[c].g = pal[c].b = (c-128)/2;

   /* build rgb_map table */
   create_rgb_table(&rgb_table, pal, NULL);
   rgb_map = &rgb_table;

   /* build a lighting table */
   create_light_table(&light_table, pal, 0, 0, 0, NULL);
   color_map = &light_table;

   /* build a transparency table */
   /* textures are 25% transparent (75% opaque) */
   create_trans_table(&trans_table, pal, 192, 192, 192, NULL);

   /* set up the truecolor blending functions */
   /* textures are 25% transparent (75% opaque) */
   set_trans_blender(0, 0, 0, 192);

   /* set the graphics mode */
   if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(desktop_palette);

   c = GFX_AUTODETECT;
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

   set_palette(pal);

   /* make a bitmap for use as a texture map */
   texture = create_bitmap(32, 32);
   clear_to_color(texture, bitmap_mask_color(texture));
   line(texture, 0, 0, 31, 31, palette_color[1]);
   line(texture, 0, 31, 31, 0, palette_color[1]);
   rect(texture, 0, 0, 31, 31, palette_color[1]);
   textout_ex(texture, font, "dead", 0, 0, palette_color[2], -1);
   textout_ex(texture, font, "pigs", 0, 8, palette_color[2], -1);
   textout_ex(texture, font, "cant", 0, 16, palette_color[2], -1);
   textout_ex(texture, font, "fly.", 0, 24, palette_color[2], -1);

   /* double buffer the animation */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   /* set up the viewport for the perspective projection */
   set_projection_viewport(0, 0, SCREEN_W, SCREEN_H);

   /* initialise the bouncing shapes */
   init_shapes();

   last_retrace_count = retrace_count;

   for (;;) {
      clear_bitmap(buffer);

      while (last_retrace_count < retrace_count) {
	 animate_shapes();
	 last_retrace_count++;
      }

      translate_shapes();
      draw_shapes(buffer);

      textprintf_ex(buffer, font, 0, 0, palette_color[192], -1, "%s, %d bpp",
		    mode_desc[render_mode], bitmap_color_depth(screen));
      textout_ex(buffer, font, "Press a key to change", 0, 12,
		 palette_color[192], -1);

      vsync();
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H); 

      if (keypressed()) {
	 if ((readkey() & 0xFF) == 27)
	    break;
	 else {
	    render_mode++;
	    if (render_mode >= last_mode) {
	       render_mode = wireframe;
	       color_map = &light_table;
	    }
	    if (render_type[render_mode] >= POLYTYPE_ATEX_TRANS)
	       color_map = &trans_table;
	 }
      }
   }

   destroy_bitmap(buffer);
   destroy_bitmap(texture);

   return 0;
}

END_OF_MAIN()
