#ifndef ALLEGRO_INTERNAL_OPENGL_NEW_H
#define ALLEGRO_INTERNAL_OPENGL_NEW_H

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
}OPENGL_INFO;


int  _al_ogl_look_for_an_extension(AL_CONST char *name, AL_CONST GLubyte *extensions);
void _al_ogl_set_extensions(ALLEGRO_OGL_EXT_API *ext);
void _al_ogl_manage_extensions(ALLEGRO_DISPLAY *disp);
void _al_ogl_unmanage_extensions(ALLEGRO_DISPLAY *disp);


#endif
