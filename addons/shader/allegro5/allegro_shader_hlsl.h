#ifndef __al_included_allegro5_allegro_shader_hlsl_h
#define __al_included_allegro5_allegro_shader_hlsl_h

#include "allegro5/allegro_shader.h"
#include "allegro5/allegro_direct3d.h"
#include <d3dx9.h>

#ifdef __cplusplus
extern "C" {
#endif

ALLEGRO_SHADER_FUNC(LPD3DXEFFECT, al_get_direct3d_effect, (ALLEGRO_SHADER *shader));

#ifdef __cplusplus
}
#endif

#endif
