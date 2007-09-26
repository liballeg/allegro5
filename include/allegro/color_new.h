#ifndef _ALLEGRO_COLORNEW_H
#define _ALLEGRO_COLORNEW_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_COLOR ALLEGRO_COLOR;

typedef struct ALLEGRO_DISPLAY ALLEGRO_DISPLAY;

struct ALLEGRO_COLOR
{
	uint64_t raw[4];
};

#ifdef __cplusplus
   }
#endif

#endif
