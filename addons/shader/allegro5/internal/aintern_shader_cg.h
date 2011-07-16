#ifndef _ALLEGRO_SHADER_INTERN_CG
#define _ALLEGRO_SHADER_INTERN_CG

#include "../allegro_shader.h"
#include "aintern_shader.h"

struct ALLEGRO_SHADER_CG_S
{
   ALLEGRO_SHADER shader;
   CGcontext context;
   CGprogram vertex_program;
   CGprogram pixel_program;
   CGparameter name_pos;
   CGparameter name_col;
   CGparameter name_tex;
   CGparameter name_projview;
#ifdef ALLEGRO_WINDOWS
   LPDIRECT3DVERTEXDECLARATION9 vertex_decl;
#endif
};

typedef struct ALLEGRO_SHADER_CG_S ALLEGRO_SHADER_CG_S;

#endif

