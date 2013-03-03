#ifndef __al_included_allegro5_allegro_hlsl_h
#define __al_included_allegro5_allegro_hlsl_h

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(char const *, al_get_default_hlsl_vertex_shader, (void));
AL_FUNC(char const *, al_get_default_hlsl_pixel_shader, (void));

#ifdef __cplusplus
}
#endif

#endif
