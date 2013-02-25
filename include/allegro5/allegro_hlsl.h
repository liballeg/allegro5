#ifndef __al_included_allegro5_allegro_hlsl_h
#define __al_included_allegro5_allegro_hlsl_h

#include "allegro5/allegro_direct3d.h"
#include "allegro5/shader.h"

#ifdef ALLEGRO_CFG_SHADER_HLSL
#include <d3dx9.h>
#endif

#ifdef __cplusplus
   extern "C" {
#endif

AL_FUNC(LPD3DXEFFECT, al_get_direct3d_effect, (ALLEGRO_SHADER *shader));

#ifdef __cplusplus
   }
#endif

#endif
