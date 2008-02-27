#ifndef ALLEGRO_INTERNAL_OPENGL_NEW_H
#define ALLEGRO_INTERNAL_OPENGL_NEW_H

#include "allegro5/internal/aintern_bitmap.h"


typedef struct ALLEGRO_BITMAP_OGL
{
   ALLEGRO_BITMAP bitmap; /* This must be the first member. */

   /* Driver specifics. */

   GLuint texture; /* 0 means, not uploaded yet. */
   GLuint fbo; /* 0 means, no fbo yet. */

   float left, top, right, bottom; /* Texture coordinates. */
   bool is_backbuffer; /* This is not a real bitmap, but the backbuffer. */
} ALLEGRO_BITMAP_OGL;


typedef struct OPENGL_INFO {
   float version;          /* OpenGL version */
   int num_texture_units;  /* Number of texture units */
   int max_texture_size;   /* Maximum texture size */
   int is_voodoo3_and_under; /* Special cases for Voodoo 1-3 */
   int is_voodoo;          /* Special cases for Voodoo cards */
   int is_matrox_g200;     /* Special cases for Matrox G200 boards */
   int is_ati_rage_pro;    /* Special cases for ATI Rage Pro boards */
   int is_ati_radeon_7000; /* Special cases for ATI Radeon 7000 */
   int is_ati_r200_chip;	/* Special cases for ATI card with chip R200 */
   int is_mesa_driver;     /* Special cases for MESA */
} OPENGL_INFO;


typedef struct OGL_PIXEL_FORMAT {
   int format;
   int doublebuffered;
   int depth_size;
   int rmethod;
   int stencil_size;
   int r_shift, g_shift, b_shift, a_shift;
   int r_size, g_size, b_size, a_size;
   int fullscreen;
   int sample_buffers;
   int samples;
   int float_color;
   int float_depth;
} OGL_PIXEL_FORMAT;


typedef struct ALLEGRO_DISPLAY_OGL
{
   ALLEGRO_DISPLAY display; /* This must be the first member. */

   /* A list of extensions supported by Allegro, for this context. */
   ALLEGRO_OGL_EXT_LIST *extension_list;
   /* A list of extension API, loaded by Allegro, for this context. */
   ALLEGRO_OGL_EXT_API *extension_api;
   /* Various info about OpenGL implementation. */
   OPENGL_INFO ogl_info;

   ALLEGRO_BITMAP_OGL *opengl_target;
   ALLEGRO_BITMAP *backbuffer;

} ALLEGRO_DISPLAY_OGL;


/* extensions */
int  _al_ogl_look_for_an_extension(AL_CONST char *name, AL_CONST GLubyte *extensions);
void _al_ogl_set_extensions(ALLEGRO_OGL_EXT_API *ext);
void _al_ogl_manage_extensions(ALLEGRO_DISPLAY_OGL *disp);
void _al_ogl_unmanage_extensions(ALLEGRO_DISPLAY_OGL *disp);

/* draw */
void _al_ogl_add_drawing_functions(ALLEGRO_DISPLAY_INTERFACE *vt);

/* bitmap */
ALLEGRO_BITMAP *_al_ogl_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h);

/* common driver */
void _al_ogl_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
ALLEGRO_BITMAP *_al_ogl_get_backbuffer(ALLEGRO_DISPLAY *d);

#endif
