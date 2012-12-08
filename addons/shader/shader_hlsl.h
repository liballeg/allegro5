#ifndef _SHADER_HLSL_H
#define _SHADER_HLSL_H

#ifdef __cplusplus
extern "C" {
#endif

ALLEGRO_SHADER *_al_create_shader_hlsl(ALLEGRO_SHADER_PLATFORM platform);
void _al_set_shader_hlsl(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader);

#ifdef __cplusplus
}
#endif

#endif
