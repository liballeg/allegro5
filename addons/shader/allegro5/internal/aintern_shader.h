#ifndef _AINTERN_SHADER_H
#define _AINTERN_SHADER_H

struct ALLEGRO_SHADER
{
   ALLEGRO_USTR *vertex_copy;
   ALLEGRO_USTR *pixel_copy;
   ALLEGRO_USTR *log;
   ALLEGRO_SHADER_PLATFORM platform;
};

#endif
