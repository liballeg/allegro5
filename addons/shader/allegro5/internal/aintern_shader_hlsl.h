#ifndef _ALLEGRO_SHADER_INTERN_HLSL
#define _ALLEGRO_SHADER_INTERN_HLSL

#include "../allegro_shader.h"
#include "aintern_shader.h"

struct ALLEGRO_SHADER_HLSL_S
{
	ALLEGRO_SHADER shader;
        LPD3DXEFFECT hlsl_shader;
};

typedef struct ALLEGRO_SHADER_HLSL_S ALLEGRO_SHADER_HLSL_S;

#endif
